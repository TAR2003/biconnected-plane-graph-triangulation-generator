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

    # also create floating version for scatter
    df['triangFloat'] = df['triangulations'].astype(float)

    # standard timing columns may vary depending on CSV version
    if ('meanTime' in df.columns):
        df['meanTime'] = df['meanTime'].astype(float)
    else:
        # fall back to avgTime for older files
        df['meanTime'] = df['avgTime'].astype(float)
    for col in ['medianTime', 'minTime', 'maxTime', 'stddevTime']:
        if col in df.columns:
            df[col] = df[col].astype(float)
        else:
            df[col] = df['meanTime']

    # compute per-triangulation values (in nanoseconds) for each metric
    df['perTriangMean'] = df['meanTime'] / df['triangulations'] * 1e9
    df['perTriangMedian'] = df['medianTime'] / df['triangulations'] * 1e9
    df['perTriangMin'] = df['minTime'] / df['triangulations'] * 1e9

    # bar chart: mean time per triangulation
    plt.figure(figsize=(12, 6))
    plt.bar(df['filename'], df['perTriangMean'], color='skyblue')
    plt.xticks(rotation=90, fontsize=8)
    plt.ylabel('mean time per triangulation (ns)')
    plt.title('Mean time per triangulation (nanoseconds)')
    plt.tight_layout()
    out_mean = 'avg_per_triangulation_mean.png'
    plt.savefig(out_mean)
    plt.close()

    # bar chart: median time per triangulation
    plt.figure(figsize=(12, 6))
    plt.bar(df['filename'], df['perTriangMedian'], color='orange')
    plt.xticks(rotation=90, fontsize=8)
    plt.ylabel('median time per triangulation (ns)')
    plt.title('Median time per triangulation (nanoseconds)')
    plt.tight_layout()
    out_med = 'avg_per_triangulation_median.png'
    plt.savefig(out_med)
    plt.close()

    # bar chart: minimum time per triangulation
    plt.figure(figsize=(12, 6))
    plt.bar(df['filename'], df['perTriangMin'], color='green')
    plt.xticks(rotation=90, fontsize=8)
    plt.ylabel('min time per triangulation (ns)')
    plt.title('Min time per triangulation (nanoseconds)')
    plt.tight_layout()
    out_min = 'avg_per_triangulation_min.png'
    plt.savefig(out_min)
    plt.close()

    # scatter chart: total time vs total triangulations (log-log) with error bars (min->max)
    plt.figure(figsize=(8, 6))
    yerr_lower = df['meanTime'] - df['minTime']
    yerr_upper = df['maxTime'] - df['meanTime']
    plt.errorbar(df['triangFloat'], df['meanTime'], yerr=[yerr_lower, yerr_upper], fmt='o', alpha=0.6, ecolor='gray', capsize=3)
    plt.xscale('log')
    plt.yscale('log')
    plt.xlabel('total triangulations (log scale)')
    plt.ylabel('mean time (s, log scale)')
    plt.title('Total time vs total triangulations (mean with error bars)')
    plt.grid(True, which='both', ls='--', lw=0.5)
    plt.tight_layout()
    out2 = 'total_time_vs_triangulations_mean_error.png'
    plt.savefig(out2)
    plt.close()

    # additional scatter charts for median/min/max
    for label, col in [('median', 'medianTime'), ('min', 'minTime'), ('max', 'maxTime')]:
        plt.figure(figsize=(8, 6))
        plt.scatter(df['triangFloat'], df[col], alpha=0.6, edgecolors='w')
        plt.xscale('log')
        plt.yscale('log')
        plt.xlabel('total triangulations (log scale)')
        plt.ylabel(f'{label} time (s, log scale)')
        plt.title(f'Total time vs total triangulations ({label})')
        plt.grid(True, which='both', ls='--', lw=0.5)
        plt.tight_layout()
        outname = f'total_time_vs_triangulations_{label}.png'
        plt.savefig(outname)
        plt.close()

    print(f"Saved plots: {out_mean}, {out_med}, {out_min}, {out2}, total_time_vs_triangulations_median.png, total_time_vs_triangulations_min.png, total_time_vs_triangulations_max.png")


if __name__ == '__main__':
    main()
