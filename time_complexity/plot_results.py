#!/usr/bin/env python3
"""
Simple plotting script for triangulation benchmark results.
Generates PNG files from a CSV produced by the C++ benchmark.

Usage:
    python3 plot_results.py results.csv

Produces:
    - avg_per_triangulation.png       (bar chart: average time per triangulation)
    - total_time_vs_triangulations.png (log-log scatter: total time vs total triangulations)
"""
import sys
import pandas as pd
import matplotlib.pyplot as plt


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 plot_results.py RESULTS_CSV")
        sys.exit(1)

    csv_file = sys.argv[1]
    try:
        df = pd.read_csv(csv_file)
    except Exception as e:
        print(f"Error reading CSV '{csv_file}': {e}")
        sys.exit(2)

    # make sure numeric columns are correct types
    df['triangulations'] = df['triangulations'].apply(lambda x: int(x))
    df['avgTime'] = df['avgTime'].astype(float)
    # some CSVs already have perTriangNs, otherwise compute it
    if 'perTriangNs' not in df.columns:
        df['perTriangNs'] = df['avgTime'] / df['triangulations'] * 1e9
    else:
        df['perTriangNs'] = df['perTriangNs'].astype(float)

    # bar chart: average time per triangulation (ns) per file
    plt.figure(figsize=(12, 6))
    plt.bar(df['filename'], df['perTriangNs'], color='skyblue')
    plt.xticks(rotation=90, fontsize=8)
    plt.ylabel('avg time per triangulation (ns)')
    plt.title('Average time per triangulation (nanoseconds)')
    plt.tight_layout()
    out1 = 'avg_per_triangulation.png'
    plt.savefig(out1)
    plt.close()

    # scatter chart: total time vs total triangulations (log-log)
    plt.figure(figsize=(8, 6))
    plt.scatter(df['triangulations'], df['avgTime'], alpha=0.6, edgecolors='w')
    plt.xscale('log')
    plt.yscale('log')
    plt.xlabel('total triangulations (log scale)')
    plt.ylabel('avg time (s, log scale)')
    plt.title('Total time vs total triangulations')
    plt.grid(True, which='both', ls='--', lw=0.5)
    plt.tight_layout()
    out2 = 'total_time_vs_triangulations.png'
    plt.savefig(out2)
    plt.close()

    print(f"Saved plots: {out1}, {out2}")


if __name__ == '__main__':
    main()
