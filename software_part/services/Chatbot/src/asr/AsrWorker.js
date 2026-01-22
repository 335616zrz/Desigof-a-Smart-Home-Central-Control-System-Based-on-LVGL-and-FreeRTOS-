const { spawn } = require('child_process');
const readline = require('readline');

class AsrWorker {
  constructor(options) {
    this.python = options.python;
    this.script = options.script;
    this.env = options.env || {};
    this.proc = null;
    this.rl = null;
    this.sessions = new Map();
  }

  start() {
    if (this.proc) return;
    this.proc = spawn(this.python, [this.script], {
      env: {
        ...process.env,
        PYTHONUNBUFFERED: '1',
        ...this.env,
      },
      stdio: ['pipe', 'pipe', 'pipe'],
    });

    this.rl = readline.createInterface({ input: this.proc.stdout });

    this.rl.on('line', (line) => {
      const trimmed = line.trim();
      if (!trimmed) return;
      // FunASR / modelscope 可能会把日志打印到 stdout，避免污染 JSON 协议。
      // 我们只处理以 `{"` 开头的 JSON 对象行（worker 输出严格是一行一个 JSON 对象）。
      if (!trimmed.startsWith('{"')) {
        if (process.env.ASR_WORKER_DEBUG_STDOUT === 'true') {
          console.log('[ASR worker stdout]', trimmed.slice(0, 500));
        }
        return;
      }
      let msg;
      try {
        msg = JSON.parse(trimmed);
      } catch (error) {
        console.error('ASR worker 返回了无法解析的 JSON：', trimmed.slice(0, 500));
        return;
      }
      this.handleMessage(msg);
    });

    this.proc.stderr.on('data', (chunk) => {
      const rawText = chunk.toString();
      if (!rawText) return;

      // FunASR/modelscope 会在 stderr 打印进度条与性能统计（rtf/load_data...）。
      // 默认过滤这些“噪声”日志，避免污染 PM2 的 error.log；
      // 如需调试，可在 .env 设置 ASR_WORKER_DEBUG_STDERR=true。
      const debugAll = process.env.ASR_WORKER_DEBUG_STDERR === 'true';
      const text = rawText.replace(/\r/g, '\n');
      const lines = text.split('\n');

      for (const line of lines) {
        const trimmed = line.trim();
        if (!trimmed) continue;
        const cleaned = trimmed.replace(/\x1b\[[0-9;]*[A-Za-z]/g, '').trim();
        if (!cleaned) continue;
        if (!debugAll) {
          if (
            cleaned.startsWith('0%|') ||
            cleaned.startsWith('100%|') ||
            cleaned.startsWith("{'load_data':") ||
            cleaned.includes('rtf_avg:') ||
            cleaned.includes('time_speech:') ||
            cleaned.includes('time_escape:') ||
            cleaned.includes('modelscope - INFO') ||
            cleaned.includes('WARNING:root:trust_remote_code')
          ) {
            continue;
          }
        }
        console.warn('[ASR worker stderr]', cleaned.slice(0, 500));
      }
    });

    this.proc.on('exit', (code, signal) => {
      console.error('ASR worker 进程退出：', code, signal);
      this.proc = null;
      if (this.rl) {
        this.rl.close();
        this.rl = null;
      }
      for (const session of this.sessions.values()) {
        if (typeof session.handleAsrError === 'function') {
          session.handleAsrError('语音识别服务已退出，请稍后重试。');
        }
      }
      this.sessions.clear();
    });
  }

  bindSession(sessionId, session) {
    this.sessions.set(sessionId, session);
    this.start();
  }

  unbindSession(sessionId) {
    this.sessions.delete(sessionId);
    if (this.proc && this.proc.stdin.writable) {
      try {
        this.proc.stdin.write(
          `${JSON.stringify({ type: 'close', session_id: sessionId })}\n`
        );
      } catch {
        // ignore
      }
    }
  }

  send(message) {
    if (!this.proc || !this.proc.stdin.writable) {
      this.start();
    }
    const payload = `${JSON.stringify(message)}\n`;
    try {
      this.proc.stdin.write(payload);
    } catch (error) {
      console.error('向 ASR worker 写入失败：', error.message);
    }
  }

  handleMessage(msg) {
    const { type } = msg || {};
    const sessionId = msg.session_id;
    if (!sessionId) {
      if (type === 'error') {
        console.error('[ASR worker error]', msg.detail || '未知错误');
      }
      return;
    }
    const session = this.sessions.get(sessionId);
    if (!session) return;

    if (type === 'transcript') {
      if (typeof session.handleAsrTranscript === 'function') {
        session.handleAsrTranscript(msg);
      }
    } else if (type === 'error') {
      if (typeof session.handleAsrError === 'function') {
        session.handleAsrError(msg.detail || '语音识别失败。');
      }
    }
  }
}

module.exports = {
  AsrWorker,
};
