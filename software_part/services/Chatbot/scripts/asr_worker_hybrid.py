#!/usr/bin/env python3
"""
Hybrid ASR worker: low-latency streaming partial + high-accuracy final.

- Streaming model (Paraformer streaming by default) emits partial transcripts quickly.
- SenseVoice runs once on utterance end (stop/is_last) and emits final transcript.
- Optional server-side denoise via noisereduce (final only).

Protocol (JSON lines over stdin/stdout), compatible with Node.js RealtimeSession:
Input:
  {"session_id": "...", "type": "init",  "sample_rate": 16000, "language": "zh"}
  {"session_id": "...", "type": "chunk", "audio": "<base64 pcm16>", "sample_rate": 16000, "is_last": false}
  {"session_id": "...", "type": "stop"}
  {"session_id": "...", "type": "close"}

Output:
  {"session_id": "...", "type": "transcript", "text": "...", "delta": "...", "is_final": false|true}
  {"session_id": "...", "type": "error", "detail": "..."}
"""

from __future__ import annotations

import base64
import json
import os
import re
import sys
import tempfile
import threading
import time
import wave
from dataclasses import dataclass, field
from typing import Any, Dict, Optional

import numpy as np

os.environ.setdefault("FUNASR_LOG_LEVEL", "ERROR")
os.environ.setdefault("FUNASR_PROGRESS_BAR", "0")

try:
  from funasr import AutoModel  # type: ignore
  try:
    from funasr.utils.postprocess_utils import rich_transcription_postprocess  # type: ignore
  except Exception:  # pylint: disable=broad-except
    rich_transcription_postprocess = None  # type: ignore
except Exception as exc:  # pylint: disable=broad-except
  print(
    json.dumps(
      {
        "type": "error",
        "detail": f"导入 funasr 失败，请先安装依赖：{exc}",
      },
      ensure_ascii=False,
    ),
    flush=True,
  )
  sys.exit(1)

try:
  import noisereduce as nr  # type: ignore
  _NR_AVAILABLE = True
  _NR_IMPORT_ERROR: Optional[Exception] = None
except Exception as exc:  # pylint: disable=broad-except
  nr = None  # type: ignore
  _NR_AVAILABLE = False
  _NR_IMPORT_ERROR = exc


def now_ms() -> int:
  return int(time.time() * 1000)


def write_json(obj: dict) -> None:
  print(json.dumps(obj, ensure_ascii=False), flush=True)


def write_error(session_id: Optional[str], detail: str) -> None:
  payload = {"type": "error", "detail": detail}
  if session_id is not None:
    payload["session_id"] = session_id
  write_json(payload)

def env_bool(name: str, default: bool) -> bool:
  raw = os.environ.get(name)
  if raw is None:
    return default
  return str(raw).strip().lower() in ("1", "true", "yes", "y", "on")


# -------------------- Streaming (partial) config --------------------

ENABLE_STREAMING_PARTIAL = env_bool("ENABLE_STREAMING_PARTIAL", True)

ASR_STREAMING_MODEL = os.environ.get("ASR_STREAMING_MODEL", "paraformer-zh-streaming")
ASR_STREAMING_MODEL_REVISION = os.environ.get("ASR_STREAMING_MODEL_REVISION", "v2.0.4")
ASR_STREAMING_DEVICE = os.environ.get(
  "ASR_STREAMING_DEVICE",
  os.environ.get("SENSEVOICE_DEVICE", "cpu"),
)

CHUNK_SIZE = [0, 10, 5]  # roughly 600ms windows (FunASR recommended defaults)
ENCODER_CHUNK_LOOK_BACK = 4
DECODER_CHUNK_LOOK_BACK = 1
SAMPLE_RATE_DEFAULT = 16_000
SAMPLES_PER_STEP = CHUNK_SIZE[1] * 960
BYTES_PER_STEP = SAMPLES_PER_STEP * 2
MIN_EMIT_INTERVAL_MS = 120


# -------------------- SenseVoice (final) config --------------------

ENABLE_SENSEVOICE_FINAL = env_bool("ENABLE_SENSEVOICE_FINAL", True)

SENSEVOICE_MODEL_NAME = os.environ.get("SENSEVOICE_MODEL", "iic/SenseVoiceSmall")
SENSEVOICE_VAD_MODEL = os.environ.get("SENSEVOICE_VAD_MODEL", "fsmn-vad")
SENSEVOICE_PUNC_MODEL = os.environ.get("SENSEVOICE_PUNC_MODEL") or None
SENSEVOICE_DEVICE = os.environ.get("SENSEVOICE_DEVICE", "cpu")
SENSEVOICE_TRUST_REMOTE_CODE = env_bool("SENSEVOICE_TRUST_REMOTE_CODE", False)
SENSEVOICE_REMOTE_CODE = (os.environ.get("SENSEVOICE_REMOTE_CODE") or "").strip() or None
SENSEVOICE_USE_ITN = os.environ.get("SENSEVOICE_USE_ITN", "true").lower() != "false"
SENSEVOICE_MERGE_VAD = os.environ.get("SENSEVOICE_MERGE_VAD", "true").lower() != "false"


# -------------------- Denoise (final) config --------------------

NOISEREDUCE_ENABLED = env_bool(
  "ENABLE_NOISEREDUCE",
  env_bool("ENABLE_NOISE_REDUCTION", False),
)
NOISEREDUCE_PROP_DECREASE = float(os.environ.get("NOISEREDUCE_PROP_DECREASE", "0.7"))
_NR_WARNED = False


# -------------------- Models (lazy init) --------------------

STREAMING_MODEL_OBJ: Any = None
SENSEVOICE_MODEL_OBJ: Any = None


def load_streaming_model() -> Any:
  return AutoModel(
    model=ASR_STREAMING_MODEL,
    model_revision=ASR_STREAMING_MODEL_REVISION,
    device=ASR_STREAMING_DEVICE,
    disable_update=True,
  )


def load_sensevoice_model() -> Any:
  kwargs: Dict[str, Any] = {
    "model": SENSEVOICE_MODEL_NAME,
    "trust_remote_code": SENSEVOICE_TRUST_REMOTE_CODE,
    "vad_model": SENSEVOICE_VAD_MODEL,
    "vad_kwargs": {"max_single_segment_time": 30000},
    "punc_model": SENSEVOICE_PUNC_MODEL,
    "device": SENSEVOICE_DEVICE,
    "disable_update": True,
  }
  if SENSEVOICE_TRUST_REMOTE_CODE and SENSEVOICE_REMOTE_CODE:
    kwargs["remote_code"] = SENSEVOICE_REMOTE_CODE
  return AutoModel(**kwargs)


def get_streaming_model() -> Any:
  global STREAMING_MODEL_OBJ  # noqa: PLW0603
  if STREAMING_MODEL_OBJ is None:
    STREAMING_MODEL_OBJ = load_streaming_model()
  return STREAMING_MODEL_OBJ


def get_sensevoice_model() -> Any:
  global SENSEVOICE_MODEL_OBJ  # noqa: PLW0603
  if SENSEVOICE_MODEL_OBJ is None:
    SENSEVOICE_MODEL_OBJ = load_sensevoice_model()
  return SENSEVOICE_MODEL_OBJ


# -------------------- Audio helpers --------------------

TOKEN_PATTERN = re.compile(r"<\\|[^|>]+\\|>")


def clean_transcript(text: str) -> str:
  raw = text or ""
  if rich_transcription_postprocess is not None:
    try:
      raw = rich_transcription_postprocess(raw)  # type: ignore[misc]
    except Exception:  # pylint: disable=broad-except
      pass
  cleaned = TOKEN_PATTERN.sub("", raw)
  cleaned = cleaned.replace("\n", " ")
  return cleaned.strip()


def pcm16_to_f32(pcm_bytes: bytes) -> np.ndarray:
  if not pcm_bytes:
    return np.zeros(0, dtype=np.float32)
  pcm = np.frombuffer(pcm_bytes, dtype="<i2").astype(np.float32)
  return pcm / 32768.0


def f32_to_pcm16_bytes(audio: np.ndarray) -> bytes:
  if audio.size == 0:
    return b""
  clipped = np.clip(audio, -1.0, 1.0)
  pcm = (clipped * 32768.0).astype("<i2", copy=False)
  return pcm.tobytes()


def maybe_denoise(audio: np.ndarray, sample_rate: int) -> np.ndarray:
  global _NR_WARNED  # noqa: PLW0603

  if audio.size == 0 or not NOISEREDUCE_ENABLED:
    return audio

  if not _NR_AVAILABLE:
    if not _NR_WARNED:
      _NR_WARNED = True
      sys.stderr.write(
        f"[asr_worker_hybrid] noisereduce 不可用，已跳过降噪：{_NR_IMPORT_ERROR}\n"
      )
    return audio

  try:
    return nr.reduce_noise(  # type: ignore[union-attr]
      y=audio,
      sr=sample_rate,
      prop_decrease=NOISEREDUCE_PROP_DECREASE,
    ).astype(np.float32, copy=False)
  except Exception as exc:  # pylint: disable=broad-except
    write_error(None, f"noisereduce 处理失败（已跳过）：{exc}")
    return audio


def write_wav_tmp(pcm16_bytes: bytes, sample_rate: int) -> str:
  tmp = tempfile.NamedTemporaryFile(delete=False, suffix=".wav")
  tmp_path = tmp.name
  tmp.close()
  with wave.open(tmp_path, "wb") as wf:
    wf.setnchannels(1)
    wf.setsampwidth(2)
    wf.setframerate(int(sample_rate))
    wf.writeframes(pcm16_bytes)
  return tmp_path


# -------------------- Session state --------------------


@dataclass
class SessionState:
  session_id: str
  sample_rate: int = SAMPLE_RATE_DEFAULT
  language: Optional[str] = None
  streaming_buffer: bytearray = field(default_factory=bytearray)
  utterance_buffer: bytearray = field(default_factory=bytearray)
  asr_cache: dict = field(default_factory=dict)
  last_text: str = ""
  last_emit_ms: int = 0
  created_ms: int = field(default_factory=now_ms)
  lock: threading.Lock = field(default_factory=threading.Lock)


SESSIONS: Dict[str, SessionState] = {}


def emit_transcript(session: SessionState, text: str, is_final: bool) -> None:
  new_text = (text or "").strip()
  if not new_text:
    return

  if new_text.startswith(session.last_text):
    delta = new_text[len(session.last_text):]
  else:
    delta = new_text

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


def run_streaming_partial(session: SessionState, speech_chunk: np.ndarray) -> None:
  if not ENABLE_STREAMING_PARTIAL:
    return
  if speech_chunk.size == 0:
    return

  try:
    model = get_streaming_model()
  except Exception as exc:  # pylint: disable=broad-except
    write_error(session.session_id, f"加载流式 ASR 模型失败：{exc}")
    return

  try:
    result = model.generate(
      input=speech_chunk,
      cache=session.asr_cache,
      is_final=False,
      chunk_size=CHUNK_SIZE,
      encoder_chunk_look_back=ENCODER_CHUNK_LOOK_BACK,
      decoder_chunk_look_back=DECODER_CHUNK_LOOK_BACK,
      language=session.language or None,
    )
  except Exception as exc:  # pylint: disable=broad-except
    write_error(session.session_id, f"流式 ASR 推理失败：{exc}")
    return

  if not result or not isinstance(result, list):
    return
  entry = result[0] if isinstance(result[0], dict) else None
  if not isinstance(entry, dict):
    return
  text = str(entry.get("text") or "").strip()

  if now_ms() - session.last_emit_ms < MIN_EMIT_INTERVAL_MS:
    if text:
      session.last_text = text
    return

  emit_transcript(session, text, is_final=False)


def run_sensevoice_final(session: SessionState, pcm16_bytes: bytes) -> None:
  if not ENABLE_SENSEVOICE_FINAL:
    # 若禁用 SenseVoice，则以流式缓存清空为“结束”（不输出 final）
    return
  if not pcm16_bytes:
    return

  audio = pcm16_to_f32(pcm16_bytes)
  if audio.size == 0:
    return

  audio = maybe_denoise(audio, session.sample_rate)
  wav_path = write_wav_tmp(f32_to_pcm16_bytes(audio), session.sample_rate)
  try:
    model = get_sensevoice_model()
  except Exception as exc:  # pylint: disable=broad-except
    try:
      os.remove(wav_path)
    except OSError:
      pass
    write_error(session.session_id, f"加载 SenseVoice 模型失败：{exc}")
    return

  try:
    infer_kwargs: Dict[str, Any] = {}
    if session.language:
      infer_kwargs["language"] = session.language
    infer_kwargs["use_itn"] = SENSEVOICE_USE_ITN
    infer_kwargs["merge_vad"] = SENSEVOICE_MERGE_VAD

    try:
      result = model.generate(input=wav_path, cache={}, **infer_kwargs)
    except TypeError:
      fallback_kwargs = {"language": session.language} if session.language else {}
      result = model.generate(input=wav_path, cache={}, **fallback_kwargs)
  except Exception as exc:  # pylint: disable=broad-except
    write_error(session.session_id, f"SenseVoice 推理失败：{exc}")
    return
  finally:
    try:
      os.remove(wav_path)
    except OSError:
      pass

  if not result or not isinstance(result, list):
    return
  entry = result[0] if isinstance(result[0], dict) else None
  if not isinstance(entry, dict):
    return
  text = clean_transcript(str(entry.get("text") or ""))
  emit_transcript(session, text, is_final=True)


def reset_for_next_utterance(session: SessionState) -> None:
  session.streaming_buffer.clear()
  session.utterance_buffer.clear()
  session.asr_cache.clear()
  session.last_text = ""
  session.last_emit_ms = 0


def handle_init(msg: dict) -> None:
  session_id = str(msg.get("session_id") or "")
  if not session_id:
    write_error(None, "缺少 session_id")
    return

  session = SESSIONS.get(session_id)
  if session is None:
    session = SessionState(session_id=session_id)
    SESSIONS[session_id] = session

  session.sample_rate = int(msg.get("sample_rate") or SAMPLE_RATE_DEFAULT)
  language = msg.get("language")
  session.language = str(language) if language else None
  reset_for_next_utterance(session)


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

  try:
    chunk_bytes = base64.b64decode(audio_b64)
  except Exception as exc:  # pylint: disable=broad-except
    write_error(session_id, f"音频数据无法解码：{exc}")
    return

  sample_rate = msg.get("sample_rate")
  if sample_rate:
    session.sample_rate = int(sample_rate)

  is_last = bool(msg.get("is_last", False))

  with session.lock:
    session.streaming_buffer.extend(chunk_bytes)
    session.utterance_buffer.extend(chunk_bytes)

    while len(session.streaming_buffer) >= BYTES_PER_STEP:
      part = bytes(session.streaming_buffer[:BYTES_PER_STEP])
      del session.streaming_buffer[:BYTES_PER_STEP]
      run_streaming_partial(session, pcm16_to_f32(part))

    if not is_last:
      return

    pcm = bytes(session.utterance_buffer)
    reset_for_next_utterance(session)

  run_sensevoice_final(session, pcm)


def handle_stop(msg: dict) -> None:
  session_id = str(msg.get("session_id") or "")
  if not session_id:
    write_error(None, "缺少 session_id")
    return

  session = SESSIONS.get(session_id)
  if session is None:
    return

  with session.lock:
    pcm = bytes(session.utterance_buffer)
    reset_for_next_utterance(session)

  run_sensevoice_final(session, pcm)


def handle_close(msg: dict) -> None:
  session_id = str(msg.get("session_id") or "")
  if not session_id:
    return
  session = SESSIONS.pop(session_id, None)
  if session is not None:
    reset_for_next_utterance(session)


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
