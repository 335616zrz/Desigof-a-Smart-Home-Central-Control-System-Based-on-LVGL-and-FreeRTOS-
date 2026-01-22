class PcmPacer {
  constructor({
    sampleRate,
    chunkMs = 40,
    prebufferMs = 120,
    maxBufferMs = 60_000,
    bytesPerSample = 2,
    onChunk,
    onDrain,
    canSend,
    onDrop,
    label = 'pcm',
  }) {
    if (!Number.isFinite(sampleRate) || sampleRate <= 0) {
      throw new Error(`Invalid sampleRate: ${sampleRate}`);
    }
    if (!Number.isFinite(chunkMs) || chunkMs <= 0) {
      throw new Error(`Invalid chunkMs: ${chunkMs}`);
    }
    if (!Number.isFinite(prebufferMs) || prebufferMs < 0) {
      throw new Error(`Invalid prebufferMs: ${prebufferMs}`);
    }
    if (!Number.isFinite(maxBufferMs) || maxBufferMs <= 0) {
      throw new Error(`Invalid maxBufferMs: ${maxBufferMs}`);
    }
    if (typeof onChunk !== 'function') {
      throw new Error('onChunk must be a function');
    }

    this.sampleRate = sampleRate;
    this.chunkMs = Math.max(10, Math.min(Math.round(chunkMs), 200));
    this.prebufferMs = Math.max(0, Math.round(prebufferMs));
    this.maxBufferMs = Math.max(this.prebufferMs, Math.round(maxBufferMs));
    this.bytesPerSample = bytesPerSample;
    this.onChunk = onChunk;
    this.onDrain = typeof onDrain === 'function' ? onDrain : null;
    this.canSend = typeof canSend === 'function' ? canSend : null;
    this.onDrop = typeof onDrop === 'function' ? onDrop : null;
    this.label = label;

    this.chunkBytes =
      Math.max(1, Math.round((this.sampleRate * this.chunkMs) / 1000)) *
      this.bytesPerSample;
    this.prebufferBytes =
      Math.max(0, Math.round((this.sampleRate * this.prebufferMs) / 1000)) *
      this.bytesPerSample;
    this.maxBufferBytes =
      Math.max(1, Math.round((this.sampleRate * this.maxBufferMs) / 1000)) *
      this.bytesPerSample;

    this.queue = [];
    this.queueOffset = 0;
    this.queueBytes = 0;
    this.timer = null;
    this.started = false;
    this.ended = false;
    this.finished = false;
    this.aborted = false;
  }

  push(buffer) {
    if (this.aborted || this.finished) return;
    if (!buffer || !buffer.length) return;

    let buf = buffer;
    if (buf.length % this.bytesPerSample !== 0) {
      const aligned = buf.length - (buf.length % this.bytesPerSample);
      if (aligned <= 0) return;
      buf = buf.subarray(0, aligned);
    }

    this.queue.push(buf);
    this.queueBytes += buf.length;

    if (this.queueBytes > this.maxBufferBytes) {
      const toDrop = this.queueBytes - this.maxBufferBytes;
      this._dropFront(toDrop);
      if (this.onDrop) {
        try {
          this.onDrop({ droppedBytes: toDrop, bufferedBytes: this.queueBytes, label: this.label });
        } catch (_) {
          // ignore onDrop errors
        }
      }
    }

    if (!this.started && this.queueBytes >= this.prebufferBytes) {
      this._start();
    }
  }

  end() {
    if (this.aborted || this.finished) return;
    this.ended = true;
    if (!this.started) {
      this._start({ ignorePrebuffer: true });
      return;
    }
    if (this.queueBytes === 0) {
      this._finish();
    }
  }

  abort() {
    if (this.aborted) return;
    this.aborted = true;
    this._stopTimer();
    this.queue = [];
    this.queueOffset = 0;
    this.queueBytes = 0;
  }

  _start(options = {}) {
    if (this.aborted || this.finished) return;
    if (this.started) return;
    const { ignorePrebuffer = false } = options;
    if (!ignorePrebuffer && this.queueBytes < this.prebufferBytes && !this.ended) {
      return;
    }
    this.started = true;

    // 先立即尝试发送一片，降低起播延迟；后续严格按 chunkMs 节流。
    this._tick();

    this.timer = setInterval(() => this._tick(), this.chunkMs);
    if (typeof this.timer.unref === 'function') {
      this.timer.unref();
    }
  }

  _stopTimer() {
    if (this.timer) {
      clearInterval(this.timer);
      this.timer = null;
    }
  }

  _tick() {
    if (this.aborted || this.finished) return;
    if (this.canSend && !this.canSend()) return;

    if (this.queueBytes === 0) {
      if (this.ended) {
        this._finish();
      }
      return;
    }

    let takeBytes = 0;
    if (this.queueBytes >= this.chunkBytes) {
      takeBytes = this.chunkBytes;
    } else if (this.ended) {
      takeBytes = this.queueBytes;
    } else {
      return;
    }

    if (takeBytes % this.bytesPerSample !== 0) {
      takeBytes -= takeBytes % this.bytesPerSample;
    }
    if (takeBytes <= 0) {
      if (this.ended) this._finish();
      return;
    }

    const out = this._dequeue(takeBytes);
    if (!out || !out.length) return;
    try {
      this.onChunk(out);
    } catch (_) {
      // ignore send errors
    }

    if (this.ended && this.queueBytes === 0) {
      this._finish();
    }
  }

  _finish() {
    if (this.finished) return;
    this.finished = true;
    this._stopTimer();
    if (this.onDrain) {
      try {
        this.onDrain();
      } catch (_) {
        // ignore onDrain errors
      }
    }
  }

  _dropFront(bytes) {
    let remaining = bytes;
    while (remaining > 0 && this.queue.length > 0) {
      const first = this.queue[0];
      const available = first.length - this.queueOffset;
      const toDrop = Math.min(available, remaining);
      this.queueOffset += toDrop;
      this.queueBytes -= toDrop;
      remaining -= toDrop;
      if (this.queueOffset >= first.length) {
        this.queue.shift();
        this.queueOffset = 0;
      }
    }
    if (this.queueBytes < 0) this.queueBytes = 0;
  }

  _dequeue(bytes) {
    if (bytes <= 0) return Buffer.alloc(0);
    if (!this.queue.length) return Buffer.alloc(0);

    const first = this.queue[0];
    const available = first.length - this.queueOffset;

    if (available >= bytes) {
      const out = first.subarray(this.queueOffset, this.queueOffset + bytes);
      this.queueOffset += bytes;
      this.queueBytes -= bytes;
      if (this.queueOffset >= first.length) {
        this.queue.shift();
        this.queueOffset = 0;
      }
      return out;
    }

    const out = Buffer.allocUnsafe(bytes);
    let outOffset = 0;
    let need = bytes;
    while (need > 0 && this.queue.length > 0) {
      const buf = this.queue[0];
      const avail = buf.length - this.queueOffset;
      const toCopy = Math.min(avail, need);
      buf.copy(out, outOffset, this.queueOffset, this.queueOffset + toCopy);
      outOffset += toCopy;
      need -= toCopy;
      this.queueOffset += toCopy;
      this.queueBytes -= toCopy;
      if (this.queueOffset >= buf.length) {
        this.queue.shift();
        this.queueOffset = 0;
      }
    }
    return outOffset === bytes ? out : out.subarray(0, outOffset);
  }
}

module.exports = {
  PcmPacer,
};

