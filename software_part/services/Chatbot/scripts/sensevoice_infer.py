#!/usr/bin/env python3
"""
使用 SenseVoice (FunASR) 模型对输入音频进行语音识别。

依赖：
    pip install funasr
    # 可选：pip install modelscope  (若从 ModelScope 自动下载模型)

环境变量：
    SENSEVOICE_MODEL                模型名称或本地路径，默认 sensevoice_small
    SENSEVOICE_VAD_MODEL            VAD 模型，默认 fsmn-vad
    SENSEVOICE_PUNC_MODEL           标点模型（可选）
    SENSEVOICE_DEVICE               推理设备，默认 cpu
"""
from __future__ import annotations

import argparse
import json
import os
import sys
import time
from typing import Any, Dict, List


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="SenseVoice inference helper")
    parser.add_argument("--audio", required=True, help="待识别的音频文件路径")
    parser.add_argument("--language", help="提示模型音频语言（可选）")
    parser.add_argument("--prompt", help="文本提示（prompt，可选）")
    return parser.parse_args()


def format_segments(raw_segments: Any) -> List[Dict[str, Any]]:
    formatted: List[Dict[str, Any]] = []
    if isinstance(raw_segments, list):
        for item in raw_segments:
            if not isinstance(item, dict):
                continue
            text = item.get("text") or item.get("value") or ""
            if not text:
                continue
            segment = {
                "text": text,
            }
            if "start" in item:
                try:
                    segment["start"] = float(item["start"])
                except (ValueError, TypeError):
                    pass
            if "end" in item:
                try:
                    segment["end"] = float(item["end"])
                except (ValueError, TypeError):
                    pass
            formatted.append(segment)
    return formatted


def main() -> int:
    args = parse_args()

    audio_path = os.path.abspath(args.audio)
    if not os.path.exists(audio_path):
        print(f"音频文件不存在：{audio_path}", file=sys.stderr)
        return 2

    try:
        from funasr import AutoModel  # type: ignore
    except Exception as exc:  # pylint: disable=broad-except
        print(
            "导入 funasr 失败，请先执行 `pip install funasr`："
            f" {exc}",
            file=sys.stderr,
        )
        return 3

    model_name = os.environ.get("SENSEVOICE_MODEL", "iic/SenseVoiceSmall")
    vad_model = os.environ.get("SENSEVOICE_VAD_MODEL", "fsmn-vad")
    punc_model = os.environ.get("SENSEVOICE_PUNC_MODEL")
    device = os.environ.get("SENSEVOICE_DEVICE", "cpu")
    trust_remote_code = os.environ.get("SENSEVOICE_TRUST_REMOTE_CODE", "true").lower() != "false"
    remote_code = os.environ.get("SENSEVOICE_REMOTE_CODE", "model.py")

    try:
        model = AutoModel(
            model=model_name,
            trust_remote_code=trust_remote_code,
            remote_code=remote_code,
            vad_model=vad_model,
            vad_kwargs={
                "max_single_segment_time": 30000,
            },
            punc_model=punc_model,
            device=device,
        )
    except Exception as exc:  # pylint: disable=broad-except
        print(f"加载模型失败：{exc}", file=sys.stderr)
        return 4

    inference_kwargs: Dict[str, Any] = {}
    if args.language:
        inference_kwargs["language"] = args.language
    if args.prompt:
        inference_kwargs["text_prompt"] = args.prompt

    start_time = time.time()
    try:
        results = model.generate(
            input=audio_path,
            cache={},
            **inference_kwargs,
        )
    except Exception as exc:  # pylint: disable=broad-except
        print(f"推理失败：{exc}", file=sys.stderr)
        return 5
    duration = time.time() - start_time

    text = ""
    segments: List[Dict[str, Any]] = []
    raw_entry: Dict[str, Any] | None = None

    if isinstance(results, list) and results:
        candidate = results[0]
        if isinstance(candidate, dict):
            raw_entry = candidate
            text = str(candidate.get("text") or "").strip()
            segments = format_segments(candidate.get("segments") or candidate.get("sentences"))

    output = {
        "text": text,
        "segments": segments,
        "inference_seconds": duration,
        "model": model_name,
        "device": device,
    }
    if raw_entry is not None:
        output["raw_result"] = raw_entry
    print(json.dumps(output, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    sys.exit(main())
