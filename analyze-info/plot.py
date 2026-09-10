#!/usr/bin/env python3
"""
analyze_benchmarks.py
======================

Research-grade analysis & plotting suite for the triangulation-enumeration
benchmark CSVs produced by the C++ benchmark harness (results_<category>.csv).

Expected CSV schema (one row per individual timed run):

    filename,runIndex,vertices,triangulations,timeSeconds,peakMemoryBytes,
    memoryPerVertex,startTime,endTime,status

Usage
-----
    python3 analyze_benchmarks.py --input-dir /path/to/csvs --out-dir plots

By default it looks for all files matching `results_*.csv` in --input-dir
(default: current directory) and writes every figure as a high-res PNG
(and PDF) into --out-dir (default: ./plots).

The script is defensive: it does not assume every CSV has the same columns,
skips empty/broken files with a warning, coerces the giant triangulation
counts (which can exceed 64-bit range because of the C++ unsigned __int128
usage) via Python's arbitrary precision integers / high-precision floats,
and never crashes the whole run because one category/file is malformed.

Main scientific claim under test: the algorithm enumerates all
triangulations in amortized O(1) time per triangulation, i.e.

    timeSeconds / triangulations  ->  constant   as triangulations -> inf

Nearly every figure in this script is built to interrogate that claim from
a different angle (raw scaling, log-log slope, per-triangulation cost decay,
residuals from a linear fit, category comparisons, distributional views,
memory-side sanity checks, etc).
"""

import argparse
import glob
import os
import re
import sys
import warnings
from pathlib import Path

import numpy as np
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import matplotlib.ticker as mticker
from matplotlib.lines import Line2D

warnings.filterwarnings("ignore")

# ----------------------------------------------------------------------------
# Global plotting style
# ----------------------------------------------------------------------------
plt.rcParams.update({
    "figure.dpi": 120,
    "savefig.dpi": 200,
    "font.size": 11,
    "axes.titlesize": 13,
    "axes.titleweight": "bold",
    "axes.labelsize": 11,
    "legend.fontsize": 9,
    "figure.facecolor": "white",
    "axes.facecolor": "white",
    "axes.grid": True,
    "grid.alpha": 0.3,
    "grid.linestyle": "--",
})

CMAP = plt.get_cmap("tab20")


# ============================================================================
# Loading & cleaning
# ============================================================================

def natural_case_key(name: str):
    """Sort helper: cat_1_2 < cat_1_10 (numeric-aware), not lexicographic."""
    parts = re.split(r"(\d+)", name)
    return [int(p) if p.isdigit() else p for p in parts]


def load_all_csvs(input_dir: str) -> pd.DataFrame:
    pattern = os.path.join(input_dir, "results_*.csv")
    paths = sorted(glob.glob(pattern))
    if not paths:
        # also accept a single combined csv or any *.csv as a fallback
        paths = sorted(glob.glob(os.path.join(input_dir, "*.csv")))
    if not paths:
        print(f"[ERROR] No CSV files found under: {input_dir}", file=sys.stderr)
        sys.exit(1)

    frames = []
    for p in paths:
        try:
            df = pd.read_csv(p, dtype=str)  # read as str first: triangulations may overflow int64
        except Exception as e:
            print(f"[WARN] Could not read {p}: {e}", file=sys.stderr)
            continue
        if df.empty:
            print(f"[WARN] Empty file, skipping: {p}", file=sys.stderr)
            continue

        cat = re.sub(r"^results_", "", Path(p).stem)
        df["category"] = cat
        df["source_file"] = os.path.basename(p)
        frames.append(df)

    if not frames:
        print("[ERROR] All CSVs were empty or unreadable.", file=sys.stderr)
        sys.exit(1)

    combined = pd.concat(frames, ignore_index=True)
    return clean(combined)


def clean(df: pd.DataFrame) -> pd.DataFrame:
    # Numeric coercions. triangulations can be a huge integer (up to __int128
    # range) stored as a decimal string -> use Python int then cast to float
    # for plotting (float64 safely handles up to ~1.8e308 in magnitude with
    # rounding, which is fine for log-log plotting purposes).
    def to_big_float(x):
        try:
            return float(int(str(x).strip()))
        except Exception:
            try:
                return float(x)
            except Exception:
                return np.nan

    if "triangulations" in df.columns:
        df["triangulations_f"] = df["triangulations"].apply(to_big_float)
    else:
        df["triangulations_f"] = np.nan

    for col in ["runIndex", "vertices", "timeSeconds", "peakMemoryBytes", "memoryPerVertex"]:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors="coerce")
        else:
            df[col] = np.nan

    if "status" not in df.columns:
        df["status"] = "completed"
    df["status"] = df["status"].fillna("completed")

    if "filename" not in df.columns:
        df["filename"] = df.get("source_file", "unknown")

    # Derived metrics
    with np.errstate(divide="ignore", invalid="ignore"):
        df["time_per_triangulation"] = df["timeSeconds"] / df["triangulations_f"]
        df["triangulations_per_sec"] = df["triangulations_f"] / df["timeSeconds"]

    df["time_per_triangulation"] = df["time_per_triangulation"].replace([np.inf, -np.inf], np.nan)
    df["triangulations_per_sec"] = df["triangulations_per_sec"].replace([np.inf, -np.inf], np.nan)

    # Drop rows with no usable time or triangulation count at all
    before = len(df)
    df = df[(df["timeSeconds"].notna()) & (df["timeSeconds"] >= 0)]
    after = len(df)
    if after < before:
        print(f"[INFO] Dropped {before - after} rows with invalid/missing timeSeconds.")

    df["case"] = df["filename"].astype(str)
    df["category"] = df["category"].astype(str)

    return df.reset_index(drop=True)


# ============================================================================
# Helpers
# ============================================================================

def category_color_map(categories):
    cats = sorted(categories, key=natural_case_key)
    colors = {}
    n = max(len(cats), 1)
    cmap = plt.get_cmap("turbo", n) if n > 12 else plt.get_cmap("tab10", max(n, 1))
    for i, c in enumerate(cats):
        colors[c] = cmap(i / max(n - 1, 1)) if n > 1 else cmap(0)
    return colors


def savefig(fig, out_dir, name):
    fig.tight_layout()
    png = os.path.join(out_dir, f"{name}.png")
    fig.savefig(png, bbox_inches="tight")
    plt.close(fig)
    print(f"  -> {png}")


def per_case_summary(df: pd.DataFrame) -> pd.DataFrame:
    """One row per (category, filename): mean/std/min/max across the repeated runs."""
    grp = df.groupby(["category", "case"], as_index=False)
    summary = grp.agg(
        vertices=("vertices", "max"),
        triangulations=("triangulations_f", "max"),
        mean_time=("timeSeconds", "mean"),
        std_time=("timeSeconds", "std"),
        min_time=("timeSeconds", "min"),
        max_time=("timeSeconds", "max"),
        n_runs=("timeSeconds", "count"),
        mean_time_per_tri=("time_per_triangulation", "mean"),
        std_time_per_tri=("time_per_triangulation", "std"),
        mean_tri_per_sec=("triangulations_per_sec", "mean"),
        mean_mem=("peakMemoryBytes", "mean"),
        mean_mem_per_vertex=("memoryPerVertex", "mean"),
        any_limit_exceeded=("status", lambda s: (s == "limit_exceeded").any()),
    )
    summary["std_time"] = summary["std_time"].fillna(0.0)
    summary["std_time_per_tri"] = summary["std_time_per_tri"].fillna(0.0)
    # sort within category by triangulation count ascending (as requested:
    # "show 2nd case column first if it has fewer triangulations")
    summary = summary.sort_values(["category", "triangulations"], kind="stable")
    return summary.reset_index(drop=True)


def sort_cases_by_triangulations(sub: pd.DataFrame) -> pd.DataFrame:
    return sub.sort_values("triangulations", kind="stable")


# ============================================================================
# PLOT 1: Average time per category, grouped bar chart, cases ordered by
#          increasing triangulation count within each category
# ============================================================================

def plot_avg_time_per_category(summary: pd.DataFrame, out_dir: str):
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)

    n_cat = len(categories)
    fig, axes = plt.subplots(n_cat, 1, figsize=(max(10, 0.5 * summary.groupby("category").size().max()), 3.2 * n_cat),
                              squeeze=False)
    for i, cat in enumerate(categories):
        ax = axes[i, 0]
        sub = sort_cases_by_triangulations(summary[summary["category"] == cat])
        x = np.arange(len(sub))
        ax.bar(x, sub["mean_time"], yerr=sub["std_time"], capsize=3,
               color=colors[cat], edgecolor="black", linewidth=0.5)
        ax.set_xticks(x)
        labels = [f"{c}\n(n={int(t):,})" if not np.isnan(t) else c
                  for c, t in zip(sub["case"], sub["triangulations"])]
        ax.set_xticklabels(labels, rotation=45, ha="right", fontsize=7)
        ax.set_ylabel("Mean time (s)")
        ax.set_title(f"Category: {cat} — avg. runtime per case (ordered by ↑ triangulation count)")
        ax.set_yscale("log")
    savefig(fig, out_dir, "01_avg_time_per_category_ordered")


def plot_avg_time_all_categories_together(summary: pd.DataFrame, out_dir: str):
    """All categories on one combined chart, different color per category."""
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)

    fig, ax = plt.subplots(figsize=(max(12, 0.3 * len(summary)), 6))
    x = 0
    xticks, xlabels = [], []
    for cat in categories:
        sub = sort_cases_by_triangulations(summary[summary["category"] == cat])
        xs = np.arange(x, x + len(sub))
        ax.bar(xs, sub["mean_time"], yerr=sub["std_time"], capsize=2,
               color=colors[cat], edgecolor="black", linewidth=0.3, label=cat)
        xticks.extend(xs)
        xlabels.extend(sub["case"])
        x += len(sub) + 1  # gap between categories

    ax.set_xticks(xticks)
    ax.set_xticklabels(xlabels, rotation=90, fontsize=6)
    ax.set_yscale("log")
    ax.set_ylabel("Mean time (s, log scale)")
    ax.set_title("Average runtime per test case — all categories (color-coded), each ordered by ↑ triangulations")
    handles = [Line2D([0], [0], color=colors[c], lw=6) for c in categories]
    ax.legend(handles, categories, ncol=min(6, len(categories)), loc="upper left", bbox_to_anchor=(0, 1.15))
    savefig(fig, out_dir, "02_avg_time_all_categories_combined")


# ============================================================================
# PLOT 2: log-log Total triangulations vs total time, category-colored
# ============================================================================

def plot_loglog_scatter(summary: pd.DataFrame, out_dir: str):
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)

    fig, ax = plt.subplots(figsize=(9, 7))
    for cat in categories:
        sub = summary[summary["category"] == cat]
        sub = sub[(sub["triangulations"] > 0) & (sub["mean_time"] > 0)]
        if sub.empty:
            continue
        ax.scatter(sub["triangulations"], sub["mean_time"], label=cat, color=colors[cat],
                   s=45, edgecolor="black", linewidth=0.4, alpha=0.85)

    valid = summary[(summary["triangulations"] > 0) & (summary["mean_time"] > 0)]
    if len(valid) > 1:
        logx = np.log10(valid["triangulations"].values)
        logy = np.log10(valid["mean_time"].values)
        slope, intercept = np.polyfit(logx, logy, 1)
        xs = np.linspace(logx.min(), logx.max(), 100)
        ax.plot(10**xs, 10**(slope * xs + intercept), "k--", lw=2,
                label=f"Global fit: slope={slope:.3f}")
        # Reference line: perfect O(n) i.e. slope=1 (amortized constant time per triangulation)
        ref_intercept = logy[np.argmin(logx)] - 1.0 * logx[np.argmin(logx)]
        ax.plot(10**xs, 10**(1.0 * xs + ref_intercept), color="gray", ls=":", lw=2,
                label="Reference slope=1 (O(n), i.e. amortized O(1)/triangulation)")

    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("Total triangulations generated (log scale)")
    ax.set_ylabel("Total time (s, log scale)")
    ax.set_title("Log-Log: Total Triangulations vs Total Time (all categories)")
    ax.legend(fontsize=8, ncol=2, loc="upper left")
    savefig(fig, out_dir, "03_loglog_triangulations_vs_time")


def plot_loglog_lines_per_category(summary: pd.DataFrame, out_dir: str):
    """Line-progression version: connect points within each category in order
    of increasing triangulation count, to visualize scaling trend/slope per
    category rather than just a scatter cloud."""
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)

    fig, ax = plt.subplots(figsize=(9, 7))
    for cat in categories:
        sub = summary[summary["category"] == cat]
        sub = sub[(sub["triangulations"] > 0) & (sub["mean_time"] > 0)]
        sub = sort_cases_by_triangulations(sub)
        if len(sub) < 1:
            continue
        ax.plot(sub["triangulations"], sub["mean_time"], "-o", color=colors[cat],
                label=cat, markersize=5, linewidth=1.6, alpha=0.9,
                markeredgecolor="black", markeredgewidth=0.3)

    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("Total triangulations (log scale)")
    ax.set_ylabel("Total time (s, log scale)")
    ax.set_title("Log-Log Line Progression: Time vs Triangulation Count, per Category")
    ax.legend(fontsize=8, ncol=2, loc="upper left")
    savefig(fig, out_dir, "04_loglog_line_progression_per_category")


# ============================================================================
# PLOT 3: The key claim — amortized O(1) per triangulation
# ============================================================================

def plot_time_per_triangulation_vs_n(summary: pd.DataFrame, out_dir: str):
    """The central plot for the paper's claim: time/triangulation should be
    flat (constant) as the triangulation count grows, across many orders
    of magnitude."""
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)

    fig, ax = plt.subplots(figsize=(9, 6.5))
    for cat in categories:
        sub = summary[summary["category"] == cat]
        sub = sub[(sub["triangulations"] > 0) & (sub["mean_time_per_tri"].notna())]
        sub = sort_cases_by_triangulations(sub)
        if sub.empty:
            continue
        ax.plot(sub["triangulations"], sub["mean_time_per_tri"], "-o", color=colors[cat],
                label=cat, markersize=5, linewidth=1.4, alpha=0.9,
                markeredgecolor="black", markeredgewidth=0.3)
        ax.fill_between(sub["triangulations"],
                         sub["mean_time_per_tri"] - sub["std_time_per_tri"].fillna(0),
                         sub["mean_time_per_tri"] + sub["std_time_per_tri"].fillna(0),
                         color=colors[cat], alpha=0.12)

    ax.set_xscale("log")
    valid = summary[summary["mean_time_per_tri"] > 0]
    if not valid.empty:
        ax.set_yscale("log")
    ax.set_xlabel("Total triangulations generated (log scale)")
    ax.set_ylabel("Mean time per triangulation (s)")
    ax.set_title("Amortized Cost per Triangulation vs Problem Size\n(flat line supports amortized O(1) claim)")
    ax.legend(fontsize=8, ncol=2, loc="best")
    savefig(fig, out_dir, "05_amortized_time_per_triangulation_vs_n")


def plot_time_per_triangulation_boxplot(summary: pd.DataFrame, out_dir: str):
    """Box plot of per-category distribution of time-per-triangulation —
    tight, low-variance boxes across categories support the constant-time
    claim regardless of category/structure."""
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)
    data = [summary[summary["category"] == c]["mean_time_per_tri"].dropna().values for c in categories]

    fig, ax = plt.subplots(figsize=(max(8, 0.6 * len(categories)), 6))
    bp = ax.boxplot(data, labels=categories, patch_artist=True, showfliers=True)
    for patch, cat in zip(bp["boxes"], categories):
        patch.set_facecolor(colors[cat])
        patch.set_alpha(0.7)
    ax.set_yscale("log")
    ax.set_ylabel("Time per triangulation (s, log scale)")
    ax.set_title("Distribution of Amortized Per-Triangulation Cost by Category")
    plt.setp(ax.get_xticklabels(), rotation=45, ha="right")
    savefig(fig, out_dir, "06_time_per_triangulation_boxplot_by_category")


def plot_triangulations_per_second(summary: pd.DataFrame, out_dir: str):
    """Throughput view: triangulations/sec should be roughly stable/increasing
    (not decaying) as n grows -> corroborates constant per-item cost."""
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)

    fig, ax = plt.subplots(figsize=(9, 6.5))
    for cat in categories:
        sub = summary[summary["category"] == cat]
        sub = sub[(sub["triangulations"] > 0) & (sub["mean_tri_per_sec"].notna())]
        sub = sort_cases_by_triangulations(sub)
        if sub.empty:
            continue
        ax.plot(sub["triangulations"], sub["mean_tri_per_sec"], "-o", color=colors[cat],
                label=cat, markersize=5, linewidth=1.4, markeredgecolor="black", markeredgewidth=0.3)

    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("Total triangulations generated (log scale)")
    ax.set_ylabel("Throughput: triangulations / second")
    ax.set_title("Enumeration Throughput vs Problem Size\n(flat/rising curve supports amortized O(1) claim)")
    ax.legend(fontsize=8, ncol=2, loc="best")
    savefig(fig, out_dir, "07_throughput_triangulations_per_second")


def plot_linear_fit_residuals(summary: pd.DataFrame, out_dir: str):
    """Fit time = a * triangulations + b globally (should be near-linear, i.e.
    a straight line through the origin in linear-linear space if truly
    amortized constant time), then plot residuals to spot systematic
    deviation (e.g. superlinear tails)."""
    valid = summary[(summary["triangulations"] > 0) & (summary["mean_time"] > 0)].copy()
    if len(valid) < 2:
        return
    x = valid["triangulations"].values
    y = valid["mean_time"].values

    # Linear fit forced through origin: y = a*x  (least squares)
    a = np.sum(x * y) / np.sum(x * x)
    pred = a * x
    resid = y - pred
    valid["residual"] = resid
    valid["pred"] = pred

    categories = sorted(valid["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)

    fig, axes = plt.subplots(1, 2, figsize=(14, 6))

    ax = axes[0]
    for cat in categories:
        sub = valid[valid["category"] == cat]
        ax.scatter(sub["triangulations"], sub["mean_time"], color=colors[cat], label=cat,
                   s=40, edgecolor="black", linewidth=0.3, alpha=0.85)
    xs = np.linspace(0, x.max(), 200)
    ax.plot(xs, a * xs, "k--", lw=2, label=f"Fit: t = {a:.3e} · n")
    ax.set_xlabel("Total triangulations (linear scale)")
    ax.set_ylabel("Total time (s)")
    ax.set_title("Linear-Scale Fit: t = a·n (through origin)")
    ax.legend(fontsize=7, ncol=2)

    ax = axes[1]
    for cat in categories:
        sub = valid[valid["category"] == cat]
        ax.scatter(sub["triangulations"], sub["residual"], color=colors[cat], label=cat,
                   s=40, edgecolor="black", linewidth=0.3, alpha=0.85)
    ax.axhline(0, color="black", lw=1.5, ls="--")
    ax.set_xscale("log")
    ax.set_xlabel("Total triangulations (log scale)")
    ax.set_ylabel("Residual: actual − predicted time (s)")
    ax.set_title("Residuals from Linear Fit\n(scattered around 0 supports amortized O(1))")
    savefig(fig, out_dir, "08_linear_fit_and_residuals")


# ============================================================================
# PLOT 4: Vertices-based scaling (structural complexity view)
# ============================================================================

def plot_time_vs_vertices(summary: pd.DataFrame, out_dir: str):
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)

    fig, ax = plt.subplots(figsize=(9, 6.5))
    for cat in categories:
        sub = summary[summary["category"] == cat]
        sub = sub[(sub["vertices"] > 0) & (sub["mean_time"] > 0)]
        sub = sub.sort_values("vertices")
        if sub.empty:
            continue
        ax.plot(sub["vertices"], sub["mean_time"], "-o", color=colors[cat], label=cat,
                markersize=5, linewidth=1.4, markeredgecolor="black", markeredgewidth=0.3)
    ax.set_yscale("log")
    ax.set_xlabel("Distinct vertices")
    ax.set_ylabel("Mean time (s, log scale)")
    ax.set_title("Runtime vs Input Size (Vertex Count)")
    ax.legend(fontsize=8, ncol=2)
    savefig(fig, out_dir, "09_time_vs_vertices")


def plot_triangulations_vs_vertices(summary: pd.DataFrame, out_dir: str):
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)

    fig, ax = plt.subplots(figsize=(9, 6.5))
    for cat in categories:
        sub = summary[summary["category"] == cat]
        sub = sub[(sub["vertices"] > 0) & (sub["triangulations"] > 0)]
        sub = sub.sort_values("vertices")
        if sub.empty:
            continue
        ax.plot(sub["vertices"], sub["triangulations"], "-o", color=colors[cat], label=cat,
                markersize=5, linewidth=1.4, markeredgecolor="black", markeredgewidth=0.3)
    ax.set_yscale("log")
    ax.set_xlabel("Distinct vertices")
    ax.set_ylabel("Total triangulations (log scale)")
    ax.set_title("Combinatorial Explosion: Triangulation Count vs Vertex Count")
    ax.legend(fontsize=8, ncol=2)
    savefig(fig, out_dir, "10_triangulations_vs_vertices")


# ============================================================================
# PLOT 5: Memory-side sanity checks
# ============================================================================

def plot_memory_vs_triangulations(summary: pd.DataFrame, out_dir: str):
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)

    fig, ax = plt.subplots(figsize=(9, 6.5))
    plotted = False
    for cat in categories:
        sub = summary[summary["category"] == cat]
        sub = sub[(sub["triangulations"] > 0) & (sub["mean_mem"].notna()) & (sub["mean_mem"] > 0)]
        if sub.empty:
            continue
        plotted = True
        ax.scatter(sub["triangulations"], sub["mean_mem"] / (1024 * 1024), color=colors[cat],
                  label=cat, s=45, edgecolor="black", linewidth=0.4, alpha=0.85)
    if not plotted:
        plt.close(fig)
        print("  (skipped memory-vs-triangulations plot: no positive memory data)")
        return
    ax.set_xscale("log")
    ax.set_xlabel("Total triangulations (log scale)")
    ax.set_ylabel("Peak memory (MB)")
    ax.set_title("Peak Memory vs Triangulation Count\n(flat = memory is O(1)/streaming, not accumulating all triangulations)")
    ax.legend(fontsize=8, ncol=2)
    savefig(fig, out_dir, "11_memory_vs_triangulations")


def plot_memory_per_vertex(summary: pd.DataFrame, out_dir: str):
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)
    data = [summary[summary["category"] == c]["mean_mem_per_vertex"].dropna() for c in categories]
    if all(len(d) == 0 for d in data):
        print("  (skipped memory-per-vertex plot: no data)")
        return
    fig, ax = plt.subplots(figsize=(max(8, 0.6 * len(categories)), 6))
    bp = ax.boxplot(data, labels=categories, patch_artist=True, showfliers=True)
    for patch, cat in zip(bp["boxes"], categories):
        patch.set_facecolor(colors[cat])
        patch.set_alpha(0.7)
    ax.set_ylabel("Memory per vertex (bytes)")
    ax.set_title("Memory-per-Vertex Distribution by Category")
    plt.setp(ax.get_xticklabels(), rotation=45, ha="right")
    savefig(fig, out_dir, "12_memory_per_vertex_boxplot")


# ============================================================================
# PLOT 6: Variance / reproducibility across the RUNS_PER_CASE repeats
# ============================================================================

def plot_run_to_run_variance(df: pd.DataFrame, summary: pd.DataFrame, out_dir: str):
    """Coefficient of variation (std/mean) of timeSeconds across repeated
    runs of the same case, to demonstrate measurement stability."""
    cov = summary.copy()
    cov["cv"] = cov["std_time"] / cov["mean_time"]
    cov = cov[cov["mean_time"] > 0]
    categories = sorted(cov["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)

    fig, ax = plt.subplots(figsize=(9, 6.5))
    for cat in categories:
        sub = cov[cov["category"] == cat]
        ax.scatter(sub["triangulations"], sub["cv"] * 100, color=colors[cat], label=cat,
                  s=40, edgecolor="black", linewidth=0.3, alpha=0.85)
    ax.set_xscale("log")
    ax.set_xlabel("Total triangulations (log scale)")
    ax.set_ylabel("Coefficient of variation of runtime (%)")
    ax.set_title(f"Run-to-Run Timing Variability Across Repeats")
    ax.axhline(5, color="gray", ls=":", lw=1.5, label="5% CoV reference")
    ax.legend(fontsize=8, ncol=2)
    savefig(fig, out_dir, "13_run_to_run_variance")


def plot_individual_runs_spread(df: pd.DataFrame, out_dir: str):
    """Show every individual run (not just the mean) as a strip/jitter plot
    per category, to visualize the raw spread the CSV actually contains."""
    categories = sorted(df["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)

    fig, ax = plt.subplots(figsize=(max(9, 0.6 * len(categories)), 6.5))
    rng = np.random.default_rng(42)
    for i, cat in enumerate(categories):
        sub = df[(df["category"] == cat) & (df["timeSeconds"] > 0)]
        if sub.empty:
            continue
        jitter = rng.uniform(-0.3, 0.3, size=len(sub))
        ax.scatter(np.full(len(sub), i) + jitter, sub["timeSeconds"], color=colors[cat],
                  s=18, alpha=0.6, edgecolor="none")
    ax.set_yscale("log")
    ax.set_xticks(range(len(categories)))
    ax.set_xticklabels(categories, rotation=45, ha="right")
    ax.set_ylabel("Individual run time (s, log scale)")
    ax.set_title("All Individual Timed Runs by Category (jittered strip plot)")
    savefig(fig, out_dir, "14_individual_runs_strip_plot")


# ============================================================================
# PLOT 7: Heatmap-style overview
# ============================================================================

def plot_heatmap_time_per_triangulation(summary: pd.DataFrame, out_dir: str):
    """Heatmap: rows = categories, cols = case rank (ordered by increasing
    triangulation count within category), color = time-per-triangulation.
    A uniformly colored heatmap (no dark/light gradient across columns)
    supports the constant-time-per-triangulation claim visually."""
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    max_cases = summary.groupby("category").size().max()

    mat = np.full((len(categories), max_cases), np.nan)
    for i, cat in enumerate(categories):
        sub = sort_cases_by_triangulations(summary[summary["category"] == cat])
        vals = sub["mean_time_per_tri"].values
        mat[i, :len(vals)] = vals

    fig, ax = plt.subplots(figsize=(max(8, 0.5 * max_cases), max(5, 0.4 * len(categories))))
    # log-scale color mapping
    logmat = np.log10(np.where(mat > 0, mat, np.nan))
    im = ax.imshow(logmat, aspect="auto", cmap="viridis")
    ax.set_yticks(range(len(categories)))
    ax.set_yticklabels(categories)
    ax.set_xlabel("Case rank within category (ordered by ↑ triangulation count)")
    ax.set_title("Heatmap: log10(Time per Triangulation)\n(uniform color per row supports amortized O(1) claim)")
    cbar = fig.colorbar(im, ax=ax)
    cbar.set_label("log10(seconds / triangulation)")
    savefig(fig, out_dir, "15_heatmap_time_per_triangulation")


# ============================================================================
# PLOT 8: Cumulative / speed-over-time style views
# ============================================================================

def plot_cumulative_triangulations_vs_time(summary: pd.DataFrame, out_dir: str):
    """For each category, assuming constant rate, plot the theoretical
    'triangulations enumerated vs time elapsed' curve (linear if truly
    O(1) amortized) using the fitted per-triangulation rate for each case,
    scaled to a common time axis — an intuitive illustrative chart."""
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)

    fig, ax = plt.subplots(figsize=(9, 6.5))
    for cat in categories:
        sub = summary[summary["category"] == cat]
        sub = sub[(sub["mean_tri_per_sec"] > 0)]
        if sub.empty:
            continue
        # pick the case with the largest triangulation count as representative
        row = sub.loc[sub["triangulations"].idxmax()]
        rate = row["mean_tri_per_sec"]
        t = np.linspace(0, row["mean_time"], 200)
        cum_tri = rate * t
        ax.plot(t, cum_tri, color=colors[cat], label=f"{cat} ({row['case']})", linewidth=1.8)

    ax.set_xlabel("Time elapsed (s)")
    ax.set_ylabel("Triangulations enumerated (theoretical, at fitted constant rate)")
    ax.set_title("Illustrative Constant-Rate Enumeration Curves\n(largest case per category, rate = mean throughput)")
    ax.legend(fontsize=8, ncol=2)
    savefig(fig, out_dir, "16_cumulative_constant_rate_illustration")


def plot_time_share_stacked(summary: pd.DataFrame, out_dir: str):
    """Stacked bar: for each category, share of total wall-clock time
    consumed by each case (largest cases should dominate proportionally
    to their triangulation count if cost truly scales linearly)."""
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)

    fig, ax = plt.subplots(figsize=(max(8, 0.6 * len(categories)), 6.5))
    bottoms = np.zeros(len(categories))
    max_cases = summary.groupby("category").size().max()
    shade_cmap = plt.get_cmap("Greys")

    for rank in range(max_cases):
        heights = []
        for cat in categories:
            sub = sort_cases_by_triangulations(summary[summary["category"] == cat]).reset_index(drop=True)
            total = sub["mean_time"].sum()
            if rank < len(sub) and total > 0:
                heights.append(sub.loc[rank, "mean_time"] / total * 100)
            else:
                heights.append(0)
        heights = np.array(heights)
        shade = 0.3 + 0.5 * (rank / max(max_cases - 1, 1))
        ax.bar(categories, heights, bottom=bottoms, color=shade_cmap(shade), edgecolor="white", linewidth=0.4)
        bottoms += heights

    ax.set_ylabel("% share of category's total runtime")
    ax.set_title("Runtime Share by Case Rank within Category\n(darker = higher triangulation-count case)")
    plt.setp(ax.get_xticklabels(), rotation=45, ha="right")
    savefig(fig, out_dir, "17_runtime_share_stacked_bars")


# ============================================================================
# PLOT 9: Scaling exponent per category (local slope estimate)
# ============================================================================

def plot_scaling_exponent_per_category(summary: pd.DataFrame, out_dir: str):
    """For each category with >=2 cases, fit log(time) = k*log(n) + c and
    report the exponent k. k≈1 supports amortized O(1) per triangulation
    (i.e. total time scales linearly with output size)."""
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)

    exps, errs, cats_used = [], [], []
    for cat in categories:
        sub = summary[summary["category"] == cat]
        sub = sub[(sub["triangulations"] > 0) & (sub["mean_time"] > 0)]
        if len(sub) < 2:
            continue
        logx = np.log10(sub["triangulations"].values)
        logy = np.log10(sub["mean_time"].values)
        if np.ptp(logx) < 1e-9:
            continue
        slope, intercept = np.polyfit(logx, logy, 1)
        resid = logy - (slope * logx + intercept)
        se = np.std(resid) / (np.sqrt(len(logx)) * (np.std(logx) + 1e-12))
        exps.append(slope)
        errs.append(se)
        cats_used.append(cat)

    if not cats_used:
        print("  (skipped scaling-exponent plot: not enough multi-case categories)")
        return

    fig, ax = plt.subplots(figsize=(max(8, 0.5 * len(cats_used)), 6))
    xs = np.arange(len(cats_used))
    bar_colors = [colors[c] for c in cats_used]
    ax.bar(xs, exps, yerr=errs, capsize=4, color=bar_colors, edgecolor="black", linewidth=0.5)
    ax.axhline(1.0, color="red", ls="--", lw=2, label="Slope = 1 (amortized O(1) per triangulation)")
    ax.set_xticks(xs)
    ax.set_xticklabels(cats_used, rotation=45, ha="right")
    ax.set_ylabel("Fitted log-log slope (scaling exponent)")
    ax.set_title("Per-Category Scaling Exponent: log(time) vs log(triangulations)")
    ax.legend()
    savefig(fig, out_dir, "18_scaling_exponent_per_category")


# ============================================================================
# PLOT 10: Overall distribution summaries
# ============================================================================

def plot_histogram_time_per_triangulation(summary: pd.DataFrame, out_dir: str):
    vals = summary["mean_time_per_tri"].replace([np.inf, -np.inf], np.nan).dropna()
    vals = vals[vals > 0]
    if vals.empty:
        print("  (skipped histogram: no positive time-per-triangulation values)")
        return
    fig, ax = plt.subplots(figsize=(8, 6))
    ax.hist(np.log10(vals), bins=30, color="steelblue", edgecolor="black", alpha=0.85)
    ax.set_xlabel("log10(time per triangulation) [s]")
    ax.set_ylabel("Number of test cases")
    ax.set_title("Distribution of Amortized Per-Triangulation Cost\n(across all categories & cases)")
    mean_log = np.log10(vals).mean()
    ax.axvline(mean_log, color="red", ls="--", lw=2, label=f"mean = {10**mean_log:.3e} s")
    ax.legend()
    savefig(fig, out_dir, "19_histogram_time_per_triangulation")


def plot_summary_table_figure(summary: pd.DataFrame, out_dir: str):
    """Render a compact numeric summary table as a figure for quick reference."""
    agg = summary.groupby("category").agg(
        n_cases=("case", "count"),
        min_tri=("triangulations", "min"),
        max_tri=("triangulations", "max"),
        mean_time_per_tri=("mean_time_per_tri", "mean"),
        std_time_per_tri=("mean_time_per_tri", "std"),
        total_time=("mean_time", "sum"),
    ).reset_index()
    agg["min_tri"] = agg["min_tri"].map(lambda v: f"{v:,.0f}" if pd.notna(v) else "-")
    agg["max_tri"] = agg["max_tri"].map(lambda v: f"{v:,.0f}" if pd.notna(v) else "-")
    agg["mean_time_per_tri"] = agg["mean_time_per_tri"].map(lambda v: f"{v:.3e}" if pd.notna(v) else "-")
    agg["std_time_per_tri"] = agg["std_time_per_tri"].map(lambda v: f"{v:.3e}" if pd.notna(v) else "-")
    agg["total_time"] = agg["total_time"].map(lambda v: f"{v:.2f}")

    fig, ax = plt.subplots(figsize=(12, 0.5 * len(agg) + 1.5))
    ax.axis("off")
    tbl = ax.table(cellText=agg.values, colLabels=agg.columns, loc="center", cellLoc="center")
    tbl.auto_set_font_size(False)
    tbl.set_fontsize(9)
    tbl.scale(1, 1.6)
    ax.set_title("Per-Category Summary Statistics", pad=20)
    savefig(fig, out_dir, "20_summary_table")


# ============================================================================
# Main
# ============================================================================

def main():
    ap = argparse.ArgumentParser(description="Generate research-grade plots from triangulation benchmark CSVs.")
    ap.add_argument("--input-dir", default=".", help="Directory containing results_*.csv files")
    ap.add_argument("--out-dir", default="plots", help="Directory to write figures into")
    args = ap.parse_args()

    os.makedirs(args.out_dir, exist_ok=True)

    print(f"[INFO] Loading CSVs from: {args.input_dir}")
    df = load_all_csvs(args.input_dir)
    print(f"[INFO] Loaded {len(df)} run rows across {df['category'].nunique()} categories, "
          f"{df['case'].nunique()} unique cases.")

    summary = per_case_summary(df)
    summary.to_csv(os.path.join(args.out_dir, "per_case_summary.csv"), index=False)
    print(f"[INFO] Wrote per-case summary -> {os.path.join(args.out_dir, 'per_case_summary.csv')}")

    print("\n[INFO] Generating plots...")
    plot_avg_time_per_category(summary, args.out_dir)
    plot_avg_time_all_categories_together(summary, args.out_dir)
    plot_loglog_scatter(summary, args.out_dir)
    plot_loglog_lines_per_category(summary, args.out_dir)
    plot_time_per_triangulation_vs_n(summary, args.out_dir)
    plot_time_per_triangulation_boxplot(summary, args.out_dir)
    plot_triangulations_per_second(summary, args.out_dir)
    plot_linear_fit_residuals(summary, args.out_dir)
    plot_time_vs_vertices(summary, args.out_dir)
    plot_triangulations_vs_vertices(summary, args.out_dir)
    plot_memory_vs_triangulations(summary, args.out_dir)
    plot_memory_per_vertex(summary, args.out_dir)
    plot_run_to_run_variance(df, summary, args.out_dir)
    plot_individual_runs_spread(df, args.out_dir)
    plot_heatmap_time_per_triangulation(summary, args.out_dir)
    plot_cumulative_triangulations_vs_time(summary, args.out_dir)
    plot_time_share_stacked(summary, args.out_dir)
    plot_scaling_exponent_per_category(summary, args.out_dir)
    plot_histogram_time_per_triangulation(summary, args.out_dir)
    plot_summary_table_figure(summary, args.out_dir)

    print(f"\n[DONE] All figures written to: {os.path.abspath(args.out_dir)}")


if __name__ == "__main__":
    main()