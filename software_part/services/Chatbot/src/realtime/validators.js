const BASE64_REGEX = /^[A-Za-z0-9+/]+={0,2}$/;

class ValidationError extends Error {
  constructor(message) {
    super(message);
    this.name = 'ValidationError';
  }
}

const clampSampleRate = (value, config) => {
  if (value === undefined || value === null || value === '') {
    return config.server.defaultSampleRate;
  }
  const parsed = Number.parseInt(value, 10);
  if (!Number.isFinite(parsed)) {
    throw new ValidationError('采样率必须为整数。');
  }
  if (parsed < config.security.minSampleRate || parsed > config.security.maxSampleRate) {
    throw new ValidationError(
      `采样率超出安全范围（${config.security.minSampleRate}-${config.security.maxSampleRate}Hz）。`
    );
  }
  return parsed;
};

const sanitizeLanguage = (language) => {
  if (language === undefined || language === null) {
    return null;
  }
  if (typeof language !== 'string') {
    throw new ValidationError('语言参数必须为字符串。');
  }
  const trimmed = language.trim();
  if (!trimmed) {
    return null;
  }
  if (trimmed.length > 16) {
    throw new ValidationError('语言代码长度不应超过 16 个字符。');
  }
  if (!/^[a-zA-Z0-9._-]+$/.test(trimmed)) {
    throw new ValidationError('语言代码只能包含字母、数字、点、下划线或连字符。');
  }
  return trimmed;
};

const estimateBase64Bytes = (input) => Math.floor((input.length * 3) / 4);

const validateStartPayload = (payload, config) => {
  const sampleRate = clampSampleRate(payload.sampleRate, config);
  const language = sanitizeLanguage(payload.language);
  return { sampleRate, language };
};

const validateAudioPayload = (payload, config) => {
  if (!payload || typeof payload !== 'object') {
    throw new ValidationError('音频消息格式错误。');
  }
  const { chunk } = payload;
  if (typeof chunk !== 'string' || !chunk.trim()) {
    throw new ValidationError('缺少音频数据。');
  }
  const sanitizedChunk = chunk.trim();
  if (!BASE64_REGEX.test(sanitizedChunk)) {
    throw new ValidationError('音频数据必须为合法的 Base64 字符串。');
  }
  if (
    config.security.maxAudioChunkBase64Length &&
    sanitizedChunk.length > config.security.maxAudioChunkBase64Length
  ) {
    throw new ValidationError('单个音频分片过大，已被服务器拒绝。');
  }
  let decodedLength = 0;
  try {
    decodedLength = Buffer.from(sanitizedChunk, 'base64').byteLength;
  } catch {
    throw new ValidationError('音频数据无法解码，请重试。');
  }
  if (decodedLength % 2 !== 0) {
    throw new ValidationError('音频数据长度不是 16-bit 对齐，已被服务器拒绝。');
  }
  const sampleRate =
    payload.sampleRate !== undefined && payload.sampleRate !== null
      ? clampSampleRate(payload.sampleRate, config)
      : null;
  const isLast = Boolean(payload.isLast);
  return {
    chunk: sanitizedChunk,
    sampleRate,
    isLast,
    estimatedBytes: decodedLength,
  };
};

const validateTextPayload = (payload, config) => {
  if (!payload || typeof payload !== 'object') {
    throw new ValidationError('文本消息格式错误。');
  }
  const { text } = payload;
  if (typeof text !== 'string') {
    throw new ValidationError('文本消息必须为字符串。');
  }
  const trimmed = text.trim();
  if (!trimmed) {
    throw new ValidationError('文本消息不能为空。');
  }
  const limit = config.security.maxTextInputChars || 2048;
  if (trimmed.length > limit) {
    throw new ValidationError(`文本消息过长，已超过 ${limit} 个字符的限制。`);
  }
  return { text: trimmed };
};

module.exports = {
  ValidationError,
  validateStartPayload,
  validateAudioPayload,
  validateTextPayload,
  estimateBase64Bytes,
};
