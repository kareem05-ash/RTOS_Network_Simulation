"""
plot_retransmissions.py
─────────────────────────────────────────────────────────────────────────────
Answers the two analytical questions from Dr. Khaled's project spec (sec. vi):

  Q1: "What is the average number of transmissions of a packet as a function
       of P_drop?"

  Q2: "How many packets were dropped due to being transmitted more than 4 times?"

Generates:
  Figure 3 — Avg Transmissions per Packet vs P_drop
              One curve per Tout value + a theoretical curve for comparison

  Figure 4 — Packets Dropped After 4 Attempts vs P_drop
              Bar chart grouped by Tout value

Also prints a summary table to stdout for quick reference / copy-paste into
the report.

Outputs (PNG):
  simulation_results/plots/fig3_avgtx_vs_pdrop.png
  simulation_results/plots/fig4_dropped4_vs_pdrop.png

Usage:
  python plot_retransmissions.py
  python plot_retransmissions.py --csv path/to/results.csv --out path/to/plots/
─────────────────────────────────────────────────────────────────────────────
"""

import os
import argparse
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

# ── Paths ─────────────────────────────────────────────────────────────────────
SCRIPT_DIR   = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
DEFAULT_CSV  = os.path.join(PROJECT_ROOT, "simulation_results", "csv", "results.csv")
DEFAULT_OUT  = os.path.join(PROJECT_ROOT, "simulation_results", "plots")

# ── Style ──────────────────────────────────────────────────────────────────────
COLORS    = ["#1f77b4", "#d62728", "#2ca02c", "#ff7f0e"]
MARKERS   = ["o", "s", "^", "D"]
LINE_W    = 2.0
MARKER_S  = 8
GRID_ALPHA= 0.35
FIG_SIZE  = (8, 5.5)
DPI       = 150


def load_data(csv_path: str) -> pd.DataFrame:
    if not os.path.isfile(csv_path):
        raise FileNotFoundError(
            f"CSV not found: {csv_path}\n"
            "Run parse_logs.py first."
        )
    df = pd.read_csv(csv_path)
    required = {"pdrop", "tout_ms", "avg_tx", "dropped_after4"}
    missing = required - set(df.columns)
    if missing:
        raise ValueError(f"CSV is missing columns: {missing}")
    return df


def theoretical_avg_tx(p: float, max_attempts: int = 4) -> float:
    """
    Theoretical expected number of transmissions per packet under S&W
    with a maximum of `max_attempts` retries.

    A packet is retransmitted if EITHER the data packet OR its ACK is dropped.
    With P_drop for data and P_ack for ACK (fixed at 0.01 per spec):
      P_fail = 1 - (1 - P_drop) * (1 - P_ack)

    For a truncated geometric distribution capped at max_attempts:
      E[tx] = sum_{k=1}^{max} k * P_fail^(k-1) * (1 - P_fail)
              + max * P_fail^(max-1)          ← capped term

    Here we use a simplified model assuming P_ack ≈ 0 for clarity,
    since P_ack = 0.01 is small. The actual E[tx] with P_ack included is
    computed if P_ack is passed.
    """
    P_ACK = 0.01
    p_fail = 1 - (1 - p) * (1 - P_ACK)

    expected = 0.0
    for k in range(1, max_attempts):
        # exactly k transmissions: first k-1 fail, k-th succeeds
        expected += k * (p_fail ** (k - 1)) * (1 - p_fail)
    # max_attempts: either succeeds or abandoned
    expected += max_attempts * (p_fail ** (max_attempts - 1))
    return expected


def style_axes(ax, xlabel, ylabel, title):
    ax.set_xlabel(xlabel, fontsize=12)
    ax.set_ylabel(ylabel, fontsize=12)
    ax.set_title(title, fontsize=13, fontweight="bold", pad=12)
    ax.grid(True, linestyle="--", alpha=GRID_ALPHA)
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    ax.legend(fontsize=10, framealpha=0.85, edgecolor="#cccccc")


# ── Figure 3: Avg Transmissions vs P_drop ─────────────────────────────────────
def plot_avgtx_vs_pdrop(df: pd.DataFrame, out_dir: str) -> str:
    fig, ax = plt.subplots(figsize=FIG_SIZE)

    tout_values  = sorted(df["tout_ms"].unique())
    pdrop_values = sorted(df["pdrop"].unique())

    # Simulated curves (one per Tout)
    for i, tout in enumerate(tout_values):
        subset = df[df["tout_ms"] == tout].sort_values("pdrop")
        ax.plot(
            subset["pdrop"],
            subset["avg_tx"],
            color=COLORS[i % len(COLORS)],
            marker=MARKERS[i % len(MARKERS)],
            markersize=MARKER_S,
            linewidth=LINE_W,
            label=f"Simulated  Tout = {tout} ms",
        )

    # Theoretical reference curve
    p_range   = np.linspace(min(pdrop_values) * 0.5, max(pdrop_values) * 1.1, 200)
    theory    = [theoretical_avg_tx(p) for p in p_range]
    ax.plot(
        p_range, theory,
        color="black",
        linewidth=1.5,
        linestyle="--",
        label="Theoretical  E[Tx]",
    )

    ax.set_xticks(pdrop_values)
    ax.set_xticklabels([str(p) for p in pdrop_values], fontsize=10)
    ax.yaxis.set_major_formatter(ticker.FormatStrFormatter("%.3f"))

    style_axes(
        ax,
        xlabel="Packet Drop Probability  P_drop",
        ylabel="Avg. Transmissions per Packet",
        title="Average Transmissions per Packet vs. P_drop\n(Stop-and-Wait, max 4 attempts)",
    )

    fig.tight_layout()
    out_path = os.path.join(out_dir, "fig3_avgtx_vs_pdrop.png")
    fig.savefig(out_path, dpi=DPI, bbox_inches="tight")
    plt.close(fig)
    print(f"[plot_retransmissions] Saved → {out_path}")
    return out_path


# ── Figure 4: Dropped-after-4 vs P_drop ───────────────────────────────────────
def plot_dropped4_vs_pdrop(df: pd.DataFrame, out_dir: str) -> str:
    fig, ax = plt.subplots(figsize=FIG_SIZE)

    tout_values  = sorted(df["tout_ms"].unique())
    pdrop_values = sorted(df["pdrop"].unique())
    n_pdrop      = len(pdrop_values)
    n_tout       = len(tout_values)

    bar_w    = 0.18
    x        = np.arange(n_pdrop)
    offsets  = np.linspace(-(n_tout - 1) / 2, (n_tout - 1) / 2, n_tout) * bar_w

    for i, tout in enumerate(tout_values):
        subset = df[df["tout_ms"] == tout].sort_values("pdrop")
        bars = ax.bar(
            x + offsets[i],
            subset["dropped_after4"].values,
            width=bar_w,
            color=COLORS[i % len(COLORS)],
            label=f"Tout = {tout} ms",
            edgecolor="white",
            linewidth=0.5,
        )
        # Value label on top of each bar
        for bar in bars:
            h = bar.get_height()
            if h > 0:
                ax.text(
                    bar.get_x() + bar.get_width() / 2,
                    h + 0.1,
                    str(int(h)),
                    ha="center", va="bottom",
                    fontsize=8,
                )

    ax.set_xticks(x)
    ax.set_xticklabels([str(p) for p in pdrop_values], fontsize=10)
    ax.yaxis.set_major_locator(ticker.MaxNLocator(integer=True))

    style_axes(
        ax,
        xlabel="Packet Drop Probability  P_drop",
        ylabel="Packets Dropped After 4 Attempts",
        title="Packets Dropped After Max Retransmissions vs. P_drop\n(Stop-and-Wait, max 4 attempts)",
    )

    fig.tight_layout()
    out_path = os.path.join(out_dir, "fig4_dropped4_vs_pdrop.png")
    fig.savefig(out_path, dpi=DPI, bbox_inches="tight")
    plt.close(fig)
    print(f"[plot_retransmissions] Saved → {out_path}")
    return out_path


# ── Summary table ──────────────────────────────────────────────────────────────
def print_summary(df: pd.DataFrame) -> None:
    print("\n" + "═" * 72)
    print("  RETRANSMISSION SUMMARY TABLE")
    print("═" * 72)
    header = (f"{'P_drop':>8}  {'Tout(ms)':>9}  {'AvgTx/pkt':>10}  "
              f"{'DroppedAfter4':>14}  {'Theory E[Tx]':>13}")
    print(header)
    print("─" * 72)
    for _, row in df.sort_values(["pdrop", "tout_ms"]).iterrows():
        theory = theoretical_avg_tx(row["pdrop"])
        print(
            f"{row['pdrop']:>8.2f}  "
            f"{int(row['tout_ms']):>9d}  "
            f"{row['avg_tx']:>10.3f}  "
            f"{int(row['dropped_after4']):>14d}  "
            f"{theory:>13.3f}"
        )
    print("═" * 72 + "\n")


def main():
    parser = argparse.ArgumentParser(
        description="Generate retransmission plots (Fig 3 & 4) + summary table"
    )
    parser.add_argument("--csv", default=DEFAULT_CSV,
                        help=f"Path to results.csv (default: {DEFAULT_CSV})")
    parser.add_argument("--out", default=DEFAULT_OUT,
                        help=f"Output directory for PNG files (default: {DEFAULT_OUT})")
    args = parser.parse_args()

    os.makedirs(args.out, exist_ok=True)

    print(f"[plot_retransmissions] Loading: {args.csv}")
    df = load_data(args.csv)
    print(f"[plot_retransmissions] Loaded {len(df)} rows")

    print_summary(df)
    plot_avgtx_vs_pdrop(df, args.out)
    plot_dropped4_vs_pdrop(df, args.out)

    print("[plot_retransmissions] Done.")


if __name__ == "__main__":
    main()