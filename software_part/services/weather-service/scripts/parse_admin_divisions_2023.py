#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Parse the pasted 2023 PRC admin division list into CSV.
Input: UTF-8 text file where each valid line looks like
  110000\t北京市
  110101   东城区
Non-code lines (headers, notes, group titles like 省直辖县级行政单位、西沙区/南沙区 without codes) are ignored.
Output 1: data/admin_divisions_2023.csv with columns:
  code,name,level,province_code,province_name,prefecture_code,prefecture_name
Output 2: data/admin_divisions_2023_min.csv with columns:
  code,name
Usage:
  python3 scripts/parse_admin_divisions_2023.py [infile [outfile]]
Default infile: data/admin_2023_raw.txt
Default outfile: data/admin_divisions_2023.csv (and _min.csv)
"""
import csv
import os
import re
import sys
from typing import Dict, List, Tuple

CODE_LINE_RE = re.compile(r"^\s*(\d{6})\s+(.+?)\s*$")

SPECIAL_PREF_PREFIXES = {"4190", "4290", "4690", "6590"}  # province/autonomous-region administered county-level units
DIRECT_MUNICIPALITY_PROV_PREFIXES = {"11", "12", "31", "50"}


def parse_code_name_lines(lines: List[str]) -> List[Tuple[str, str]]:
    items: List[Tuple[str, str]] = []
    for raw in lines:
        line = raw.rstrip("\n").rstrip()
        if not line:
            continue
        m = CODE_LINE_RE.match(line)
        if not m:
            # ignore headers/notes/group titles like "省直辖县级行政单位" or lines without a 6-digit code
            continue
        code, name = m.group(1), m.group(2)
        # strip trailing footnote stars and extra whitespace
        name = name.rstrip().rstrip("*").rstrip()
        items.append((code, name))
    return items


def compute_level(code: str) -> str:
    if code.endswith("0000"):
        return "province"
    if code.endswith("00"):
        return "prefecture"
    return "county"


def main(infile: str, outfile: str) -> None:
    # read all lines
    with open(infile, "r", encoding="utf-8") as f:
        lines = f.readlines()

    code_name_list = parse_code_name_lines(lines)
    if not code_name_list:
        raise SystemExit(f"No valid code-name lines parsed from {infile}.")

    code2name: Dict[str, str] = {c: n for c, n in code_name_list}

    # prepare output paths
    outdir = os.path.dirname(outfile) or "."
    os.makedirs(outdir, exist_ok=True)
    min_outfile = os.path.join(outdir, os.path.splitext(os.path.basename(outfile))[0] + "_min.csv")

    # write minimal CSV
    with open(min_outfile, "w", encoding="utf-8", newline="") as fmin:
        wmin = csv.writer(fmin)
        wmin.writerow(["code", "name"])
        for code, name in code_name_list:
            wmin.writerow([code, name])

    # write normalized CSV with hierarchy columns
    with open(outfile, "w", encoding="utf-8", newline="") as fout:
        w = csv.writer(fout)
        w.writerow([
            "code",
            "name",
            "level",
            "province_code",
            "province_name",
            "prefecture_code",
            "prefecture_name",
        ])
        for code, name in code_name_list:
            level = compute_level(code)
            prov_code = code[:2] + "0000"
            prov_name = code2name.get(prov_code, "")

            pref_code = ""
            pref_name = ""
            if level == "province":
                pass
            else:
                # default prefecture is the xxYY00 bucket
                pref_code_candidate = code[:4] + "00"
                if code[:2] in DIRECT_MUNICIPALITY_PROV_PREFIXES:
                    # Beijing/Tianjin/Shanghai/Chongqing: use province as prefecture
                    pref_code, pref_name = prov_code, prov_name
                elif pref_code_candidate in code2name:
                    pref_code, pref_name = pref_code_candidate, code2name[pref_code_candidate]
                elif code[:4] in SPECIAL_PREF_PREFIXES:
                    # Province or autonomous region administered county-level unit bucket
                    pref_code = pref_code_candidate
                    pref_name = "直辖县级行政单位"
                else:
                    # leave empty if we cannot resolve prefecture name
                    pref_code = pref_code_candidate
                    pref_name = code2name.get(pref_code_candidate, "")

            w.writerow([
                code,
                name,
                level,
                prov_code,
                prov_name,
                pref_code,
                pref_name,
            ])

    print(
        f"Parsed {len(code_name_list)} rows.\n"
        f"Wrote: {outfile}\n"
        f"       {min_outfile}"
    )


if __name__ == "__main__":
    infile = sys.argv[1] if len(sys.argv) > 1 else os.path.join("data", "admin_2023_raw.txt")
    outfile = sys.argv[2] if len(sys.argv) > 2 else os.path.join("data", "admin_divisions_2023.csv")
    main(infile, outfile)
