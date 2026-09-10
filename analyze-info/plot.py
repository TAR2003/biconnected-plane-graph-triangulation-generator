#!/usr/bin/env python3
"""
analyze_benchmarks.py
======================

Research-grade analysis & plotting suite for the triangulation-enumeration
benchmark CSVs produced by the C++ benchmark harness (results_<category>.csv).

Designed to work with a directory layout like:

    analyze-info/
      results-with-vgs/
        results_case_group_a.csv
        results_case_group_b.csv
        ...
      results-without-vgs/
        results_case_group_a.csv
        ...

i.e. one or more "dataset" folders (e.g. an ablation: with vs without some
optimization), each containing one CSV per category. Each CSV holds
multiple repeated runs per test case:

    filename,runIndex,vertices,triangulations,timeSeconds,peakMemoryBytes,
    memoryPerVertex,startTime,endTime,status

`status` is either "completed" or "time_limit_exceeded" (or the older
"limit_exceeded" / "error" strings are also tolerated). For time-limited
runs, `timeSeconds` is the fixed wall-clock cutoff (e.g. 360s) and
`triangulations` is the partial count reached by that point -- this is
still a fully valid, independent sample of the achieved
triangulations-per-second rate, and is treated as such (it is NOT dropped;
it is one of the strongest data points for an amortized-O(1) claim because
it comes from a fixed time budget rather than a fixed output size).

ALL summary statistics use the MEDIAN (with MAD / IQR-based spread) across
the repeated runs of a case, not the mean, as requested.

Output layout
-------------
For each dataset subfolder found under --input-dir, a sibling folder
`plots_<dataset-name>/` is created next to --out-dir containing that
dataset's own 20+ figures. A further `plots_comparison/` folder is created
with every figure re-drawn with datasets overlaid/faceted so they can be
compared directly (e.g. with-vgs vs without-vgs).

If --input-dir contains CSVs directly (no subfolders), it is treated as a
single dataset called "results".

Usage
-----
    python3 analyze_benchmarks.py --input-dir /path/to/analyze-info --out-dir plots
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

STATUS_OK = {"completed", "ok", "success"}
STATUS_LIMIT = {"time_limit_exceeded", "limit_exceeded", "triangulation_limit_exceeded"}


# ============================================================================
# Loading & cleaning
# ============================================================================

def natural_case_key(name: str):
    """Sort helper: case_1_2 < case_1_10 (numeric-aware), not lexicographic."""
    parts = re.split(r"(\d+)", str(name))
    return [int(p) if p.isdigit() else p for p in parts]


def discover_datasets(input_dir: str):
    """Return dict: dataset_name -> list of csv paths.
    If input_dir has subfolders containing CSVs, each subfolder is a dataset.
    Otherwise, CSVs directly in input_dir form a single dataset."""
    input_dir = os.path.abspath(input_dir)
    datasets = {}

    subdirs = [d for d in sorted(os.listdir(input_dir))
               if os.path.isdir(os.path.join(input_dir, d))]

    for d in subdirs:
        full = os.path.join(input_dir, d)
        csvs = sorted(glob.glob(os.path.join(full, "results_*.csv")))
        if not csvs:
            csvs = sorted(glob.glob(os.path.join(full, "*.csv")))
        if csvs:
            datasets[d] = csvs

    if not datasets:
        csvs = sorted(glob.glob(os.path.join(input_dir, "results_*.csv")))
        if not csvs:
            csvs = sorted(glob.glob(os.path.join(input_dir, "*.csv")))
        if csvs:
            datasets["results"] = csvs

    return datasets


def to_big_float(x):
    """Safe conversion for numbers that may exceed 64-bit range (__int128)."""
    try:
        return float(int(str(x).strip()))
    except Exception:
        try:
            return float(x)
        except Exception:
            return np.nan


def load_dataset(csv_paths, dataset_name: str) -> pd.DataFrame:
    frames = []
    for p in csv_paths:
        try:
            df = pd.read_csv(p, dtype=str)
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
        return pd.DataFrame()

    df = pd.concat(frames, ignore_index=True)
    return clean(df, dataset_name)


def clean(df: pd.DataFrame, dataset_name: str) -> pd.DataFrame:
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
    df["status"] = df["status"].fillna("completed").str.strip().str.lower()

    def norm_status(s):
        if s in STATUS_OK:
            return "completed"
        if s in STATUS_LIMIT:
            return "time_limit_exceeded"
        return s  # keep 'error' or anything else as-is

    df["status_norm"] = df["status"].apply(norm_status)

    if "filename" not in df.columns:
        df["filename"] = df.get("source_file", "unknown")

    # Drop hard errors (no usable measurement), but KEEP time_limit_exceeded rows
    df = df[df["status_norm"] != "error"]

    with np.errstate(divide="ignore", invalid="ignore"):
        df["time_per_triangulation"] = df["timeSeconds"] / df["triangulations_f"]
        df["triangulations_per_sec"] = df["triangulations_f"] / df["timeSeconds"]

    df["time_per_triangulation"] = df["time_per_triangulation"].replace([np.inf, -np.inf], np.nan)
    df["triangulations_per_sec"] = df["triangulations_per_sec"].replace([np.inf, -np.inf], np.nan)

    before = len(df)
    df = df[(df["timeSeconds"].notna()) & (df["timeSeconds"] >= 0) & (df["triangulations_f"] > 0)]
    after = len(df)
    if after < before:
        print(f"[INFO] [{dataset_name}] Dropped {before - after} rows with invalid/missing time or triangulations.")

    df["case"] = df["filename"].astype(str)
    df["category"] = df["category"].astype(str)
    df["dataset"] = dataset_name

    return df.reset_index(drop=True)


# ============================================================================
# Helpers
# ============================================================================

def mad(x):
    """Median absolute deviation (scaled to be comparable to std for normal
    data: *1.4826), robust spread estimator paired with the median."""
    x = np.asarray(x, dtype=float)
    x = x[~np.isnan(x)]
    if len(x) == 0:
        return np.nan
    m = np.median(x)
    return 1.4826 * np.median(np.abs(x - m))


def category_color_map(categories):
    cats = sorted(set(categories), key=natural_case_key)
    n = max(len(cats), 1)
    cmap = plt.get_cmap("turbo", n) if n > 12 else plt.get_cmap("tab10", max(n, 1))
    colors = {}
    for i, c in enumerate(cats):
        colors[c] = cmap(i / max(n - 1, 1)) if n > 1 else cmap(0)
    return colors


def dataset_color_map(datasets):
    cats = sorted(set(datasets))
    n = max(len(cats), 1)
    cmap = plt.get_cmap("Set1", max(n, 1))
    return {d: cmap(i / max(n - 1, 1)) if n > 1 else cmap(0) for i, d in enumerate(cats)}


def savefig(fig, out_dir, name):
    png = os.path.join(out_dir, f"{name}.png")
    os.makedirs(os.path.dirname(png), exist_ok=True)
    fig.tight_layout()
    fig.savefig(png, bbox_inches="tight")
    plt.close(fig)
    print(f"  -> {png}")


def sort_cases_by_triangulations(sub: pd.DataFrame) -> pd.DataFrame:
    return sub.sort_values("triangulations", kind="stable")


def per_case_summary(df: pd.DataFrame) -> pd.DataFrame:
    """One row per (dataset, category, filename): MEDIAN (+MAD) across the
    repeated runs, plus min/max and the fraction of runs that hit the
    time limit for that case."""
    if df.empty:
        return df

    def agg_block(g):
        time_limit_frac = (g["status_norm"] == "time_limit_exceeded").mean()
        return pd.Series({
            "vertices": g["vertices"].max(),
            "triangulations": g["triangulations_f"].median(),
            "median_time": g["timeSeconds"].median(),
            "mad_time": mad(g["timeSeconds"]),
            "min_time": g["timeSeconds"].min(),
            "max_time": g["timeSeconds"].max(),
            "n_runs": g["timeSeconds"].count(),
            "median_time_per_tri": g["time_per_triangulation"].median(),
            "mad_time_per_tri": mad(g["time_per_triangulation"]),
            "median_tri_per_sec": g["triangulations_per_sec"].median(),
            "mad_tri_per_sec": mad(g["triangulations_per_sec"]),
            # Requested metric: NOT median-of-per-run-ratios, but
            # (total/median triangulations) / (median total time) for the case.
            # i.e. "median average time per triangulation" = total_triangulations / median_time.
            "median_avg_time_per_tri": (
                g["timeSeconds"].median() / g["triangulations_f"].median()
                if g["triangulations_f"].median() > 0 else np.nan
            ),
            "median_mem": g["peakMemoryBytes"].replace(0, np.nan).median(),
            "median_mem_per_vertex": g["memoryPerVertex"].replace(0, np.nan).median(),
            "time_limit_frac": time_limit_frac,
            "any_time_limit": time_limit_frac > 0,
        })

    summary = df.groupby(["dataset", "category", "case"], as_index=False).apply(
        lambda g: agg_block(g)
    )
    if isinstance(summary.columns, pd.MultiIndex):
        summary.columns = [c[-1] if c[-1] else c[0] for c in summary.columns]
    summary = summary.reset_index(drop=True)
    summary = summary.sort_values(["dataset", "category", "triangulations"], kind="stable")
    return summary.reset_index(drop=True)


# ============================================================================
# PER-DATASET PLOTS
# (each function takes a single dataset's df + summary and an out_dir)
# ============================================================================

def plot_01_median_time_per_category(summary, out_dir):
    """Median AVERAGE time per triangulation (= total triangulations / median
    total time), per category, one case per bar, cases spaced widely enough
    that bars never sit on top of each other (each bar gets its own x slot,
    with generous per-category width so labels don't collide either)."""
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)
    n_cat = len(categories)
    max_cases = summary.groupby("category").size().max()
    # Give each bar real width: ~0.6in per case, minimum figure width 10in.
    fig, axes = plt.subplots(n_cat, 1, figsize=(max(10, 0.6 * max_cases), 3.4 * n_cat), squeeze=False)
    for i, cat in enumerate(categories):
        ax = axes[i, 0]
        sub = sort_cases_by_triangulations(summary[summary["category"] == cat])
        sub = sub[sub["median_avg_time_per_tri"].notna()]
        # Bars are placed on integer ticks with a fixed gap (width<1) so that
        # even with many cases, adjacent bars never touch/overlap.
        x = np.arange(len(sub))
        bar_colors = [colors[cat] if not lim else "#888888" for lim in sub["any_time_limit"]]
        ax.bar(x, sub["median_avg_time_per_tri"], width=0.65,
               color=bar_colors, edgecolor="black", linewidth=0.5)
        ax.set_xticks(x)
        labels = [f"{c}\n(n≈{int(t):,}{'*' if lim else ''})"
                  for c, t, lim in zip(sub["case"], sub["triangulations"], sub["any_time_limit"])]
        ax.set_xticklabels(labels, rotation=45, ha="right", fontsize=7)
        ax.set_xlim(-0.6, len(sub) - 0.4)
        ax.set_ylabel("Median avg time / triangulation (s)")
        ax.set_title(f"Category: {cat} — median average time per triangulation per case\n"
                      f"(ordered by ↑ triangulation count; gray bars / * = time limit hit)")
        ax.set_yscale("log")
    savefig(fig, out_dir, "01_median_avg_time_per_triangulation_per_category")


def plot_02_median_time_all_categories_combined(summary, out_dir):
    """Same metric as plot_01, all categories combined into one wide figure.
    Bars are given real width plus a gap between categories, and the figure
    scales with case count so bars never overlap regardless of how many
    categories/cases exist."""
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)
    n_total = sum((summary["category"] == c).sum() for c in categories)
    fig, ax = plt.subplots(figsize=(max(12, 0.45 * n_total), 6.5))
    x = 0
    xticks, xlabels = [], []
    for cat in categories:
        sub = sort_cases_by_triangulations(summary[summary["category"] == cat])
        sub = sub[sub["median_avg_time_per_tri"].notna()]
        xs = np.arange(x, x + len(sub))
        bar_colors = [colors[cat] if not lim else "#888888" for lim in sub["any_time_limit"]]
        ax.bar(xs, sub["median_avg_time_per_tri"], width=0.7,
               color=bar_colors, edgecolor="black", linewidth=0.3)
        xticks.extend(xs)
        xlabels.extend(sub["case"])
        x += len(sub) + 2  # gap between categories so groups are visually separated
    ax.set_xticks(xticks)
    ax.set_xticklabels(xlabels, rotation=90, fontsize=6)
    ax.set_yscale("log")
    ax.set_ylabel("Median avg time / triangulation (s, log scale)")
    ax.set_title("Median Average Time per Triangulation — all categories (color-coded), ↑ triangulations within each\n"
                  "(gray = time-limited run present)")
    handles = [Line2D([0], [0], color=colors[c], lw=6) for c in categories]
    ax.legend(handles, categories, ncol=min(6, len(categories)), loc="upper left", bbox_to_anchor=(0, 1.15))
    savefig(fig, out_dir, "02_median_avg_time_per_triangulation_all_categories_combined")


def plot_03_loglog_scatter(summary, out_dir):
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)
    fig, ax = plt.subplots(figsize=(9, 7))
    for cat in categories:
        sub = summary[summary["category"] == cat]
        sub_ok = sub[~sub["any_time_limit"]]
        sub_lim = sub[sub["any_time_limit"]]
        if not sub_ok.empty:
            ax.scatter(sub_ok["triangulations"], sub_ok["median_time"], label=cat, color=colors[cat],
                       s=45, edgecolor="black", linewidth=0.4, alpha=0.85, marker="o")
        if not sub_lim.empty:
            ax.scatter(sub_lim["triangulations"], sub_lim["median_time"], color=colors[cat],
                       s=70, edgecolor="black", linewidth=0.8, alpha=0.85, marker="^")

    valid = summary[(summary["triangulations"] > 0) & (summary["median_time"] > 0)]
    if len(valid) > 1:
        logx = np.log10(valid["triangulations"].values)
        logy = np.log10(valid["median_time"].values)
        slope, intercept = np.polyfit(logx, logy, 1)
        xs = np.linspace(logx.min(), logx.max(), 100)
        ax.plot(10**xs, 10**(slope * xs + intercept), "k--", lw=2, label=f"Global fit: slope={slope:.3f}")
        ref_intercept = logy[np.argmin(logx)] - 1.0 * logx[np.argmin(logx)]
        ax.plot(10**xs, 10**(1.0 * xs + ref_intercept), color="gray", ls=":", lw=2,
                label="Reference slope=1 (amortized O(1)/triangulation)")

    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("Total triangulations generated (log scale)")
    ax.set_ylabel("Median time (s, log scale)")
    ax.set_title("Log-Log: Triangulations vs Time (● completed, ▲ time-limit-hit)")
    ax.legend(fontsize=8, ncol=2, loc="upper left")
    savefig(fig, out_dir, "03_loglog_triangulations_vs_time")


def plot_04_loglog_lines_per_category(summary, out_dir):
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)
    fig, ax = plt.subplots(figsize=(9, 7))
    for cat in categories:
        sub = sort_cases_by_triangulations(summary[summary["category"] == cat])
        sub = sub[(sub["triangulations"] > 0) & (sub["median_time"] > 0)]
        if sub.empty:
            continue
        ax.plot(sub["triangulations"], sub["median_time"], "-", color=colors[cat], label=cat,
                linewidth=1.6, alpha=0.9)
        ok = sub[~sub["any_time_limit"]]
        lim = sub[sub["any_time_limit"]]
        ax.scatter(ok["triangulations"], ok["median_time"], color=colors[cat], s=40,
                  edgecolor="black", linewidth=0.3, marker="o", zorder=3)
        ax.scatter(lim["triangulations"], lim["median_time"], color=colors[cat], s=65,
                  edgecolor="black", linewidth=0.7, marker="^", zorder=3)
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("Total triangulations (log scale)")
    ax.set_ylabel("Median time (s, log scale)")
    ax.set_title("Log-Log Line Progression per Category (▲ = time-limit hit)")
    ax.legend(fontsize=8, ncol=2, loc="upper left")
    savefig(fig, out_dir, "04_loglog_line_progression_per_category")


def plot_05_amortized_time_per_triangulation(summary, out_dir):
    """Primary amortized-cost plot, now using the requested metric:
    median_avg_time_per_tri = total_triangulations / median_total_time
    (NOT the median of per-run ratios)."""
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)
    fig, ax = plt.subplots(figsize=(9, 6.5))
    for cat in categories:
        sub = sort_cases_by_triangulations(summary[summary["category"] == cat])
        sub = sub[(sub["triangulations"] > 0) & (sub["median_avg_time_per_tri"].notna())]
        if sub.empty:
            continue
        ax.plot(sub["triangulations"], sub["median_avg_time_per_tri"], "-", color=colors[cat],
                label=cat, linewidth=1.4, alpha=0.9)
        ok = sub[~sub["any_time_limit"]]
        lim = sub[sub["any_time_limit"]]
        ax.scatter(ok["triangulations"], ok["median_avg_time_per_tri"], color=colors[cat], s=40,
                  edgecolor="black", linewidth=0.3, marker="o", zorder=3)
        ax.scatter(lim["triangulations"], lim["median_avg_time_per_tri"], color=colors[cat], s=65,
                  edgecolor="black", linewidth=0.7, marker="^", zorder=3,
                  label=f"{cat} (time-limit sample)" if not lim.empty else None)

    ax.set_xscale("log")
    valid = summary[summary["median_avg_time_per_tri"] > 0]
    if not valid.empty:
        ax.set_yscale("log")
    ax.set_xlabel("Total triangulations generated (log scale)")
    ax.set_ylabel("Median average time per triangulation (s)")
    ax.set_title("Amortized Cost per Triangulation vs Problem Size\n"
                  "(median_avg = total triangulations / median total time; ▲ = time-limit-cutoff sample)")
    ax.legend(fontsize=8, ncol=2, loc="best")
    savefig(fig, out_dir, "05_amortized_time_per_triangulation_vs_n")


def plot_06_time_per_triangulation_boxplot(df, out_dir):
    """Boxplot built from the raw per-RUN time_per_triangulation values
    (not just the per-case median), giving a true distributional view."""
    categories = sorted(df["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)
    data = [df[df["category"] == c]["time_per_triangulation"].dropna().values for c in categories]
    data = [d[d > 0] for d in data]
    fig, ax = plt.subplots(figsize=(max(8, 0.6 * len(categories)), 6))
    bp = ax.boxplot(data, labels=categories, patch_artist=True, showfliers=True, medianprops=dict(color="black", linewidth=2))
    for patch, cat in zip(bp["boxes"], categories):
        patch.set_facecolor(colors[cat])
        patch.set_alpha(0.7)
    ax.set_yscale("log")
    ax.set_ylabel("Time per triangulation (s, log scale)")
    ax.set_title("Distribution of Per-Run Amortized Cost per Triangulation, by Category\n(median line shown in box)")
    plt.setp(ax.get_xticklabels(), rotation=45, ha="right")
    savefig(fig, out_dir, "06_time_per_triangulation_boxplot_by_category")


def plot_07_throughput(summary, out_dir):
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)
    fig, ax = plt.subplots(figsize=(9, 6.5))
    for cat in categories:
        sub = sort_cases_by_triangulations(summary[summary["category"] == cat])
        sub = sub[(sub["triangulations"] > 0) & (sub["median_tri_per_sec"].notna())]
        if sub.empty:
            continue
        ax.plot(sub["triangulations"], sub["median_tri_per_sec"], "-", color=colors[cat], label=cat, linewidth=1.4)
        ok = sub[~sub["any_time_limit"]]
        lim = sub[sub["any_time_limit"]]
        ax.scatter(ok["triangulations"], ok["median_tri_per_sec"], color=colors[cat], s=40,
                  edgecolor="black", linewidth=0.3, marker="o")
        ax.scatter(lim["triangulations"], lim["median_tri_per_sec"], color=colors[cat], s=65,
                  edgecolor="black", linewidth=0.7, marker="^")
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("Total triangulations generated (log scale)")
    ax.set_ylabel("Median throughput: triangulations / second")
    ax.set_title("Enumeration Throughput vs Problem Size\n(flat/rising = supports amortized O(1); ▲ = time-limit sample)")
    ax.legend(fontsize=8, ncol=2, loc="best")
    savefig(fig, out_dir, "07_throughput_triangulations_per_second")


def plot_08_linear_fit_residuals(summary, out_dir):
    valid = summary[(summary["triangulations"] > 0) & (summary["median_time"] > 0)].copy()
    if len(valid) < 2:
        return
    x = valid["triangulations"].values
    y = valid["median_time"].values
    a = np.sum(x * y) / np.sum(x * x)
    valid["pred"] = a * x
    valid["residual"] = y - valid["pred"]

    categories = sorted(valid["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)
    fig, axes = plt.subplots(1, 2, figsize=(14, 6))

    ax = axes[0]
    for cat in categories:
        sub = valid[valid["category"] == cat]
        ax.scatter(sub["triangulations"], sub["median_time"], color=colors[cat], label=cat,
                   s=40, edgecolor="black", linewidth=0.3, alpha=0.85)
    xs = np.linspace(0, x.max(), 200)
    ax.plot(xs, a * xs, "k--", lw=2, label=f"Fit: t = {a:.3e} · n")
    ax.set_xlabel("Total triangulations (linear scale)")
    ax.set_ylabel("Median time (s)")
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


def plot_09_time_vs_vertices(summary, out_dir):
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)
    fig, ax = plt.subplots(figsize=(9, 6.5))
    for cat in categories:
        sub = summary[summary["category"] == cat]
        sub = sub[(sub["vertices"] > 0) & (sub["median_time"] > 0)].sort_values("vertices")
        if sub.empty:
            continue
        ax.plot(sub["vertices"], sub["median_time"], "-o", color=colors[cat], label=cat,
                markersize=5, linewidth=1.4, markeredgecolor="black", markeredgewidth=0.3)
    ax.set_yscale("log")
    ax.set_xlabel("Distinct vertices")
    ax.set_ylabel("Median time (s, log scale)")
    ax.set_title("Runtime vs Input Size (Vertex Count)")
    ax.legend(fontsize=8, ncol=2)
    savefig(fig, out_dir, "09_time_vs_vertices")


def plot_10_triangulations_vs_vertices(summary, out_dir):
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)
    fig, ax = plt.subplots(figsize=(9, 6.5))
    for cat in categories:
        sub = summary[summary["category"] == cat]
        sub = sub[(sub["vertices"] > 0) & (sub["triangulations"] > 0)].sort_values("vertices")
        if sub.empty:
            continue
        ax.plot(sub["vertices"], sub["triangulations"], "-o", color=colors[cat], label=cat,
                markersize=5, linewidth=1.4, markeredgecolor="black", markeredgewidth=0.3)
    ax.set_yscale("log")
    ax.set_xlabel("Distinct vertices")
    ax.set_ylabel("Median triangulations (log scale)")
    ax.set_title("Combinatorial Explosion: Triangulation Count vs Vertex Count")
    ax.legend(fontsize=8, ncol=2)
    savefig(fig, out_dir, "10_triangulations_vs_vertices")


def plot_11_memory_vs_triangulations(summary, out_dir):
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)
    fig, ax = plt.subplots(figsize=(9, 6.5))
    plotted = False
    for cat in categories:
        sub = summary[summary["category"] == cat]
        sub = sub[(sub["triangulations"] > 0) & (sub["median_mem"].notna()) & (sub["median_mem"] > 0)]
        if sub.empty:
            continue
        plotted = True
        ax.scatter(sub["triangulations"], sub["median_mem"] / (1024 * 1024), color=colors[cat],
                  label=cat, s=45, edgecolor="black", linewidth=0.4, alpha=0.85)
    if not plotted:
        plt.close(fig)
        print("  (skipped memory-vs-triangulations: no usable memory data)")
        return
    ax.set_xscale("log")
    ax.set_xlabel("Total triangulations (log scale)")
    ax.set_ylabel("Median peak memory (MB)")
    ax.set_title("Peak Memory vs Triangulation Count\n(flat = O(1)/streaming memory, not accumulating output)")
    ax.legend(fontsize=8, ncol=2)
    savefig(fig, out_dir, "11_memory_vs_triangulations")


def plot_12_memory_per_vertex(summary, out_dir):
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)
    data = [summary[summary["category"] == c]["median_mem_per_vertex"].dropna() for c in categories]
    if all(len(d) == 0 for d in data):
        print("  (skipped memory-per-vertex: no data)")
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


def plot_13_run_to_run_variance(summary, out_dir):
    """Robust CoV analogue: MAD/median of timeSeconds across repeats."""
    cov = summary.copy()
    cov["rcv"] = cov["mad_time"] / cov["median_time"]
    cov = cov[cov["median_time"] > 0]
    categories = sorted(cov["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)
    fig, ax = plt.subplots(figsize=(9, 6.5))
    for cat in categories:
        sub = cov[cov["category"] == cat]
        ax.scatter(sub["triangulations"], sub["rcv"] * 100, color=colors[cat], label=cat,
                  s=40, edgecolor="black", linewidth=0.3, alpha=0.85)
    ax.set_xscale("log")
    ax.set_xlabel("Total triangulations (log scale)")
    ax.set_ylabel("Robust variability: MAD / median runtime (%)")
    ax.set_title("Run-to-Run Timing Variability Across Repeats (median-based)")
    ax.axhline(5, color="gray", ls=":", lw=1.5, label="5% reference")
    ax.legend(fontsize=8, ncol=2)
    savefig(fig, out_dir, "13_run_to_run_variance")


def plot_14_individual_runs_strip(df, out_dir):
    categories = sorted(df["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)
    fig, ax = plt.subplots(figsize=(max(9, 0.6 * len(categories)), 6.5))
    rng = np.random.default_rng(42)
    for i, cat in enumerate(categories):
        sub = df[(df["category"] == cat) & (df["timeSeconds"] > 0)]
        if sub.empty:
            continue
        jitter = rng.uniform(-0.3, 0.3, size=len(sub))
        marker_colors = np.where(sub["status_norm"] == "time_limit_exceeded", "#888888", "none")
        colors_arr = [colors[cat] if s != "time_limit_exceeded" else "#888888" for s in sub["status_norm"]]
        ax.scatter(np.full(len(sub), i) + jitter, sub["timeSeconds"], c=colors_arr, s=18, alpha=0.6, edgecolor="none")
    ax.set_yscale("log")
    ax.set_xticks(range(len(categories)))
    ax.set_xticklabels(categories, rotation=45, ha="right")
    ax.set_ylabel("Individual run time (s, log scale)")
    ax.set_title("All Individual Timed Runs by Category (gray = time-limit hit)")
    savefig(fig, out_dir, "14_individual_runs_strip_plot")


def plot_15_heatmap(summary, out_dir):
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    max_cases = summary.groupby("category").size().max()
    mat = np.full((len(categories), max_cases), np.nan)
    for i, cat in enumerate(categories):
        sub = sort_cases_by_triangulations(summary[summary["category"] == cat])
        vals = sub["median_avg_time_per_tri"].values
        mat[i, :len(vals)] = vals
    fig, ax = plt.subplots(figsize=(max(8, 0.5 * max_cases), max(5, 0.4 * len(categories))))
    logmat = np.log10(np.where(mat > 0, mat, np.nan))
    im = ax.imshow(logmat, aspect="auto", cmap="viridis")
    ax.set_yticks(range(len(categories)))
    ax.set_yticklabels(categories)
    ax.set_xlabel("Case rank within category (ordered by ↑ triangulation count)")
    ax.set_title("Heatmap: log10(Median Average Time per Triangulation)\n(uniform color per row supports amortized O(1) claim)")
    cbar = fig.colorbar(im, ax=ax)
    cbar.set_label("log10(seconds / triangulation)")
    savefig(fig, out_dir, "15_heatmap_time_per_triangulation")


def plot_16_cumulative_rate_illustration(summary, out_dir):
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)
    fig, ax = plt.subplots(figsize=(9, 6.5))
    for cat in categories:
        sub = summary[summary["category"] == cat]
        sub = sub[sub["median_tri_per_sec"] > 0]
        if sub.empty:
            continue
        row = sub.loc[sub["triangulations"].idxmax()]
        rate = row["median_tri_per_sec"]
        t = np.linspace(0, row["median_time"], 200)
        cum_tri = rate * t
        ax.plot(t, cum_tri, color=colors[cat], label=f"{cat} ({row['case']})", linewidth=1.8)
    ax.set_xlabel("Time elapsed (s)")
    ax.set_ylabel("Triangulations enumerated (theoretical, at fitted constant rate)")
    ax.set_title("Illustrative Constant-Rate Enumeration Curves\n(largest case per category, rate = median throughput)")
    ax.legend(fontsize=8, ncol=2)
    savefig(fig, out_dir, "16_cumulative_constant_rate_illustration")


def plot_17_runtime_share_stacked(summary, out_dir):
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    fig, ax = plt.subplots(figsize=(max(8, 0.6 * len(categories)), 6.5))
    bottoms = np.zeros(len(categories))
    max_cases = summary.groupby("category").size().max()
    shade_cmap = plt.get_cmap("Greys")
    for rank in range(max_cases):
        heights = []
        for cat in categories:
            sub = sort_cases_by_triangulations(summary[summary["category"] == cat]).reset_index(drop=True)
            total = sub["median_time"].sum()
            if rank < len(sub) and total > 0:
                heights.append(sub.loc[rank, "median_time"] / total * 100)
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


def plot_18_scaling_exponent(summary, out_dir):
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)
    exps, errs, cats_used = [], [], []
    for cat in categories:
        sub = summary[summary["category"] == cat]
        sub = sub[(sub["triangulations"] > 0) & (sub["median_time"] > 0)]
        if len(sub) < 2:
            continue
        logx = np.log10(sub["triangulations"].values)
        logy = np.log10(sub["median_time"].values)
        if np.ptp(logx) < 1e-9:
            continue
        slope, intercept = np.polyfit(logx, logy, 1)
        resid = logy - (slope * logx + intercept)
        se = np.std(resid) / (np.sqrt(len(logx)) * (np.std(logx) + 1e-12))
        exps.append(slope)
        errs.append(se)
        cats_used.append(cat)
    if not cats_used:
        print("  (skipped scaling-exponent: not enough multi-case categories)")
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


def plot_19_histogram(summary, out_dir):
    vals = summary["median_avg_time_per_tri"].replace([np.inf, -np.inf], np.nan).dropna()
    vals = vals[vals > 0]
    if vals.empty:
        print("  (skipped histogram: no positive values)")
        return
    fig, ax = plt.subplots(figsize=(8, 6))
    ax.hist(np.log10(vals), bins=30, color="steelblue", edgecolor="black", alpha=0.85)
    ax.set_xlabel("log10(median average time per triangulation) [s]")
    ax.set_ylabel("Number of test cases")
    ax.set_title("Distribution of Amortized Per-Triangulation Cost\n(median_avg = total triangulations / median total time; across all cases)")
    mean_log = np.log10(vals).mean()
    med_log = np.median(np.log10(vals))
    ax.axvline(med_log, color="red", ls="--", lw=2, label=f"median = {10**med_log:.3e} s")
    ax.legend()
    savefig(fig, out_dir, "19_histogram_time_per_triangulation")


def plot_20_summary_table(summary, out_dir):
    agg = summary.groupby("category").agg(
        n_cases=("case", "count"),
        min_tri=("triangulations", "min"),
        max_tri=("triangulations", "max"),
        median_avg_time_per_tri=("median_avg_time_per_tri", "median"),
        median_time_per_tri=("median_time_per_tri", "median"),
        total_time=("median_time", "sum"),
        pct_time_limited=("any_time_limit", "mean"),
    ).reset_index()
    agg["min_tri"] = agg["min_tri"].map(lambda v: f"{v:,.0f}" if pd.notna(v) else "-")
    agg["max_tri"] = agg["max_tri"].map(lambda v: f"{v:,.0f}" if pd.notna(v) else "-")
    agg["median_avg_time_per_tri"] = agg["median_avg_time_per_tri"].map(lambda v: f"{v:.3e}" if pd.notna(v) else "-")
    agg["median_time_per_tri"] = agg["median_time_per_tri"].map(lambda v: f"{v:.3e}" if pd.notna(v) else "-")
    agg["total_time"] = agg["total_time"].map(lambda v: f"{v:.2f}")
    agg["pct_time_limited"] = agg["pct_time_limited"].map(lambda v: f"{v*100:.0f}%")

    fig, ax = plt.subplots(figsize=(13, 0.5 * len(agg) + 1.5))
    ax.axis("off")
    tbl = ax.table(cellText=agg.values, colLabels=agg.columns, loc="center", cellLoc="center")
    tbl.auto_set_font_size(False)
    tbl.set_fontsize(9)
    tbl.scale(1, 1.6)
    ax.set_title("Per-Category Summary Statistics (median-based)", pad=20)
    savefig(fig, out_dir, "20_summary_table")


def plot_21_time_limit_rate_check(df, summary, out_dir):
    """Dedicated plot: for cases that hit the time limit, show the achieved
    rate (triangulations/sec at the moment of cutoff) side-by-side with the
    rate from cases that completed normally. If the time-limited rate sits
    in the same band as completed-case rates, that is strong, independent
    evidence the per-triangulation cost stays constant even under a fixed
    time budget rather than a fixed output-size budget."""
    sub = df[df["triangulations_per_sec"].notna() & (df["triangulations_per_sec"] > 0)]
    if sub.empty:
        print("  (skipped time-limit rate check: no usable rate data)")
        return
    categories = sorted(sub["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)
    fig, ax = plt.subplots(figsize=(max(9, 0.7 * len(categories)), 6.5))
    positions = []
    labels = []
    for i, cat in enumerate(categories):
        c_ok = sub[(sub["category"] == cat) & (sub["status_norm"] == "completed")]["triangulations_per_sec"]
        c_lim = sub[(sub["category"] == cat) & (sub["status_norm"] == "time_limit_exceeded")]["triangulations_per_sec"]
        rng = np.random.default_rng(0)
        if len(c_ok):
            ax.scatter(np.full(len(c_ok), i - 0.15) + rng.uniform(-0.05, 0.05, len(c_ok)), c_ok,
                      color=colors[cat], marker="o", s=30, alpha=0.7, edgecolor="black", linewidth=0.3)
        if len(c_lim):
            ax.scatter(np.full(len(c_lim), i + 0.15) + rng.uniform(-0.05, 0.05, len(c_lim)), c_lim,
                      color=colors[cat], marker="^", s=45, alpha=0.85, edgecolor="black", linewidth=0.5)
        positions.append(i)
        labels.append(cat)
    ax.set_yscale("log")
    ax.set_xticks(positions)
    ax.set_xticklabels(labels, rotation=45, ha="right")
    ax.set_ylabel("Triangulations / second (log scale)")
    ax.set_title("Throughput: Completed runs (●, left) vs Time-Limit-Cutoff runs (▲, right)\n"
                  "Overlapping bands support constant-rate enumeration regardless of budget type")
    savefig(fig, out_dir, "21_completed_vs_time_limited_rate_check")


def plot_22_per_category_avg_time_lines(summary, out_dir):
    """One line graph PER CATEGORY (own figure, own file), x = case ordered by
    triangulation count, y = median average time per triangulation for that
    case. Points where the run hit the time limit are colored differently
    from normal-completion points, and are connected in a distinct color
    segment so it is immediately visible which cases were affected."""
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    made_any = False
    for cat in categories:
        sub = sort_cases_by_triangulations(summary[summary["category"] == cat]).reset_index(drop=True)
        sub = sub[sub["median_avg_time_per_tri"].notna() & (sub["median_avg_time_per_tri"] > 0)]
        if sub.empty:
            continue
        made_any = True
        fig, ax = plt.subplots(figsize=(max(8, 0.5 * len(sub)), 6))
        x = np.arange(len(sub))

        # Draw connecting line, colored per-segment by whether either
        # endpoint hit the time limit (so the "excursion" into limited
        # territory is visually obvious along the line itself).
        for j in range(len(sub) - 1):
            seg_limited = bool(sub["any_time_limit"].iloc[j]) or bool(sub["any_time_limit"].iloc[j + 1])
            ax.plot(x[j:j + 2], sub["median_avg_time_per_tri"].values[j:j + 2],
                    color="#d62728" if seg_limited else "#1f77b4", linewidth=1.8, zorder=2)

        ok_mask = ~sub["any_time_limit"].values
        lim_mask = sub["any_time_limit"].values
        ax.scatter(x[ok_mask], sub["median_avg_time_per_tri"].values[ok_mask],
                   color="#1f77b4", s=55, edgecolor="black", linewidth=0.5, marker="o",
                   zorder=3, label="Completed within time limit")
        ax.scatter(x[lim_mask], sub["median_avg_time_per_tri"].values[lim_mask],
                   color="#d62728", s=80, edgecolor="black", linewidth=0.7, marker="^",
                   zorder=3, label="Time limit exceeded")

        ax.set_xticks(x)
        labels = [f"{c}\n(n≈{int(t):,})" for c, t in zip(sub["case"], sub["triangulations"])]
        ax.set_xticklabels(labels, rotation=45, ha="right", fontsize=7)
        ax.set_xlim(-0.6, len(sub) - 0.4)
        ax.set_yscale("log")
        ax.set_ylabel("Median average time / triangulation (s, log scale)")
        ax.set_title(f"Category: {cat} — Median Average Time per Triangulation by Case\n"
                     f"(ordered by ↑ triangulation count; red = time limit exceeded)")
        ax.legend(loc="best")
        safe_cat = re.sub(r"[^A-Za-z0-9_.-]+", "_", str(cat))
        savefig(fig, out_dir, f"22_per_category_lines/22_{safe_cat}_avg_time_per_triangulation")
    if not made_any:
        print("  (skipped per-category avg-time line plots: no usable data)")


def plot_23_all_categories_avg_time_per_tri_lines(summary, out_dir):
    """All categories on ONE graph, median average time per triangulation,
    each category its own colored line, x = triangulations (log scale)."""
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)
    fig, ax = plt.subplots(figsize=(10, 7))
    for cat in categories:
        sub = sort_cases_by_triangulations(summary[summary["category"] == cat])
        sub = sub[(sub["triangulations"] > 0) & (sub["median_avg_time_per_tri"] > 0)]
        if sub.empty:
            continue
        ax.plot(sub["triangulations"], sub["median_avg_time_per_tri"], "-", color=colors[cat],
                label=cat, linewidth=1.6, alpha=0.9)
        ok = sub[~sub["any_time_limit"]]
        lim = sub[sub["any_time_limit"]]
        ax.scatter(ok["triangulations"], ok["median_avg_time_per_tri"], color=colors[cat], s=40,
                  edgecolor="black", linewidth=0.3, marker="o", zorder=3)
        ax.scatter(lim["triangulations"], lim["median_avg_time_per_tri"], color=colors[cat], s=70,
                  edgecolor="red", linewidth=1.0, marker="^", zorder=4)
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("Total triangulations generated (log scale)")
    ax.set_ylabel("Median average time per triangulation (s, log scale)")
    ax.set_title("All Categories: Median Average Time per Triangulation vs Problem Size\n"
                 "(median_avg = total triangulations / median total time; ▲ red-ringed = time limit exceeded)")
    ax.legend(fontsize=8, ncol=2, loc="best")
    savefig(fig, out_dir, "23_all_categories_avg_time_per_tri_vs_n")


def plot_24_all_categories_avg_time_per_tri_by_case_rank(summary, out_dir):
    """All categories overlaid, x = case rank WITHIN its category (not
    triangulation count), useful when categories have different numbers of
    cases / scales and you just want to see the shape of the curve."""
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)
    fig, ax = plt.subplots(figsize=(10, 7))
    for cat in categories:
        sub = sort_cases_by_triangulations(summary[summary["category"] == cat]).reset_index(drop=True)
        sub = sub[sub["median_avg_time_per_tri"] > 0].reset_index(drop=True)
        if sub.empty:
            continue
        x = np.arange(len(sub))
        ax.plot(x, sub["median_avg_time_per_tri"], "-", color=colors[cat], label=cat, linewidth=1.6, alpha=0.9)
        ok_mask = ~sub["any_time_limit"].values
        lim_mask = sub["any_time_limit"].values
        ax.scatter(x[ok_mask], sub["median_avg_time_per_tri"].values[ok_mask],
                  color=colors[cat], s=35, edgecolor="black", linewidth=0.3, marker="o", zorder=3)
        ax.scatter(x[lim_mask], sub["median_avg_time_per_tri"].values[lim_mask],
                  color=colors[cat], s=65, edgecolor="red", linewidth=0.9, marker="^", zorder=4)
    ax.set_yscale("log")
    ax.set_xlabel("Case rank within category (ordered by ↑ triangulation count)")
    ax.set_ylabel("Median average time per triangulation (s, log scale)")
    ax.set_title("All Categories: Median Average Time per Triangulation by Case Rank\n"
                 "(▲ red-ringed = time limit exceeded)")
    ax.legend(fontsize=8, ncol=2, loc="best")
    savefig(fig, out_dir, "24_all_categories_avg_time_per_tri_by_case_rank")


def plot_25_all_categories_avg_time_per_tri_grouped_bars(summary, out_dir):
    """All categories, all cases, as non-overlapping grouped bars (one bar
    per case, grouped and gapped by category) so every individual case's
    median average time per triangulation is directly readable."""
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)
    n_total = sum((summary["category"] == c).sum() for c in categories)
    fig, ax = plt.subplots(figsize=(max(12, 0.45 * n_total), 6.5))
    x = 0
    xticks, xlabels = [], []
    for cat in categories:
        sub = sort_cases_by_triangulations(summary[summary["category"] == cat])
        sub = sub[sub["median_avg_time_per_tri"].notna()]
        xs = np.arange(x, x + len(sub))
        bar_colors = [colors[cat] if not lim else "#d62728" for lim in sub["any_time_limit"]]
        ax.bar(xs, sub["median_avg_time_per_tri"], width=0.7,
               color=bar_colors, edgecolor="black", linewidth=0.3)
        xticks.extend(xs)
        xlabels.extend(sub["case"])
        x += len(sub) + 2
    ax.set_xticks(xticks)
    ax.set_xticklabels(xlabels, rotation=90, fontsize=6)
    ax.set_yscale("log")
    ax.set_ylabel("Median avg time / triangulation (s, log scale)")
    ax.set_title("All Categories & Cases: Median Average Time per Triangulation\n"
                 "(red bars = at least one run hit the time limit)")
    handles = [Line2D([0], [0], color=colors[c], lw=6) for c in categories] + \
              [Line2D([0], [0], color="#d62728", lw=6)]
    ax.legend(handles, categories + ["time limit exceeded"], ncol=min(6, len(categories) + 1),
              loc="upper left", bbox_to_anchor=(0, 1.15))
    savefig(fig, out_dir, "25_all_categories_avg_time_per_tri_grouped_bars")


def plot_26_avg_time_per_tri_boxplot_by_category(summary, out_dir):
    """Distribution (across cases) of median average time per triangulation,
    one box per category -- quick view of typical value & spread per category."""
    categories = sorted(summary["category"].unique(), key=natural_case_key)
    colors = category_color_map(categories)
    data = [summary[summary["category"] == c]["median_avg_time_per_tri"].dropna() for c in categories]
    data = [d[d > 0] for d in data]
    if all(len(d) == 0 for d in data):
        print("  (skipped avg-time-per-tri boxplot by category: no data)")
        return
    fig, ax = plt.subplots(figsize=(max(8, 0.6 * len(categories)), 6))
    bp = ax.boxplot(data, labels=categories, patch_artist=True, showfliers=True,
                     medianprops=dict(color="black", linewidth=2))
    for patch, cat in zip(bp["boxes"], categories):
        patch.set_facecolor(colors[cat])
        patch.set_alpha(0.7)
    ax.set_yscale("log")
    ax.set_ylabel("Median average time per triangulation (s, log scale)")
    ax.set_title("Distribution of Median Average Time per Triangulation, by Category\n(across that category's cases)")
    plt.setp(ax.get_xticklabels(), rotation=45, ha="right")
    savefig(fig, out_dir, "26_avg_time_per_tri_boxplot_by_category")


PER_DATASET_PLOTS_SUMMARY = [
    plot_01_median_time_per_category,
    plot_02_median_time_all_categories_combined,
    plot_03_loglog_scatter,
    plot_04_loglog_lines_per_category,
    plot_05_amortized_time_per_triangulation,
    plot_07_throughput,
    plot_08_linear_fit_residuals,
    plot_09_time_vs_vertices,
    plot_10_triangulations_vs_vertices,
    plot_11_memory_vs_triangulations,
    plot_12_memory_per_vertex,
    plot_13_run_to_run_variance,
    plot_15_heatmap,
    plot_16_cumulative_rate_illustration,
    plot_17_runtime_share_stacked,
    plot_18_scaling_exponent,
    plot_19_histogram,
    plot_20_summary_table,
    plot_22_per_category_avg_time_lines,
    plot_23_all_categories_avg_time_per_tri_lines,
    plot_24_all_categories_avg_time_per_tri_by_case_rank,
    plot_25_all_categories_avg_time_per_tri_grouped_bars,
    plot_26_avg_time_per_tri_boxplot_by_category,
]

PER_DATASET_PLOTS_RAW_DF = [
    plot_06_time_per_triangulation_boxplot,
    plot_14_individual_runs_strip,
]

PER_DATASET_PLOTS_BOTH = [
    plot_21_time_limit_rate_check,  # needs raw df + summary
]


def run_per_dataset_plots(df, summary, out_dir):
    for fn in PER_DATASET_PLOTS_SUMMARY:
        try:
            fn(summary, out_dir)
        except Exception as e:
            print(f"  [WARN] {fn.__name__} failed: {e}")
    for fn in PER_DATASET_PLOTS_RAW_DF:
        try:
            fn(df, out_dir)
        except Exception as e:
            print(f"  [WARN] {fn.__name__} failed: {e}")
    for fn in PER_DATASET_PLOTS_BOTH:
        try:
            fn(df, summary, out_dir)
        except Exception as e:
            print(f"  [WARN] {fn.__name__} failed: {e}")


# ============================================================================
# COMPARISON PLOTS (across datasets, e.g. with-vgs vs without-vgs)
# ============================================================================

def cmp_01_median_time_per_triangulation_overlay(all_summary, out_dir):
    """The headline comparison chart: amortized cost curve for every
    dataset overlaid, colored by dataset, one line per (dataset, category)."""
    datasets = sorted(all_summary["dataset"].unique())
    dcolors = dataset_color_map(datasets)
    fig, ax = plt.subplots(figsize=(10, 7))
    for ds in datasets:
        sub_ds = all_summary[all_summary["dataset"] == ds]
        for cat in sorted(sub_ds["category"].unique(), key=natural_case_key):
            sub = sort_cases_by_triangulations(sub_ds[sub_ds["category"] == cat])
            sub = sub[(sub["triangulations"] > 0) & (sub["median_avg_time_per_tri"] > 0)]
            if sub.empty:
                continue
            ax.plot(sub["triangulations"], sub["median_avg_time_per_tri"], "-", color=dcolors[ds],
                    alpha=0.85, linewidth=1.3)
            ok = sub[~sub["any_time_limit"]]
            lim = sub[sub["any_time_limit"]]
            ax.scatter(ok["triangulations"], ok["median_avg_time_per_tri"], color=dcolors[ds], s=30,
                      edgecolor="black", linewidth=0.3, marker="o", zorder=3)
            ax.scatter(lim["triangulations"], lim["median_avg_time_per_tri"], color=dcolors[ds], s=50,
                      edgecolor="black", linewidth=0.6, marker="^", zorder=3)
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("Total triangulations (log scale)")
    ax.set_ylabel("Median average time per triangulation (s, log scale)")
    ax.set_title("Dataset Comparison: Amortized Per-Triangulation Cost\n(median_avg metric; color = dataset, every category/case overlaid; ▲ = time-limit sample)")
    handles = [Line2D([0], [0], color=dcolors[d], lw=6) for d in datasets]
    ax.legend(handles, datasets, loc="best")
    savefig(fig, out_dir, "01_dataset_comparison_amortized_cost")


def cmp_02_loglog_time_vs_triangulations(all_summary, out_dir):
    datasets = sorted(all_summary["dataset"].unique())
    dcolors = dataset_color_map(datasets)
    fig, ax = plt.subplots(figsize=(10, 7))
    for ds in datasets:
        sub = all_summary[(all_summary["dataset"] == ds) & (all_summary["triangulations"] > 0) & (all_summary["median_time"] > 0)]
        if sub.empty:
            continue
        ok = sub[~sub["any_time_limit"]]
        lim = sub[sub["any_time_limit"]]
        ax.scatter(ok["triangulations"], ok["median_time"], color=dcolors[ds], s=35, alpha=0.75,
                  edgecolor="black", linewidth=0.3, marker="o", label=f"{ds} (completed)")
        ax.scatter(lim["triangulations"], lim["median_time"], color=dcolors[ds], s=55, alpha=0.85,
                  edgecolor="black", linewidth=0.6, marker="^", label=f"{ds} (time-limit hit)")
        if len(sub) > 1:
            logx = np.log10(sub["triangulations"].values)
            logy = np.log10(sub["median_time"].values)
            slope, intercept = np.polyfit(logx, logy, 1)
            xs = np.linspace(logx.min(), logx.max(), 50)
            ax.plot(10**xs, 10**(slope * xs + intercept), "--", color=dcolors[ds], linewidth=1.8,
                    label=f"{ds} fit slope={slope:.3f}")
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("Total triangulations (log scale)")
    ax.set_ylabel("Median time (s, log scale)")
    ax.set_title("Dataset Comparison: Log-Log Time vs Triangulations")
    ax.legend(fontsize=8, ncol=2)
    savefig(fig, out_dir, "02_dataset_comparison_loglog")


def cmp_03_speedup_ratio(all_summary, out_dir):
    """If exactly 2+ datasets share the same (category, case) identifiers,
    compute the per-case speed ratio between datasets (using the first
    dataset alphabetically as baseline)."""
    datasets = sorted(all_summary["dataset"].unique())
    if len(datasets) < 2:
        print("  (skipped speedup-ratio: need >=2 datasets)")
        return
    baseline = datasets[0]
    base = all_summary[all_summary["dataset"] == baseline][["category", "case", "median_time", "triangulations"]]
    base = base.rename(columns={"median_time": "baseline_time", "triangulations": "baseline_tri"})

    fig, ax = plt.subplots(figsize=(10, 6.5))
    dcolors = dataset_color_map(datasets)
    plotted_any = False
    for ds in datasets[1:]:
        other = all_summary[all_summary["dataset"] == ds][["category", "case", "median_time", "triangulations"]]
        merged = other.merge(base, on=["category", "case"], how="inner")
        if merged.empty:
            continue
        merged["speedup"] = merged["baseline_time"] / merged["median_time"]
        merged = merged.sort_values("baseline_tri")
        ax.plot(merged["baseline_tri"], merged["speedup"], "-o", color=dcolors[ds],
                label=f"{ds} vs {baseline}", markersize=5, linewidth=1.4,
                markeredgecolor="black", markeredgewidth=0.3)
        plotted_any = True

    if not plotted_any:
        plt.close(fig)
        print("  (skipped speedup-ratio: no matching case/category names across datasets)")
        return

    ax.axhline(1.0, color="black", ls="--", lw=1.5, label="No change")
    ax.set_xscale("log")
    ax.set_xlabel(f"Triangulation count in '{baseline}' (log scale)")
    ax.set_ylabel(f"Speedup ratio (baseline_time / dataset_time)")
    ax.set_title(f"Per-Case Speedup Relative to '{baseline}'\n(>1 = faster than baseline, <1 = slower)")
    ax.legend(fontsize=8)
    savefig(fig, out_dir, "03_per_case_speedup_ratio")


def cmp_04_throughput_boxplot_by_dataset(all_summary, out_dir):
    datasets = sorted(all_summary["dataset"].unique())
    dcolors = dataset_color_map(datasets)
    data = [all_summary[all_summary["dataset"] == d]["median_tri_per_sec"].dropna() for d in datasets]
    data = [d[d > 0] for d in data]
    if all(len(d) == 0 for d in data):
        print("  (skipped throughput-boxplot: no data)")
        return
    fig, ax = plt.subplots(figsize=(max(6, 1.5 * len(datasets)), 6))
    bp = ax.boxplot(data, labels=datasets, patch_artist=True, showfliers=True)
    for patch, ds in zip(bp["boxes"], datasets):
        patch.set_facecolor(dcolors[ds])
        patch.set_alpha(0.7)
    ax.set_yscale("log")
    ax.set_ylabel("Median throughput (triangulations/sec, log scale)")
    ax.set_title("Overall Throughput Distribution by Dataset")
    savefig(fig, out_dir, "04_throughput_boxplot_by_dataset")


def cmp_05_time_per_triangulation_boxplot_by_dataset(all_df, out_dir):
    datasets = sorted(all_df["dataset"].unique())
    dcolors = dataset_color_map(datasets)
    data = [all_df[all_df["dataset"] == d]["time_per_triangulation"].dropna() for d in datasets]
    data = [d[d > 0] for d in data]
    if all(len(d) == 0 for d in data):
        print("  (skipped time-per-tri boxplot by dataset: no data)")
        return
    fig, ax = plt.subplots(figsize=(max(6, 1.5 * len(datasets)), 6))
    bp = ax.boxplot(data, labels=datasets, patch_artist=True, showfliers=True, medianprops=dict(color="black", linewidth=2))
    for patch, ds in zip(bp["boxes"], datasets):
        patch.set_facecolor(dcolors[ds])
        patch.set_alpha(0.7)
    ax.set_yscale("log")
    ax.set_ylabel("Time per triangulation (s, log scale)")
    ax.set_title("Amortized Per-Triangulation Cost: All Runs, by Dataset\n(lower/tighter = better & more consistent)")
    savefig(fig, out_dir, "05_time_per_triangulation_boxplot_by_dataset")


def cmp_06_scaling_exponent_by_dataset(all_summary, out_dir):
    datasets = sorted(all_summary["dataset"].unique())
    dcolors = dataset_color_map(datasets)
    exps, errs, labels = [], [], []
    for ds in datasets:
        sub = all_summary[(all_summary["dataset"] == ds) & (all_summary["triangulations"] > 0) & (all_summary["median_time"] > 0)]
        if len(sub) < 2:
            continue
        logx = np.log10(sub["triangulations"].values)
        logy = np.log10(sub["median_time"].values)
        if np.ptp(logx) < 1e-9:
            continue
        slope, intercept = np.polyfit(logx, logy, 1)
        resid = logy - (slope * logx + intercept)
        se = np.std(resid) / (np.sqrt(len(logx)) * (np.std(logx) + 1e-12))
        exps.append(slope)
        errs.append(se)
        labels.append(ds)
    if not labels:
        print("  (skipped scaling-exponent-by-dataset: insufficient data)")
        return
    fig, ax = plt.subplots(figsize=(max(6, 1.5 * len(labels)), 6))
    xs = np.arange(len(labels))
    ax.bar(xs, exps, yerr=errs, capsize=4, color=[dcolors[d] for d in labels], edgecolor="black")
    ax.axhline(1.0, color="red", ls="--", lw=2, label="Slope = 1 (amortized O(1))")
    ax.set_xticks(xs)
    ax.set_xticklabels(labels)
    ax.set_ylabel("Fitted log-log slope")
    ax.set_title("Overall Scaling Exponent by Dataset\n(log(time) vs log(triangulations), pooled across categories)")
    ax.legend()
    savefig(fig, out_dir, "06_scaling_exponent_by_dataset")


def cmp_07_memory_comparison(all_summary, out_dir):
    datasets = sorted(all_summary["dataset"].unique())
    dcolors = dataset_color_map(datasets)
    fig, ax = plt.subplots(figsize=(9, 6.5))
    plotted = False
    for ds in datasets:
        sub = all_summary[(all_summary["dataset"] == ds) & (all_summary["median_mem"] > 0) & (all_summary["triangulations"] > 0)]
        if sub.empty:
            continue
        plotted = True
        ax.scatter(sub["triangulations"], sub["median_mem"] / (1024 * 1024), color=dcolors[ds],
                  label=ds, s=40, edgecolor="black", linewidth=0.3, alpha=0.8)
    if not plotted:
        plt.close(fig)
        print("  (skipped memory comparison: no usable memory data)")
        return
    ax.set_xscale("log")
    ax.set_xlabel("Total triangulations (log scale)")
    ax.set_ylabel("Median peak memory (MB)")
    ax.set_title("Peak Memory vs Triangulation Count, by Dataset")
    ax.legend()
    savefig(fig, out_dir, "07_memory_comparison_by_dataset")


def cmp_08_time_limited_fraction(all_summary, out_dir):
    """How often each dataset/category combo hit the wall-clock cutoff --
    a direct signal of which configuration is actually faster in practice."""
    grp = all_summary.groupby(["dataset", "category"])["any_time_limit"].mean().reset_index()
    grp["pct"] = grp["any_time_limit"] * 100
    datasets = sorted(grp["dataset"].unique())
    categories = sorted(grp["category"].unique(), key=natural_case_key)
    dcolors = dataset_color_map(datasets)

    fig, ax = plt.subplots(figsize=(max(9, 0.6 * len(categories)), 6.5))
    width = 0.8 / max(len(datasets), 1)
    x = np.arange(len(categories))
    for i, ds in enumerate(datasets):
        heights = []
        for cat in categories:
            row = grp[(grp["dataset"] == ds) & (grp["category"] == cat)]
            heights.append(row["pct"].values[0] if not row.empty else 0)
        ax.bar(x + i * width - 0.4 + width / 2, heights, width=width, color=dcolors[ds], label=ds, edgecolor="black", linewidth=0.4)
    ax.set_xticks(x)
    ax.set_xticklabels(categories, rotation=45, ha="right")
    ax.set_ylabel("% of cases with ≥1 time-limited run")
    ax.set_title("Fraction of Cases Hitting the Time Limit, by Dataset & Category\n(lower = configuration completes more cases within budget)")
    ax.legend()
    savefig(fig, out_dir, "08_time_limited_fraction_by_dataset")


def cmp_09_paired_case_scatter(all_summary, out_dir):
    """Direct pairwise comparison: for cases with identical (category, case)
    across exactly 2 datasets, scatter dataset-A time vs dataset-B time on
    a log-log plot with the y=x line -- points below the line mean the
    y-axis dataset was faster."""
    datasets = sorted(all_summary["dataset"].unique())
    if len(datasets) != 2:
        print(f"  (skipped paired-case-scatter: designed for exactly 2 datasets, found {len(datasets)})")
        return
    a, b = datasets
    da = all_summary[all_summary["dataset"] == a][["category", "case", "median_time", "triangulations"]]
    db = all_summary[all_summary["dataset"] == b][["category", "case", "median_time", "triangulations"]]
    merged = da.merge(db, on=["category", "case"], suffixes=(f"_{a}", f"_{b}"))
    if merged.empty:
        print("  (skipped paired-case-scatter: no matching case names across the two datasets)")
        return

    categories = sorted(merged["category"].unique(), key=natural_case_key)
    ccolors = category_color_map(categories)
    fig, ax = plt.subplots(figsize=(8, 8))
    for cat in categories:
        sub = merged[merged["category"] == cat]
        ax.scatter(sub[f"median_time_{a}"], sub[f"median_time_{b}"], color=ccolors[cat], label=cat,
                  s=50, edgecolor="black", linewidth=0.4, alpha=0.85)
    lims = [min(merged[f"median_time_{a}"].min(), merged[f"median_time_{b}"].min()) * 0.5,
            max(merged[f"median_time_{a}"].max(), merged[f"median_time_{b}"].max()) * 2]
    ax.plot(lims, lims, "k--", lw=1.5, label="y = x (no difference)")
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlim(lims)
    ax.set_ylim(lims)
    ax.set_xlabel(f"Median time, {a} (s)")
    ax.set_ylabel(f"Median time, {b} (s)")
    ax.set_title(f"Paired Case Comparison: {a} vs {b}\n(below diagonal = {b} faster)")
    ax.legend(fontsize=8, ncol=2)
    savefig(fig, out_dir, "09_paired_case_scatter")


COMPARISON_PLOTS_SUMMARY = [
    cmp_01_median_time_per_triangulation_overlay,
    cmp_02_loglog_time_vs_triangulations,
    cmp_03_speedup_ratio,
    cmp_04_throughput_boxplot_by_dataset,
    cmp_06_scaling_exponent_by_dataset,
    cmp_07_memory_comparison,
    cmp_08_time_limited_fraction,
    cmp_09_paired_case_scatter,
]


def run_comparison_plots(all_df, all_summary, out_dir):
    for fn in COMPARISON_PLOTS_SUMMARY:
        try:
            fn(all_summary, out_dir)
        except Exception as e:
            print(f"  [WARN] {fn.__name__} failed: {e}")
    try:
        cmp_05_time_per_triangulation_boxplot_by_dataset(all_df, out_dir)
    except Exception as e:
        print(f"  [WARN] cmp_05_time_per_triangulation_boxplot_by_dataset failed: {e}")


# ============================================================================
# Main
# ============================================================================

def main():
    ap = argparse.ArgumentParser(description="Generate research-grade plots from triangulation benchmark CSVs.")
    ap.add_argument("--input-dir", default=".", help="Directory containing dataset subfolders (each with results_*.csv), or CSVs directly.")
    ap.add_argument("--out-dir", default="plots", help="Base name for output folders (plots_<dataset>/ and plots_comparison/ are created next to this).")
    args = ap.parse_args()

    base_out = os.path.abspath(args.out_dir)
    parent = os.path.dirname(base_out) or "."
    out_prefix = os.path.basename(base_out)

    datasets = discover_datasets(args.input_dir)
    if not datasets:
        print(f"[ERROR] No CSV files found under: {args.input_dir}", file=sys.stderr)
        sys.exit(1)

    print(f"[INFO] Discovered {len(datasets)} dataset(s): {list(datasets.keys())}")

    all_df_frames = []
    all_summary_frames = []

    for ds_name, csv_paths in datasets.items():
        print(f"\n=== Dataset: {ds_name} ({len(csv_paths)} CSV file(s)) ===")
        df = load_dataset(csv_paths, ds_name)
        if df.empty:
            print(f"[WARN] No usable rows for dataset '{ds_name}', skipping.")
            continue
        print(f"[INFO] Loaded {len(df)} run rows across {df['category'].nunique()} categories, "
              f"{df['case'].nunique()} unique cases.")

        summary = per_case_summary(df)
        ds_out_dir = os.path.join(parent, f"{out_prefix}_{ds_name}")
        os.makedirs(ds_out_dir, exist_ok=True)
        summary.to_csv(os.path.join(ds_out_dir, "per_case_summary.csv"), index=False)
        print(f"[INFO] Wrote per-case summary -> {os.path.join(ds_out_dir, 'per_case_summary.csv')}")

        print(f"[INFO] Generating per-dataset plots in {ds_out_dir} ...")
        run_per_dataset_plots(df, summary, ds_out_dir)

        all_df_frames.append(df)
        all_summary_frames.append(summary)

    if not all_df_frames:
        print("[ERROR] No dataset produced usable data. Exiting.", file=sys.stderr)
        sys.exit(1)

    if len(all_df_frames) >= 2:
        all_df = pd.concat(all_df_frames, ignore_index=True)
        all_summary = pd.concat(all_summary_frames, ignore_index=True)
        cmp_out_dir = os.path.join(parent, f"{out_prefix}_comparison")
        os.makedirs(cmp_out_dir, exist_ok=True)
        all_summary.to_csv(os.path.join(cmp_out_dir, "all_datasets_summary.csv"), index=False)
        print(f"\n=== Comparison across {len(all_df_frames)} datasets ===")
        print(f"[INFO] Generating comparison plots in {cmp_out_dir} ...")
        run_comparison_plots(all_df, all_summary, cmp_out_dir)
    else:
        print("\n[INFO] Only one dataset found — skipping comparison folder.")

    print(f"\n[DONE] All figures written under: {parent}")


if __name__ == "__main__":
    main()