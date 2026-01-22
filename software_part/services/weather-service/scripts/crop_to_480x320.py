#!/usr/bin/env python3
"""
Crop a region from an input image and place it on a 480x320 canvas.

Usage examples:
  python3 scripts/crop_to_480x320.py input.png --crop 40,95,920,295 --out data/out.png
  python3 scripts/crop_to_480x320.py input.png --crop 40,95,880,200 --xywh --fit contain --bg "#000000"

Arguments:
  positional: input image path
  --crop: crop rectangle. Default format is x1,y1,x2,y2 unless --xywh is given
  --xywh: interpret --crop as x,y,width,height
  --out: output path (default: output_480x320.png next to input)
  --fit: how to place the cropped region on 480x320: 'contain' (default) or 'cover'
  --bg: background color for letterboxing in 'contain' mode (default: #000000)
  --quality: JPEG quality if saving .jpg/.jpeg (default: 90)
"""

from __future__ import annotations

import argparse
import os
from typing import Tuple

from PIL import Image, ImageColor

TARGET_W, TARGET_H = 480, 320


def parse_crop_box(s: str, xywh: bool) -> Tuple[int, int, int, int]:
    parts = [p.strip() for p in s.split(",")]
    if len(parts) != 4:
        raise argparse.ArgumentTypeError("--crop expects 4 comma-separated integers")
    try:
        nums = list(map(int, parts))
    except ValueError:
        raise argparse.ArgumentTypeError("--crop values must be integers")
    if xywh:
        x, y, w, h = nums
        if w <= 0 or h <= 0:
            raise argparse.ArgumentTypeError("width and height must be positive")
        return (x, y, x + w, y + h)
    else:
        x1, y1, x2, y2 = nums
        if x2 <= x1 or y2 <= y1:
            raise argparse.ArgumentTypeError("x2>x1 and y2>y1 required")
        return (x1, y1, x2, y2)


def clamp_box(box: Tuple[int, int, int, int], w: int, h: int) -> Tuple[int, int, int, int]:
    x1, y1, x2, y2 = box
    x1 = max(0, min(x1, w - 1))
    y1 = max(0, min(y1, h - 1))
    x2 = max(1, min(x2, w))
    y2 = max(1, min(y2, h))
    if x2 <= x1:
        x2 = min(w, x1 + 1)
    if y2 <= y1:
        y2 = min(h, y1 + 1)
    return (x1, y1, x2, y2)


def place_on_canvas(img: Image.Image, fit: str, bg_color: str) -> Image.Image:
    fit = fit.lower()
    if fit not in {"contain", "cover"}:
        raise ValueError("--fit must be 'contain' or 'cover'")

    if fit == "cover":
        # Scale so that the image covers the entire canvas; then center-crop
        scale = max(TARGET_W / img.width, TARGET_H / img.height)
        new_w = max(1, int(round(img.width * scale)))
        new_h = max(1, int(round(img.height * scale)))
        resized = img.resize((new_w, new_h), Image.LANCZOS)
        left = (new_w - TARGET_W) // 2
        top = (new_h - TARGET_H) // 2
        return resized.crop((left, top, left + TARGET_W, top + TARGET_H))
    else:
        # contain: fit inside canvas, preserve aspect, pad with bg
        scale = min(TARGET_W / img.width, TARGET_H / img.height)
        new_w = max(1, int(round(img.width * scale)))
        new_h = max(1, int(round(img.height * scale)))
        resized = img.resize((new_w, new_h), Image.LANCZOS)
        bg = Image.new("RGB", (TARGET_W, TARGET_H), ImageColor.getrgb(bg_color))
        ox = (TARGET_W - new_w) // 2
        oy = (TARGET_H - new_h) // 2
        bg.paste(resized, (ox, oy))
        return bg


def main():
    ap = argparse.ArgumentParser(description="Crop and place image onto 480x320 canvas")
    ap.add_argument("input", help="input image path")
    ap.add_argument("--crop", required=True, help="crop rectangle: x1,y1,x2,y2 or x,y,w,h with --xywh")
    ap.add_argument("--xywh", action="store_true", help="interpret --crop as x,y,width,height")
    ap.add_argument("--out", default=None, help="output path, default next to input")
    ap.add_argument("--fit", default="contain", choices=["contain", "cover"], help="contain or cover the 480x320 canvas")
    ap.add_argument("--bg", default="#000000", help="background color for padding in contain mode")
    ap.add_argument("--quality", type=int, default=90, help="JPEG quality when saving .jpg/.jpeg")
    args = ap.parse_args()

    if not os.path.exists(args.input):
        ap.error(f"Input not found: {args.input}")

    with Image.open(args.input) as im:
        im = im.convert("RGB")
        box = parse_crop_box(args.crop, args.xywh)
        box = clamp_box(box, im.width, im.height)
        cropped = im.crop(box)
        out_img = place_on_canvas(cropped, args.fit, args.bg)

    out_path = args.out
    if not out_path:
        base, ext = os.path.splitext(args.input)
        out_path = f"{base}.480x320.png"

    # Ensure directory exists
    os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)

    save_kwargs = {}
    if out_path.lower().endswith((".jpg", ".jpeg")):
        save_kwargs["quality"] = args.quality
        save_kwargs["optimize"] = True
    else:
        save_kwargs["optimize"] = True

    out_img.save(out_path, **save_kwargs)
    print(f"Saved: {out_path} ({TARGET_W}x{TARGET_H})")


if __name__ == "__main__":
    main()

