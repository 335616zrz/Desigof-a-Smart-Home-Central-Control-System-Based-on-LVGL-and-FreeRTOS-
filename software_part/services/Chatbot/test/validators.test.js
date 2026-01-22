const test = require('node:test');
const assert = require('node:assert');

const {
  ValidationError,
  validateStartPayload,
  validateAudioPayload,
  validateTextPayload,
} = require('../src/realtime/validators');

const mockConfig = {
  server: {
    defaultSampleRate: 16_000,
  },
  security: {
    minSampleRate: 8_000,
    maxSampleRate: 48_000,
    maxAudioChunkBase64Length: 1024,
    maxTextInputChars: 200,
  },
};

test('validateStartPayload applies defaults and bounds', () => {
  const result = validateStartPayload({}, mockConfig);
  assert.strictEqual(result.sampleRate, 16_000);
  assert.strictEqual(result.language, null);

  const custom = validateStartPayload({ sampleRate: 24_000, language: 'en' }, mockConfig);
  assert.strictEqual(custom.sampleRate, 24_000);
  assert.strictEqual(custom.language, 'en');
});

test('validateStartPayload rejects out of range sample rates', () => {
  assert.throws(
    () => validateStartPayload({ sampleRate: 1000 }, mockConfig),
    (error) => error instanceof ValidationError && /采样率超出安全范围/.test(error.message)
  );
});

test('validateAudioPayload sanitizes chunk and returns metadata', () => {
  const payload = {
    chunk: 'AQIDBA==', // bytes: [1,2,3,4], length 4 (16-bit 对齐)
    sampleRate: 16_000,
    isLast: true,
  };
  const result = validateAudioPayload(payload, mockConfig);
  assert.deepStrictEqual(result, {
    chunk: 'AQIDBA==',
    sampleRate: 16_000,
    isLast: true,
    estimatedBytes: 4,
  });
});

test('validateAudioPayload rejects invalid base64 or oversize chunk', () => {
  assert.throws(
    () => validateAudioPayload({ chunk: '***' }, mockConfig),
    (error) => error instanceof ValidationError && /合法的 Base64/.test(error.message)
  );

  const longChunk = 'A'.repeat(mockConfig.security.maxAudioChunkBase64Length + 1);
  assert.throws(
    () => validateAudioPayload({ chunk: longChunk }, mockConfig),
    (error) => error instanceof ValidationError && /分片过大/.test(error.message)
  );
});

test('validateTextPayload trims text and enforces limits', () => {
  const result = validateTextPayload({ text: '  hello  ' }, mockConfig);
  assert.deepStrictEqual(result, { text: 'hello' });

  assert.throws(
    () => validateTextPayload({ text: '   ' }, mockConfig),
    (error) => error instanceof ValidationError && /不能为空/.test(error.message)
  );

  const tooLong = 'a'.repeat(mockConfig.security.maxTextInputChars + 1);
  assert.throws(
    () => validateTextPayload({ text: tooLong }, mockConfig),
    (error) => error instanceof ValidationError && /文本消息过长/.test(error.message)
  );
});
