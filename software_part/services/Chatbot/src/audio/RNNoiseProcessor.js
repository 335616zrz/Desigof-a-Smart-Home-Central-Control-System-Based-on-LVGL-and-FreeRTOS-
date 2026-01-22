/**
 * RNNoise WASM 降噪处理器
 * 基于 @jitsi/rnnoise-wasm 的底层 Emscripten 绑定
 */

const path = require('path');

class RNNoiseProcessor {
  constructor() {
    this.module = null;
    this.denoiseState = null;
    this.initialized = false;
    this.frameSize = 480; // RNNoise 固定帧大小 (48kHz 10ms)
    this.inputSampleRate = 16000;
    this.rnnoiseSampleRate = 48000;

    // WASM 内存指针
    this.pcmInputPointer = null;
    this.pcmOutputPointer = null;
  }

  async init() {
    if (this.initialized) return;

    try {
      // 使用同步版本（内联了 WASM）
      const rnnoiseModulePath = path.join(
        require.resolve('@jitsi/rnnoise-wasm'),
        '../dist/rnnoise-sync.js'
      );

      // CommonJS require 同步模块（ES Module 转译后需要 .default）
      const rnnoiseModule = require(rnnoiseModulePath);
      const createRNNWasmModuleSync = rnnoiseModule.default || rnnoiseModule;

      // 创建 WASM 模块实例
      this.module = await createRNNWasmModuleSync();

      // 创建降噪状态
      this.denoiseState = this.module._rnnoise_create();
      if (!this.denoiseState) {
        throw new Error('rnnoise_create 返回 null');
      }

      // 分配 WASM 内存 (480 个 float32 = 1920 bytes)
      const bufferBytes = this.frameSize * 4;
      this.pcmInputPointer = this.module._malloc(bufferBytes);
      this.pcmOutputPointer = this.module._malloc(bufferBytes);

      if (!this.pcmInputPointer || !this.pcmOutputPointer) {
        throw new Error('WASM 内存分配失败');
      }

      this.initialized = true;
      console.log('[RNNoise] 初始化成功');
    } catch (error) {
      console.error('[RNNoise] 初始化失败:', error.message);
      this.cleanup();
      throw error;
    }
  }

  /**
   * 简单线性重采样 16kHz -> 48kHz
   */
  resample16to48(pcm16Buffer) {
    const ratio = 3; // 48000 / 16000 = 3
    const inputSamples = pcm16Buffer.length / 2;
    const outputSamples = inputSamples * ratio;
    const output = Buffer.alloc(outputSamples * 2);

    for (let i = 0; i < inputSamples; i++) {
      const sample = pcm16Buffer.readInt16LE(i * 2);
      for (let j = 0; j < ratio; j++) {
        output.writeInt16LE(sample, (i * ratio + j) * 2);
      }
    }
    return output;
  }

  /**
   * 简单线性重采样 48kHz -> 16kHz
   */
  resample48to16(pcm16Buffer) {
    const ratio = 3; // 48000 / 16000 = 3
    const inputSamples = pcm16Buffer.length / 2;
    const outputSamples = Math.floor(inputSamples / ratio);
    const output = Buffer.alloc(outputSamples * 2);

    for (let i = 0; i < outputSamples; i++) {
      const sample = pcm16Buffer.readInt16LE(i * ratio * 2);
      output.writeInt16LE(sample, i * 2);
    }
    return output;
  }

  /**
   * 处理单个 480 样本帧 (Float32)
   */
  processFrame(inputFloat32) {
    if (!this.initialized || !this.module || !this.denoiseState) {
      return inputFloat32;
    }

    // 写入输入到 WASM 内存
    this.module.HEAPF32.set(inputFloat32, this.pcmInputPointer >> 2);

    // 调用 RNNoise 处理
    this.module._rnnoise_process_frame(
      this.denoiseState,
      this.pcmOutputPointer,
      this.pcmInputPointer
    );

    // 读取输出
    const output = new Float32Array(this.frameSize);
    output.set(
      this.module.HEAPF32.subarray(
        this.pcmOutputPointer >> 2,
        (this.pcmOutputPointer >> 2) + this.frameSize
      )
    );

    return output;
  }

  /**
   * 处理 PCM16 音频数据
   * @param {Buffer} pcm16Buffer - 16kHz PCM16 单声道音频
   * @param {number} sampleRate - 输入采样率（默认 16000）
   * @returns {Buffer} 降噪后的 16kHz PCM16 音频
   */
  processPCM16(pcm16Buffer, sampleRate = 16000) {
    if (!this.initialized || !this.module) {
      return pcm16Buffer;
    }

    if (sampleRate !== this.inputSampleRate) {
      console.warn(`[RNNoise] 不支持的采样率 ${sampleRate}，跳过降噪`);
      return pcm16Buffer;
    }

    try {
      // 1. 重采样到 48kHz
      const pcm48 = this.resample16to48(pcm16Buffer);

      // 2. 转换为 Float32Array
      const samples48 = new Float32Array(pcm48.length / 2);
      for (let i = 0; i < samples48.length; i++) {
        samples48[i] = pcm48.readInt16LE(i * 2) / 32768.0;
      }

      // 3. 按 480 样本帧处理
      const denoisedFrames = [];
      for (let i = 0; i + this.frameSize <= samples48.length; i += this.frameSize) {
        const frame = samples48.slice(i, i + this.frameSize);
        const denoised = this.processFrame(frame);
        denoisedFrames.push(denoised);
      }

      // 4. 合并帧
      const totalSamples = denoisedFrames.length * this.frameSize;
      const denoised48 = new Float32Array(totalSamples);
      for (let i = 0; i < denoisedFrames.length; i++) {
        denoised48.set(denoisedFrames[i], i * this.frameSize);
      }

      // 5. 转换回 PCM16
      const pcm48Denoised = Buffer.alloc(denoised48.length * 2);
      for (let i = 0; i < denoised48.length; i++) {
        const sample = Math.max(-1, Math.min(1, denoised48[i]));
        pcm48Denoised.writeInt16LE(Math.round(sample * 32767), i * 2);
      }

      // 6. 重采样回 16kHz
      const pcm16Denoised = this.resample48to16(pcm48Denoised);
      return pcm16Denoised;
    } catch (error) {
      console.error('[RNNoise] 处理失败:', error.message);
      return pcm16Buffer;
    }
  }

  cleanup() {
    if (this.module) {
      if (this.denoiseState) {
        try {
          this.module._rnnoise_destroy(this.denoiseState);
        } catch (e) { /* ignore */ }
        this.denoiseState = null;
      }
      if (this.pcmInputPointer) {
        try {
          this.module._free(this.pcmInputPointer);
        } catch (e) { /* ignore */ }
        this.pcmInputPointer = null;
      }
      if (this.pcmOutputPointer) {
        try {
          this.module._free(this.pcmOutputPointer);
        } catch (e) { /* ignore */ }
        this.pcmOutputPointer = null;
      }
    }
    this.module = null;
    this.initialized = false;
  }

  destroy() {
    this.cleanup();
    console.log('[RNNoise] 已销毁');
  }
}

module.exports = { RNNoiseProcessor };
