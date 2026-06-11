"""
parse_logs.py
─────────────────────────────────────────────────────────────────────────────
Parses all raw simulation log files from simulation_results/raw_logs/ and
produces a single structured CSV at simulation_results/csv/results.csv.

Expected RESULT line format (one per log file, at the end):
  RESULT: Pdrop=0.01 Tout=150 Throughput=6679.52 AvgTx=1.02 DroppedAfter4=0 Duration_ms=298077

Usage:
  python parse_logs.py
  python parse_logs.py --logs_dir path/to/raw_logs --out path/to/results.csv
─────────────────────────────────────────────────────────────────────────────
"""

import os
import re
import csv
import argparse
import sys

# ── Paths (relative to this script's location) ───────────────────────────────
SCRIPT_DIR   = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
DEFAULT_LOGS = os.path.join(PROJECT_ROOT, "simulation_results", "raw_logs")
DEFAULT_OUT  = os.path.join(PROJECT_ROOT, "simulation_results", "csv", "results.csv")

# ── Regex: matches the RESULT line ───────────────────────────────────────────
RESULT_PATTERN = re.compile(
    r"RESULT:\s+"
    r"Pdrop=(?P<pdrop>\S+)\s+"
    r"Tout=(?P<tout>\S+)\s+"
    r"Throughput=(?P<throughput>\S+)\s+"
    r"AvgTx=(?P<avgtx>\S+)\s+"
    r"DroppedAfter4=(?P<dropped4>\S+)\s+"
    r"Duration_ms=(?P<duration_ms>\S+)"
)

CSV_FIELDS = [
    "pdrop",          # packet drop probability  (float)
    "tout_ms",        # timeout period           (int, ms)
    "throughput",     # bytes/sec                (float)
    "avg_tx",         # average transmissions per packet (float)
    "dropped_after4", # packets abandoned after 4 failed attempts (int)
    "duration_ms",    # total simulation wall time (int, ms)
    "source_file",    # which log file this came from
]


def parse_file(filepath: str) -> dict | None:
    """
    Reads one log file and returns a dict of extracted fields,
    or None if no RESULT line is found.
    """
    with open(filepath, "r", encoding="utf-8") as f:
        for line in f:
            m = RESULT_PATTERN.search(line)
            if m:
                return {
                    "pdrop":          float(m.group("pdrop")),
                    "tout_ms":        int(m.group("tout")),
                    "throughput":     float(m.group("throughput")),
                    "avg_tx":         float(m.group("avgtx")),
                    "dropped_after4": int(m.group("dropped4")),
                    "duration_ms":    int(m.group("duration_ms")),
                    "source_file":    os.path.basename(filepath),
                }
    return None  # no RESULT line found in this file


def parse_all(logs_dir: str, out_csv: str) -> int:
    """
    Iterates over all .txt files in logs_dir, parses each one,
    and writes the aggregated results to out_csv.
    Returns the number of successfully parsed files.
    """
    if not os.path.isdir(logs_dir):
        print(f"[ERROR] Log directory not found: {logs_dir}")
        sys.exit(1)

    log_files = sorted(
        f for f in os.listdir(logs_dir) if f.endswith(".txt")
    )

    if not log_files:
        print(f"[ERROR] No .txt files found in {logs_dir}")
        sys.exit(1)

    rows = []
    failed = []

    for fname in log_files:
        fpath = os.path.join(logs_dir, fname)
        result = parse_file(fpath)
        if result:
            rows.append(result)
        else:
            failed.append(fname)
            print(f"  [WARN] No RESULT line found in: {fname}")

    # Sort by (pdrop ASC, tout_ms ASC) for clean table ordering
    rows.sort(key=lambda r: (r["pdrop"], r["tout_ms"]))

    # Write CSV
    os.makedirs(os.path.dirname(out_csv), exist_ok=True)
    with open(out_csv, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=CSV_FIELDS)
        writer.writeheader()
        writer.writerows(rows)

    return rows, failed


def print_table(rows: list[dict]) -> None:
    """Pretty-prints the parsed data to stdout for quick verification."""
    header = f"{'Pdrop':>8} {'Tout(ms)':>9} {'Throughput(B/s)':>16} {'AvgTx':>7} {'Drop4':>6} {'Duration(s)':>12}"
    print("\n" + "─" * len(header))
    print(header)
    print("─" * len(header))
    for r in rows:
        print(
            f"{r['pdrop']:>8.2f} "
            f"{r['tout_ms']:>9d} "
            f"{r['throughput']:>16.2f} "
            f"{r['avg_tx']:>7.3f} "
            f"{r['dropped_after4']:>6d} "
            f"{r['duration_ms']/1000:>12.2f}"
        )
    print("─" * len(header))


def main():
    parser = argparse.ArgumentParser(description="Parse RTOS simulation logs → CSV")
    parser.add_argument("--logs_dir", default=DEFAULT_LOGS,
                        help=f"Directory containing raw .txt log files (default: {DEFAULT_LOGS})")
    parser.add_argument("--out", default=DEFAULT_OUT,
                        help=f"Output CSV path (default: {DEFAULT_OUT})")
    args = parser.parse_args()

    print(f"[parse_logs] Scanning: {args.logs_dir}")
    rows, failed = parse_all(args.logs_dir, args.out)

    print_table(rows)
    print(f"\n[parse_logs] Parsed {len(rows)} / {len(rows) + len(failed)} files")
    if failed:
        print(f"[parse_logs] Failed files: {failed}")
    print(f"[parse_logs] CSV saved → {args.out}")


if __name__ == "__main__":
    main()