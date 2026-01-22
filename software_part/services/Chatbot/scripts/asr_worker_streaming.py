#!/usr/bin/env python3
"""
Multi-session ASR worker using FunASR.

Goals:
- Single long-lived process shared by many WebSocket sessions.
- Incremental inference (streaming-style) with per-session caches.
- Simple JSON-line protocol over stdin/stdout, compatible with the existing
  Node.js RealtimeSession expectations.
"""
from __future__ import annotations

import base64
import json
import os
import sys
import threading
import time
from dataclasses import dataclass, field
from typing import Dict, Optional, Any

import numpy as np

os.environ.setdefault("FUNASR_LOG_LEVEL", "ERROR")
os.environ.setdefault("FUNASR_PROGRESS_BAR", "0")

try:
  # FunASR is optional at runtime; if missing, we report an error once.
  from funasr import AutoModel  # type: ignore
except Exception as exc:  # pylint: disable=broad-except
  payload = {
    "type": "error",
    "detail": f"导入 funasr 失败，请先安装依赖：{exc}",
  }
  print(json.dumps(payload, ensure_ascii=False), flush=True)
  sys.exit(1)


ASR_MODEL = None


def load_asr_model() -> Any:
  """
  Load a streaming-capable ASR model.

  By default, prefer paraformer-zh-streaming. The actual model can be
  overridden via environment variable ASR_STREAMING_MODEL if needed.
  """
  model_name = os.environ.get("ASR_STREAMING_MODEL", "paraformer-zh-streaming")
  model_revision = os.environ.get("ASR_STREAMING_MODEL_REVISION", "v2.0.4")
  device = os.environ.get("ASR_STREAMING_DEVICE", os.environ.get("SENSEVOICE_DEVICE", "cpu"))
  return AutoModel(
    model=model_name,
    model_revision=model_revision,
    device=device,
    disable_update=True,
  )


CHUNK_SIZE = [0, 10, 5]  # roughly 600ms windows
ENCODER_CHUNK_LOOK_BACK = 4
DECODER_CHUNK_LOOK_BACK = 1

SAMPLE_RATE_DEFAULT = 16_000
SAMPLES_PER_STEP = CHUNK_SIZE[1] * 960
BYTES_PER_STEP = SAMPLES_PER_STEP * 2


@dataclass
class SessionState:
  session_id: str
  sample_rate: int = SAMPLE_RATE_DEFAULT
  language: Optional[str] = None
  pcm_buffer: bytearray = field(default_factory=bytearray)
  asr_cache: dict = field(default_factory=dict)
  last_text: str = ""
  last_emit_ms: int = 0
  created_ms: int = field(default_factory=lambda: int(time.time() * 1000))
  lock: threading.Lock = field(default_factory=threading.Lock)


SESSIONS: Dict[str, SessionState] = {}
MIN_EMIT_INTERVAL_MS = 120


def now_ms() -> int:
  return int(time.time() * 1000)


def write_json(obj: dict) -> None:
  print(json.dumps(obj, ensure_ascii=False), flush=True)


def write_error(session_id: Optional[str], detail: str) -> None:
  payload = {"type": "error", "detail": detail}
  if session_id is not None:
    payload["session_id"] = session_id
  write_json(payload)


def pcm16_to_f32(pcm_bytes: bytes) -> np.ndarray:
  if not pcm_bytes:
    return np.zeros(0, dtype=np.float32)
  pcm = np.frombuffer(pcm_bytes, dtype="<i2").astype(np.float32)
  return pcm / 32768.0


def emit_transcript(session: SessionState, text: str, is_final: bool) -> None:
  new_text = text or ""
  if not new_text and not is_final:
    return

  if new_text.startswith(session.last_text):
    delta = new_text[len(session.last_text):]
  else:
    delta = new_text

  if not new_text and not delta:
    return

  session.last_text = new_text
  session.last_emit_ms = now_ms()

  write_json(
    {
      "type": "transcript",
      "session_id": session.session_id,
      "text": new_text,
      "delta": delta,
      "is_final": bool(is_final),
    }
  )


def run_asr_streaming(session: SessionState, speech_chunk: np.ndarray, is_final: bool) -> None:
  global ASR_MODEL  # noqa: PLW0603
  if ASR_MODEL is None:
    try:
      ASR_MODEL = load_asr_model()
    except Exception as exc:  # pylint: disable=broad-except
      write_error(session.session_id, f"加载 ASR 模型失败：{exc}")
      return

  if speech_chunk.size == 0 and not is_final:
    return

  try:
    result = ASR_MODEL.generate(
      input=speech_chunk,
      cache=session.asr_cache,
      is_final=is_final,
      chunk_size=CHUNK_SIZE,
      encoder_chunk_look_back=ENCODER_CHUNK_LOOK_BACK,
      decoder_chunk_look_back=DECODER_CHUNK_LOOK_BACK,
      language=session.language or None,
    )
  except Exception as exc:  # pylint: disable=broad-except
    write_error(session.session_id, f"ASR 推理失败：{exc}")
    return

  if not result or not isinstance(result, list):
    return
  entry = result[0] if isinstance(result[0], dict) else None
  if not isinstance(entry, dict):
    return
  text = str(entry.get("text") or "").strip()

  if not is_final and now_ms() - session.last_emit_ms < MIN_EMIT_INTERVAL_MS:
    if text:
      session.last_text = text
    return

  emit_transcript(session, text, is_final=is_final)


def handle_init(msg: dict) -> None:
  session_id = str(msg.get("session_id") or "")
  if not session_id:
    write_error(None, "缺少 session_id")
    return

  sample_rate = int(msg.get("sample_rate") or SAMPLE_RATE_DEFAULT)
  language = msg.get("language")
  if language is not None:
    language = str(language) or None

  session = SESSIONS.get(session_id)
  if session is None:
    session = SessionState(session_id=session_id)
    SESSIONS[session_id] = session

  session.sample_rate = sample_rate
  session.language = language
  session.pcm_buffer.clear()
  session.asr_cache.clear()
  session.last_text = ""
  session.last_emit_ms = 0


def handle_chunk(msg: dict) -> None:
  session_id = str(msg.get("session_id") or "")
  if not session_id:
    write_error(None, "缺少 session_id")
    return

  session = SESSIONS.get(session_id)
  if session is None:
    session = SessionState(session_id=session_id)
    SESSIONS[session_id] = session

  audio_b64 = msg.get("audio")
  if not audio_b64 or not isinstance(audio_b64, str):
    write_error(session_id, "缺少音频数据。")
    return

  is_last = bool(msg.get("is_last", False))

  try:
    chunk_bytes = base64.b64decode(audio_b64)
  except Exception as exc:  # pylint: disable=broad-except
    write_error(session_id, f"音频数据无法解码：{exc}")
    return

  with session.lock:
    session.pcm_buffer.extend(chunk_bytes)

    while len(session.pcm_buffer) >= BYTES_PER_STEP or (is_last and session.pcm_buffer):
      take = min(BYTES_PER_STEP, len(session.pcm_buffer))
      part = bytes(session.pcm_buffer[:take])
      del session.pcm_buffer[:take]

      speech = pcm16_to_f32(part)
      final_chunk = is_last and not session.pcm_buffer
      run_asr_streaming(session, speech, is_final=final_chunk)


def handle_stop(msg: dict) -> None:
  session_id = str(msg.get("session_id") or "")
  if not session_id:
    write_error(None, "缺少 session_id")
    return

  session = SESSIONS.get(session_id)
  if session is None:
    return

  with session.lock:
    remaining = bytes(session.pcm_buffer)
    session.pcm_buffer.clear()

  speech = pcm16_to_f32(remaining)
  run_asr_streaming(session, speech, is_final=True)


def handle_close(msg: dict) -> None:
  session_id = str(msg.get("session_id") or "")
  if not session_id:
    return
  session = SESSIONS.pop(session_id, None)
  if session is not None:
    session.pcm_buffer.clear()
    session.asr_cache.clear()


def handle_message(msg: dict) -> None:
  msg_type = msg.get("type")
  if msg_type == "init":
    handle_init(msg)
  elif msg_type == "chunk":
    handle_chunk(msg)
  elif msg_type == "stop":
    handle_stop(msg)
  elif msg_type == "close":
    handle_close(msg)
  else:
    session_id = msg.get("session_id")
    write_error(str(session_id) if session_id else None, f"未知的消息类型：{msg_type}")


def main() -> None:
  for line in sys.stdin:
    raw = line.strip()
    if not raw:
      continue
    try:
      msg = json.loads(raw)
    except json.JSONDecodeError as exc:
      write_error(None, f"无法解析输入：{exc}")
      continue
    try:
      handle_message(msg)
    except Exception as exc:  # pylint: disable=broad-except
      session_id = msg.get("session_id")
      write_error(str(session_id) if session_id else None, f"处理消息时出现异常：{exc}")


if __name__ == "__main__":
  main()
