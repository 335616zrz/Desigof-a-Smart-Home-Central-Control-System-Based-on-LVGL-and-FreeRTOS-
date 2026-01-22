# -*- coding: utf-8 -*-
# 从设备串口日志中解析以 "PING_CSV," / "PING_SUM," 开头的行，
# 生成 rtt_line.png / rtt_box.png / rtt_time.png 到 tools/result/wifi/（或自定义输出目录）
import argparse
import os
import shutil
import sys
from importlib import import_module
from typing import Any, Dict, List, Optional, Tuple, Union

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


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Parse ping logs and plot RTT charts.",
        epilog="日志需包含以 'PING_CSV,' 和可选 'PING_SUM,' 开头的行。",
    )
    parser.add_argument(
        "logfile",
        help="包含 PING_CSV 行的日志文件路径",
    )
    parser.add_argument(
        "outdir",
        nargs="?",
        default=os.path.join("tools", "result", "wifi"),
        help="输出目录（默认 tools/result/wifi）",
    )
    return parser.parse_args()


def parse_ping_log(path: str) -> Tuple[List[Dict[str, Any]], Optional[Dict[str, Union[int, float]]]]:
    rows: List[Dict[str, Any]] = []
    summary: Optional[Dict[str, Union[int, float]]] = None

    with open(path, "r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            s = line.strip()
            if s.startswith("PING_CSV,"):
                # PING_CSV,ts_ms,seq,status,rtt_ms,bytes,ttl,ip
                parts = s.split(",")
                if len(parts) >= 8:
                    try:
                        ts = int(parts[1])
                        seq = int(parts[2])
                        stat = parts[3]
                        rtt = None if (parts[4] == "" or stat != "ok") else float(parts[4])
                    except ValueError:
                        continue
                    rows.append({"ts_ms": ts, "seq": seq, "status": stat, "rtt_ms": rtt})
            elif s.startswith("PING_SUM,") and "tx,rx" not in s:
                # PING_SUM,<tx>,<rx>,<loss_pct>,<min_ms>,<avg_ms>,<max_ms>,<std_ms>,<duration_ms>
                parts = s.split(",")
                if len(parts) >= 9:
                    try:
                        summary = {
                            "tx": int(parts[1]),
                            "rx": int(parts[2]),
                            "loss_pct": float(parts[3]),
                            "min_ms": float(parts[4]),
                            "avg_ms": float(parts[5]),
                            "max_ms": float(parts[6]),
                            "std_ms": float(parts[7]),
                            "duration_ms": int(parts[8]),
                        }
                    except ValueError:
                        summary = None
    return rows, summary


def filter_success_rows(rows: List[Dict[str, Any]]) -> List[Dict[str, Union[int, float]]]:
    ok_rows: List[Dict[str, Union[int, float]]] = []
    for row in rows:
        if row["status"] == "ok" and row["rtt_ms"] is not None:
            ok_rows.append(
                {
                    "ts_ms": float(row["ts_ms"]),
                    "seq": int(row["seq"]),
                    "rtt_ms": float(row["rtt_ms"]),
                }
            )
    return ok_rows


def make_plots(rows_ok: List[Dict[str, Union[int, float]]], outdir: str) -> None:
    plt = get_matplotlib_pyplot()
    seqs = [int(row["seq"]) for row in rows_ok]
    rtts = [float(row["rtt_ms"]) for row in rows_ok]
    ts = [float(row["ts_ms"]) for row in rows_ok]

    # 折线：rtt vs seq
    plt.figure()
    plt.plot(seqs, rtts, marker=".", linewidth=1)
    plt.xlabel("ICMP seq")
    plt.ylabel("RTT (ms)")
    plt.title("Ping RTT vs seq")
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(os.path.join(outdir, "rtt_line.png"), dpi=200)

    # 箱线：总体分布
    plt.figure()
    plt.boxplot(rtts, showfliers=True)
    plt.ylabel("RTT (ms)")
    plt.title("Ping RTT distribution")
    plt.tight_layout()
    plt.savefig(os.path.join(outdir, "rtt_box.png"), dpi=200)

    # 按时间轴
    plt.figure()
    t0 = min(ts)
    elapsed = [t - t0 for t in ts]
    plt.plot(elapsed, rtts, marker=".", linewidth=1)
    plt.xlabel("Elapsed (ms)")
    plt.ylabel("RTT (ms)")
    plt.title("Ping RTT vs time")
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(os.path.join(outdir, "rtt_time.png"), dpi=200)


def copy_log(logfile: str, outdir: str) -> None:
    dst_log = os.path.join(outdir, "ping_log.txt")
    try:
        if os.path.abspath(logfile) != os.path.abspath(dst_log):
            shutil.copy2(logfile, dst_log)
    except Exception as exc:  # pragma: no cover - 仅提示
        print("Warn: failed to copy log ->", exc)


def main() -> int:
    args = parse_args()

    logfile = args.logfile
    outdir = args.outdir
    os.makedirs(outdir, exist_ok=True)

    rows, summary = parse_ping_log(logfile)
    ok_rows = filter_success_rows(rows)

    total = len(rows)
    ok_count = len(ok_rows)
    print("Samples:", total, "OK:", ok_count, "Loss:", total - ok_count)
    if summary:
        print("Summary:", summary)

    if not ok_rows:
        print("No valid RTT samples found. Make sure your log contains lines starting with 'PING_CSV,'.")
        return 0

    make_plots(ok_rows, outdir)
    copy_log(logfile, outdir)

    print("Saved to:", outdir)
    print(" - rtt_line.png")
    print(" - rtt_box.png")
    print(" - rtt_time.png")
    print(" - ping_log.txt")
    return 0


if __name__ == "__main__":
    sys.exit(main())
