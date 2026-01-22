#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
扫描音乐资产目录，生成 index.json（标题+可播放 URL）。
- 默认资源目录指向仓库里的 services/Music_Server，可用环境变量 MUSIC_ROOT_DIR 覆盖。
- 默认 BASE_URL 使用 https://music-server.local/（mDNS 域名），亦可用
  MUSIC_BASE_URL 指定，例如 https://192.168.70.149/。
- 自动 URL 编码中文、空格及特殊字符。
- 默认只扫描当前目录；如需递归，把 RECURSIVE=True。
"""

import os, json, sys
import wave
from pathlib import Path
from urllib.parse import quote

PROJECT_ROOT   = Path(__file__).resolve().parents[2]
DEFAULT_ROOT   = PROJECT_ROOT / "services" / "Music_Server"
ROOT_DIR       = Path(os.environ.get("MUSIC_ROOT_DIR", DEFAULT_ROOT)).expanduser()
DEFAULT_URL    = "https://music-server.local/"   # mDNS 域名，固件与文档保持一致
BASE_URL       = os.environ.get("MUSIC_BASE_URL", DEFAULT_URL)
if not BASE_URL.endswith('/'):
    BASE_URL += '/'
RECURSIVE      = False                      # 需要递归改为 True
EXTS           = {".wav", ".mp3", ".flac", ".aac", ".m4a", ".ogg"}

if not ROOT_DIR.exists():
    raise SystemExit(f"音频目录不存在：{ROOT_DIR}")

def is_audio(p: str) -> bool:
    return os.path.splitext(p)[1].lower() in EXTS

def rel_unix(p: str) -> str:
    # 相对路径统一成 / 分隔
    return p.replace(os.sep, "/")

def wav_duration(path: Path):
    try:
        with wave.open(path.as_posix(), "rb") as wf:
            frames = wf.getnframes()
            rate   = wf.getframerate() or 0
            if frames > 0 and rate > 0:
                seconds = frames / float(rate)
                total_ms = int(round(seconds * 1000.0))
                total_sec = int(round(seconds))
                mm, ss = divmod(total_sec, 60)
                if ss == 60:
                    mm += 1
                    ss = 0
                return f"{mm:02d}:{ss:02d}", total_ms
    except (wave.Error, OSError):
        pass
    return None, None

def compute_duration(path: Path):
    ext = path.suffix.lower()
    if ext == ".wav":
        return wav_duration(path)
    return (None, None)


def build_items():
    items = []
    root_str = str(ROOT_DIR)
    if RECURSIVE:
        for root, _, files in os.walk(root_str):
            for fn in files:
                if not is_audio(fn): continue
                rel = os.path.relpath(os.path.join(root, fn), root_str)
                rel_u = rel_unix(rel)
                url   = BASE_URL + quote(rel_u, safe="")  # 编码整个相对路径
                title = os.path.splitext(os.path.basename(fn))[0]
                dur_txt, dur_ms = compute_duration(Path(root) / fn)
                item = {"title": title, "url": url}
                if dur_txt:
                    item["duration"] = dur_txt
                    item["duration_ms"] = dur_ms
                items.append(item)
    else:
        for fn in sorted(os.listdir(root_str)):
            full = os.path.join(root_str, fn)
            if not os.path.isfile(full):  continue
            if not is_audio(fn):          continue
            url   = BASE_URL + quote(rel_unix(fn), safe="")
            title = os.path.splitext(fn)[0]
            dur_txt, dur_ms = compute_duration(Path(full))
            item = {"title": title, "url": url}
            if dur_txt:
                item["duration"] = dur_txt
                item["duration_ms"] = dur_ms
            items.append(item)
    # 简单稳定排序：按文件名（原始 UTF-8）
    items.sort(key=lambda x: x["title"])
    return items

def main():
    items = build_items()
    out_path = ROOT_DIR / "index.json"
    out   = out_path.as_posix()
    with open(out, "w", encoding="utf-8") as f:
        json.dump(items, f, ensure_ascii=False, indent=2)
        f.write("\n")
    print(f"Wrote {out} with {len(items)} items.")

if __name__ == "__main__":
    sys.exit(main())
