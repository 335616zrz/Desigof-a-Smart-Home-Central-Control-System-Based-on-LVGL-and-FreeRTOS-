const express = require('express');

const { metrics } = require('../metrics');
const {
  createVoice,
  listVoices,
  queryVoice,
  updateVoice,
  deleteVoice,
} = require('./voiceEnrollmentClient');

class HttpError extends Error {
  constructor(status, message, details) {
    super(message);
    this.name = 'HttpError';
    this.status = status;
    this.details = details;
  }
}

const getDashScopeApiKey = (config) =>
  process.env.DASHSCOPE_API_KEY || config?.tts?.cosyvoice?.apiKey || process.env.COSYVOICE_API_KEY;

const requireAdminKeyIfConfigured = (req) => {
  const adminKey = process.env.ADMIN_API_KEY;
  if (!adminKey) return;
  const provided =
    req.get('x-api-key') || req.get('x-admin-key') || req.get('authorization');
  if (!provided) {
    throw new HttpError(401, '缺少管理员密钥（x-api-key）。');
  }
  const normalized = String(provided).trim();
  const token = normalized.toLowerCase().startsWith('bearer ')
    ? normalized.slice(7).trim()
    : normalized;
  if (token !== adminKey) {
    throw new HttpError(403, '管理员密钥无效。');
  }
};

const requireNonEmptyString = (value, label, { max = 2048 } = {}) => {
  if (typeof value !== 'string') {
    throw new HttpError(400, `${label} 必须为字符串。`);
  }
  const trimmed = value.trim();
  if (!trimmed) {
    throw new HttpError(400, `${label} 不能为空。`);
  }
  if (trimmed.length > max) {
    throw new HttpError(400, `${label} 长度不能超过 ${max}。`);
  }
  return trimmed;
};

const requireHttpUrl = (value, label) => {
  const trimmed = requireNonEmptyString(value, label, { max: 2048 });
  let parsed;
  try {
    parsed = new URL(trimmed);
  } catch {
    throw new HttpError(400, `${label} 必须为 http(s) 公网可访问链接。`);
  }
  if (parsed.protocol !== 'http:' && parsed.protocol !== 'https:') {
    throw new HttpError(400, `${label} 必须为 http(s) 公网可访问链接。`);
  }
  return trimmed;
};

const parseOptionalInt = (value, label, { min = 0, max = 1000 } = {}) => {
  if (value === undefined || value === null || value === '') return undefined;
  const parsed = Number.parseInt(value, 10);
  if (!Number.isFinite(parsed)) {
    throw new HttpError(400, `${label} 必须为整数。`);
  }
  if (parsed < min || parsed > max) {
    throw new HttpError(400, `${label} 必须在 ${min}~${max} 范围内。`);
  }
  return parsed;
};

const parseLanguageHints = (value) => {
  if (value === undefined || value === null || value === '') return undefined;
  const rawList = Array.isArray(value) ? value : [value];
  const supported = new Set(['zh', 'en', 'fr', 'de', 'ja', 'ko', 'ru']);
  const list = rawList
    .map((item) => String(item || '').trim())
    .filter(Boolean);
  if (!list.length) return undefined;
  const hint = list[0];
  if (!supported.has(hint)) {
    throw new HttpError(400, `languageHints 不支持：${hint}。`, {
      supported: Array.from(supported),
    });
  }
  return [hint];
};

const normalizeDashScopeError = (error) => {
  if (!error) return new HttpError(502, 'DashScope 请求失败。');
  if (error instanceof HttpError) return error;

  const status = error?.response?.status;
  const payload = error?.response?.data;
  const message =
    payload?.message ||
    payload?.error?.message ||
    payload?.error_message ||
    error.message ||
    'DashScope 请求失败。';

  return new HttpError(502, `DashScope 请求失败：${message}`, {
    upstreamStatus: status,
    upstream: payload,
  });
};

const createVoiceEnrollmentRouter = ({ config }) => {
  const router = express.Router();

  router.post('/voices', async (req, res, next) => {
    try {
      requireAdminKeyIfConfigured(req);
      const apiKey = getDashScopeApiKey(config);

      const body = req.body && typeof req.body === 'object' ? req.body : {};
      const targetModel = requireNonEmptyString(
        body.targetModel || body.target_model || config?.tts?.cosyvoice?.model,
        'targetModel',
        { max: 64 }
      );
      const prefix = requireNonEmptyString(body.prefix, 'prefix', { max: 10 });
      if (!/^[A-Za-z0-9_]{1,10}$/.test(prefix)) {
        throw new HttpError(400, 'prefix 仅允许数字、字母和下划线，且长度不超过 10。');
      }
      const url = requireHttpUrl(body.url, 'url');
      const languageHints = parseLanguageHints(body.languageHints || body.language_hints);

      const startAt = Date.now();
      const result = await createVoice(
        { apiKey, targetModel, prefix, url, languageHints },
        { timeoutMs: config?.server?.requestTimeoutMs }
      );
      metrics.increment('cosyvoice_enrollment_create_total');
      metrics.observe('cosyvoice_enrollment_create_latency_ms', Date.now() - startAt);

      res.json({
        voiceId: result?.output?.voice_id || null,
        requestId: result?.request_id || null,
        usage: result?.usage || null,
      });
    } catch (error) {
      next(normalizeDashScopeError(error));
    }
  });

  router.get('/voices', async (req, res, next) => {
    try {
      requireAdminKeyIfConfigured(req);
      const apiKey = getDashScopeApiKey(config);

      const prefixRaw = req.query.prefix;
      const prefix =
        typeof prefixRaw === 'string' && prefixRaw.trim() ? prefixRaw.trim() : undefined;
      const pageIndex = parseOptionalInt(req.query.pageIndex ?? req.query.page_index, 'pageIndex', {
        min: 0,
        max: 10_000,
      });
      const pageSize = parseOptionalInt(req.query.pageSize ?? req.query.page_size, 'pageSize', {
        min: 1,
        max: 100,
      });

      const startAt = Date.now();
      const result = await listVoices(
        {
          apiKey,
          prefix,
          pageIndex: pageIndex ?? 0,
          pageSize: pageSize ?? 10,
        },
        { timeoutMs: config?.server?.requestTimeoutMs }
      );
      metrics.increment('cosyvoice_enrollment_list_total');
      metrics.observe('cosyvoice_enrollment_list_latency_ms', Date.now() - startAt);

      res.json({
        voices: result?.output?.voice_list || [],
        requestId: result?.request_id || null,
        usage: result?.usage || null,
      });
    } catch (error) {
      next(normalizeDashScopeError(error));
    }
  });

  router.get('/voices/:voiceId', async (req, res, next) => {
    try {
      requireAdminKeyIfConfigured(req);
      const apiKey = getDashScopeApiKey(config);
      const voiceId = requireNonEmptyString(req.params.voiceId, 'voiceId', { max: 128 });

      const startAt = Date.now();
      const result = await queryVoice(
        { apiKey, voiceId },
        { timeoutMs: config?.server?.requestTimeoutMs }
      );
      metrics.increment('cosyvoice_enrollment_query_total');
      metrics.observe('cosyvoice_enrollment_query_latency_ms', Date.now() - startAt);

      res.json({
        voice: result?.output || null,
        requestId: result?.request_id || null,
        usage: result?.usage || null,
      });
    } catch (error) {
      next(normalizeDashScopeError(error));
    }
  });

  router.put('/voices/:voiceId', async (req, res, next) => {
    try {
      requireAdminKeyIfConfigured(req);
      const apiKey = getDashScopeApiKey(config);
      const voiceId = requireNonEmptyString(req.params.voiceId, 'voiceId', { max: 128 });

      const body = req.body && typeof req.body === 'object' ? req.body : {};
      const url = requireHttpUrl(body.url, 'url');

      const startAt = Date.now();
      const result = await updateVoice(
        { apiKey, voiceId, url },
        { timeoutMs: config?.server?.requestTimeoutMs }
      );
      metrics.increment('cosyvoice_enrollment_update_total');
      metrics.observe('cosyvoice_enrollment_update_latency_ms', Date.now() - startAt);

      res.json({
        ok: true,
        requestId: result?.request_id || null,
        usage: result?.usage || null,
      });
    } catch (error) {
      next(normalizeDashScopeError(error));
    }
  });

  router.delete('/voices/:voiceId', async (req, res, next) => {
    try {
      requireAdminKeyIfConfigured(req);
      const apiKey = getDashScopeApiKey(config);
      const voiceId = requireNonEmptyString(req.params.voiceId, 'voiceId', { max: 128 });

      const startAt = Date.now();
      const result = await deleteVoice(
        { apiKey, voiceId },
        { timeoutMs: config?.server?.requestTimeoutMs }
      );
      metrics.increment('cosyvoice_enrollment_delete_total');
      metrics.observe('cosyvoice_enrollment_delete_latency_ms', Date.now() - startAt);

      res.json({
        ok: true,
        requestId: result?.request_id || null,
        usage: result?.usage || null,
      });
    } catch (error) {
      next(normalizeDashScopeError(error));
    }
  });

  router.use((error, req, res, next) => {
    if (!error) return next();
    const status = error instanceof HttpError ? error.status : 500;
    res.status(status).json({
      error: error.message || '请求失败。',
      details: error.details,
    });
  });

  return router;
};

module.exports = {
  createVoiceEnrollmentRouter,
};
