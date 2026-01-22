#!/usr/bin/env python3
"""
Generate a LVGL `lv_pinyin_dict_t[]` dictionary using pinyin-data's kHanyuPinlu.txt.

Goals for this project:
- Simplified Chinese only (OpenCC t2s)
- More candidates than LVGL's tiny built-in dict
- Keep each candidate as a single UTF-8 3-byte CJK char (LVGL IME splits by 3 bytes)

Usage:
  tools/venv/bin/python tools/pinyin/gen_lvgl_pinyin_dict.py \
    --input tools/pinyin/kHanyuPinlu.txt \
    --out-c components/01_application/my_ui/custom/pinyin_dict_simplified.c \
    --out-h components/01_application/my_ui/custom/pinyin_dict_simplified.h
"""

from __future__ import annotations

import argparse
import re
from collections import OrderedDict

from opencc import OpenCC


_TONE_MAP = str.maketrans(
    {
        # a
        "ā": "a",
        "á": "a",
        "ǎ": "a",
        "à": "a",
        # e
        "ē": "e",
        "é": "e",
        "ě": "e",
        "è": "e",
        # i
        "ī": "i",
        "í": "i",
        "ǐ": "i",
        "ì": "i",
        # o
        "ō": "o",
        "ó": "o",
        "ǒ": "o",
        "ò": "o",
        # u
        "ū": "u",
        "ú": "u",
        "ǔ": "u",
        "ù": "u",
        # ü -> v
        "ǖ": "v",
        "ǘ": "v",
        "ǚ": "v",
        "ǜ": "v",
        "ü": "v",
        # n
        "ń": "n",
        "ň": "n",
        "ǹ": "n",
        # m
        "ḿ": "m",
        # ê
        "ê": "e",
    }
)


def _normalize_pinyin(py: str) -> str:
    py = py.strip().lower()
    # Some data sources might use "u:" for ü.
    py = py.replace("u:", "v")
    py = py.translate(_TONE_MAP)
    py = re.sub(r"[^a-zv]", "", py)
    return py


def _is_single_cjk_3byte(ch: str) -> bool:
    if len(ch) != 1:
        return False
    b = ch.encode("utf-8")
    return len(b) == 3


def _chunks(s: str, chunk_chars: int = 48) -> list[str]:
    return [s[i : i + chunk_chars] for i in range(0, len(s), chunk_chars)]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--input", required=True)
    ap.add_argument("--out-c", required=True)
    ap.add_argument("--out-h", required=True)
    ap.add_argument("--max-per-pinyin", type=int, default=256)
    args = ap.parse_args()

    cc = OpenCC("t2s")

    # Keep insertion order for deterministic output.
    py_to_chars: dict[str, list[str]] = {}

    # Example line:
    #   U+4E0A: shàng,shang  # 上
    line_re = re.compile(r"^U\+([0-9A-Fa-f]{4,6}):\s*([^#]+)#\s*(.+?)\s*$")

    with open(args.input, "r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            m = line_re.match(line)
            if not m:
                continue

            pys_raw = m.group(2).strip()
            ch_raw = m.group(3).strip()
            if not ch_raw:
                continue

            # Take first visible char after '#'.
            ch0 = ch_raw[0]
            ch = cc.convert(ch0)
            if not _is_single_cjk_3byte(ch):
                continue

            pys = []
            for p in pys_raw.split(","):
                p = _normalize_pinyin(p)
                if not p:
                    continue
                pys.append(p)

            for py in pys:
                lst = py_to_chars.setdefault(py, [])
                if ch not in lst:
                    lst.append(ch)

    # Aliases: allow both "nv/lv" style and "nu/lu" style (some users type ü as u).
    # NOTE: This will intentionally mix some ambiguous cases (e.g. nu can mean 奴 or 女).
    for py, lst in list(py_to_chars.items()):
        if "v" not in py:
            continue
        if not (py.startswith("n") or py.startswith("l")):
            continue
        alias = py.replace("v", "u")
        ali = py_to_chars.setdefault(alias, [])
        for ch in lst:
            if ch not in ali:
                ali.append(ch)

    # Promote some super-common characters so they're in the first page.
    promote: dict[str, list[str]] = {
        "ni": ["你"],
        "hao": ["好"],
        "wo": ["我"],
        "shi": ["是"],
        "de": ["的"],
        "le": ["了"],
        "zai": ["在"],
        "bu": ["不"],
        "zhong": ["中"],
        "guo": ["国"],
    }
    for py, tops in promote.items():
        if py not in py_to_chars:
            continue
        lst = py_to_chars[py]
        # Insert in given order at the front.
        for ch in reversed(tops):
            if ch in lst:
                lst.remove(ch)
            lst.insert(0, ch)

    # Hard cap per pinyin to control flash size.
    for py, lst in py_to_chars.items():
        if len(lst) > args.max_per_pinyin:
            py_to_chars[py] = lst[: args.max_per_pinyin]

    # Sort keys for LVGL's init_pinyin_dict() (expects grouped by leading letter).
    items = sorted(py_to_chars.items(), key=lambda kv: kv[0])

    # Emit header.
    with open(args.out_h, "w", encoding="utf-8", newline="\n") as f:
        f.write("#pragma once\n\n")
        f.write('#include "lv_ime_pinyin.h"\n\n')
        f.write("/* Simplified-only pinyin dictionary for LVGL IME. */\n")
        f.write("extern lv_pinyin_dict_t app_pinyin_dict_simplified[];\n")

    # Emit C.
    with open(args.out_c, "w", encoding="utf-8", newline="\n") as f:
        f.write("/* Auto-generated. Do not edit manually. */\n")
        f.write('#include "pinyin_dict_simplified.h"\n\n')
        f.write("lv_pinyin_dict_t app_pinyin_dict_simplified[] = {\n")
        for py, chars in items:
            s = "".join(chars)
            chunks = _chunks(s, 48)
            if len(chunks) == 1:
                f.write(f'    {{"{py}", "{chunks[0]}" }},\n')
            else:
                f.write(f'    {{"{py}",\n')
                for c in chunks:
                    f.write(f'      "{c}"\n')
                f.write("    },\n")
        f.write("    {NULL, NULL}\n")
        f.write("};\n")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

