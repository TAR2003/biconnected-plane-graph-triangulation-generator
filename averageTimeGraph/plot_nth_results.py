#!/usr/bin/env python3
"""
Plot per-triangulation cumulative average generation times up to the Nth triangulation.

Usage:
    python3 plot_nth_results.py results/
    python3 plot_nth_results.py results/ --n 200

For each input CSV (e.g. results/grid4x4.csv), produces:
    nthGraphs/grid4x4.png

Also produces a combined overlay:
    nthGraphs/combined.png

Change N below or pass --n on the command line.
"""
import argparse
import glob
import os
import sys

import matplotlib.pyplot as plt
import pandas as pd

# Default cutoff: plot cumulative averages for triangulations 1 .. N.
N = 100


def load_csv(path):
    df = pd.read_csv(path)
    if 'cumulative_avg_ns' in df.columns:
        df['avg_ms'] = df['cumulative_avg_ns'].astype(float) / 1e6
    elif 'incremental_ns' in df.columns:
        inc = df['incremental_ns'].astype(float)
        df['avg_ms'] = inc.cumsum() / (df.index + 1) / 1e6
    else:
        raise ValueError(f"CSV {path} missing timing columns")
    return df


def truncate_to_n(df, n):
    if 'triangulation_n' in df.columns:
        return df[df['triangulation_n'] <= n].copy()
    return df.iloc[:n].copy()


def final_average_ms(df):
    return float(df['avg_ms'].iloc[-1])


def get_xy(df):
    x = df['triangulation_n'] if 'triangulation_n' in df.columns else df.index + 1
    return x, df['avg_ms']


def add_stabilization_marker(ax, x_end, final_avg_ms):
    """Draw a horizontal guide on the right showing the final cumulative average."""
    xmin, xmax = ax.get_xlim()
    ymin, ymax = ax.get_ylim()
    pad = max((xmax - xmin) * 0.08, 1.0)
    marker_x = xmax + pad

    ax.axhline(
        final_avg_ms,
        color='#c0392b',
        linestyle='--',
        linewidth=1.4,
        alpha=0.9,
        zorder=2,
    )
    ax.plot(
        [x_end, marker_x],
        [final_avg_ms, final_avg_ms],
        color='#c0392b',
        linestyle='-',
        linewidth=2.0,
        marker='>',
        markersize=7,
        zorder=3,
    )
    ax.scatter(
        [marker_x],
        [final_avg_ms],
        s=70,
        color='#c0392b',
        zorder=4,
        clip_on=False,
    )
    ax.annotate(
        f'final avg: {final_avg_ms:.4f} ms',
        xy=(marker_x, final_avg_ms),
        xytext=(12, 0),
        textcoords='offset points',
        va='center',
        ha='left',
        fontsize=9,
        color='#c0392b',
        fontweight='bold',
        clip_on=False,
    )

    ax.set_xlim(xmin, marker_x + pad * 0.6)
    margin = max((ymax - ymin) * 0.05, final_avg_ms * 0.05, 1e-6)
    ax.set_ylim(min(ymin, final_avg_ms - margin), max(ymax, final_avg_ms + margin))


def add_combined_stabilization_guide(ax, x_end, final_avg_ms, color):
    """Lightweight final-average guide for the combined overlay."""
    xmin, xmax = ax.get_xlim()
    pad = max((xmax - xmin) * 0.06, 1.0)
    marker_x = xmax + pad

    ax.plot(
        [x_end, marker_x],
        [final_avg_ms, final_avg_ms],
        color=color,
        linestyle='--',
        linewidth=1.0,
        alpha=0.75,
        zorder=2,
    )
    ax.scatter(
        [marker_x],
        [final_avg_ms],
        s=18,
        color=color,
        alpha=0.9,
        zorder=3,
        clip_on=False,
    )
    return marker_x + pad * 0.35


def main():
    parser = argparse.ArgumentParser(
        description='Plot cumulative average triangulation times up to the Nth triangulation.'
    )
    parser.add_argument('results_dir', help='Directory containing per-input CSV files')
    parser.add_argument(
        '--n',
        type=int,
        default=N,
        help=f'Plot only through this triangulation number (default: {N})',
    )
    args = parser.parse_args()

    results_dir = args.results_dir
    n_cutoff = args.n
    if n_cutoff < 1:
        print('Error: --n must be at least 1')
        sys.exit(2)

    if not os.path.isdir(results_dir):
        print(f"Error: '{results_dir}' is not a directory")
        sys.exit(2)

    csv_files = sorted(glob.glob(os.path.join(results_dir, '*.csv')))
    if not csv_files:
        print(f"No CSV files found in '{results_dir}'")
        sys.exit(0)

    graphs_dir = 'nthGraphs'
    os.makedirs(graphs_dir, exist_ok=True)

    combined_fig, combined_ax = plt.subplots(figsize=(12, 7))
    plotted = 0
    combined_series = []

    for csv_path in csv_files:
        label = os.path.splitext(os.path.basename(csv_path))[0]
        try:
            df_full = load_csv(csv_path)
        except Exception as exc:
            print(f"Skipping {csv_path}: {exc}")
            continue

        final_avg_ms = final_average_ms(df_full)
        df = truncate_to_n(df_full, n_cutoff)
        x, y = get_xy(df)
        x_end = float(x.iloc[-1])

        fig, ax = plt.subplots(figsize=(10, 6))
        ax.plot(x, y, linewidth=1.2)
        add_stabilization_marker(ax, x_end, final_avg_ms)
        ax.set_xlabel('nth triangulation')
        ax.set_ylabel('average time per triangulation (ms)')
        ax.set_title(
            f'Cumulative average generation time: {label} (first {n_cutoff} triangulations)'
        )
        ax.grid(True, linestyle='--', linewidth=0.5, alpha=0.7)
        fig.tight_layout()
        out_path = os.path.join(graphs_dir, f'{label}.png')
        fig.savefig(out_path, dpi=150, bbox_inches='tight')
        plt.close(fig)

        line = combined_ax.plot(x, y, linewidth=1.0, label=label, alpha=0.85)[0]
        combined_series.append((x_end, final_avg_ms, line.get_color()))
        plotted += 1

    if plotted == 0:
        print('No plots were generated.')
        sys.exit(0)

    right_edge = float(combined_ax.get_xlim()[1])
    ymin, ymax = combined_ax.get_ylim()
    for x_end, final_avg_ms, color in combined_series:
        right_edge = max(right_edge, add_combined_stabilization_guide(
            combined_ax, x_end, final_avg_ms, color
        ))
        ymin = min(ymin, final_avg_ms)
        ymax = max(ymax, final_avg_ms)

    combined_ax.set_xlim(combined_ax.get_xlim()[0], right_edge)
    margin = max((ymax - ymin) * 0.05, 1e-6)
    combined_ax.set_ylim(ymin - margin, ymax + margin)
    combined_ax.set_xlabel('nth triangulation')
    combined_ax.set_ylabel('average time per triangulation (ms)')
    combined_ax.set_title(
        f'Cumulative average generation time (all inputs, first {n_cutoff} triangulations)'
    )
    combined_ax.grid(True, linestyle='--', linewidth=0.5, alpha=0.7)
    combined_ax.legend(fontsize=7, loc='best', ncol=2)
    combined_fig.tight_layout()
    combined_path = os.path.join(graphs_dir, 'combined.png')
    combined_fig.savefig(combined_path, dpi=150, bbox_inches='tight')
    plt.close(combined_fig)

    print(
        f"Saved {plotted} per-input plots and combined graph in '{graphs_dir}/' "
        f"(n={n_cutoff})"
    )


if __name__ == '__main__':
    main()
