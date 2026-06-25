#!/usr/bin/env python3
"""
Plot per-triangulation cumulative average generation times.

Usage:
    python3 plot_results.py results/

For each input CSV (e.g. results/grid4x4.csv), produces:
    graphs/grid4x4.png

Also produces a combined overlay:
    graphs/combined.png
"""
import glob
import os
import sys

import matplotlib.pyplot as plt
import pandas as pd


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


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 plot_results.py RESULTS_DIR")
        sys.exit(1)

    results_dir = sys.argv[1]
    if not os.path.isdir(results_dir):
        print(f"Error: '{results_dir}' is not a directory")
        sys.exit(2)

    csv_files = sorted(glob.glob(os.path.join(results_dir, '*.csv')))
    if not csv_files:
        print(f"No CSV files found in '{results_dir}'")
        sys.exit(0)

    graphs_dir = 'graphs'
    os.makedirs(graphs_dir, exist_ok=True)

    combined_fig, combined_ax = plt.subplots(figsize=(12, 7))

    for csv_path in csv_files:
        label = os.path.splitext(os.path.basename(csv_path))[0]
        try:
            df = load_csv(csv_path)
        except Exception as exc:
            print(f"Skipping {csv_path}: {exc}")
            continue

        x = df['triangulation_n'] if 'triangulation_n' in df.columns else df.index + 1
        y = df['avg_ms']

        fig, ax = plt.subplots(figsize=(10, 6))
        ax.plot(x, y, linewidth=1.2)
        ax.set_xlabel('nth triangulation')
        ax.set_ylabel('average time per triangulation (ms)')
        ax.set_title(f'Cumulative average generation time: {label}')
        ax.grid(True, linestyle='--', linewidth=0.5, alpha=0.7)
        fig.tight_layout()
        out_path = os.path.join(graphs_dir, f'{label}.png')
        fig.savefig(out_path, dpi=150)
        plt.close(fig)

        combined_ax.plot(x, y, linewidth=1.0, label=label, alpha=0.85)

    combined_ax.set_xlabel('nth triangulation')
    combined_ax.set_ylabel('average time per triangulation (ms)')
    combined_ax.set_title('Cumulative average generation time (all inputs)')
    combined_ax.grid(True, linestyle='--', linewidth=0.5, alpha=0.7)
    combined_ax.legend(fontsize=7, loc='best', ncol=2)
    combined_fig.tight_layout()
    combined_path = os.path.join(graphs_dir, 'combined.png')
    combined_fig.savefig(combined_path, dpi=150)
    plt.close(combined_fig)

    print(f"Saved {len(csv_files)} per-input plots and combined graph in '{graphs_dir}/'")


if __name__ == '__main__':
    main()
