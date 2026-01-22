const fs = require('fs');
const path = require('path');

/**
 * 尝试自动检测可用的 Python 可执行文件。
 * 优先级：虚拟环境 -> 项目内 .venv -> python3 -> python。
 */
function detectPythonExecutable(rootDir) {
  const isWindows = process.platform === 'win32';
  const binDir = isWindows ? 'Scripts' : 'bin';
  const binName = isWindows ? 'python.exe' : 'python';

  const candidates = [];

  if (process.env.VIRTUAL_ENV) {
    candidates.push(path.join(process.env.VIRTUAL_ENV, binDir, binName));
  }

  if (rootDir) {
    candidates.push(path.join(rootDir, '.venv', binDir, binName));
  }

  if (!isWindows) {
    candidates.push('python3');
  }

  candidates.push('python');

  for (const candidate of candidates) {
    if (candidate.includes(path.sep)) {
      if (fs.existsSync(candidate)) {
        return candidate;
      }
    } else {
      return candidate;
    }
  }

  return isWindows ? 'python' : 'python3';
}

module.exports = {
  detectPythonExecutable,
};
