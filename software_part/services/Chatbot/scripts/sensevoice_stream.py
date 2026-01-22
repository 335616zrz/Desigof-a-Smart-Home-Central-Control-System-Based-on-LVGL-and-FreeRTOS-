#!/usr/bin/env python3
"""
SenseVoice streaming helper.

通过标准输入接收 JSON 行：
{
  "session_id": "...",
  "type": "init" | "chunk" | "stop",
  "sample_rate": 16000,
  "language": "zh",
  "data": "<base64 pcm16>",
  "is_last": false
}

并通过标准输出返回 JSON 行：
{
  "session_id": "...",
  "type": "transcript" | "error",
  "text": "完整文本",
  "delta": "新增文本",
  "is_final": false
}
"""
from __future__ import annotations

import base64
import io
import json
import os
import re
import sys
import tempfile
import threading
from dataclasses import dataclass, field
from typing import Dict, Optional

os.environ.setdefault("FUNASR_LOG_LEVEL", "ERROR")
os.environ.setdefault("FUNASR_PROGRESS_BAR", "0")

import numpy as np
import soundfile as sf

try:
    from funasr import AutoModel  # type: ignore
except Exception as exc:  # pylint: disable=broad-except
    print(
        json.dumps(
            {
                "type": "error",
                "detail": f"导入 funasr 失败，请先安装依赖：{exc}",
            }
        ),
        flush=True,
    )
    sys.exit(1)


@dataclass
class SessionState:
    sample_rate: int = 16000
    language: Optional[str] = None
    buffer: bytearray = field(default_factory=bytearray)
    last_text: str = ""
    lock: threading.Lock = field(default_factory=threading.Lock)


MODEL_NAME = os.environ.get("SENSEVOICE_MODEL", "iic/SenseVoiceSmall")
VAD_MODEL = os.environ.get("SENSEVOICE_VAD_MODEL", "fsmn-vad")
PUNC_MODEL = os.environ.get("SENSEVOICE_PUNC_MODEL") or None
DEVICE = os.environ.get("SENSEVOICE_DEVICE", "cpu")
TRUST_REMOTE_CODE = os.environ.get("SENSEVOICE_TRUST_REMOTE_CODE", "true").lower() != "false"
REMOTE_CODE = os.environ.get("SENSEVOICE_REMOTE_CODE", "model.py")


def load_model() -> AutoModel:
    return AutoModel(
        model=MODEL_NAME,
        trust_remote_code=TRUST_REMOTE_CODE,
        remote_code=REMOTE_CODE,
        vad_model=VAD_MODEL,
        vad_kwargs={"max_single_segment_time": 30000},
        punc_model=PUNC_MODEL,
        device=DEVICE,
    )


MODEL = load_model()
SESSIONS: Dict[str, SessionState] = {}


def write_error(session_id: Optional[str], message: str) -> None:
    payload = {"type": "error", "detail": message}
    if session_id is not None:
        payload["session_id"] = session_id
    print(json.dumps(payload, ensure_ascii=False), flush=True)


def run_inference(session: SessionState) -> str:
    if not session.buffer:
        return ""

    pcm = np.frombuffer(session.buffer, dtype="<i2").astype(np.float32) / 32768.0
    if pcm.size == 0:
        return ""

    with tempfile.NamedTemporaryFile(delete=False, suffix=".wav") as tmp:
        sf.write(tmp.name, pcm, session.sample_rate, subtype="PCM_16")
        tmp_path = tmp.name

    try:
        kwargs = {}
        if session.language:
            kwargs["language"] = session.language
        result = MODEL.generate(input=tmp_path, cache={}, **kwargs)
    finally:
        try:
            os.remove(tmp_path)
        except OSError:
            pass

    if isinstance(result, list) and result:
        entry = result[0]
        if isinstance(entry, dict):
            raw_text = str(entry.get("text") or "")
            return clean_transcript(raw_text)
    return ""


TOKEN_PATTERN = re.compile(r"<\|[^|>]+\|>")


def clean_transcript(text: str) -> str:
    cleaned = TOKEN_PATTERN.sub("", text)
    cleaned = cleaned.replace("\n", " ")
    return cleaned.strip()


def handle_message(message: Dict[str, object]) -> None:
    session_id = str(message.get("session_id", ""))
    msg_type = message.get("type")

    if not session_id:
        write_error(None, "缺少 session_id")
        return

    if msg_type == "init":
        session = SESSIONS.setdefault(session_id, SessionState())
        session.sample_rate = int(message.get("sample_rate") or session.sample_rate)
        language = message.get("language")
        session.language = str(language) if language else None
        return

    session = SESSIONS.setdefault(session_id, SessionState())

    if msg_type == "chunk":
        data_b64 = message.get("data")
        if not data_b64:
            write_error(session_id, "缺少音频数据。")
            return

        try:
            chunk_bytes = base64.b64decode(data_b64)
        except Exception as exc:  # pylint: disable=broad-except
            write_error(session_id, f"音频数据无法解码：{exc}")
            return

        sample_rate = message.get("sample_rate")
        if sample_rate:
            session.sample_rate = int(sample_rate)

        is_last = bool(message.get("is_last"))

        with session.lock:
            session.buffer.extend(chunk_bytes)
            text = run_inference(session)

        delta = text[len(session.last_text) :] if text.startswith(session.last_text) else text
        session.last_text = text

        payload = {
            "session_id": session_id,
            "type": "transcript",
            "text": text,
            "delta": delta,
            "is_final": is_last and bool(text),
        }
        print(json.dumps(payload, ensure_ascii=False), flush=True)

        if is_last:
            cleanup_session(session_id)
        return

    if msg_type == "stop":
        with session.lock:
            text = run_inference(session)

        delta = text[len(session.last_text) :] if text.startswith(session.last_text) else text
        session.last_text = text

        payload = {
            "session_id": session_id,
            "type": "transcript",
            "text": text,
            "delta": delta,
            "is_final": True,
        }
        print(json.dumps(payload, ensure_ascii=False), flush=True)
        cleanup_session(session_id)
        return

    write_error(session_id, f"未知的消息类型：{msg_type}")


def cleanup_session(session_id: str) -> None:
    session = SESSIONS.pop(session_id, None)
    if session is not None:
        session.buffer.clear()


def main() -> None:
    for line in sys.stdin:
        raw = line.strip()
        if not raw:
            continue
        try:
            message = json.loads(raw)
        except json.JSONDecodeError as exc:
            write_error(None, f"无法解析输入：{exc}")
            continue
        try:
            handle_message(message)
        except Exception as exc:  # pylint: disable=broad-except
            session_id = message.get("session_id")
            write_error(str(session_id) if session_id else None, f"处理消息时出现异常：{exc}")


if __name__ == "__main__":
    main()
