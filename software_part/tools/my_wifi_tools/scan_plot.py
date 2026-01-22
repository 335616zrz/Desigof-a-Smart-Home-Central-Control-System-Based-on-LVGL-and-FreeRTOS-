# -*- coding: utf-8 -*-
# 解析包含 SCAN_CSV/SCAN_SUM 的扫描日志，输出到 tools/result/wifi：
# - channels_bar.png        各主信道数量柱状图
# - rssi_hist.png           RSSI 直方图
# - rssi_vs_channel_*.png   RSSI vs 主信道散点图（按 band 分面）
# - ssid_top.png            同 SSID 聚合 TopN
# - ssid_agg.csv            同 SSID 聚合明细
#
# 用法：
#   python3 tools/my_wifi_tools/scan_plot.py scan_log.txt [--band all|2.4|5|6] [--top 10]
#
import argparse
import csv
import os
import shutil
from collections import Counter, defaultdict
from importlib import import_module
from statistics import mean
from typing import Any, Dict, Iterable, List, Optional, Tuple

_plt = None


def get_matplotlib_pyplot():
    """延迟导入 matplotlib，避免在 --help 时触发二进制兼容检查。"""
    global _plt
    if _plt is None:
        try:
            matplotlib = import_module("matplotlib")
            matplotlib.use("Agg", force=True)
            _plt = import_module("matplotlib.pyplot")
        except Exception as exc:  # pragma: no cover - 仅在环境不兼容时触发
            raise SystemExit(
                "Failed to import matplotlib. 请安装与当前 NumPy 版本兼容的 matplotlib（建议 pip install --upgrade matplotlib）。\n"
                f"原始错误：{exc}"
            )
    return _plt


def infer_band(ch: int) -> str:
    """根据主信道粗略推断频段。"""
    if 1 <= ch <= 14:
        return "2.4G"
    if 32 <= ch <= 177:
        return "5G"
    # 兜底：当无法判定时归为 6G（日志里一般还是 2.4/5）
    return "6G"


def ensure_outdir() -> str:
    outdir = os.path.join("tools", "result", "wifi")
    os.makedirs(outdir, exist_ok=True)
    return outdir


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Plot Wi-Fi scan log",
        epilog="日志需包含以 'SCAN_CSV,' 开头的扫描条目，可选 'SCAN_SUM,' 汇总行。",
    )
    parser.add_argument("logfile", help="scan_log.txt（包含 SCAN_CSV/SCAN_SUM 行）")
    parser.add_argument(
        "--band",
        choices=["all", "2.4", "5", "6"],
        default="all",
        help="按频段过滤/分面绘图；默认 all",
    )
    parser.add_argument(
        "--top", type=int, default=10, help="SSID 聚合 TopN，默认 10"
    )
    return parser.parse_args()


def read_scan_log(path: str) -> Tuple[List[Dict[str, Any]], Optional[str]]:
    rows: List[Dict[str, Any]] = []
    summary: Optional[str] = None

    with open(path, "r", encoding="utf-8", errors="ignore") as fh:
        for line in fh:
            s = line.strip()
            if s.startswith("SCAN_CSV,"):
                # SCAN_CSV,idx,ssid,bssid,rssi,primary,second,auth,pair,group,phy,wps,ftm
                parts = s.split(",")
                if len(parts) >= 13:
                    try:
                        idx = int(parts[1])
                        ssid = parts[2]
                        bssid = parts[3]
                        rssi = int(parts[4])
                        primary = int(parts[5])
                        second = parts[6]
                        auth = parts[7]
                        pair = parts[8]
                        group = parts[9]
                        phy = parts[10]
                        wps = int(parts[11])
                        ftm = int(parts[12])
                    except ValueError:
                        continue
                    rows.append(
                        {
                            "idx": idx,
                            "ssid": ssid,
                            "bssid": bssid,
                            "rssi": rssi,
                            "primary": primary,
                            "second": second,
                            "auth": auth,
                            "pair": pair,
                            "group": group,
                            "phy": phy,
                            "wps": wps,
                            "ftm": ftm,
                            "band": infer_band(primary),
                        }
                    )
            elif s.startswith("SCAN_SUM,") and "aps=" in s:
                summary = s
    return rows, summary


def save_basic_plots(rows: List[Dict[str, Any]], outdir: str) -> List[str]:
    if not rows:
        print("No SCAN_CSV rows found.")
        return []

    plt = get_matplotlib_pyplot()
    saved: List[str] = []

    # 信道柱状图
    counter = Counter(row["primary"] for row in rows)
    ordered = sorted(counter.items())
    plt.figure()
    if ordered:
        xs, ys = zip(*ordered)
    else:
        xs, ys = [], []
    plt.bar(xs, ys, width=0.8)
    plt.xlabel("Primary channel")
    plt.ylabel("#APs")
    plt.title("Channel distribution")
    plt.grid(axis="y", linestyle="--", alpha=0.4)
    plt.tight_layout()
    pth = os.path.join(outdir, "channels_bar.png")
    plt.savefig(pth, dpi=200)
    saved.append(pth)

    # RSSI 直方图
    plt.figure()
    bins = list(range(-100, 1, 5))  # 5 dB 步进
    rssis = [row["rssi"] for row in rows]
    plt.hist(rssis, bins=bins, edgecolor="black")
    plt.xlabel("RSSI (dBm)")
    plt.ylabel("Count")
    plt.title("RSSI histogram")
    plt.grid(axis="y", linestyle="--", alpha=0.4)
    plt.tight_layout()
    pth = os.path.join(outdir, "rssi_hist.png")
    plt.savefig(pth, dpi=200)
    saved.append(pth)

    return saved


def scatter_by_band(
    rows_band: Iterable[Dict[str, Any]], band_tag: str, outdir: str
) -> Optional[str]:
    rows_band = list(rows_band)
    if not rows_band:
        return None

    plt = get_matplotlib_pyplot()
    plt.figure()
    plt.scatter(
        [row["primary"] for row in rows_band],
        [row["rssi"] for row in rows_band],
        s=20,
    )
    plt.xlabel("Primary channel")
    plt.ylabel("RSSI (dBm)")
    plt.title(f"RSSI vs channel ({band_tag})")
    plt.grid(True, linestyle="--", alpha=0.4)
    plt.tight_layout()
    fname = f"rssi_vs_channel_{band_tag}.png"
    pth = os.path.join(outdir, fname)
    plt.savefig(pth, dpi=200)
    return pth


def plot_ssid_agg(
    rows: List[Dict[str, Any]], outdir: str, top_n: int
) -> Tuple[Optional[str], Optional[str]]:
    if not rows:
        return None, None

    plt = get_matplotlib_pyplot()
    agg: Dict[str, Dict[str, Any]] = defaultdict(
        lambda: {"bssids": set(), "rssis": [], "channels": set()}
    )
    for row in rows:
        entry = agg[row["ssid"]]
        entry["bssids"].add(row["bssid"])
        entry["rssis"].append(row["rssi"])
        entry["channels"].add(row["primary"])

    summary_rows: List[Dict[str, Any]] = []
    for ssid, data in agg.items():
        rssis: List[int] = data["rssis"]
        channels_sorted = ",".join(str(ch) for ch in sorted(data["channels"]))
        max_rssi = max(rssis) if rssis else None
        avg_rssi = mean(rssis) if rssis else None
        summary_rows.append(
            {
                "ssid": ssid,
                "label": ssid if ssid else "<hidden>",
                "ap_count": len(data["bssids"]),
                "max_rssi": max_rssi,
                "avg_rssi": avg_rssi,
                "channels": channels_sorted,
            }
        )

    if not summary_rows:
        return None, None

    summary_rows.sort(
        key=lambda item: (
            item["ap_count"],
            item["max_rssi"] if item["max_rssi"] is not None else -999,
        ),
        reverse=True,
    )

    csv_path = os.path.join(outdir, "ssid_agg.csv")
    with open(csv_path, "w", encoding="utf-8", newline="") as fh:
        writer = csv.writer(fh)
        writer.writerow(["ssid", "ap_count", "max_rssi", "avg_rssi", "channels"])
        for item in summary_rows:
            max_rssi = (
                f"{item['max_rssi']:.1f}" if item["max_rssi"] is not None else ""
            )
            avg_rssi = (
                f"{item['avg_rssi']:.1f}" if item["avg_rssi"] is not None else ""
            )
            writer.writerow(
                [item["ssid"], item["ap_count"], max_rssi, avg_rssi, item["channels"]]
            )

    top = summary_rows[:top_n]
    png_path: Optional[str] = None
    if top:
        plt.figure()
        plt.bar([item["label"] for item in top], [item["ap_count"] for item in top])
        plt.xlabel(f"SSID (Top {top_n})")
        plt.ylabel("#BSSIDs")
        plt.title("Top SSIDs by BSSID count")
        plt.grid(axis="y", linestyle="--", alpha=0.4)
        plt.xticks(rotation=45, ha="right")
        plt.tight_layout()
        png_path = os.path.join(outdir, "ssid_top.png")
        plt.savefig(png_path, dpi=200)

    return csv_path, png_path


def copy_log(logfile: str, outdir: str) -> None:
    try:
        shutil.copy2(logfile, os.path.join(outdir, "scan_log.txt"))
    except Exception:
        pass


def main() -> int:
    args = parse_args()
    outdir = ensure_outdir()
    copy_log(args.logfile, outdir)

    rows, summary = read_scan_log(args.logfile)
    print("Rows:", len(rows))
    if summary:
        print(summary)

    if not rows:
        print("No valid SCAN_CSV samples found.")
        return 0

    saved: List[str] = []
    saved += save_basic_plots(rows, outdir)

    present_bands = sorted({row["band"] for row in rows})
    band_map = {"2.4": "2.4G", "5": "5G", "6": "6G"}
    tag_map = {"2.4G": "24g", "5G": "5g", "6G": "6g"}

    if args.band == "all":
        bands_to_plot = present_bands
    else:
        bands_to_plot = [band_map[args.band]]

    for band_name in bands_to_plot:
        band_rows = [row for row in rows if row["band"] == band_name]
        tag = tag_map.get(band_name, band_name.lower())
        plot_path = scatter_by_band(band_rows, tag, outdir)
        if plot_path:
            saved.append(plot_path)

    csv_pth, png_pth = plot_ssid_agg(rows, outdir, top_n=args.top)
    if csv_pth:
        saved.append(csv_pth)
    if png_pth:
        saved.append(png_pth)

    print("Saved:")
    for path in saved:
        print(" -", path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
