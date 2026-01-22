const chatWindow = document.getElementById('chat-window');
const recordBtn = document.getElementById('record-btn');
const stopBtn = document.getElementById('stop-btn');
const clearBtn = document.getElementById('clear-btn');
const languageSelect = document.getElementById('language-select');
const statusText = document.getElementById('status-text');
const textForm = document.getElementById('text-chat-form');
const textInput = document.getElementById('text-input');
const sendTextBtn = document.getElementById('send-text-btn');
const stopTtsBtn = document.getElementById('stop-tts-btn');
const toastContainer = document.getElementById('toast-container');

// Allow overriding WebSocket endpoint via a global variable (useful when static files
// are served from a different host/port than the Node backend).
const WS_URL =
  window.CHATBOT_WS_URL ||
  (window.location.protocol === 'https:' ? 'wss://' : 'ws://') +
    window.location.host +
    '/ws';

const TARGET_SAMPLE_RATE = 16_000;
const MIN_SAMPLE_RATE = 8_000;
const MAX_SAMPLE_RATE = 48_000;

let socket = null;
let sessionId = null;
let reconnectTimer = null;
let mediaStream = null;
let audioContext = null;
let processor = null;
let isStreaming = false;
let isSendingText = false;
let isAudioSuspended = false;

let partialUserBubble = null;
let assistantBubble = null;
let pendingAssistantText = '';
let lastTtsSampleRate = 24000;
let currentTranscript = '';

const ttsPlayer = createTTSPlayer();

function formatClock() {
  const now = new Date();
  return now.toLocaleTimeString([], {
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit',
  });
}

function updateStatus(text) {
  statusText.textContent = text;
}

function scrollToBottom() {
  chatWindow.scrollTop = chatWindow.scrollHeight;
}

function autoResizeTextarea(element) {
  if (!element) return;
  element.style.height = 'auto';
  const minHeight = 44;
  const maxHeight = 160;
  const scrollHeight = Math.min(element.scrollHeight, maxHeight);
  element.style.height = `${Math.max(scrollHeight, minHeight)}px`;
}

function createBubble(role, text, { streaming = false } = {}) {
  const wrapper = document.createElement('div');
  wrapper.classList.add('message');

  const bubble = document.createElement('div');
  bubble.classList.add('message-bubble', role === 'user' ? 'message-user' : 'message-assistant');
  bubble.textContent = text;
  if (streaming) {
    bubble.classList.add('message-streaming');
  }

  const meta = document.createElement('div');
  meta.classList.add('message-meta');
  meta.textContent = `${role === 'user' ? '你' : '助手'} · ${formatClock()}`;

  wrapper.appendChild(bubble);
  wrapper.appendChild(meta);
  chatWindow.appendChild(wrapper);

  scrollToBottom();
  return bubble;
}

function clearChat() {
  chatWindow.innerHTML = '';
  partialUserBubble = null;
  assistantBubble = null;
  pendingAssistantText = '';
  currentTranscript = '';
}

function showToast(message, type = 'info') {
  if (!toastContainer) {
    console[type === 'error' ? 'warn' : 'log'](message);
    return;
  }
  const toast = document.createElement('div');
  toast.className = `toast toast-${type}`;
  toast.textContent = message;
  toastContainer.appendChild(toast);
  setTimeout(() => {
    toast.remove();
  }, 4000);
}

function base64ToArrayBuffer(base64) {
  const binary = atob(base64);
  const len = binary.length;
  const buffer = new ArrayBuffer(len);
  const bytes = new Uint8Array(buffer);
  for (let i = 0; i < len; i += 1) {
    bytes[i] = binary.charCodeAt(i);
  }
  return buffer;
}

function clampSampleRate(rate) {
  if (!Number.isFinite(rate)) return TARGET_SAMPLE_RATE;
  return Math.max(MIN_SAMPLE_RATE, Math.min(MAX_SAMPLE_RATE, rate));
}

function downsampleBuffer(input, inputSampleRate, outputSampleRate) {
  if (outputSampleRate === inputSampleRate) {
    return input;
  }
  if (!Number.isFinite(inputSampleRate) || !Number.isFinite(outputSampleRate)) {
    return input;
  }
  if (outputSampleRate > inputSampleRate) {
    return input;
  }
  const sampleRateRatio = inputSampleRate / outputSampleRate;
  const newLength = Math.round(input.length / sampleRateRatio);
  const result = new Float32Array(newLength);
  let offsetResult = 0;
  let offsetBuffer = 0;
  while (offsetResult < result.length) {
    const nextOffsetBuffer = Math.round((offsetResult + 1) * sampleRateRatio);
    let accum = 0;
    let count = 0;
    for (let i = offsetBuffer; i < nextOffsetBuffer && i < input.length; i += 1) {
      accum += input[i];
      count += 1;
    }
    result[offsetResult] = count ? accum / count : 0;
    offsetResult += 1;
    offsetBuffer = nextOffsetBuffer;
  }
  return result;
}

function createTTSPlayer() {
  let audioCtx = null;
  let queue = [];
  let playing = false;

  const ensureContext = async (throwOnError = false) => {
    if (!audioCtx) {
      try {
        audioCtx = new AudioContext();
      } catch (error) {
        if (throwOnError) throw error;
        console.warn('创建 AudioContext 失败：', error);
        return null;
      }
    }
    if (audioCtx.state === 'suspended') {
      try {
        await audioCtx.resume();
      } catch (error) {
        if (throwOnError) throw error;
        console.warn('恢复 AudioContext 失败：', error);
        return null;
      }
    }
    return audioCtx;
  };

  const playNext = () => {
    if (!audioCtx || playing || queue.length === 0) return;
    const buffer = queue.shift();
    const source = audioCtx.createBufferSource();
    source.buffer = buffer;
    source.connect(audioCtx.destination);
    source.onended = () => {
      playing = false;
      playNext();
    };
    source.start();
    playing = true;
  };

  return {
    async warmup() {
      await ensureContext(true).catch((error) => {
        console.warn('初始化 AudioContext 失败：', error);
      });
    },
    reset() {
      queue = [];
      playing = false;
    },
    async enqueue(base64Chunk, sampleRate) {
      if (!base64Chunk) return;
      const ctx = await ensureContext();
      if (!ctx) return;

      const pcmBuffer = base64ToArrayBuffer(base64Chunk);
      const byteLength = pcmBuffer.byteLength - (pcmBuffer.byteLength % 2);
      if (byteLength <= 0) return;
      const viewBuffer = byteLength === pcmBuffer.byteLength ? pcmBuffer : pcmBuffer.slice(0, byteLength);
      const pcmView = new Int16Array(viewBuffer);
      const audioBuffer = ctx.createBuffer(1, pcmView.length, sampleRate);
      const channelData = audioBuffer.getChannelData(0);
      for (let i = 0; i < pcmView.length; i += 1) {
        channelData[i] = Math.max(-1, Math.min(1, pcmView[i] / 32768));
      }

      queue.push(audioBuffer);
      playNext();
    },
  };
}

function connectWebSocket() {
  if (socket && socket.readyState === WebSocket.OPEN) {
    return Promise.resolve(socket);
  }

  return new Promise((resolve, reject) => {
    socket = new WebSocket(WS_URL);

    socket.addEventListener('open', () => {
      updateStatus('WebSocket 已连接，等待语音输入。');
      resolve(socket);
    });

    socket.addEventListener('message', (event) => {
      try {
        const payload = JSON.parse(event.data);
        handleServerMessage(payload);
      } catch (error) {
        console.error('解析服务器消息失败：', error);
      }
    });

    socket.addEventListener('close', () => {
      updateStatus('WebSocket 已断开，尝试重连…');
      if (isStreaming) {
        stopStreaming(false);
      }
      if (reconnectTimer) {
        clearTimeout(reconnectTimer);
      }
      reconnectTimer = setTimeout(() => {
        connectWebSocket().catch(() => {
          updateStatus('WebSocket 重连失败，请稍后重试。');
        });
      }, 3000);
    });

    socket.addEventListener('error', (error) => {
      console.error('WebSocket 错误：', error);
      reject(error);
    });
  });
}

function sendMessage(message) {
  if (socket && socket.readyState === WebSocket.OPEN) {
    socket.send(JSON.stringify({ sessionId, ...message }));
  }
}

async function sendTextMessage(text, fallbackValue = '') {
  if (!text) return;
  if (isSendingText) return;
  isSendingText = true;
  if (sendTextBtn) {
    sendTextBtn.disabled = true;
  }

  try {
    await connectWebSocket();
    createBubble('user', text);
    sendMessage({ type: 'text', text });
    updateStatus('文本消息已发送，助手正在生成回复…');
  } catch (error) {
    console.error('发送文本消息失败：', error);
    updateStatus(`发送文本消息失败：${error.message || error}`);
    if (textInput) {
      textInput.value = fallbackValue || text;
      autoResizeTextarea(textInput);
      textInput.focus();
    }
  } finally {
    isSendingText = false;
    if (sendTextBtn) {
      sendTextBtn.disabled = false;
    }
  }
}

function handleServerMessage(payload) {
  switch (payload.type) {
    case 'session':
      sessionId = payload.sessionId;
      updateStatus('会话就绪，点击“开始实时对话”即可开始。');
      break;
    case 'asr.partial':
      handleAsrPartial(payload);
      break;
    case 'asr.final':
      handleAsrFinal(payload);
      break;
    case 'llm.start': {
      const source = payload.source || 'final';
      suspendAudioCapture();
      startAssistantMessage(source);
      const statusMap = {
        interim: '助手正在实时生成回复…',
        text: '助手正在生成回复…',
        final: '助手正在整理最终回复…',
        auto: '助手正在整理最终回复…',
      };
      updateStatus(statusMap[source] || '助手正在生成回复…');
      break;
    }
    case 'llm.delta':
      appendAssistantDelta(payload.delta || '');
      break;
    case 'llm.final':
      finalizeAssistant(payload.text || pendingAssistantText);
      resumeAudioCapture();
      updateStatus('助手回复完成，可继续说话或输入文字。');
      break;
    case 'llm.abort':
      handleAssistantAbort();
      resumeAudioCapture();
      updateStatus('助手已更新上下文，等待最新语音…');
      break;
    case 'tts.start':
      ttsPlayer.reset();
      updateStatus('助手语音合成中…');
      break;
    case 'tts.meta':
      lastTtsSampleRate = payload.sampleRate || lastTtsSampleRate;
      updateStatus('助手语音合成中…');
      break;
    case 'tts.chunk':
      ttsPlayer
        .enqueue(payload.chunk, payload.sampleRate || lastTtsSampleRate || 24000)
        .catch((error) => {
          console.warn('播放语音合成数据失败：', error);
          showToast('语音播放失败，请稍后重试。', 'error');
        });
      updateStatus('助手语音播报中…');
      break;
    case 'tts.complete':
      updateStatus('助手语音播报完成，可继续对话。');
      showToast('助手播报完成。', 'success');
      break;
    case 'tts.error':
      updateStatus(`⚠️ 语音合成失败：${payload.detail || '未知错误'}`);
      showToast(payload.detail || '语音合成失败，请稍后重试。', 'error');
      break;
    case 'error':
      updateStatus(`⚠️ ${payload.detail || '服务器错误'}`);
      resumeAudioCapture();
      stopStreaming(false);
      showToast(payload.detail || '服务器错误', 'error');
      break;
    default:
      break;
  }
}

function handleAsrPartial(payload) {
  const text = payload.text || '';
  const delta = payload.delta || '';

  if (delta) {
    currentTranscript += delta;
  } else if (text) {
    if (text.startsWith(currentTranscript)) {
      currentTranscript = text;
    } else {
      currentTranscript = text;
    }
  }

  const display = currentTranscript || text || delta || '（正在识别…）';
  if (!partialUserBubble) {
    partialUserBubble = createBubble('user', display, { streaming: true });
  } else {
    partialUserBubble.textContent = display;
  }
  updateStatus(delta ? `语音识别中：${delta}` : '语音识别中…');
}

function handleAsrFinal(payload) {
  const text = payload.text || '';

  if (text) {
    if (text.startsWith(currentTranscript)) {
      currentTranscript = text;
    } else if (currentTranscript.startsWith(text)) {
      currentTranscript = currentTranscript;
    } else {
      currentTranscript = text.length >= currentTranscript.length ? text : currentTranscript;
    }
  }
  const finalText = currentTranscript || text || '（未识别到文本）';

  if (!partialUserBubble) {
    partialUserBubble = createBubble('user', finalText, { streaming: false });
  } else {
    partialUserBubble.textContent = finalText;
    partialUserBubble.classList.remove('message-streaming');
  }
  partialUserBubble = null;
  currentTranscript = '';
  updateStatus('识别完成，助手正在整理回复…');
}

function startAssistantMessage(source = 'final') {
  let placeholder = '（生成中…）';
  if (source === 'interim') {
    placeholder = '（实时生成中…）';
  } else if (source === 'auto') {
    placeholder = '（整理回复中…）';
  }
  if (!assistantBubble) {
    assistantBubble = createBubble('assistant', placeholder, { streaming: true });
  } else {
    assistantBubble.textContent = placeholder;
    assistantBubble.classList.add('message-streaming');
  }
  pendingAssistantText = '';
}

function appendAssistantDelta(delta) {
  if (!assistantBubble) {
    startAssistantMessage('interim');
  }
  pendingAssistantText += delta;
  assistantBubble.textContent = pendingAssistantText;
  assistantBubble.classList.add('message-streaming');
  updateStatus('助手正在回复…');
}

function handleAssistantAbort() {
  if (!assistantBubble) return;
  assistantBubble.textContent = '（等待最新语音…）';
  assistantBubble.classList.add('message-streaming');
  pendingAssistantText = '';
}

function suspendAudioCapture() {
  isAudioSuspended = true;
}

function resumeAudioCapture() {
  isAudioSuspended = false;
}

function finalizeAssistant(text) {
  if (!assistantBubble) {
    assistantBubble = createBubble('assistant', text || '');
  } else {
    assistantBubble.textContent = text || pendingAssistantText;
    assistantBubble.classList.remove('message-streaming');
  }
  assistantBubble = null;
  pendingAssistantText = '';
}

async function startAudioProcessing() {
  mediaStream = await navigator.mediaDevices.getUserMedia({ audio: true });
  audioContext = new AudioContext();
  const source = audioContext.createMediaStreamSource(mediaStream);
  processor = audioContext.createScriptProcessor(4096, 1, 1);
  const sourceSampleRate = audioContext.sampleRate;
  const targetSampleRate = clampSampleRate(TARGET_SAMPLE_RATE);
  currentTranscript = '';

  processor.onaudioprocess = (event) => {
    if (isAudioSuspended) {
      return;
    }
    const input = event.inputBuffer.getChannelData(0).slice();
    const downsampled = downsampleBuffer(input, sourceSampleRate, targetSampleRate);
    const pcmBuffer = new ArrayBuffer(downsampled.length * 2);
    const view = new DataView(pcmBuffer);
    for (let i = 0; i < downsampled.length; i += 1) {
      const sample = Math.max(-1, Math.min(1, downsampled[i]));
      view.setInt16(i * 2, sample < 0 ? sample * 0x8000 : sample * 0x7fff, true);
    }
    const bytes = new Uint8Array(pcmBuffer);
    let binary = '';
    for (let i = 0; i < bytes.length; i += 1) {
      binary += String.fromCharCode(bytes[i]);
    }
    const base64 = btoa(binary);
    sendMessage({
      type: 'audio',
      chunk: base64,
      sampleRate: targetSampleRate,
    });
  };

  source.connect(processor);
  processor.connect(audioContext.destination);

  sendMessage({
    type: 'start',
    sampleRate: targetSampleRate,
    language: languageSelect.value || undefined,
  });
}

function stopAudioCapture(sendStop = true) {
  if (processor) {
    processor.disconnect();
    processor.onaudioprocess = null;
    processor = null;
  }
  if (audioContext) {
    audioContext.close();
    audioContext = null;
  }
  if (mediaStream) {
    mediaStream.getTracks().forEach((track) => track.stop());
    mediaStream = null;
  }
  currentTranscript = '';
  if (sendStop) {
    sendMessage({ type: 'stop' });
  }
}

async function startStreaming() {
  if (isStreaming) return;
  if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia) {
    updateStatus('当前浏览器不支持实时录音。');
    return;
  }

  try {
    await connectWebSocket();
    await startAudioProcessing();
    recordBtn.disabled = true;
    recordBtn.classList.add('recording');
    stopBtn.disabled = false;
    isStreaming = true;
    isAudioSuspended = false;
    updateStatus('语音流开启，开始讲话吧。');
  } catch (error) {
    console.error('启动实时语音失败：', error);
    updateStatus(`无法启动实时语音：${error.message || error}`);
    stopStreaming(false);
  }
}

function stopStreaming(sendStop = true) {
  if (!isStreaming && !sendStop) {
    stopAudioCapture(false);
    return;
  }
  if (!isStreaming) return;
  stopAudioCapture(sendStop);
  recordBtn.disabled = false;
  recordBtn.classList.remove('recording');
  stopBtn.disabled = true;
  isStreaming = false;
  isAudioSuspended = false;
  updateStatus(sendStop ? '已停止录音，等待识别结果…' : '录音停止。');
}

recordBtn.addEventListener('click', () => {
  startStreaming();
});

stopBtn.addEventListener('click', () => {
  stopStreaming(true);
});

if (stopTtsBtn) {
  stopTtsBtn.addEventListener('click', () => {
    ttsPlayer.reset();
    updateStatus('语音播报已手动停止。');
    showToast('已停止当前语音播放。');
  });
}

clearBtn.addEventListener('click', () => {
  clearChat();
  updateStatus('聊天记录已清空。');
});

window.addEventListener('beforeunload', () => {
  stopStreaming(false);
  if (socket && socket.readyState === WebSocket.OPEN) {
    socket.close();
  }
});

if (textInput) {
  autoResizeTextarea(textInput);
  textInput.addEventListener('input', () => autoResizeTextarea(textInput));
  textInput.addEventListener('keydown', (event) => {
    if (event.key === 'Enter' && !event.shiftKey) {
      event.preventDefault();
      if (textForm) {
        textForm.requestSubmit();
      }
    }
  });
}

if (textForm) {
  textForm.addEventListener('submit', (event) => {
    event.preventDefault();
    if (!textInput) return;
    const rawValue = textInput.value;
    const text = rawValue.trim();
    if (!text || isSendingText) return;
    textInput.value = '';
    autoResizeTextarea(textInput);
    sendTextMessage(text, rawValue);
    textInput.focus();
  });
}

window.addEventListener(
  'click',
  () => {
    ttsPlayer.warmup();
  },
  { once: true }
);

updateStatus('加载完成，等待连接服务器…');
