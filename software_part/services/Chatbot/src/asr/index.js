const path = require('path');
const config = require('../config');
const { AsrWorker } = require('./AsrWorker');

let worker = null;

function getAsrWorker() {
  if (worker) return worker;
  const asrConfig = config.asr || {};
  const python = asrConfig.python || 'python3';
  const script =
    process.env.ASR_WORKER_SCRIPT ||
    path.join(config.rootDir, 'scripts', 'asr_worker_streaming.py');
  worker = new AsrWorker({
    python,
    script,
    env: asrConfig.env || {},
  });
  worker.start();
  return worker;
}

module.exports = {
  getAsrWorker,
};
