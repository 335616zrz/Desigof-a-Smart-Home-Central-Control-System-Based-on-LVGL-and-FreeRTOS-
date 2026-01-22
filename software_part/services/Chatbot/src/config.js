const path = require('path');
const fs = require('fs');

const { detectPythonExecutable } = require('./utils/detectPython');

require('dotenv').config();

const ROOT_DIR = path.resolve(__dirname, '..');

const parseList = (value) =>
  value
    ? value
        .split(',')
        .map((item) => item.trim())
        .filter(Boolean)
    : [];

const toPositiveInt = (value, fallback) => {
  const parsed = Number.parseInt(value, 10);
  if (Number.isFinite(parsed) && parsed > 0) {
    return parsed;
  }
  return fallback;
};

const limitNumber = (value, { min, max, fallback }) => {
  if (value === undefined || value === null || value === '') return fallback;
  const parsed = Number.parseInt(value, 10);
  if (!Number.isFinite(parsed)) return fallback;
  const clamped = Math.max(min, Math.min(parsed, max));
  return clamped;
};

const pickOne = (value, allowed, fallback) => {
  if (value && allowed.includes(value)) {
    return value;
  }
  return fallback;
};

const resolvePath = (input, fallback) => {
  if (!input) return fallback;
  return path.isAbsolute(input) ? input : path.join(ROOT_DIR, input);
};

const PORT = Number.parseInt(process.env.PORT, 10) || 3000;
const SESSION_TIMEOUT_MS = Number.parseInt(process.env.SESSION_TIMEOUT_MS, 10) || 5 * 60 * 1000;
const REQUEST_TIMEOUT_MS = Number.parseInt(process.env.REQUEST_TIMEOUT_MS, 10) || 30 * 1000;
const DEFAULT_SAMPLE_RATE = Number.parseInt(process.env.DEFAULT_SAMPLE_RATE, 10) || 16_000;

const LOG_DIR = resolvePath(process.env.LOG_DIR, path.join(ROOT_DIR, 'logs'));
const LOG_FILE = resolvePath(process.env.LOG_FILE, path.join(LOG_DIR, 'chat.log'));
const LOGGING_ENABLED = process.env.LOGGING_ENABLED !== 'false';
const LOG_INCLUDE_CONTENT = process.env.LOG_INCLUDE_CONTENT === 'true';
const LOG_MAX_BYTES =
  Number.parseInt(process.env.LOG_MAX_BYTES, 10) || 5 * 1024 * 1024;

const corsOrigins = parseList(process.env.CORS_ALLOW_ORIGINS);

const TRUST_PROXY = process.env.TRUST_PROXY;

const systemPrompt =
  process.env.SYSTEM_PROMPT ||
  '你是一名中文实时对话助手。请保持礼貌、准确并及时回应用户的语音内容。';

const SENSEVOICE_STREAM_SCRIPT = resolvePath(
  process.env.SENSEVOICE_STREAM_SCRIPT,
  path.join(ROOT_DIR, 'scripts', 'sensevoice_stream.py')
);

const TTS_STREAM_SCRIPT = resolvePath(
  process.env.TTS_STREAM_SCRIPT,
  path.join(ROOT_DIR, 'scripts', 'tts_stream.py')
);

const detectedPython = detectPythonExecutable(ROOT_DIR);

const SENSEVOICE_PYTHON = process.env.SENSEVOICE_PYTHON || detectedPython;
const TTS_PYTHON = process.env.TTS_PYTHON || detectedPython;

const WS_ALLOWED_ORIGINS = parseList(process.env.WS_ALLOW_ORIGINS);
const MAX_WS_PAYLOAD_BYTES = toPositiveInt(process.env.WS_MAX_PAYLOAD_BYTES, 512 * 1024);
const MAX_CLIENT_MESSAGE_BYTES = toPositiveInt(
  process.env.MAX_CLIENT_MESSAGE_BYTES,
  Math.min(MAX_WS_PAYLOAD_BYTES, 512 * 1024)
);
const MAX_AUDIO_CHUNK_BASE64_LENGTH = toPositiveInt(
  process.env.MAX_AUDIO_CHUNK_BASE64_LENGTH,
  64 * 1024
);
const MAX_SESSION_AUDIO_BYTES = toPositiveInt(process.env.MAX_SESSION_AUDIO_BYTES, 20 * 1024 * 1024);
const MAX_TRANSCRIPT_CHARS = toPositiveInt(process.env.MAX_TRANSCRIPT_CHARS, 2048);
const MAX_ASSISTANT_CHARS = toPositiveInt(process.env.MAX_ASSISTANT_CHARS, 4096);
const MAX_TEXT_INPUT_CHARS = toPositiveInt(process.env.MAX_TEXT_INPUT_CHARS, 2048);
const MIN_SAMPLE_RATE = limitNumber(process.env.MIN_SAMPLE_RATE, {
  min: 4_000,
  max: 48_000,
  fallback: 8_000,
});
const MAX_SAMPLE_RATE = limitNumber(process.env.MAX_SAMPLE_RATE, {
  min: MIN_SAMPLE_RATE,
  max: 96_000,
  fallback: 48_000,
});
const ASR_SILENCE_TIMEOUT_MS = toPositiveInt(process.env.ASR_SILENCE_TIMEOUT_MS, 1000);
const COSYVOICE_LANGUAGE_HINTS = parseList(process.env.COSYVOICE_LANGUAGE_HINTS);
const TTS_STRIP_STAGE_DIRECTIONS =
  process.env.TTS_STRIP_STAGE_DIRECTIONS === undefined
    ? true
    : String(process.env.TTS_STRIP_STAGE_DIRECTIONS).toLowerCase() !== 'false';
const COSYVOICE_PACE_ENABLED =
  process.env.COSYVOICE_PACE_ENABLED === undefined
    ? true
    : String(process.env.COSYVOICE_PACE_ENABLED).toLowerCase() !== 'false';
const COSYVOICE_PACE_CHUNK_MS = toPositiveInt(process.env.COSYVOICE_PACE_CHUNK_MS, 40);
const COSYVOICE_PACE_PREBUFFER_MS = toPositiveInt(process.env.COSYVOICE_PACE_PREBUFFER_MS, 120);
const COSYVOICE_PACE_MAX_BUFFER_MS = toPositiveInt(process.env.COSYVOICE_PACE_MAX_BUFFER_MS, 60_000);

const config = {
  rootDir: ROOT_DIR,
  server: {
    port: PORT,
    https: {
      enabled:
        String(process.env.HTTPS_ENABLED || '').toLowerCase() === 'true' ||
        process.env.HTTPS_ENABLED === '1',
      port: Number.parseInt(process.env.HTTPS_PORT, 10) || PORT,
      keyFile: resolvePath(process.env.HTTPS_KEY, ''),
      certFile: resolvePath(process.env.HTTPS_CERT, ''),
      caFile: resolvePath(process.env.HTTPS_CA, ''),
    },
    // if >0, start an additional HTTP server that redirects to HTTPS
    httpRedirectPort: Number.parseInt(process.env.HTTP_REDIRECT_PORT, 10) || 0,
    sessionTimeoutMs: SESSION_TIMEOUT_MS,
    defaultSampleRate: DEFAULT_SAMPLE_RATE,
    requestTimeoutMs: REQUEST_TIMEOUT_MS,
    staticDir: path.join(ROOT_DIR, 'public'),
  },
  security: {
    corsOrigins,
    trustProxy: TRUST_PROXY,
    wsAllowedOrigins: WS_ALLOWED_ORIGINS.length ? WS_ALLOWED_ORIGINS : corsOrigins,
    maxWsPayloadBytes: MAX_WS_PAYLOAD_BYTES,
    maxClientMessageBytes: MAX_CLIENT_MESSAGE_BYTES,
    maxAudioChunkBase64Length: MAX_AUDIO_CHUNK_BASE64_LENGTH,
    maxSessionAudioBytes: MAX_SESSION_AUDIO_BYTES,
    maxTranscriptChars: MAX_TRANSCRIPT_CHARS,
    maxAssistantChars: MAX_ASSISTANT_CHARS,
    maxTextInputChars: MAX_TEXT_INPUT_CHARS,
    minSampleRate: MIN_SAMPLE_RATE,
    maxSampleRate: MAX_SAMPLE_RATE,
  },
  logging: {
    enabled: LOGGING_ENABLED,
    file: LOG_FILE,
    includeContent: LOG_INCLUDE_CONTENT,
    maxBytes: LOG_MAX_BYTES,
  },
  systemPrompt,
  deepseek: {
    apiKey: process.env.DEEPSEEK_API_KEY,
    model: process.env.DEEPSEEK_MODEL || 'deepseek-chat',
    temperature: Number.parseFloat(process.env.TEMPERATURE) || 0.7,
    maxTokens: Number.parseInt(process.env.MAX_TOKENS, 10) || 1024,
    maxHistory: Number.parseInt(process.env.MAX_HISTORY_MESSAGES, 10) || 10,
  },
  asr: {
    python: SENSEVOICE_PYTHON,
    script: SENSEVOICE_STREAM_SCRIPT,
    env: {
      SENSEVOICE_MODEL: process.env.SENSEVOICE_MODEL || 'iic/SenseVoiceSmall',
      SENSEVOICE_DEVICE: process.env.SENSEVOICE_DEVICE || 'cpu',
      SENSEVOICE_VAD_MODEL: process.env.SENSEVOICE_VAD_MODEL || 'fsmn-vad',
      SENSEVOICE_PUNC_MODEL: process.env.SENSEVOICE_PUNC_MODEL || '',
    },
    silenceTimeoutMs: ASR_SILENCE_TIMEOUT_MS,
  },
  tts: {
    enabled: process.env.TTS_ENABLED === 'true',
    provider: pickOne(process.env.TTS_PROVIDER, ['local', 'cosyvoice'], 'local'),
    python: TTS_PYTHON,
    script: TTS_STREAM_SCRIPT,
    voice: process.env.TTS_VOICE || '',
    stripStageDirections: TTS_STRIP_STAGE_DIRECTIONS,
    rate: Number.parseInt(process.env.TTS_RATE, 10) || null,
    volume: Number.parseFloat(process.env.TTS_VOLUME) || null,
    chunkMs: Number.parseInt(process.env.TTS_CHUNK_MS, 10) || 80,
    cosyvoice: {
      apiKey: process.env.COSYVOICE_API_KEY || process.env.DASHSCOPE_API_KEY || '',
      model: process.env.COSYVOICE_MODEL || 'cosyvoice-v3-plus',
      voice: process.env.COSYVOICE_VOICE || '',
      format: process.env.COSYVOICE_FORMAT || 'pcm',
      sampleRate: Number.parseInt(process.env.COSYVOICE_SAMPLE_RATE, 10) || 24000,
      rate: Number.parseFloat(process.env.COSYVOICE_RATE) || 1,
      pitch: Number.parseFloat(process.env.COSYVOICE_PITCH) || 1,
      volume:
        process.env.COSYVOICE_VOLUME !== undefined
          ? Number.parseInt(process.env.COSYVOICE_VOLUME, 10)
          : 50,
      bitRate:
        process.env.COSYVOICE_BIT_RATE !== undefined
          ? Number.parseInt(process.env.COSYVOICE_BIT_RATE, 10)
          : null,
      enableSSML: process.env.COSYVOICE_ENABLE_SSML === 'true',
      wordTimestamps: process.env.COSYVOICE_WORD_TIMESTAMPS === 'true',
      languageHints: COSYVOICE_LANGUAGE_HINTS.length ? COSYVOICE_LANGUAGE_HINTS : undefined,
      instruction: process.env.COSYVOICE_INSTRUCTION || '',
      pace: {
        enabled: COSYVOICE_PACE_ENABLED,
        chunkMs: COSYVOICE_PACE_CHUNK_MS,
        prebufferMs: COSYVOICE_PACE_PREBUFFER_MS,
        maxBufferMs: COSYVOICE_PACE_MAX_BUFFER_MS,
      },
    },
    fallbackProvider: pickOne(process.env.TTS_FALLBACK_PROVIDER, ['none', 'local'], 'local'),
  },
};

if (config.logging.enabled) {
  const logDir = path.dirname(config.logging.file);
  if (!fs.existsSync(logDir)) {
    fs.mkdirSync(logDir, { recursive: true });
  }
}

if (config.tts.fallbackProvider === 'local' && (!config.tts.script || !fs.existsSync(config.tts.script))) {
  console.warn('警告：未找到本地 TTS 脚本，已禁用本地降级策略。');
  config.tts.fallbackProvider = 'none';
}

if (config.tts.provider === 'cosyvoice' && !config.tts.cosyvoice.apiKey) {
  console.warn('警告：TTS 提供方设为 CosyVoice，但未配置 COSYVOICE_API_KEY。');
}

module.exports = config;
