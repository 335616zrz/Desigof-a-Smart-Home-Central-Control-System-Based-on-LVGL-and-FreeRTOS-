const axios = require('axios');
const fs = require('fs');
const { spawn } = require('child_process');
const { v4: uuidv4 } = require('uuid');
const WebSocket = require('ws');

const { PcmPacer } = require('../audio/PcmPacer');

const {
  ValidationError,
  validateStartPayload,
  validateAudioPayload,
  validateTextPayload,
} = require('./validators');
const { metrics } = require('../metrics');
const { getAsrWorker } = require('../asr');

// RNNoise 降噪（可选）
const ENABLE_RNNOISE = process.env.ENABLE_RNNOISE === 'true';
let RNNoiseProcessor = null;
let rnnoiseInstance = null;
let rnnoiseInitPromise = null;

if (ENABLE_RNNOISE) {
  try {
    const mod = require('../audio/RNNoiseProcessor');
    RNNoiseProcessor = mod.RNNoiseProcessor;
    console.log('[RNNoise] 模块加载成功，等待初始化...');
  } catch (err) {
    console.warn('[RNNoise] 模块加载失败，已禁用降噪:', err.message);
  }
}

async function getRNNoiseInstance() {
  if (!RNNoiseProcessor) return null;
  if (rnnoiseInstance) return rnnoiseInstance;
  if (rnnoiseInitPromise) return rnnoiseInitPromise;

  rnnoiseInitPromise = (async () => {
    try {
      rnnoiseInstance = new RNNoiseProcessor();
      await rnnoiseInstance.init();
      console.log('[RNNoise] 初始化完成');
      return rnnoiseInstance;
    } catch (err) {
      console.error('[RNNoise] 初始化失败:', err.message);
      rnnoiseInstance = null;
      return null;
    }
  })();
  return rnnoiseInitPromise;
}

class RealtimeSession {
  constructor({ socket, request, config, logger }) {
    this.socket = socket;
    this.request = request;
    this.config = config;
    this.logger = logger;

    this.id = uuidv4();
    this.sampleRate = config.server.defaultSampleRate;
    this.language = null;

    // 可选：用于 ASR 调试/测试的会话级开关
    // - disableLLM: ASR final 不触发 LLM
    // - disableTTS: 禁用 TTS（即使 LLM 输出也不播报）
    // - asrFinalOnly: 仅向客户端推送 asr.final（忽略 asr.partial）
    this.disableLLM = false;
    this.disableTTS = false;
    this.asrFinalOnly = false;

    this.asrStdoutBuffer = '';
    this.ttsProcess = null;
    this.ttsStdoutBuffer = '';
    this.ttsActive = false;
    this.ttsWs = null;
    this.ttsTaskId = null;
    this.ttsPacer = null;
    this.ttsAwaitingDrain = false;
    this.ttsStripInParen = false;
    this.ttsStripParenClose = '';
    this.ttsPending = '';
    this.lastTtsSendAt = 0;
    this.minTtsStartChars = 12;  // 累计到一定字数再启动，避免极短��频繁起停
    this.minTtsContinueChars = 18; // 追加发送的最小字数阈值（无标点时）

    // CosyVoice 流式 TTS 状态
    this.ttsReady = false;        // task-started 后变为 true，表示可以发送 continue-task
    this.ttsTextQueue = [];       // 在 ttsReady 前缓存待发送的文本
    this.ttsLlmFinished = false;  // LLM 输出完成标志

    this.history = [{ role: 'system', content: config.systemPrompt }];
    this.partialTranscript = '';
    this.currentAssistant = '';
    this.sessionAudioBytes = 0;

    this.lastStreamedUserMessage = null;
    this.lastStreamedUserText = '';
    this.lastAcceptedTranscript = '';
    this.lastAcceptedAt = 0;
    this.partialAutoTimer = null;
    this.partialTranscript = '';
    this.lastPartialAt = 0;
    this.autoFinalizedText = '';
    this.autoFinalizedAt = 0;
    this.silenceTimeoutMs =
      Number.isFinite(this.config.asr?.silenceTimeoutMs) && this.config.asr.silenceTimeoutMs > 0
        ? this.config.asr.silenceTimeoutMs
        : 1000;
    this.ttsStartedAt = null;
    this.currentTtsText = '';

    this.activeLLMController = null;
    this.activeLLMStream = null;

    this.isClosed = false;
    this.lastActivity = Date.now();

    this.setupSocket();
    this.logEvent({
      level: 'info',
      event: 'session.start',
      status: 'connected',
    });
    metrics.increment('sessions_started_total');
  }

  handleInterimTranscript(text) {
    this.partialTranscript = typeof text === 'string' ? text : '';
    this.lastPartialAt = Date.now();
    this.resetPartialTimer();
  }

  resetPartialTimer() {
    if (this.partialAutoTimer) {
      clearTimeout(this.partialAutoTimer);
      this.partialAutoTimer = null;
    }
    if (!this.partialTranscript || !this.partialTranscript.trim()) {
      return;
    }
    this.partialAutoTimer = setTimeout(() => {
      this.partialAutoTimer = null;
      this.autoFinalizePartial();
    }, this.silenceTimeoutMs);
  }

  clearPartialTimer() {
    if (this.partialAutoTimer) {
      clearTimeout(this.partialAutoTimer);
      this.partialAutoTimer = null;
    }
  }

  autoFinalizePartial() {
    const candidate = typeof this.partialTranscript === 'string' ? this.partialTranscript.trim() : '';
    if (!candidate) {
      return;
    }
    if (Date.now() - this.lastPartialAt < this.silenceTimeoutMs - 50) {
      return;
    }

    this.partialTranscript = '';
    this.autoFinalizedText = candidate;
    this.autoFinalizedAt = Date.now();
    this.lastAcceptedTranscript = candidate;
    this.lastAcceptedAt = this.autoFinalizedAt;

    this.sendToClient({ type: 'asr.final', text: candidate });

    const lastMessage = this.history[this.history.length - 1];
    if (!lastMessage || lastMessage.role !== 'user' || lastMessage.content !== candidate) {
      this.history.push({ role: 'user', content: candidate });
    }

    this.logEvent({
      level: 'info',
      event: 'asr.auto_final',
      transcriptLength: candidate.length,
      transcript: this.logger.includeContent ? candidate : undefined,
    });

    if (!this.disableLLM) {
      this.streamLLMResponse({ source: 'auto', force: true });
    }
  }

  setupSocket() {
    this.sendToClient({
      type: 'session',
      sessionId: this.id,
    });

    this.socket.on('message', (raw) => {
      this.lastActivity = Date.now();
      try {
        const maxBytes = this.config.security.maxClientMessageBytes;
        if (maxBytes && raw.length > maxBytes) {
          throw new ValidationError('单条消息大小超过安全限制。');
        }
        const payload = JSON.parse(raw.toString());
        this.handleClientMessage(payload);
      } catch (error) {
        const detail =
          error instanceof ValidationError
            ? error.message
            : '消息格式错误，必须为 JSON。';
        this.handleFatalViolation(detail);
      }
    });

    this.socket.on('close', () => {
      this.terminate();
    });

    this.socket.on('error', (error) => {
      console.error('WebSocket 错误：', error.message);
      this.terminate();
    });
  }

  handleClientMessage(payload) {
    if (!payload || typeof payload !== 'object') {
      this.handleFatalViolation('消息载荷必须为对象。');
      return;
    }

    switch (payload.type) {
      case 'start':
        this.handleStart(payload);
        break;
      case 'audio':
        this.handleAudio(payload);
        break;
      case 'text':
        this.handleText(payload);
        break;
      case 'stop':
        this.sendToAsr({ type: 'stop' });
        break;
      case 'abort':
        this.abortLLMStream();
        this.abortTTS();
        break;
      default:
        this.handleFatalViolation(`未知的消息类型：${payload.type}`);
    }
  }

  handleStart(payload) {
    try {
      const { sampleRate, language } = validateStartPayload(payload, this.config);
      this.sampleRate = sampleRate;
      this.language = language;
      const toBool = (value) => value === true || value === 'true' || value === 1 || value === '1';
      const asrOnly = toBool(payload.asrOnly) || toBool(payload.asr_only);
      this.disableLLM =
        asrOnly || toBool(payload.disableLLM) || toBool(payload.noLLM) || toBool(payload.no_llm);
      this.disableTTS =
        asrOnly || toBool(payload.disableTTS) || toBool(payload.noTTS) || toBool(payload.no_tts);
      this.asrFinalOnly =
        asrOnly ||
        toBool(payload.asrFinalOnly) ||
        toBool(payload.asr_final_only) ||
        toBool(payload.finalOnly) ||
        toBool(payload.final_only);
      this.sendToAsr({
        type: 'init',
        sample_rate: this.sampleRate,
        language: this.language,
      });
    } catch (error) {
      const detail = error instanceof ValidationError ? error.message : '初始化会话失败。';
      this.handleFatalViolation(detail);
    }
  }

  handleAudio(payload) {
    try {
      const validated = validateAudioPayload(payload, this.config);
      if (
        this.config.security.maxSessionAudioBytes &&
        Number.isFinite(validated.estimatedBytes)
      ) {
        this.sessionAudioBytes += validated.estimatedBytes;
        if (this.sessionAudioBytes > this.config.security.maxSessionAudioBytes) {
          throw new ValidationError('累计音频数据超出会话的安全上限。');
        }
      }

      // RNNoise 降噪处理（异步但不阻塞）
      const sampleRate = validated.sampleRate || this.sampleRate;
      this.processAudioWithRNNoise(validated.chunk, sampleRate, validated.isLast);
    } catch (error) {
      const detail =
        error instanceof ValidationError ? error.message : '接收音频分片时出现异常。';
      this.handleFatalViolation(detail);
    }
  }

  async processAudioWithRNNoise(chunk, sampleRate, isLast) {
    let audioChunk = chunk;

    // 尝试 RNNoise 降噪
    if (ENABLE_RNNOISE && sampleRate === 16000) {
      try {
        const rnnoise = await getRNNoiseInstance();
        if (rnnoise) {
          const pcmBuffer = Buffer.from(chunk, 'base64');
          const denoisedBuffer = rnnoise.processPCM16(pcmBuffer, sampleRate);
          audioChunk = denoisedBuffer.toString('base64');
        }
      } catch (err) {
        // 降噪失败时使用原始音频
        console.warn('[RNNoise] 处理失败，使用原始音频:', err.message);
      }
    }

    this.sendToAsr({
      type: 'chunk',
      audio: audioChunk,
      sample_rate: sampleRate,
      is_last: isLast,
    });
  }

  handleText(payload) {
    try {
      const { text } = validateTextPayload(payload, this.config);
      const sanitized = text.trim();
      if (!sanitized) {
        throw new ValidationError('文本消息不能为空。');
      }
      this.partialTranscript = '';
      this.history.push({ role: 'user', content: sanitized });
      this.logEvent({
        level: 'info',
        event: 'text.input',
        textLength: sanitized.length,
        transcript: this.logger.includeContent ? sanitized : undefined,
      });
      this.streamLLMResponse({ source: 'text', force: true });
    } catch (error) {
      const detail =
        error instanceof ValidationError ? error.message : '处理文本消息时出现异常。';
      if (error instanceof ValidationError) {
        this.sendError(detail);
        this.logEvent({
          level: 'warn',
          event: 'text.input.invalid',
          error: detail,
        });
      } else {
        this.handleFatalViolation(detail);
      }
    }
  }

  ensureAsrProcess() {
    // no-op: ASR 进程由共享 AsrWorker 统一管理
  }

  processAsrMessage(message) {
    if (message.type === 'transcript') {
      const rawText = typeof message.text === 'string' ? message.text : '';
      const rawDelta = typeof message.delta === 'string' ? message.delta : '';
      const isFinal = !!message.is_final;
      const maxTranscriptChars = this.config.security.maxTranscriptChars;
      const text = maxTranscriptChars ? rawText.slice(0, maxTranscriptChars) : rawText;
      const delta = maxTranscriptChars ? rawDelta.slice(0, maxTranscriptChars) : rawDelta;

      if (!isFinal && this.asrFinalOnly) {
        return;
      }

      const payload = {
        type: isFinal ? 'asr.final' : 'asr.partial',
        text,
      };
      if (!isFinal) {
        payload.delta = delta;
      }

      const comparisonText = typeof text === 'string' ? text.trim() : '';
      const now = Date.now();
      if (
        comparisonText &&
        comparisonText === this.lastAcceptedTranscript &&
        now - this.lastAcceptedAt < 1500
      ) {
        return;
      }

      this.sendToClient(payload);
      if (isFinal) {
        metrics.increment('asr_final_total');
        const autoMatch =
          this.autoFinalizedText &&
          comparisonText === this.autoFinalizedText &&
          now - this.autoFinalizedAt < 5000;
        if (autoMatch) {
          this.autoFinalizedText = '';
          this.autoFinalizedAt = 0;
          this.clearPartialTimer();
          this.partialTranscript = '';
          return;
        }

        if (comparisonText) {
          this.lastAcceptedTranscript = comparisonText;
          this.lastAcceptedAt = now;
        }

        this.clearPartialTimer();
        this.partialTranscript = '';
        this.handleFinalTranscript(text);
        this.autoFinalizedText = '';
        this.autoFinalizedAt = 0;
      } else {
        metrics.increment('asr_partial_total');
        this.partialTranscript = text;
        this.handleInterimTranscript(text);
        if (comparisonText) {
          this.lastAcceptedTranscript = comparisonText;
          this.lastAcceptedAt = now;
        }
      }

      this.logEvent({
        level: 'debug',
        event: 'asr.transcript',
        isFinal,
        transcriptLength: text.length,
        transcript: isFinal && this.logger.includeContent ? text : undefined,
      });
      return;
    }

    if (message.type === 'error') {
      const detail = message.detail || '语音识别失败';
      this.sendError(detail);
      this.logEvent({
        level: 'error',
        event: 'asr.error',
        error: detail,
      });
      metrics.increment('asr_error_total');
    }
  }

  handleAsrTranscript(message) {
    const safe = {
      type: 'transcript',
      text: message && typeof message.text === 'string' ? message.text : '',
      delta: message && typeof message.delta === 'string' ? message.delta : '',
      is_final: !!(message && message.is_final),
    };
    this.processAsrMessage(safe);
  }

  handleAsrError(detail) {
    const msg = detail || '语音识别失败';
    this.sendError(msg);
    this.logEvent({
      level: 'error',
      event: 'asr.error',
      error: msg,
    });
    metrics.increment('asr_error_total');
  }

  handleFinalTranscript(text) {
    const sanitized = typeof text === 'string' ? text.trim() : '';
    if (!sanitized) return;

    const lastMessage = this.history[this.history.length - 1];
    if (!lastMessage || lastMessage.role !== 'user' || lastMessage.content !== sanitized) {
      this.history.push({ role: 'user', content: sanitized });
    }

    this.lastAcceptedTranscript = sanitized;
    this.lastAcceptedAt = Date.now();

    this.logEvent({
      level: 'info',
      event: 'asr.final',
      transcriptLength: sanitized.length,
      transcript: this.logger.includeContent ? sanitized : undefined,
    });

    if (!this.disableLLM) {
      this.streamLLMResponse({ source: 'final', force: true });
    }
  }

  async streamLLMResponse(options = {}) {
    const { source = 'final', force = false } = options;
    const { deepseek } = this.config;
    if (!deepseek.apiKey) {
      this.sendError('服务器未配置 DEEPSEEK_API_KEY，无法生成回复。');
      return;
    }

    const lastMessage = this.history[this.history.length - 1];
    if (!lastMessage || lastMessage.role !== 'user') {
      return;
    }

    const textContent =
      typeof lastMessage.content === 'string' ? lastMessage.content.trim() : '';
    if (!textContent) {
      return;
    }

    if (
      !force &&
      lastMessage === this.lastStreamedUserMessage &&
      textContent === this.lastStreamedUserText
    ) {
      if (this.activeLLMStream) {
        return;
      }
      return;
    }

    this.abortLLMStream();

    const controller = new AbortController();
    this.activeLLMController = controller;

    const payload = {
      model: deepseek.model,
      messages: this.history.slice(-deepseek.maxHistory),
      temperature: deepseek.temperature,
      max_tokens: deepseek.maxTokens,
      stream: true,
    };

    try {
      const response = await axios({
        method: 'post',
        url: 'https://api.deepseek.com/chat/completions',
        headers: {
          Authorization: `Bearer ${deepseek.apiKey}`,
          'Content-Type': 'application/json',
        },
        data: payload,
        responseType: 'stream',
        signal: controller.signal,
        timeout: this.config.server.requestTimeoutMs,
      });

      this.currentAssistant = '';
      this.lastStreamedUserMessage = lastMessage;
      this.lastStreamedUserText = textContent;
      // Reset stage-direction stripping state for the new assistant reply (TTS only).
      this.ttsStripInParen = false;
      this.ttsStripParenClose = '';
      this.sendToClient({ type: 'llm.start', source });

      let buffer = '';
      response.data.on('data', (chunk) => {
        buffer += chunk.toString();
        const lines = buffer.split('\n');
        buffer = lines.pop() ?? '';
        for (const raw of lines) {
          const line = raw.trim();
          if (!line.startsWith('data:')) continue;
          const data = line.slice(5).trim();
          if (!data || data === '[DONE]') {
            // LLM 流式输出结束，通知 CosyVoice 完成
            if (this.ttsActive && this.config.tts?.provider === 'cosyvoice') {
              this.finalizeCosyVoiceStreaming();
            }
            this.finishLLMStream();
            return;
          }
          try {
            const parsed = JSON.parse(data);
            const delta = parsed?.choices?.[0]?.delta?.content;
            if (delta) {
              this.currentAssistant += delta;
              this.sendToClient({ type: 'llm.delta', delta });
              this.maybeStreamTTS(delta);
            }
          } catch (error) {
            console.error('解析 DeepSeek 流式数据失败：', error.message);
          }
        }
      });

      response.data.on('end', () => {
        // 如果 CosyVoice 流式 TTS 正在进行，通知它 LLM 已完成
        if (this.ttsActive && this.config.tts?.provider === 'cosyvoice') {
          this.finalizeCosyVoiceStreaming();
        }
        this.finishLLMStream();
      });

      response.data.on('error', (error) => {
        if (axios.isCancel(error)) return;
        this.sendError(`DeepSeek 流式输出失败：${error.message}`);
        this.logEvent({
          level: 'error',
          event: 'llm.stream.error',
          error: error.message,
        });
        metrics.increment('llm_error_total');
        if (this.lastStreamedUserMessage === lastMessage) {
          this.lastStreamedUserMessage = null;
          this.lastStreamedUserText = '';
        }
        this.abortLLMStream();
      });

      this.activeLLMStream = response.data;
    } catch (error) {
      if (axios.isCancel(error)) return;
      const detail =
        error?.response?.data?.error?.message ||
        error?.message ||
        '调用 DeepSeek API 失败';
      this.sendError(detail);
      this.logEvent({
        level: 'error',
        event: 'llm.stream.error',
        error: detail,
      });
      metrics.increment('llm_error_total');
      if (this.lastStreamedUserMessage === lastMessage) {
        this.lastStreamedUserMessage = null;
        this.lastStreamedUserText = '';
      }
    }
  }

  textHasBoundary(text) {
    // 简单边界：中文句号/问号/叹号、英文句点/问号/叹号、换行、分号、逗号（弱边界）
    return /[。！？!?\n]/.test(text);
  }

  sanitizeTtsText(text) {
    const raw = typeof text === 'string' ? text : String(text || '');
    if (!raw) return '';
    if (!this.config?.tts?.stripStageDirections) {
      return raw;
    }
    // 处理舞台提示/旁白括号：支持全角（...）与半角(...)
    const stripped = raw
      .replace(/（[^）]*）/g, '')
      .replace(/\([^)]*\)/g, '')
      .replace(/\s+/g, ' ')
      .trim();
    return stripped;
  }

  sanitizeTtsDelta(delta) {
    const raw = typeof delta === 'string' ? delta : String(delta || '');
    if (!raw) return '';
    if (!this.config?.tts?.stripStageDirections) {
      return raw;
    }
    // 流式场景：括号可能跨 delta；用会话级状态机过滤（...） / (...)
    let out = '';
    for (const ch of raw) {
      if (!this.ttsStripInParen && (ch === '（' || ch === '(')) {
        this.ttsStripInParen = true;
        this.ttsStripParenClose = ch === '（' ? '）' : ')';
        continue;
      }
      if (this.ttsStripInParen) {
        if (ch === this.ttsStripParenClose || ch === '）' || ch === ')') {
          this.ttsStripInParen = false;
          this.ttsStripParenClose = '';
        }
        continue;
      }
      out += ch;
    }
    return out;
  }

  escapeXml(text) {
    return String(text)
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;')
      .replace(/'/g, '&apos;');
  }

  normalizeSsmlText(text) {
    const raw = typeof text === 'string' ? text.trim() : '';
    if (!raw) return '';
    // 如果已经包含 <speak ...>，认为是用户提供的完整 SSML，直接透传。
    if (/<\s*speak[\s>]/i.test(raw)) {
      return raw;
    }
    // 否则自动包装为最基础的 SSML（并对 XML 特殊字符转义），避免 enable_ssml 打开后报错。
    return `<speak>${this.escapeXml(raw)}</speak>`;
  }

  ensureCosyVoiceStarted(initialText) {
    if (this.ttsActive) return true;
    if (this.config.tts.provider !== 'cosyvoice') return false;
    // 启动 CosyVoice，会立即向客户端发送 tts.start 和 tts.meta
    this.startCosyVoiceTTS(String(initialText || ''));
    return this.ttsActive;
  }

  sendCosyVoiceContinue(text) {
    if (!text) return;
    if (!this.ttsWs || !this.ttsActive || !this.ttsTaskId) return;
    try {
      const continueTask = {
        header: { action: 'continue-task', task_id: this.ttsTaskId },
        payload: {
          task_group: 'audio',
          task: 'tts',
          function: 'SpeechSynthesizer',
          input: { text },
        },
      };
      this.ttsWs.send(JSON.stringify(continueTask));
      this.lastTtsSendAt = Date.now();
    } catch (error) {
      console.error('CosyVoice 追加文本发送失败：', error.message);
    }
  }

  sendCosyVoiceFinish() {
    if (!this.ttsWs || !this.ttsActive || !this.ttsTaskId) {
      return;
    }
    const finishTask = {
      header: { action: 'finish-task', task_id: this.ttsTaskId, streaming: 'duplex' },
      payload: { input: {} },
    };
    this.ttsWs.send(JSON.stringify(finishTask));
  }

  // 流式 TTS：启动 CosyVoice WebSocket 连接（不带初始文本）
  // 注意：用于 LLM 边生成边播报的双向流式（PlainText）。SSML 不支持该模式。
  startCosyVoiceStreaming(initialQueuedText = '') {
    const cosy = this.config.tts?.cosyvoice;
    if (!cosy || !cosy.apiKey) {
      this.sendToClient({
        type: 'tts.error',
        detail: '服务器未配置 CosyVoice API Key。',
      });
      return false;
    }
    if (Boolean(cosy.enableSSML)) {
      this.sendToClient({
        type: 'tts.error',
        detail: 'SSML 模式下（enable_ssml=true）WebSocket 仅允许发送一次 continue-task，不支持双向流式播报。',
      });
      return false;
    }

    this.abortTTS();
    this.ttsActive = true;
    this.ttsReady = false;
    this.ttsTextQueue = [];
    this.ttsLlmFinished = false;
    this.ttsTaskId = uuidv4();
    this.ttsStartedAt = Date.now();
    metrics.increment('tts_requests_total');

    const initialText = typeof initialQueuedText === 'string' ? initialQueuedText : '';
    if (initialText) {
      // 关键：不要丢掉触发启动时已累计的第一段文本（startCosyVoiceStreaming 内部会 abortTTS）。
      this.ttsTextQueue.push(initialText);
    }

    this.sendToClient({
      type: 'tts.start',
      voice: cosy.voice || null,
    });
    this.sendToClient({
      type: 'tts.meta',
      sampleRate: cosy.sampleRate || 24000,
      format: cosy.format || 'pcm',
    });

    this.setupCosyVoicePacer(cosy);

    const headers = {
      Authorization: `bearer ${cosy.apiKey}`,
      'X-DashScope-DataInspection': 'enable',
    };

    const ws = new WebSocket('wss://dashscope.aliyuncs.com/api-ws/v1/inference', {
      headers,
    });
    this.ttsWs = ws;

    ws.on('open', () => {
      try {
        const runTask = {
          header: {
            action: 'run-task',
            task_id: this.ttsTaskId,
            streaming: 'duplex',
          },
          payload: {
            task_group: 'audio',
            task: 'tts',
            function: 'SpeechSynthesizer',
            model: cosy.model || 'cosyvoice-v3-plus',
            parameters: {
              text_type: 'PlainText',
              voice: cosy.voice,
              format: cosy.format || 'pcm',
              sample_rate: cosy.sampleRate || 24000,
              volume: cosy.volume !== undefined && cosy.volume !== null ? cosy.volume : 50,
              rate: cosy.rate || 1,
              pitch: cosy.pitch || 1,
              bit_rate: cosy.bitRate || undefined,
              enable_ssml: Boolean(cosy.enableSSML),
              word_timestamp_enabled: Boolean(cosy.wordTimestamps),
              language_hints: cosy.languageHints || undefined,
              instruction: cosy.instruction || undefined,
            },
            input: {},
          },
        };
        ws.send(JSON.stringify(runTask));
        console.log('[CosyVoice 流式] WebSocket 已连接，发送 run-task');
      } catch (error) {
        console.error('[CosyVoice 流式] run-task 发送失败：', error.message);
        this.sendToClient({ type: 'tts.error', detail: `CosyVoice 连接失败：${error.message}` });
        this.abortTTS();
        metrics.increment('tts_error_total');
        this.maybeFallbackToLocal(error.message);
      }
    });

    ws.on('message', (data, isBinary) => {
      if (!this.ttsActive || this.ttsWs !== ws) return;

      if (isBinary) {
        const chunk = Buffer.isBuffer(data) ? data : Buffer.from(data);
        if (chunk.length) {
          this.forwardCosyVoiceAudioChunk(chunk, cosy);
        }
        return;
      }

      let message;
      try {
        const textPayload = Buffer.isBuffer(data) ? data.toString('utf8') : String(data);
        message = JSON.parse(textPayload);
      } catch (error) {
        console.error('[CosyVoice 流式] 解析事件失败：', error.message);
        return;
      }

      // 兼容：部分 CosyVoice 返回将音频以 base64 内嵌在 JSON 中（非二进制帧）。
      // 尝试从 JSON 中提取音频字段并按统一通道转发。
      try {
        const b64s = [];
        const visit = (obj) => {
          if (!obj || typeof obj !== 'object') return;
          for (const [k, v] of Object.entries(obj)) {
            if (v && typeof v === 'string' && /^(audio|audio_data|pcm|data)$/i.test(k)) {
              if (/^[A-Za-z0-9+/=]+$/.test(v) && v.length > 32) {
                b64s.push(v);
              }
            } else if (v && typeof v === 'object') {
              visit(v);
            }
          }
        };
        visit(message?.payload);
        if (b64s.length) {
          for (const b64 of b64s) {
            this.forwardCosyVoiceAudioBase64(b64, cosy);
          }
        }
      } catch (err) {
        console.warn('[CosyVoice 流式] 提取 JSON 音频失败：', err?.message || err);
      }

      this.handleCosyVoiceStreamingEvent({ message, ws, cosy });
    });

    ws.on('error', (error) => {
      if (!this.ttsActive) return;
      console.error('[CosyVoice 流式] WebSocket 错误：', error.message);
      this.sendToClient({ type: 'tts.error', detail: `CosyVoice 连接错误：${error.message}` });
      this.abortTTS();
      metrics.increment('tts_error_total');
      this.maybeFallbackToLocal(error.message);
    });

    ws.on('close', () => {
      if (this.ttsWs === ws) {
        this.ttsWs = null;
        this.ttsTaskId = null;
        this.ttsReady = false;
        if (!this.ttsAwaitingDrain) {
          this.sendToClient({ type: 'tts.error', detail: 'CosyVoice 连接已关闭。' });
          metrics.increment('tts_error_total');
          this.abortTTS();
        }
      }
    });

    return true;
  }

  // 处理流式 TTS 的 CosyVoice 事件
  handleCosyVoiceStreamingEvent({ message, ws, cosy }) {
    if (!message) return;
    const event = message?.header?.event;
    if (!event) return;

    switch (event) {
      case 'task-started': {
        console.log('[CosyVoice 流式] task-started，开始发送队列中的文本');
        this.ttsReady = true;

        // 清空队列，发送所有缓存的文本
        while (this.ttsTextQueue.length > 0) {
          const chunk = this.ttsTextQueue.shift();
          if (chunk) {
            this.sendCosyVoiceContinue(chunk);
          }
        }

        // 如果 LLM 已经结束，发送 finish-task
        if (this.ttsLlmFinished) {
          // 发送残留的 pending 文本
          if (this.ttsPending) {
            this.sendCosyVoiceContinue(this.ttsPending);
            this.ttsPending = '';
          }
          this.sendCosyVoiceFinish();
          console.log('[CosyVoice 流式] LLM 已结束，发送 finish-task');
        }
        break;
      }
      case 'result-generated': {
        const usage = message?.payload?.usage?.characters;
        if (Number.isFinite(usage)) {
          this.logEvent({ level: 'debug', event: 'tts.usage', characters: usage });
        }
        break;
      }
      case 'task-finished': {
        console.log('[CosyVoice 流式] task-finished');
        const words = message?.payload?.output?.sentence?.words;
        if (Boolean(cosy.wordTimestamps) && Array.isArray(words) && words.length) {
          this.sendToClient({ type: 'tts.timestamps', words });
        }
        this.finishCosyVoiceAudio({ ok: true });
        break;
      }
      case 'task-failed': {
        const detail =
          message?.header?.error_message || message?.payload?.error_message || 'CosyVoice 任务失败';
        console.error('[CosyVoice 流式] task-failed：', detail);
        this.sendToClient({ type: 'tts.error', detail });
        this.logEvent({ level: 'error', event: 'tts.error', error: detail });
        metrics.increment('tts_error_total');
        this.abortTTS();
        this.maybeFallbackToLocal(detail);
        break;
      }
      default:
        break;
    }
  }

  // LLM 输出结束时调用，通知流式 TTS 完成
  finalizeCosyVoiceStreaming() {
    this.ttsLlmFinished = true;

    // 发送残留的 pending 文本
    if (this.ttsPending) {
      if (this.ttsReady) {
        this.sendCosyVoiceContinue(this.ttsPending);
      } else {
        this.ttsTextQueue.push(this.ttsPending);
      }
      this.ttsPending = '';
    }

    // 如果 WebSocket 已就绪，发送 finish-task
    if (this.ttsReady && this.ttsWs && this.ttsActive) {
      // 先清空队列
      while (this.ttsTextQueue.length > 0) {
        const chunk = this.ttsTextQueue.shift();
        if (chunk) {
          this.sendCosyVoiceContinue(chunk);
        }
      }
      this.sendCosyVoiceFinish();
    }
    // 如果 WebSocket 尚未就绪，task-started 事件会处理 finish-task
  }

  maybeStreamTTS(delta) {
    const { tts } = this.config;
    if (!tts.enabled) return;
    if (this.disableTTS) return;
    if (tts.provider !== 'cosyvoice') return; // 仅对 CosyVoice 做流式合成
    // SSML 模式下（enable_ssml=true）WebSocket 仅允许发送一次 continue-task，不支持边生成边播报的双向流式。
    if (Boolean(this.config.tts?.cosyvoice?.enableSSML)) return;

    const now = Date.now();
    this.ttsPending += this.sanitizeTtsDelta(delta);

    const boundary = this.textHasBoundary(this.ttsPending);
    const elapsed = now - this.lastTtsSendAt;

    // 未启动 CosyVoice：累计足够文本后启动 WebSocket 连接
    if (!this.ttsActive) {
      const enough = boundary || this.ttsPending.length >= this.minTtsStartChars || elapsed > 500;
      if (enough) {
        // 启动 CosyVoice WebSocket（不传文本，文本会通过队列发送）
        const initialQueuedText = this.ttsPending;
        this.ttsPending = '';
        this.startCosyVoiceStreaming(initialQueuedText);
        this.lastTtsSendAt = now;
      }
      return;
    }

    // 已启动：判断是否达到发送阈值
    const enoughContinue = boundary || this.ttsPending.length >= this.minTtsContinueChars || elapsed > 480;
    if (enoughContinue && this.ttsPending) {
      const chunk = this.ttsPending;
      this.ttsPending = '';

      if (this.ttsReady) {
        // WebSocket 就绪，直接发送
        this.sendCosyVoiceContinue(chunk);
      } else {
        // WebSocket 未就绪，加入队列
        this.ttsTextQueue.push(chunk);
      }
      this.lastTtsSendAt = now;
    }
  }

  finishLLMStream() {
    if (!this.currentAssistant) {
      this.abortLLMStream({ silent: true });
      return;
    }

    let responseText = this.currentAssistant;
    const maxAssistantChars = this.config.security.maxAssistantChars;
    if (maxAssistantChars && responseText.length > maxAssistantChars) {
      responseText = responseText.slice(0, maxAssistantChars);
      this.sendError('助手回复超出长度限制，已自动截断。');
    }
    this.history.push({ role: 'assistant', content: responseText });
    this.sendToClient({ type: 'llm.final', text: responseText });

    this.logEvent({
      level: 'info',
      event: 'llm.stream.success',
      replyLength: responseText.length,
      reply: this.logger.includeContent ? responseText : undefined,
    });
    metrics.increment('llm_success_total');

    this.currentAssistant = '';
    this.abortLLMStream({ silent: true });
    this.beginTTS(responseText);
  }

  beginTTS(text) {
    if (!text) return;
    if (this.disableTTS) return;
    const { tts } = this.config;
    if (!tts.enabled) return;

    // 如果 CosyVoice 流式 TTS 正在进行，不再重新启动（已由 maybeStreamTTS 处理）
    if (tts.provider === 'cosyvoice' && this.ttsActive) {
      return;
    }

    // 非 CosyVoice 或 CosyVoice 未启动（如纯文本输入）时，使用传统方式
    this.abortTTS();
    const ttsText = this.sanitizeTtsText(text);
    if (!ttsText) {
      return;
    }
    this.currentTtsText = ttsText;

    if (tts.provider === 'cosyvoice') {
      this.startCosyVoiceTTS(ttsText);
    } else {
      this.ttsActive = true;
      this.ttsStartedAt = Date.now();
      metrics.increment('tts_requests_total');
      this.startLocalTTS(ttsText);
    }
  }

  startLocalTTS(text) {
    try {
      const { tts } = this.config;

      let scriptExists = false;
      try {
        scriptExists = tts.script && fs.existsSync(tts.script);
      } catch (fsError) {
        console.error('检查 TTS 脚本失败：', fsError);
        scriptExists = false;
      }

      if (!tts.script || !scriptExists) {
        this.sendToClient({
          type: 'tts.error',
          detail: '服务器未配置语音合成脚本。',
        });
        metrics.increment('tts_error_total');
        this.abortTTS();
        return;
      }

      this.sendToClient({
        type: 'tts.start',
        voice: tts.voice || null,
      });

      this.ttsProcess = spawn(tts.python, [tts.script], {
        env: {
          ...process.env,
          PYTHONUNBUFFERED: '1',
        },
      });

      this.ttsProcess.stdout.setEncoding('utf8');
      this.ttsProcess.stdout.on('data', (chunk) => {
        this.handleTTSStdout(chunk);
      });
      this.ttsProcess.stderr.setEncoding('utf8');
      this.ttsProcess.stderr.on('data', (chunk) => {
        const textChunk = chunk.trim();
        if (textChunk) {
          console.warn('TTS 脚本标准错误输出：', textChunk);
        }
      });
      this.ttsProcess.on('exit', (code, signal) => {
        if (this.ttsActive && !this.isClosed) {
          this.sendToClient({
            type: 'tts.error',
            detail: `TTS 进程退出：code=${code}, signal=${signal}`,
          });
          metrics.increment('tts_error_total');
        }
        this.ttsProcess = null;
        this.ttsStdoutBuffer = '';
        this.ttsActive = false;
      });

      const payload = {
        session_id: this.id,
        text,
        voice: tts.voice || undefined,
        rate: tts.rate || undefined,
        volume: tts.volume || undefined,
        chunk_ms: tts.chunkMs,
      };

      try {
        this.ttsProcess.stdin.write(`${JSON.stringify(payload)}\n`);
        this.ttsProcess.stdin.end();
      } catch (error) {
        this.sendToClient({
          type: 'tts.error',
          detail: `写入 TTS 进程失败：${error.message}`,
        });
        metrics.increment('tts_error_total');
        this.abortTTS();
      }
    } catch (outerError) {
      console.error('TTS 启动失败：', outerError);
      this.sendToClient({
        type: 'tts.error',
        detail: `TTS 启动失败：${outerError.message}`,
      });
      metrics.increment('tts_error_total');
      this.abortTTS();
    }
  }

  maybeFallbackToLocal(detail) {
    const fallback = this.config.tts?.fallbackProvider;
    if (!fallback || fallback !== 'local') {
      return false;
    }
    if (!this.currentTtsText) {
      return false;
    }
    console.warn('CosyVoice 调用失败，尝试降级本地 TTS：', detail || '未知原因');
    metrics.increment('tts_fallback_total');
    this.ttsStartedAt = Date.now();
    this.ttsActive = true;
    this.startLocalTTS(this.currentTtsText);
    return true;
  }

  startCosyVoiceTTS(text) {
    const cosy = this.config.tts?.cosyvoice;
    if (!cosy || !cosy.apiKey) {
      this.sendToClient({
        type: 'tts.error',
        detail: '服务器未配置 CosyVoice API Key。',
      });
      return;
    }

    this.abortTTS();
    this.ttsActive = true;
    this.ttsStartedAt = Date.now();
    metrics.increment('tts_requests_total');
    this.ttsTaskId = uuidv4();
    this.sendToClient({
      type: 'tts.start',
      voice: cosy.voice || null,
    });
    this.sendToClient({
      type: 'tts.meta',
      sampleRate: cosy.sampleRate || 24000,
      format: cosy.format || 'pcm',
    });

    this.setupCosyVoicePacer(cosy);

    const headers = {
      Authorization: `bearer ${cosy.apiKey}`,
      'X-DashScope-DataInspection': 'enable',
    };

    const ws = new WebSocket('wss://dashscope.aliyuncs.com/api-ws/v1/inference', {
      headers,
    });
    this.ttsWs = ws;

    const cleanup = () => {
      if (this.ttsWs === ws) {
        this.ttsWs = null;
        this.ttsTaskId = null;
      }
    };

    ws.on('open', () => {
      try {
        const runTask = {
          header: {
            action: 'run-task',
            task_id: this.ttsTaskId,
            streaming: 'duplex',
          },
          payload: {
            task_group: 'audio',
            task: 'tts',
            function: 'SpeechSynthesizer',
            model: cosy.model || 'cosyvoice-v3-plus',
            parameters: {
              text_type: 'PlainText',
              voice: cosy.voice,
              format: cosy.format || 'pcm',
              sample_rate: cosy.sampleRate || 24000,
              volume:
                cosy.volume !== undefined && cosy.volume !== null ? cosy.volume : 50,
              rate: cosy.rate || 1,
              pitch: cosy.pitch || 1,
              bit_rate: cosy.bitRate || undefined,
              enable_ssml: Boolean(cosy.enableSSML),
              word_timestamp_enabled: Boolean(cosy.wordTimestamps),
              language_hints: cosy.languageHints || undefined,
              instruction: cosy.instruction || undefined,
            },
            input: {},
          },
        };
        ws.send(JSON.stringify(runTask));
      } catch (error) {
        console.error('CosyVoice run-task 发送失败：', error.message);
        this.sendToClient({ type: 'tts.error', detail: `CosyVoice 连接失败：${error.message}` });
        this.abortTTS();
        metrics.increment('tts_error_total');
        this.maybeFallbackToLocal(error.message);
      }
    });

    ws.on('message', (data, isBinary) => {
      if (!this.ttsActive || this.ttsWs !== ws) return;

      if (isBinary) {
        const chunk = Buffer.isBuffer(data) ? data : Buffer.from(data);
        if (chunk.length) {
          this.forwardCosyVoiceAudioChunk(chunk, cosy);
        }
        return;
      }

      let message;
      try {
        const textPayload = Buffer.isBuffer(data) ? data.toString('utf8') : String(data);
        message = JSON.parse(textPayload);
      } catch (error) {
        console.error('解析 CosyVoice 事件失败：', error.message);
        return;
      }
      // 兼容：部分 CosyVoice 返回将音频以 base64 内嵌在 JSON 中（非二进制帧）。
      // 尝试从 JSON 中提取名为 audio/audio_data 等字段的 base64 内容并转发为 tts.chunk。
      try {
        const b64s = [];
        const visit = (obj) => {
          if (!obj || typeof obj !== 'object') return;
          for (const [k, v] of Object.entries(obj)) {
            if (v && typeof v === 'string' && /^(audio|audio_data|pcm|data)$/i.test(k)) {
              if (/^[A-Za-z0-9+/=]+$/.test(v) && v.length > 32) {
                b64s.push(v);
              }
            } else if (v && typeof v === 'object') {
              visit(v);
            }
          }
        };
        visit(message?.payload);
        if (b64s.length) {
          for (const b64 of b64s) {
            this.forwardCosyVoiceAudioBase64(b64, cosy);
          }
        }
      } catch (err) {
        console.warn('提取 CosyVoice JSON 音频失败：', err?.message || err);
      }

      this.handleCosyVoiceEvent({ message, text, ws, cosy });
    });

    ws.on('error', (error) => {
      if (!this.ttsActive) return;
      console.error('CosyVoice WebSocket 错误：', error.message);
      this.sendToClient({ type: 'tts.error', detail: `CosyVoice 连接错误：${error.message}` });
      this.abortTTS();
      metrics.increment('tts_error_total');
      this.maybeFallbackToLocal(error.message);
    });

    ws.on('close', () => {
      if (this.ttsWs !== ws) return;
      cleanup();
      if (!this.ttsAwaitingDrain) {
        this.sendToClient({ type: 'tts.error', detail: 'CosyVoice 连接已关闭。' });
        metrics.increment('tts_error_total');
        this.abortTTS();
      }
    });
  }

  handleCosyVoiceEvent({ message, text, ws, cosy }) {
    if (!message) return;
    const event = message?.header?.event;
    if (!event) return;

    switch (event) {
      case 'task-started': {
        const trimmed = typeof text === 'string' ? text.trim() : '';
        if (!trimmed) {
          this.sendToClient({
            type: 'tts.error',
            detail: 'CosyVoice 任务失败：待合成文本为空。',
          });
          this.logEvent({ level: 'error', event: 'tts.error', error: 'empty_text' });
          metrics.increment('tts_error_total');
          this.abortTTS();
          this.maybeFallbackToLocal('empty_text');
          return;
        }

        const isSsmlEnabled = Boolean(cosy.enableSSML);
        const payloadText = isSsmlEnabled ? this.normalizeSsmlText(trimmed) : trimmed;
        const chunks = isSsmlEnabled ? [payloadText] : this.chunkCosyVoiceText(payloadText);
        for (const chunk of chunks) {
          console.debug('CosyVoice continue chunk length:', chunk.length, chunk.slice(0, 32));
          const continueTask = {
            header: {
              action: 'continue-task',
              task_id: this.ttsTaskId,
              streaming: 'duplex',
            },
            payload: {
              input: {
                text: chunk,
              },
            },
          };
          ws.send(JSON.stringify(continueTask), (error) => {
            if (error) {
              console.error('CosyVoice continue-task 发送失败：', error.message);
            }
          });
        }
        const finishTask = {
          header: {
            action: 'finish-task',
            task_id: this.ttsTaskId,
            streaming: 'duplex',
          },
          payload: {
            input: {},
          },
        };
        ws.send(JSON.stringify(finishTask), (error) => {
          if (error) {
            console.error('CosyVoice finish-task 发送失败：', error.message);
          }
        });
        break;
      }
      case 'result-generated': {
        const usage = message?.payload?.usage?.characters;
        if (Number.isFinite(usage)) {
          this.logEvent({
            level: 'debug',
            event: 'tts.usage',
            characters: usage,
          });
        }
        break;
      }
      case 'task-finished': {
        const words = message?.payload?.output?.sentence?.words;
        if (Boolean(cosy.wordTimestamps) && Array.isArray(words) && words.length) {
          this.sendToClient({ type: 'tts.timestamps', words });
        }
        this.finishCosyVoiceAudio({ ok: true });
        break;
      }
      case 'task-failed': {
        const detail =
          message?.header?.error_message || message?.payload?.error_message || 'CosyVoice 任务失败';
        this.sendToClient({ type: 'tts.error', detail });
        this.logEvent({ level: 'error', event: 'tts.error', error: detail });
        metrics.increment('tts_error_total');
        this.abortTTS();
        this.maybeFallbackToLocal(detail);
        break;
      }
      default:
        break;
    }
  }

  chunkCosyVoiceText(text) {
    const limit = 2000;
    if (!text || text.length <= limit) {
      return [text];
    }
    const chunks = [];
    let start = 0;
    while (start < text.length) {
      chunks.push(text.slice(start, start + limit));
      start += limit;
    }
    return chunks;
  }

  handleTTSStdout(chunk) {
    this.ttsStdoutBuffer += chunk;
    let newlineIndex = this.ttsStdoutBuffer.indexOf('\n');
    while (newlineIndex >= 0) {
      const line = this.ttsStdoutBuffer.slice(0, newlineIndex).trim();
      this.ttsStdoutBuffer = this.ttsStdoutBuffer.slice(newlineIndex + 1);
      if (line) {
        // 跳过非 JSON 行（如 pyttsx3 的调试输出）
        if (!line.startsWith('{')) {
          newlineIndex = this.ttsStdoutBuffer.indexOf('\n');
          continue;
        }
        try {
          const message = JSON.parse(line);
          this.processTTSMessage(message);
        } catch (error) {
          console.error('解析 TTS 输出失败：', error.message, 'Line:', line.substring(0, 200));
        }
      }
      newlineIndex = this.ttsStdoutBuffer.indexOf('\n');
    }
  }

  processTTSMessage(message) {
    if (!message) return;
    switch (message.type) {
      case 'meta':
        this.sendToClient({
          type: 'tts.meta',
          sampleRate: message.sample_rate,
          channels: message.channels,
          voice: message.voice || null,
        });
        break;
      case 'audio_chunk':
        this.sendToClient({
          type: 'tts.chunk',
          chunk: message.chunk,
          sampleRate: message.sample_rate,
          isLast: !!message.is_last,
        });
        break;
      case 'complete':
        this.sendToClient({
          type: 'tts.complete',
          duration: message.duration,
        });
        this.logEvent({
          level: 'info',
          event: 'tts.success',
          duration: message.duration,
        });
        metrics.increment('tts_success_total');
        if (this.ttsStartedAt) {
          metrics.observe('tts_latency_ms', Date.now() - this.ttsStartedAt);
        }
        this.ttsStartedAt = null;
        this.abortTTS();
        break;
      case 'error':
        this.sendToClient({
          type: 'tts.error',
          detail: message.detail || 'TTS 失败',
        });
        this.logEvent({
          level: 'error',
          event: 'tts.error',
          error: message.detail || 'TTS 失败',
        });
        metrics.increment('tts_error_total');
        this.ttsStartedAt = null;
        this.abortTTS();
        break;
      default:
        break;
    }
  }

  sendToAsr(payload) {
    if (!payload) return;
    const worker = getAsrWorker();
    worker.bindSession(this.id, this);
    const enriched = { ...payload, session_id: this.id };
    worker.send(enriched);
  }

  abortLLMStream(options = {}) {
    const { silent = false } = options;
    const hadActiveStream =
      !!this.activeLLMController ||
      (this.activeLLMStream && !this.activeLLMStream.destroyed);
    if (this.activeLLMController) {
      this.activeLLMController.abort();
      this.activeLLMController = null;
    }
    if (this.activeLLMStream && !this.activeLLMStream.destroyed) {
      try {
        this.activeLLMStream.destroy();
      } catch (error) {
        console.warn('中止 LLM 流失败：', error.message);
      }
    }
    this.activeLLMStream = null;
    this.currentAssistant = '';
    if (hadActiveStream && !silent) {
      this.sendToClient({ type: 'llm.abort' });
    }
  }

  abortTTS() {
    this.ttsActive = false;
    this.ttsStartedAt = null;
    this.ttsAwaitingDrain = false;
    if (this.ttsPacer) {
      try {
        this.ttsPacer.abort();
      } catch (error) {
        console.warn('停止 TTS 节流器失败：', error.message);
      }
      this.ttsPacer = null;
    }
    this.ttsReady = false;
    this.ttsTextQueue = [];
    this.ttsLlmFinished = false;
    this.ttsPending = '';
    if (this.ttsWs) {
      try {
        this.ttsWs.close();
      } catch (error) {
        console.warn('关闭 CosyVoice WebSocket 失败：', error.message);
      }
      this.ttsWs = null;
    }
    this.ttsTaskId = null;
    if (this.ttsProcess) {
      try {
        this.ttsProcess.kill();
      } catch (error) {
        console.error('终止 TTS 进程失败：', error.message);
      }
      this.ttsProcess = null;
    }
    this.ttsStdoutBuffer = '';
  }

  cosyVoicePaceEnabled(cosy) {
    const format = String(cosy?.format || 'pcm').toLowerCase();
    const pace = cosy?.pace;
    return format === 'pcm' && Boolean(pace?.enabled);
  }

  setupCosyVoicePacer(cosy) {
    this.ttsAwaitingDrain = false;
    if (!this.cosyVoicePaceEnabled(cosy)) {
      this.ttsPacer = null;
      return;
    }
    const sampleRate = cosy.sampleRate || 24000;
    const pace = cosy.pace || {};
    this.ttsPacer = new PcmPacer({
      sampleRate,
      chunkMs: pace.chunkMs || 40,
      prebufferMs: pace.prebufferMs || 120,
      maxBufferMs: pace.maxBufferMs || 60_000,
      label: `cosyvoice:${cosy.model || 'default'}`,
      canSend: () => {
        if (!this.socket || this.socket.readyState !== WebSocket.OPEN) return false;
        const buffered = typeof this.socket.bufferedAmount === 'number' ? this.socket.bufferedAmount : 0;
        // 避免 Node 侧发送缓存无限堆积；超过阈值则暂停本次 tick，等待网络/客户端消化。
        return buffered < 512 * 1024;
      },
      onChunk: (pcmChunk) => {
        this.sendToClient({
          type: 'tts.chunk',
          chunk: pcmChunk.toString('base64'),
          sampleRate,
        });
      },
      onDrop: ({ droppedBytes, bufferedBytes, label }) => {
        console.warn(
          `[TTS Pace] buffer overflow, dropped ${droppedBytes} bytes, remain ${bufferedBytes} bytes (${label})`
        );
        metrics.increment('tts_pace_dropped_total');
      },
      onDrain: () => {
        this.sendToClient({ type: 'tts.complete' });
        if (this.ttsStartedAt) {
          metrics.observe('tts_latency_ms', Date.now() - this.ttsStartedAt);
        }
        metrics.increment('tts_success_total');
        this.abortTTS();
      },
    });
  }

  forwardCosyVoiceAudioChunk(chunk, cosy) {
    if (!chunk || !chunk.length) return;
    if (this.ttsPacer) {
      this.ttsPacer.push(chunk);
      return;
    }
    this.sendToClient({
      type: 'tts.chunk',
      chunk: chunk.toString('base64'),
      sampleRate: cosy.sampleRate || 24000,
    });
  }

  forwardCosyVoiceAudioBase64(base64, cosy) {
    if (!base64) return;
    if (this.ttsPacer) {
      try {
        const buf = Buffer.from(base64, 'base64');
        if (buf.length) this.ttsPacer.push(buf);
      } catch (error) {
        console.warn('CosyVoice base64 音频解码失败：', error.message);
      }
      return;
    }
    this.sendToClient({
      type: 'tts.chunk',
      chunk: base64,
      sampleRate: cosy.sampleRate || 24000,
    });
  }

  finishCosyVoiceAudio({ ok }) {
    if (!ok) {
      this.abortTTS();
      return;
    }
    if (this.ttsPacer) {
      this.ttsAwaitingDrain = true;
      this.ttsPacer.end();
      return;
    }
    this.sendToClient({ type: 'tts.complete' });
    if (this.ttsStartedAt) {
      metrics.observe('tts_latency_ms', Date.now() - this.ttsStartedAt);
    }
    metrics.increment('tts_success_total');
    this.abortTTS();
  }

  sendToClient(message) {
    if (this.socket.readyState === WebSocket.OPEN) {
      this.socket.send(JSON.stringify(message));
    }
  }

  handleFatalViolation(detail) {
    const message = detail || '请求违反安全策略。';
    this.sendError(message);
    this.logEvent({
      level: 'warn',
      event: 'session.policy_violation',
      error: message,
    });
    this.closeWithPolicyViolation(message);
  }

  sendError(detail) {
    this.sendToClient({ type: 'error', detail });
  }

  logEvent(event) {
    if (!this.logger || typeof this.logger.record !== 'function') return;
    const entry = {
      requestId: this.id,
      ip: this.logger.anonymizeIp(this.request.socket.remoteAddress),
      userAgent: this.request.headers['user-agent']?.slice(0, 200),
      timestamp: new Date().toISOString(),
      ...event,
    };
    if (!this.logger.includeContent) {
      delete entry.transcript;
      delete entry.reply;
    }
    this.logger.record(entry);
  }

  closeWithPolicyViolation(reason) {
    if (
      this.socket.readyState === WebSocket.OPEN ||
      this.socket.readyState === WebSocket.CONNECTING
    ) {
      try {
        this.socket.close(1008, (reason || 'Policy violation').slice(0, 120));
      } catch (error) {
        console.error('关闭会话时出错：', error.message);
      }
    }
    this.terminate();
  }

  terminate() {
    if (this.isClosed) return;
    this.isClosed = true;
    this.abortLLMStream();
    this.abortTTS();
    this.clearPartialTimer();
    try {
      const worker = getAsrWorker();
      worker.unbindSession(this.id);
    } catch (error) {
      console.warn('解除 ASR 会话绑定失败：', error.message);
    }
    this.logEvent({
      level: 'info',
      event: 'session.end',
      status: 'disconnected',
      historyLength: this.history.length,
    });
    metrics.increment('sessions_ended_total');
  }
}

module.exports = {
  RealtimeSession,
};
