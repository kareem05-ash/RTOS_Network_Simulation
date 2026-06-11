"""
plot_throughput.py
─────────────────────────────────────────────────────────────────────────────
Generates the two throughput plots required by Dr. Khaled's project spec:

  Figure 1 — Throughput vs P_drop
              X-axis : P_drop values {0.01, 0.02, 0.04, 0.08}
              Y-axis : Throughput (bytes/sec)
              Curves : one per Tout value (150, 175, 200, 225 ms)

  Figure 2 — Throughput vs Tout
              X-axis : Tout values {150, 175, 200, 225} ms
              Y-axis : Throughput (bytes/sec)
              Curves : one per P_drop value (0.01, 0.02, 0.04, 0.08)

Outputs (PNG):
  simulation_results/plots/fig1_throughput_vs_pdrop.png
  simulation_results/plots/fig2_throughput_vs_tout.png

Usage:
  python plot_throughput.py
  python plot_throughput.py --csv path/to/results.csv --out path/to/plots/
─────────────────────────────────────────────────────────────────────────────
"""

import os
import argparse
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

# ── Paths ─────────────────────────────────────────────────────────────────────
SCRIPT_DIR   = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
DEFAULT_CSV  = os.path.join(PROJECT_ROOT, "simulation_results", "csv", "results.csv")
DEFAULT_OUT  = os.path.join(PROJECT_ROOT, "simulation_results", "plots")

# ── Plot style constants ───────────────────────────────────────────────────────
COLORS    = ["#1f77b4", "#d62728", "#2ca02c", "#ff7f0e"]   # blue, red, green, orange
MARKERS   = ["o", "s", "^", "D"]                            # circle, square, triangle, diamond
LINE_W    = 2.0
MARKER_S  = 8
GRID_ALPHA= 0.35
FIG_SIZE  = (8, 5.5)
DPI       = 150


def load_data(csv_path: str) -> pd.DataFrame:
    if not os.path.isfile(csv_path):
        raise FileNotFoundError(
            f"CSV not found: {csv_path}\n"
            "Run parse_logs.py first to generate it."
        )
    df = pd.read_csv(csv_path)
    required = {"pdrop", "tout_ms", "throughput"}
    missing = required - set(df.columns)
    if missing:
        raise ValueError(f"CSV is missing columns: {missing}")
    return df


def style_axes(ax, xlabel, ylabel, title):
    """Apply consistent formatting to any axes object."""
    ax.set_xlabel(xlabel, fontsize=12)
    ax.set_ylabel(ylabel, fontsize=12)
    ax.set_title(title, fontsize=13, fontweight="bold", pad=12)
    ax.grid(True, linestyle="--", alpha=GRID_ALPHA)
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    ax.legend(fontsize=10, framealpha=0.85, edgecolor="#cccccc")
    ax.yaxis.set_major_formatter(ticker.FuncFormatter(lambda x, _: f"{x:,.0f}"))


# ── Figure 1: Throughput vs P_drop ────────────────────────────────────────────
def plot_throughput_vs_pdrop(df: pd.DataFrame, out_dir: str) -> str:
    fig, ax = plt.subplots(figsize=FIG_SIZE)

    tout_values = sorted(df["tout_ms"].unique())

    for i, tout in enumerate(tout_values):
        subset = df[df["tout_ms"] == tout].sort_values("pdrop")
        ax.plot(
            subset["pdrop"],
            subset["throughput"],
            color=COLORS[i % len(COLORS)],
            marker=MARKERS[i % len(MARKERS)],
            markersize=MARKER_S,
            linewidth=LINE_W,
            label=f"Tout = {tout} ms",
        )

    # X-axis: show exact P_drop tick values (not interpolated)
    pdrop_vals = sorted(df["pdrop"].unique())
    ax.set_xticks(pdrop_vals)
    ax.set_xticklabels([str(p) for p in pdrop_vals], fontsize=10)

    style_axes(
        ax,
        xlabel="Packet Drop Probability  P_drop",
        ylabel="Throughput  (bytes/sec)",
        title="Throughput vs. Packet Drop Probability\n(Stop-and-Wait Protocol, C = 100 kbps)",
    )

    # Annotation: show throughput at highest drop rate
    for i, tout in enumerate(tout_values):
        subset = df[df["tout_ms"] == tout].sort_values("pdrop")
        last = subset.iloc[-1]
        ax.annotate(
            f"{last['throughput']:,.0f}",
            xy=(last["pdrop"], last["throughput"]),
            xytext=(6, 4),
            textcoords="offset points",
            fontsize=8,
            color=COLORS[i % len(COLORS)],
        )

    fig.tight_layout()
    out_path = os.path.join(out_dir, "fig1_throughput_vs_pdrop.png")
    fig.savefig(out_path, dpi=DPI, bbox_inches="tight")
    plt.close(fig)
    print(f"[plot_throughput] Saved → {out_path}")
    return out_path


# ── Figure 2: Throughput vs Tout ──────────────────────────────────────────────
def plot_throughput_vs_tout(df: pd.DataFrame, out_dir: str) -> str:
    fig, ax = plt.subplots(figsize=FIG_SIZE)

    pdrop_values = sorted(df["pdrop"].unique())

    for i, pdrop in enumerate(pdrop_values):
        subset = df[df["pdrop"] == pdrop].sort_values("tout_ms")
        ax.plot(
            subset["tout_ms"],
            subset["throughput"],
            color=COLORS[i % len(COLORS)],
            marker=MARKERS[i % len(MARKERS)],
            markersize=MARKER_S,
            linewidth=LINE_W,
            label=f"P_drop = {pdrop}",
        )

    # X-axis: show exact Tout tick values
    tout_vals = sorted(df["tout_ms"].unique())
    ax.set_xticks(tout_vals)
    ax.set_xticklabels([f"{t} ms" for t in tout_vals], fontsize=10)

    style_axes(
        ax,
        xlabel="Timeout Period  Tout  (ms)",
        ylabel="Throughput  (bytes/sec)",
        title="Throughput vs. Timeout Period\n(Stop-and-Wait Protocol, C = 100 kbps)",
    )

    # Annotation: show throughput at lowest tout (best case per curve)
    for i, pdrop in enumerate(pdrop_values):
        subset = df[df["pdrop"] == pdrop].sort_values("tout_ms")
        first = subset.iloc[0]
        ax.annotate(
            f"{first['throughput']:,.0f}",
            xy=(first["tout_ms"], first["throughput"]),
            xytext=(-30, 6),
            textcoords="offset points",
            fontsize=8,
            color=COLORS[i % len(COLORS)],
        )

    fig.tight_layout()
    out_path = os.path.join(out_dir, "fig2_throughput_vs_tout.png")
    fig.savefig(out_path, dpi=DPI, bbox_inches="tight")
    plt.close(fig)
    print(f"[plot_throughput] Saved → {out_path}")
    return out_path


def main():
    parser = argparse.ArgumentParser(description="Generate throughput plots (Fig 1 & 2)")
    parser.add_argument("--csv", default=DEFAULT_CSV,
                        help=f"Path to results.csv (default: {DEFAULT_CSV})")
    parser.add_argument("--out", default=DEFAULT_OUT,
                        help=f"Output directory for PNG files (default: {DEFAULT_OUT})")
    args = parser.parse_args()

    os.makedirs(args.out, exist_ok=True)

    print(f"[plot_throughput] Loading: {args.csv}")
    df = load_data(args.csv)
    print(f"[plot_throughput] Loaded {len(df)} rows  |  "
          f"P_drop values: {sorted(df['pdrop'].unique())}  |  "
          f"Tout values: {sorted(df['tout_ms'].unique())}")

    plot_throughput_vs_pdrop(df, args.out)
    plot_throughput_vs_tout(df, args.out)

    print("[plot_throughput] Done.")


if __name__ == "__main__":
    main()