#!/usr/bin/env python3
"""
简单的 TTS 流式脚本。

通过标准输入接收单条 JSON：
{
  "session_id": "...",
  "text": "...",
  "voice": "zh",
  "rate": 180,
  "volume": 1.0,
  "chunk_ms": 80
}

标准输出逐行返回 JSON：
- meta：语音信息
- audio_chunk：包含 base64 编码的 PCM16 数据
- complete：合成结束
- error：错误提示
"""
from __future__ import annotations

import audioop
import base64
import json
import os
import sys
import tempfile
import wave
from typing import Optional

try:
    import pyttsx3  # type: ignore
except Exception as exc:  # pylint: disable=broad-except
    payload = {
        "type": "error",
        "detail": f"导入 pyttsx3 失败，请先安装依赖：{exc}",
    }
    print(json.dumps(payload, ensure_ascii=False), flush=True)
    sys.exit(1)


def encode_chunk(data: bytes) -> str:
    return base64.b64encode(data).decode("ascii")


def select_voice(engine: "pyttsx3.engine.Engine", target: Optional[str]) -> Optional[str]:
    if not target:
        return None
    voices = engine.getProperty("voices")
    if not voices:
        return None
    target_lower = target.lower()
    for voice in voices:
        voice_id = getattr(voice, "id", "") or ""
        voice_name = getattr(voice, "name", "") or ""
        languages = getattr(voice, "languages", []) or []
        if target_lower in voice_id.lower() or target_lower in voice_name.lower():
            engine.setProperty("voice", voice_id)
            return voice_id
        if any(target_lower in str(lang).lower() for lang in languages):
            engine.setProperty("voice", voice_id)
            return voice_id
    return None


def synthesize(
    text: str,
    voice: Optional[str],
    rate: Optional[int],
    volume: Optional[float],
) -> tuple[str, Optional[str]]:
    engine = pyttsx3.init()
    final_voice = None
    if voice:
        final_voice = select_voice(engine, voice)
    if not final_voice:
        try:
            default_voice = engine.getProperty("voice")
            final_voice = str(default_voice) if default_voice else None
        except Exception:  # pylint: disable=broad-except
            final_voice = None
    if rate:
        engine.setProperty("rate", int(rate))
    if volume is not None:
        engine.setProperty("volume", max(0.1, min(float(volume), 1.0)))

    with tempfile.NamedTemporaryFile(delete=False, suffix=".wav") as tmp_file:
        tmp_path = tmp_file.name

    engine.save_to_file(text, tmp_path)
    engine.runAndWait()

    return tmp_path, final_voice


def stream_audio(session_id: str, wav_path: str, chunk_ms: int, voice: Optional[str]) -> None:
    try:
        with wave.open(wav_path, "rb") as wav_file:
            sample_rate = wav_file.getframerate()
            sample_width = wav_file.getsampwidth()
            channels = wav_file.getnchannels()
            total_frames = wav_file.getnframes()

            frames_per_chunk = max(int(sample_rate * max(chunk_ms, 20) / 1000.0), 1024)

            print(
                json.dumps(
                    {
                        "type": "meta",
                        "session_id": session_id,
                        "sample_rate": sample_rate,
                        "channels": 1 if channels > 1 else channels,
                        "voice": voice,
                    },
                    ensure_ascii=False,
                ),
                flush=True,
            )

            while True:
                raw = wav_file.readframes(frames_per_chunk)
                if not raw:
                    break

                chunk = raw
                if channels > 1:
                    chunk = audioop.tomono(chunk, sample_width, 0.5, 0.5)
                if sample_width != 2:
                    chunk = audioop.lin2lin(chunk, sample_width, 2)

                is_last = wav_file.tell() >= total_frames
                print(
                    json.dumps(
                        {
                            "type": "audio_chunk",
                            "session_id": session_id,
                            "chunk": encode_chunk(chunk),
                            "sample_rate": sample_rate,
                            "is_last": is_last,
                        },
                        ensure_ascii=False,
                    ),
                    flush=True,
                )

            duration = total_frames / float(sample_rate or 1)
            print(
                json.dumps(
                    {
                        "type": "complete",
                        "session_id": session_id,
                        "duration": duration,
                    },
                    ensure_ascii=False,
                ),
                flush=True,
            )
    finally:
        try:
            os.remove(wav_path)
        except OSError:
            pass


def main() -> None:
    raw = sys.stdin.readline()
    if not raw:
        return
    try:
        message = json.loads(raw)
    except json.JSONDecodeError as exc:
        print(json.dumps({"type": "error", "detail": f"无法解析输入：{exc}"}), flush=True)
        return

    session_id = str(message.get("session_id") or "")
    text = (message.get("text") or "").strip()

    if not session_id:
        print(json.dumps({"type": "error", "detail": "缺少 session_id"}), flush=True)
        return
    if not text:
        print(
            json.dumps(
                {"type": "complete", "session_id": session_id, "duration": 0.0},
                ensure_ascii=False,
            ),
            flush=True,
        )
        return

    voice = message.get("voice") or os.environ.get("TTS_VOICE")
    rate = message.get("rate") or os.environ.get("TTS_RATE")
    volume = message.get("volume") or os.environ.get("TTS_VOLUME")
    chunk_ms = int(message.get("chunk_ms") or os.environ.get("TTS_CHUNK_MS") or 80)

    try:
        wav_path, actual_voice = synthesize(
            text,
            str(voice) if voice else None,
            int(rate) if rate else None,
            float(volume) if volume else None,
        )
        stream_audio(session_id, wav_path, chunk_ms, actual_voice)
    except Exception as exc:  # pylint: disable=broad-except
        print(
            json.dumps(
                {
                    "type": "error",
                    "session_id": session_id,
                    "detail": f"TTS 合成失败：{exc}",
                },
                ensure_ascii=False,
            ),
            flush=True,
        )


if __name__ == "__main__":
    main()
