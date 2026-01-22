#!/usr/bin/env bash
set -Eeuo pipefail
export LC_ALL=C

OUTDIR="./converted_44100_16bit"
mkdir -p "$OUTDIR"

need(){ command -v "$1" >/dev/null 2>&1 || { echo "缺少依赖：$1"; exit 127; }; }
need ffmpeg
need ffprobe

# 稳妥：分别取三个字段（csv p=0 不用自定义分隔符，兼容性最好）
probe(){
  local f="$1" SR CH FMT
  SR="$(ffprobe -v error -select_streams a:0 -show_entries stream=sample_rate -of csv=p=0 "$f" | head -n1 || true)"
  CH="$(ffprobe -v error -select_streams a:0 -show_entries stream=channels     -of csv=p=0 "$f" | head -n1 || true)"
  FMT="$(ffprobe -v error -select_streams a:0 -show_entries stream=sample_fmt  -of csv=p=0 "$f" | head -n1 || true)"
  printf '%s|%s|%s\n' "$SR" "$CH" "$FMT"
}

# 扫描常见后缀（0 分隔，安全处理空格/中文）
find . -maxdepth 1 -type f \( \
  -iname '*.wav'  -o -iname '*.flac' -o -iname '*.mp3'  -o -iname '*.ogg'  -o \
  -iname '*.opus' -o -iname '*.aac'  -o -iname '*.m4a' \
\) -print0 |
while IFS= read -r -d '' f; do
  # 已是目标文件名的直接跳过
  [[ "$f" == *_44100_16bit.wav ]] && { echo "跳过（已是目标文件）: $f"; continue; }

  info="$(probe "$f" 2>/dev/null || true)"
  IFS='|' read -r SR CH FMT <<<"${info:-|||}"

  if [[ -z "$SR" || -z "$CH" || -z "$FMT" ]]; then
    echo "跳过（无音频流/探测失败）: $f"
    continue
  fi

  # 已达标：44.1k / 2ch / 16bit（packed 或 planar 都算）
  if [[ "$SR" == "44100" && "$CH" == "2" && "$FMT" == s16* ]]; then
    echo "已符合: $f"
    continue
  fi

  base="$(basename "$f")"
  name="${base%.*}"
  out="$OUTDIR/${name}_44100_16bit.wav"

  echo "转换: $f  ->  $out   ($SR/$CH/$FMT -> 44100/2/s16)"
  ffmpeg -nostdin -hide_banner -v error -y -i "$f" -vn -map_metadata 0 \
    -af "aresample=resampler=soxr:precision=28:osf=s16:dither_method=shibata" \
    -ar 44100 -ac 2 -c:a pcm_s16le "$out" \
    && echo "OK: $out" || echo "失败: $f"
done

echo "================ 完成 ================"
echo "输出目录: $OUTDIR"
find "$OUTDIR" -type f -name '*_44100_16bit.wav' | wc -l | awk '{print "共生成 "$1" 个文件"}'
