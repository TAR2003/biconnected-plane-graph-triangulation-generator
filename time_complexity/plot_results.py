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

    # ensure graphs directory exists and clear obsolete PNGs in cwd
    outdir = "graphs"
    import os, glob
    os.makedirs(outdir, exist_ok=True)
    # remove any previously generated png in root to avoid confusion
    for f in glob.glob("*.png"):
        try:
            os.remove(f)
        except Exception:
            pass

    # helper to prefix output filenames
    def path(name):
        return os.path.join(outdir, name)

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
    out_mean = path('avg_per_triangulation_mean.png')
    plt.savefig(out_mean)
    plt.close()

    # bar chart: median time per triangulation
    plt.figure(figsize=(12, 6))
    plt.bar(df['filename'], df['perTriangMedian'], color='orange')
    plt.xticks(rotation=90, fontsize=8)
    plt.ylabel('median time per triangulation (ns)')
    plt.title('Median time per triangulation (nanoseconds)')
    plt.tight_layout()
    out_med = path('avg_per_triangulation_median.png')
    plt.savefig(out_med)
    plt.close()

    # bar chart: minimum time per triangulation
    plt.figure(figsize=(12, 6))
    plt.bar(df['filename'], df['perTriangMin'], color='green')
    plt.xticks(rotation=90, fontsize=8)
    plt.ylabel('min time per triangulation (ns)')
    plt.title('Min time per triangulation (nanoseconds)')
    plt.tight_layout()
    out_min = path('avg_per_triangulation_min.png')
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
    out2 = path('total_time_vs_triangulations_mean_error.png')
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
        outname = path(f'total_time_vs_triangulations_{label}.png')
        plt.savefig(outname)
        plt.close()

    # regression line (log-log) through mean points + straight envelope lines
    import numpy as np
    logx = np.log10(df['triangFloat'].values)
    logy = np.log10(df['meanTime'].values)

    # linear fit in log space
    coeffs = np.polyfit(logx, logy, 1)
    fit_fn = np.poly1d(coeffs)

    # predicted values in linear domain
    xs_sorted = np.sort(df['triangFloat'].values)
    logx_sorted = np.log10(xs_sorted)
    y_pred = 10 ** (fit_fn(logx_sorted))

    # compute range ratios based on error bars (mean +/- stddev)
    ratios_upper = []
    ratios_lower = []
    for xi, yi, sd in zip(df['triangFloat'].values, df['meanTime'].values, df['stddevTime'].values):
        pred = 10 ** (fit_fn(np.log10(xi)))
        if pred > 0:
            ratios_upper.append((yi + sd) / pred)
            ratios_lower.append(max((yi - sd) / pred, 0.0))
    max_ratio = max(ratios_upper) if ratios_upper else 1.0
    min_ratio = min(ratios_lower) if ratios_lower else 1.0

    upper_vals = y_pred * max_ratio
    lower_vals = y_pred * min_ratio

    plt.figure(figsize=(8,6))
    # plot regression lines with distinct colors
    plt.plot(xs_sorted, y_pred, label='regression mean', color='navy', linewidth=2)
    plt.plot(xs_sorted, upper_vals, label='regression upper', color='firebrick', linestyle='--', linewidth=1.5)
    plt.plot(xs_sorted, lower_vals, label='regression lower', color='forestgreen', linestyle='--', linewidth=1.5)
    # fill region between upper and lower with light color
    plt.fill_between(xs_sorted, lower_vals, upper_vals, color='lightgray', alpha=0.4)
    # overlay actual mean points with error bars
    yerr_lower = df['meanTime'] - df['minTime']
    yerr_upper = df['maxTime'] - df['meanTime']
    plt.errorbar(df['triangFloat'], df['meanTime'], yerr=[yerr_lower, yerr_upper], fmt='o', color='black', alpha=0.6, capsize=3, label='mean points')
    plt.xscale('log'); plt.yscale('log')
    plt.xlabel('total triangulations (log scale)')
    plt.ylabel('time (s, log scale)')
    plt.title('Regression envelope across means')
    plt.legend()
    plt.grid(True, which='both', ls='--', lw=0.5)
    plt.tight_layout()
    out3 = path('total_time_vs_triangulations_global_range.png')
    plt.savefig(out3)
    plt.close()

    # smoothed envelope line graph following sorted triangulations
    df_sorted = df.sort_values('triangulations')
    smoothed = df_sorted['meanTime'].rolling(window=3, min_periods=1, center=True).mean()
    upper_env = (df_sorted['meanTime'] + df_sorted['stddevTime']).rolling(window=3, min_periods=1, center=True).mean()
    lower_env = (df_sorted['meanTime'] - df_sorted['stddevTime']).rolling(window=3, min_periods=1, center=True).mean()
    plt.figure(figsize=(8,6))
    plt.plot(df_sorted['triangFloat'], smoothed, label='smoothed mean', color='blue')
    plt.plot(df_sorted['triangFloat'], upper_env, label='smoothed upper', color='red')
    plt.plot(df_sorted['triangFloat'], lower_env, label='smoothed lower', color='green')
    plt.xscale('log'); plt.yscale('log')
    plt.xlabel('total triangulations (log scale)')
    plt.ylabel('time (s, log scale)')
    plt.title('Smoothed mean and envelope')
    plt.legend()
    plt.grid(True, which='both', ls='--', lw=0.5)
    plt.tight_layout()
    out4 = path('total_time_vs_triangulations_smoothed.png')
    plt.savefig(out4)
    plt.close()

    print(f"Saved plots in '{outdir}': {os.path.basename(out_mean)}, {os.path.basename(out_med)}, {os.path.basename(out_min)}, {os.path.basename(out2)}, {os.path.basename(out3)}, {os.path.basename(out4)}, total_time_vs_triangulations_median.png, total_time_vs_triangulations_min.png, total_time_vs_triangulations_max.png")


if __name__ == '__main__':
    main()
