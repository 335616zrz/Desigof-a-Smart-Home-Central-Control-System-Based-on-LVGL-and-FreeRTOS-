const fs = require('fs');
const http = require('http');
const https = require('https');
const path = require('path');
const express = require('express');
const cors = require('cors');
const helmet = require('helmet');
const rateLimit = require('express-rate-limit');
const WebSocket = require('ws');

const config = require('./src/config');
const { createLogger } = require('./src/logger');
const { RealtimeSession } = require('./src/realtime/RealtimeSession');
const { metrics } = require('./src/metrics');
const { createVoiceEnrollmentRouter } = require('./src/cosyvoice/voiceEnrollmentRouter');

const { WebSocketServer } = WebSocket;

const app = express();
const logger = createLogger(config.logging);

if (!config.deepseek.apiKey) {
  console.warn(
    '警告：未检测到环境变量 DEEPSEEK_API_KEY。请在运行服务器前配置 DeepSeek API Key。'
  );
}

if (!fs.existsSync(config.asr.script)) {
  console.warn('警告：未找到 SenseVoice 脚本，请确认路径是否正确：', config.asr.script);
}

if (config.tts.enabled && !fs.existsSync(config.tts.script)) {
  console.warn('警告：未找到 TTS 脚本，将无法返回语音合成结果：', config.tts.script);
}

if (config.security.trustProxy) {
  const parsed =
    config.security.trustProxy === 'true'
      ? 1
      : Number.parseInt(config.security.trustProxy, 10);
  if (!Number.isNaN(parsed) && parsed >= 0) {
    app.set('trust proxy', parsed);
  }
}

app.disable('x-powered-by');

app.use(
  helmet({
    contentSecurityPolicy: {
      useDefaults: true,
      directives: {
        defaultSrc: ["'self'"],
        scriptSrc: ["'self'"],
        styleSrc: ["'self'"],
        imgSrc: ["'self'", 'data:'],
        // 允许 WebSocket 连接（同源 ws:// 或 wss://），避免浏览器因 CSP 拦截 WS 握手
        connectSrc: ["'self'", 'ws:', 'wss:'],
        fontSrc: ["'self'", 'data:'],
        baseUri: ["'self'"],
        formAction: ["'self'"],
      },
    },
    crossOriginEmbedderPolicy: false,
  })
);

app.use(
  helmet.referrerPolicy({
    policy: 'same-origin',
  })
);

app.use(
  cors({
    origin: config.security.corsOrigins.length
      ? (origin, callback) => {
          if (!origin || config.security.corsOrigins.includes(origin)) {
            callback(null, true);
          } else {
            callback(new Error('不被允许的来源。'));
          }
        }
      : true,
  })
);

app.use(
  rateLimit({
    windowMs: 60 * 1000,
    max: Number.parseInt(process.env.RATE_LIMIT_MAX, 10) || 120,
    standardHeaders: true,
    legacyHeaders: false,
    message: { error: '请求过于频繁，请稍后重试。' },
  })
);

app.use(express.json({ limit: '256kb' }));
app.use('/api/cosyvoice', createVoiceEnrollmentRouter({ config }));

app.use(express.static(config.server.staticDir));

app.get('/healthz', (req, res) => {
  res.json({ ok: true });
});

app.get('/metrics', (req, res) => {
  res.json({
    uptimeSeconds: process.uptime(),
    memory: process.memoryUsage(),
    metrics: metrics.snapshot(),
    sessions: {
      active: sessions.size,
    },
  });
});

app.use((req, res) => {
  res.status(404).json({ error: '未找到请求的资源。' });
});

// Create HTTP or HTTPS server based on config
let server;
let actualProto = 'http';
let actualPort = config.server.port;

if (config.server.https && config.server.https.enabled) {
  try {
    const key = fs.readFileSync(config.server.https.keyFile);
    const cert = fs.readFileSync(config.server.https.certFile);
    const tlsOptions = { key, cert };
    if (config.server.https.caFile) {
      try {
        tlsOptions.ca = fs.readFileSync(config.server.https.caFile);
      } catch (err) {
        console.warn('加载 CA 失败（可忽略）：', err.message);
      }
    }
    server = https.createServer(tlsOptions, app);
    actualProto = 'https';
    actualPort = config.server.https.port;
  } catch (err) {
    console.error('启用 HTTPS 失败，回退到 HTTP：', err.message);
    server = http.createServer(app);
  }
} else {
  server = http.createServer(app);
}
const allowedWsOrigins = new Set(config.security.wsAllowedOrigins || []);

const wss = new WebSocketServer({
  server,
  path: '/ws',
  perMessageDeflate: false,
  maxPayload: config.security.maxWsPayloadBytes || undefined,
  verifyClient: (info, done) => {
    const { origin } = info;
    if (allowedWsOrigins.size) {
      if (!origin || !allowedWsOrigins.has(origin)) {
        console.warn('阻止来自未授权 Origin 的 WebSocket 连接：', origin);
        done(false, 403, 'Origin not allowed');
        return;
      }
    }
    done(true);
  },
});

const sessions = new Map();

wss.on('connection', (socket, req) => {
  const session = new RealtimeSession({ socket, request: req, config, logger });
  sessions.set(session.id, session);

  const timeoutInterval = setInterval(() => {
    if (Date.now() - session.lastActivity > config.server.sessionTimeoutMs) {
      session.sendError('会话超时，已自动结束。');
      socket.close();
    }
  }, 30 * 1000);

  socket.on('close', () => {
    clearInterval(timeoutInterval);
    sessions.delete(session.id);
  });
});

wss.on('error', (error) => {
  console.error('WebSocket 服务错误：', error.message);
});

server.listen(actualPort, () => {
  const wsScheme = actualProto === 'https' ? 'wss' : 'ws';
  console.log(`服务器已启动：${actualProto}://localhost:${actualPort}`);
  console.log('WebSocket 实时语音端点：%s://localhost:%d/ws', wsScheme, actualPort);
});

// Optional: start a tiny HTTP redirect server if configured
if (config.server.httpRedirectPort && Number.isFinite(config.server.httpRedirectPort) && config.server.httpRedirectPort > 0) {
  const redirectApp = (req, res) => {
    const host = req.headers.host || `localhost:${actualPort}`;
    // preserve path and query
    const location = `https://${host}${req.url || '/'}`;
    res.statusCode = 301;
    res.setHeader('Location', location);
    res.end();
  };
  http.createServer(redirectApp).listen(config.server.httpRedirectPort, () => {
    console.log('HTTP 重定向服务已启动：http://localhost:%d -> https://localhost:%d', config.server.httpRedirectPort, actualPort);
  });
}
