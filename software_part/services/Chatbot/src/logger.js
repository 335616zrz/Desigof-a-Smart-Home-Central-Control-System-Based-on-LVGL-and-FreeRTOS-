const fs = require('fs');
const path = require('path');

function createLogger(options) {
  const {
    enabled,
    file: logFile,
    includeContent,
    maxBytes,
  } = options;

  if (!enabled) {
    return {
      record: () => {},
      anonymizeIp,
      includeContent: false,
    };
  }

  const logDir = path.dirname(logFile);
  if (!fs.existsSync(logDir)) {
    fs.mkdirSync(logDir, { recursive: true });
  }

  const state = {
    rotating: false,
  };

  async function rotateIfNeeded() {
    if (!maxBytes || state.rotating) return;
    state.rotating = true;
    try {
      const stats = await fs.promises.stat(logFile).catch(() => null);
      if (stats && stats.size >= maxBytes) {
        const { dir, name, ext } = path.parse(logFile);
        const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
        const archiveName = `${name}-${timestamp}${ext || '.log'}`;
        await fs.promises.rename(logFile, path.join(dir, archiveName));
      }
    } catch (error) {
      console.error('日志轮转失败：', error.message);
    } finally {
      state.rotating = false;
    }
  }

  async function record(entry) {
    const serialized = `${JSON.stringify(entry)}\n`;
    try {
      await rotateIfNeeded();
      await fs.promises.appendFile(logFile, serialized, 'utf8');
    } catch (error) {
      console.error('写入日志失败：', error.message);
    }
  }

  return {
    record,
    anonymizeIp,
    includeContent,
  };
}

function anonymizeIp(ip) {
  if (!ip) return undefined;
  const normalized = ip.replace(/^::ffff:/, '');
  if (normalized === '127.0.0.1' || normalized === '::1') {
    return normalized;
  }
  const parts = normalized.split('.');
  if (parts.length === 4) {
    parts[3] = '0';
    return parts.join('.');
  }
  return normalized;
}

module.exports = {
  createLogger,
};
