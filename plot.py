#!/usr/bin/env python3
"""
generate_graphs.py

Generates a full suite of comparison graphs (Biconnected vs Oneconnected
triangulation algorithms) from benchmark CSV output.

EXPECTED FOLDER LAYOUT (relative to this script, or pass --root):

    benchmark-results/
        Biconnected/
            individual/
                05_cycles/
                    case_001.txt.csv
                    case_002.txt.csv
                    ...
                08_chord_sequence/
                    ...
            total/
                05_cycles_total.csv        (or 05_cycles/total.csv -- see discovery logic)
                08_chord_sequence_total.csv
        Oneconnected/
            individual/
                05_cycles/
                    case_001.txt.csv
                    ...
            total/
                05_cycles_total.csv
                ...

INDIVIDUAL CSV SCHEMA:
    run,triangulation,cumulativeNs,deltaNs

TOTAL CSV SCHEMA:
    filename,runIndex,vertices,triangulations,timeSeconds,peakMemoryBytes,
    memoryPerVertex,startTime,endTime,status,totalChecks,successfulChecks,
    failedChecks,checkSuccessRate,invalidTraversals,totalTraversalsExtended,
    traversalSuccessRate

All output PNGs are written to ./graphs_output/, organized into subfolders:
    graphs_output/
        overall/                 -> cross-category, algo-wide comparisons
        per_category/<cat>/      -> per-category comparisons (all cases in one plot)
        per_case/<cat>/          -> per-case individual-run comparisons (M graphs)

Run:
    python3 generate_graphs.py --root ./benchmark-results
"""

import argparse
import os
import re
import sys
import glob
import warnings
from collections import defaultdict

import numpy as np
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

warnings.filterwarnings("ignore")

# ----------------------------------------------------------------------------
# Styling
# ----------------------------------------------------------------------------

ALGO_COLORS = {
    "Biconnected": "#2E86AB",
    "Oneconnected": "#E76F51",
}
ALGO_ORDER = ["Biconnected", "Oneconnected"]

plt.rcParams.update({
    "figure.figsize": (10, 6),
    "figure.dpi": 120,
    "axes.grid": True,
    "grid.alpha": 0.3,
    "axes.titlesize": 13,
    "axes.titleweight": "bold",
    "axes.labelsize": 11,
    "legend.fontsize": 9,
    "font.size": 10,
})


def savefig(fig, path):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    fig.tight_layout()
    fig.savefig(path)
    plt.close(fig)
    print(f"  wrote {path}")


# ----------------------------------------------------------------------------
# Discovery
# ----------------------------------------------------------------------------

def discover_categories(root, algo):
    """Return list of category names under <root>/<algo>/individual/"""
    ind_dir = os.path.join(root, algo, "individual")
    if not os.path.isdir(ind_dir):
        return []
    return sorted([d for d in os.listdir(ind_dir)
                   if os.path.isdir(os.path.join(ind_dir, d))])


def discover_case_files(root, algo, category):
    """Return sorted list of (case_name, filepath) for individual per-case CSVs."""
    cat_dir = os.path.join(root, algo, "individual", category)
    if not os.path.isdir(cat_dir):
        return []
    files = glob.glob(os.path.join(cat_dir, "*.csv"))
    out = []
    for f in sorted(files):
        base = os.path.basename(f)
        # strip .csv, then strip trailing .txt if present -> case_001
        case_name = re.sub(r"\.csv$", "", base)
        case_name = re.sub(r"\.txt$", "", case_name)
        out.append((case_name, f))
    return out


def find_total_csv(root, algo, category):
    """Locate the total-summary CSV for a category, trying a few naming conventions."""
    candidates = [
        os.path.join(root, algo, "total", f"{category}_total.csv"),
        os.path.join(root, algo, "total", f"{category}.csv"),
        os.path.join(root, algo, "total", category, "total.csv"),
        os.path.join(root, algo, "total", category, f"{category}_total.csv"),
    ]
    for c in candidates:
        if os.path.isfile(c):
            return c
    # fallback: any csv in total/ whose name contains the category
    total_dir = os.path.join(root, algo, "total")
    if os.path.isdir(total_dir):
        for f in glob.glob(os.path.join(total_dir, "*.csv")):
            if category in os.path.basename(f):
                return f
    return None


def load_total_df(path, algo, category):
    df = pd.read_csv(path)
    df["algo"] = algo
    df["category"] = category
    # normalize case name from filename column: case_001.txt -> case_001
    df["case"] = df["filename"].astype(str).str.replace(r"\.txt$", "", regex=True)
    return df


def load_individual_df(path, algo, category, case):
    df = pd.read_csv(path)
    df["algo"] = algo
    df["category"] = category
    df["case"] = case
    # average time per triangulation up to point n = cumulativeNs / triangulation
    df["avgNsPerTri"] = df["cumulativeNs"] / df["triangulation"]
    return df


# ----------------------------------------------------------------------------
# Load everything
# ----------------------------------------------------------------------------

def load_all(root):
    all_totals = []
    all_individuals = []  # list of dicts: algo, category, case, df

    categories = set()
    for algo in ALGO_ORDER:
        categories.update(discover_categories(root, algo))
    categories = sorted(categories)

    for category in categories:
        for algo in ALGO_ORDER:
            tpath = find_total_csv(root, algo, category)
            if tpath:
                try:
                    all_totals.append(load_total_df(tpath, algo, category))
                except Exception as e:
                    print(f"  [warn] failed reading total csv {tpath}: {e}")

            for case, fpath in discover_case_files(root, algo, category):
                try:
                    idf = load_individual_df(fpath, algo, category, case)
                    all_individuals.append({
                        "algo": algo, "category": category, "case": case, "df": idf
                    })
                except Exception as e:
                    print(f"  [warn] failed reading individual csv {fpath}: {e}")

    totals_df = pd.concat(all_totals, ignore_index=True) if all_totals else pd.DataFrame()
    return totals_df, all_individuals, categories


# ----------------------------------------------------------------------------
# Aggregations on totals_df
# ----------------------------------------------------------------------------

def add_derived_columns(totals_df):
    if totals_df.empty:
        return totals_df
    df = totals_df.copy()
    df["avgTimePerTriangulation"] = df["timeSeconds"] / df["triangulations"].replace(0, np.nan)
    df["unsuccessfulCheckRate"] = 100.0 - df["checkSuccessRate"]
    return df


# ----------------------------------------------------------------------------
# OVERALL GRAPHS (cross-category / whole-dataset)
# ----------------------------------------------------------------------------

def plot_overall(totals_df, individuals, outdir):
    print("\n[overall] generating cross-category comparison graphs...")
    od = os.path.join(outdir, "overall")

    if totals_df.empty:
        print("  [warn] no total data loaded, skipping overall graphs")
        return

    df = totals_df

    # 1. Linear memory according to vertex number (peakMemoryBytes vs vertices)
    fig, ax = plt.subplots()
    for algo in ALGO_ORDER:
        sub = df[df.algo == algo].sort_values("vertices")
        if sub.empty:
            continue
        grp = sub.groupby("vertices")["peakMemoryBytes"].mean().reset_index()
        ax.plot(grp.vertices, grp.peakMemoryBytes, "o-", label=algo, color=ALGO_COLORS[algo])
    ax.set_xlabel("Number of vertices")
    ax.set_ylabel("Peak memory (bytes)")
    ax.set_title("Peak Memory vs Vertex Count (mean across runs)")
    ax.legend()
    savefig(fig, os.path.join(od, "memory_vs_vertices.png"))

    # 1b. Memory per vertex vs vertex count
    fig, ax = plt.subplots()
    for algo in ALGO_ORDER:
        sub = df[df.algo == algo].sort_values("vertices")
        if sub.empty:
            continue
        grp = sub.groupby("vertices")["memoryPerVertex"].mean().reset_index()
        ax.plot(grp.vertices, grp.memoryPerVertex, "o-", label=algo, color=ALGO_COLORS[algo])
    ax.set_xlabel("Number of vertices")
    ax.set_ylabel("Memory per vertex (bytes)")
    ax.set_title("Memory per Vertex vs Vertex Count")
    ax.legend()
    savefig(fig, os.path.join(od, "memory_per_vertex_vs_vertices.png"))

    # 2. Average time per triangulation vs vertices (amortized constant-time check)
    fig, ax = plt.subplots()
    for algo in ALGO_ORDER:
        sub = df[df.algo == algo].sort_values("vertices")
        if sub.empty:
            continue
        grp = sub.groupby("vertices")["avgTimePerTriangulation"].mean().reset_index()
        ax.plot(grp.vertices, grp.avgTimePerTriangulation, "o-", label=algo, color=ALGO_COLORS[algo])
    ax.set_xlabel("Number of vertices")
    ax.set_ylabel("Avg time per triangulation (s)")
    ax.set_title("Amortized Time per Triangulation vs Vertex Count")
    ax.legend()
    savefig(fig, os.path.join(od, "avg_time_per_triangulation_vs_vertices.png"))

    # 3. Total time vs total triangulations, log-log
    fig, ax = plt.subplots()
    for algo in ALGO_ORDER:
        sub = df[df.algo == algo]
        if sub.empty:
            continue
        ax.scatter(sub.triangulations, sub.timeSeconds, label=algo,
                   color=ALGO_COLORS[algo], alpha=0.6, s=25)
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("Total triangulations (log scale)")
    ax.set_ylabel("Total time in seconds (log scale)")
    ax.set_title("Total Time vs Total Triangulations (log-log)")
    ax.legend()
    savefig(fig, os.path.join(od, "loglog_time_vs_triangulations.png"))

    # 4. Total triangulations vs vertices, log-log (growth rate)
    fig, ax = plt.subplots()
    for algo in ALGO_ORDER:
        sub = df[df.algo == algo].sort_values("vertices")
        if sub.empty:
            continue
        grp = sub.groupby("vertices")["triangulations"].mean().reset_index()
        ax.plot(grp.vertices, grp.triangulations, "o-", label=algo, color=ALGO_COLORS[algo])
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("Number of vertices (log scale)")
    ax.set_ylabel("Total triangulations (log scale)")
    ax.set_title("Triangulation Count Growth vs Vertices (log-log)")
    ax.legend()
    savefig(fig, os.path.join(od, "loglog_triangulations_vs_vertices.png"))

    # 5. Total time vs vertices, log-log
    fig, ax = plt.subplots()
    for algo in ALGO_ORDER:
        sub = df[df.algo == algo].sort_values("vertices")
        if sub.empty:
            continue
        grp = sub.groupby("vertices")["timeSeconds"].mean().reset_index()
        ax.plot(grp.vertices, grp.timeSeconds, "o-", label=algo, color=ALGO_COLORS[algo])
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("Number of vertices (log scale)")
    ax.set_ylabel("Total time in seconds (log scale)")
    ax.set_title("Total Time vs Vertices (log-log)")
    ax.legend()
    savefig(fig, os.path.join(od, "loglog_time_vs_vertices.png"))

    # 6. Box plot: time distribution per algo (overall)
    fig, ax = plt.subplots()
    data = [df[df.algo == a]["timeSeconds"].dropna() for a in ALGO_ORDER]
    bp = ax.boxplot(data, labels=ALGO_ORDER, patch_artist=True)
    for patch, algo in zip(bp["boxes"], ALGO_ORDER):
        patch.set_facecolor(ALGO_COLORS[algo])
        patch.set_alpha(0.6)
    ax.set_ylabel("Total time (s)")
    ax.set_title("Distribution of Total Run Time by Algorithm")
    savefig(fig, os.path.join(od, "boxplot_time_by_algo.png"))

    # 7. Bar: average time per triangulation by category (grouped by algo)
    fig, ax = plt.subplots(figsize=(12, 6))
    cats = sorted(df.category.unique())
    x = np.arange(len(cats))
    width = 0.35
    for i, algo in enumerate(ALGO_ORDER):
        vals = []
        for c in cats:
            sub = df[(df.algo == algo) & (df.category == c)]
            vals.append(sub.avgTimePerTriangulation.mean() if not sub.empty else 0)
        ax.bar(x + (i - 0.5) * width, vals, width, label=algo, color=ALGO_COLORS[algo])
    ax.set_xticks(x)
    ax.set_xticklabels(cats, rotation=40, ha="right")
    ax.set_ylabel("Avg time per triangulation (s)")
    ax.set_title("Average Time per Triangulation by Category")
    ax.legend()
    savefig(fig, os.path.join(od, "bar_avg_time_per_triangulation_by_category.png"))

    # 8. Bar: mean check success rate by category
    fig, ax = plt.subplots(figsize=(12, 6))
    for i, algo in enumerate(ALGO_ORDER):
        vals = []
        for c in cats:
            sub = df[(df.algo == algo) & (df.category == c)]
            vals.append(sub.checkSuccessRate.mean() if not sub.empty else 0)
        ax.bar(x + (i - 0.5) * width, vals, width, label=algo, color=ALGO_COLORS[algo])
    ax.set_xticks(x)
    ax.set_xticklabels(cats, rotation=40, ha="right")
    ax.set_ylabel("Check success rate (%)")
    ax.set_title("Mean Check Success Rate by Category")
    ax.legend()
    savefig(fig, os.path.join(od, "bar_check_success_rate_by_category.png"))

    # 9. Bar: mean traversal success rate by category
    fig, ax = plt.subplots(figsize=(12, 6))
    for i, algo in enumerate(ALGO_ORDER):
        vals = []
        for c in cats:
            sub = df[(df.algo == algo) & (df.category == c)]
            vals.append(sub.traversalSuccessRate.mean() if not sub.empty else 0)
        ax.bar(x + (i - 0.5) * width, vals, width, label=algo, color=ALGO_COLORS[algo])
    ax.set_xticks(x)
    ax.set_xticklabels(cats, rotation=40, ha="right")
    ax.set_ylabel("Traversal success rate (%)")
    ax.set_title("Mean Traversal Success Rate by Category")
    ax.legend()
    savefig(fig, os.path.join(od, "bar_traversal_success_rate_by_category.png"))

    # 10. Unsuccessful check rate vs avg time per triangulation (does failure correlate w/ slowness?)
    fig, ax = plt.subplots()
    for algo in ALGO_ORDER:
        sub = df[df.algo == algo].sort_values("unsuccessfulCheckRate")
        if sub.empty or sub.unsuccessfulCheckRate.max() == 0:
            continue
        ax.scatter(sub.unsuccessfulCheckRate, sub.avgTimePerTriangulation,
                   label=algo, color=ALGO_COLORS[algo], alpha=0.6, s=25)
    ax.set_xlabel("Unsuccessful check rate (%)")
    ax.set_ylabel("Avg time per triangulation (s)")
    ax.set_title("Unsuccessful Check Rate vs Avg Time per Triangulation")
    if any(df.unsuccessfulCheckRate > 0):
        ax.legend()
        savefig(fig, os.path.join(od, "unsuccessful_checks_vs_avg_time.png"))
    else:
        plt.close(fig)
        print("  [info] no unsuccessful checks found in dataset; "
              "skipping unsuccessful_checks_vs_avg_time.png")

    # 10b. Binned version: as unsuccessful-check-rate bucket increases, is oneconnected slower?
    if (df.unsuccessfulCheckRate > 0).any():
        fig, ax = plt.subplots()
        bins = [0, 1, 5, 10, 20, 50, 100]
        df2 = df.copy()
        df2["uc_bucket"] = pd.cut(df2.unsuccessfulCheckRate, bins=bins, include_lowest=True)
        for algo in ALGO_ORDER:
            sub = df2[df2.algo == algo]
            grp = sub.groupby("uc_bucket")["avgTimePerTriangulation"].mean()
            ax.plot(range(len(grp)), grp.values, "o-", label=algo, color=ALGO_COLORS[algo])
        ax.set_xticks(range(len(bins) - 1))
        ax.set_xticklabels([f"{bins[i]}-{bins[i+1]}%" for i in range(len(bins) - 1)], rotation=30)
        ax.set_xlabel("Unsuccessful check rate bucket")
        ax.set_ylabel("Avg time per triangulation (s)")
        ax.set_title("Slowdown vs Unsuccessful Check Rate Buckets")
        ax.legend()
        savefig(fig, os.path.join(od, "slowdown_vs_unsuccessful_check_buckets.png"))

    # 11. Failed checks count vs vertices
    fig, ax = plt.subplots()
    for algo in ALGO_ORDER:
        sub = df[df.algo == algo].sort_values("vertices")
        if sub.empty:
            continue
        grp = sub.groupby("vertices")["failedChecks"].mean().reset_index()
        ax.plot(grp.vertices, grp.failedChecks, "o-", label=algo, color=ALGO_COLORS[algo])
    ax.set_xlabel("Number of vertices")
    ax.set_ylabel("Mean failed checks")
    ax.set_title("Failed Checks vs Vertex Count")
    ax.legend()
    savefig(fig, os.path.join(od, "failed_checks_vs_vertices.png"))

    # 12. Invalid traversals vs vertices
    fig, ax = plt.subplots()
    for algo in ALGO_ORDER:
        sub = df[df.algo == algo].sort_values("vertices")
        if sub.empty:
            continue
        grp = sub.groupby("vertices")["invalidTraversals"].mean().reset_index()
        ax.plot(grp.vertices, grp.invalidTraversals, "o-", label=algo, color=ALGO_COLORS[algo])
    ax.set_xlabel("Number of vertices")
    ax.set_ylabel("Mean invalid traversals")
    ax.set_title("Invalid Traversals vs Vertex Count")
    ax.legend()
    savefig(fig, os.path.join(od, "invalid_traversals_vs_vertices.png"))

    # 13. Speed ratio (Oneconnected time / Biconnected time) per category
    pivot = df.groupby(["category", "algo"])["timeSeconds"].mean().unstack()
    if "Biconnected" in pivot.columns and "Oneconnected" in pivot.columns:
        pivot = pivot.dropna()
        ratio = pivot["Oneconnected"] / pivot["Biconnected"]
        fig, ax = plt.subplots(figsize=(12, 6))
        colors = ["#E76F51" if r > 1 else "#2E86AB" for r in ratio.values]
        ax.bar(range(len(ratio)), ratio.values, color=colors)
        ax.axhline(1.0, color="gray", linestyle="--", linewidth=1)
        ax.set_xticks(range(len(ratio)))
        ax.set_xticklabels(ratio.index, rotation=40, ha="right")
        ax.set_ylabel("Oneconnected time / Biconnected time")
        ax.set_title("Relative Slowdown of Oneconnected vs Biconnected by Category\n"
                      "(>1 = Oneconnected slower)")
        savefig(fig, os.path.join(od, "speed_ratio_oneconnected_vs_biconnected.png"))

    # 14. Total checks / successfulChecks / failedChecks stacked view vs vertices
    fig, axes = plt.subplots(1, 2, figsize=(14, 6), sharey=False)
    for ax, algo in zip(axes, ALGO_ORDER):
        sub = df[df.algo == algo].sort_values("vertices")
        if sub.empty:
            continue
        grp = sub.groupby("vertices")[["totalChecks", "successfulChecks", "failedChecks"]].mean().reset_index()
        ax.plot(grp.vertices, grp.totalChecks, "o-", label="totalChecks")
        ax.plot(grp.vertices, grp.successfulChecks, "s-", label="successfulChecks")
        ax.plot(grp.vertices, grp.failedChecks, "^-", label="failedChecks")
        ax.set_xlabel("Vertices")
        ax.set_ylabel("Count")
        ax.set_title(f"Checks Breakdown vs Vertices — {algo}")
        ax.legend()
    savefig(fig, os.path.join(od, "checks_breakdown_vs_vertices.png"))


# ----------------------------------------------------------------------------
# PER-CATEGORY GRAPHS (all cases of a category in one plot, algo comparison)
# ----------------------------------------------------------------------------

def plot_per_category(totals_df, individuals, categories, outdir):
    print("\n[per_category] generating per-category comparison graphs...")

    if totals_df.empty:
        print("  [warn] no total data loaded, skipping per-category graphs")
        return

    for category in categories:
        cat_df = totals_df[totals_df.category == category]
        if cat_df.empty:
            continue
        od = os.path.join(outdir, "per_category", category)

        # order cases naturally (case_001, case_002, ...)
        cases = sorted(cat_df["case"].unique(),
                       key=lambda s: (len(s), s))

        # 1. Per test case (x) vs total time (y), both algos
        fig, ax = plt.subplots(figsize=(max(10, len(cases) * 0.5), 6))
        x = np.arange(len(cases))
        width = 0.35
        for i, algo in enumerate(ALGO_ORDER):
            vals = []
            for c in cases:
                sub = cat_df[(cat_df.algo == algo) & (cat_df.case == c)]
                vals.append(sub.timeSeconds.mean() if not sub.empty else np.nan)
            ax.bar(x + (i - 0.5) * width, vals, width, label=algo, color=ALGO_COLORS[algo])
        ax.set_xticks(x)
        ax.set_xticklabels(cases, rotation=60, ha="right", fontsize=7)
        ax.set_ylabel("Total time (s)")
        ax.set_title(f"[{category}] Total Time per Test Case")
        ax.legend()
        savefig(fig, os.path.join(od, f"{category}_time_per_case.png"))

        # 2. Per test case vs avg time per triangulation, both algos (line)
        fig, ax = plt.subplots(figsize=(max(10, len(cases) * 0.4), 6))
        for algo in ALGO_ORDER:
            vals = []
            for c in cases:
                sub = cat_df[(cat_df.algo == algo) & (cat_df.case == c)]
                vals.append(sub.avgTimePerTriangulation.mean() if not sub.empty else np.nan)
            ax.plot(range(len(cases)), vals, "o-", label=algo, color=ALGO_COLORS[algo])
        ax.set_xticks(range(len(cases)))
        ax.set_xticklabels(cases, rotation=60, ha="right", fontsize=7)
        ax.set_ylabel("Avg time per triangulation (s)")
        ax.set_title(f"[{category}] Avg Time per Triangulation per Test Case")
        ax.legend()
        savefig(fig, os.path.join(od, f"{category}_avg_time_per_case.png"))

        # 3. Per test case vs triangulation count, both algos
        fig, ax = plt.subplots(figsize=(max(10, len(cases) * 0.4), 6))
        for algo in ALGO_ORDER:
            vals = []
            for c in cases:
                sub = cat_df[(cat_df.algo == algo) & (cat_df.case == c)]
                vals.append(sub.triangulations.mean() if not sub.empty else np.nan)
            ax.plot(range(len(cases)), vals, "o-", label=algo, color=ALGO_COLORS[algo])
        ax.set_xticks(range(len(cases)))
        ax.set_xticklabels(cases, rotation=60, ha="right", fontsize=7)
        ax.set_yscale("log")
        ax.set_ylabel("Triangulations (log scale)")
        ax.set_title(f"[{category}] Triangulation Count per Test Case")
        ax.legend()
        savefig(fig, os.path.join(od, f"{category}_triangulations_per_case.png"))

        # 4. Per test case vs peak memory, both algos
        fig, ax = plt.subplots(figsize=(max(10, len(cases) * 0.4), 6))
        for algo in ALGO_ORDER:
            vals = []
            for c in cases:
                sub = cat_df[(cat_df.algo == algo) & (cat_df.case == c)]
                vals.append(sub.peakMemoryBytes.mean() if not sub.empty else np.nan)
            ax.plot(range(len(cases)), vals, "o-", label=algo, color=ALGO_COLORS[algo])
        ax.set_xticks(range(len(cases)))
        ax.set_xticklabels(cases, rotation=60, ha="right", fontsize=7)
        ax.set_ylabel("Peak memory (bytes)")
        ax.set_title(f"[{category}] Peak Memory per Test Case")
        ax.legend()
        savefig(fig, os.path.join(od, f"{category}_memory_per_case.png"))

        # 5. Check success rate per case
        fig, ax = plt.subplots(figsize=(max(10, len(cases) * 0.4), 6))
        for algo in ALGO_ORDER:
            vals = []
            for c in cases:
                sub = cat_df[(cat_df.algo == algo) & (cat_df.case == c)]
                vals.append(sub.checkSuccessRate.mean() if not sub.empty else np.nan)
            ax.plot(range(len(cases)), vals, "o-", label=algo, color=ALGO_COLORS[algo])
        ax.set_xticks(range(len(cases)))
        ax.set_xticklabels(cases, rotation=60, ha="right", fontsize=7)
        ax.set_ylabel("Check success rate (%)")
        ax.set_title(f"[{category}] Check Success Rate per Test Case")
        ax.legend()
        savefig(fig, os.path.join(od, f"{category}_check_success_rate_per_case.png"))

        # 6. Traversal success rate per case ("how they behave")
        fig, ax = plt.subplots(figsize=(max(10, len(cases) * 0.4), 6))
        for algo in ALGO_ORDER:
            vals = []
            for c in cases:
                sub = cat_df[(cat_df.algo == algo) & (cat_df.case == c)]
                vals.append(sub.traversalSuccessRate.mean() if not sub.empty else np.nan)
            ax.plot(range(len(cases)), vals, "o-", label=algo, color=ALGO_COLORS[algo])
        ax.set_xticks(range(len(cases)))
        ax.set_xticklabels(cases, rotation=60, ha="right", fontsize=7)
        ax.set_ylabel("Traversal success rate (%)")
        ax.set_title(f"[{category}] Traversal Success Rate per Test Case")
        ax.legend()
        savefig(fig, os.path.join(od, f"{category}_traversal_success_rate_per_case.png"))

        # 7. Log-log: total time vs triangulations, within this category
        fig, ax = plt.subplots()
        for algo in ALGO_ORDER:
            sub = cat_df[cat_df.algo == algo]
            if sub.empty:
                continue
            ax.scatter(sub.triangulations, sub.timeSeconds, label=algo,
                       color=ALGO_COLORS[algo], alpha=0.7, s=30)
        ax.set_xscale("log")
        ax.set_yscale("log")
        ax.set_xlabel("Triangulations (log)")
        ax.set_ylabel("Total time (s, log)")
        ax.set_title(f"[{category}] Log-Log: Total Time vs Triangulations")
        ax.legend()
        savefig(fig, os.path.join(od, f"{category}_loglog_time_vs_triangulations.png"))


# ----------------------------------------------------------------------------
# PER-CASE GRAPHS (individual run data: one graph per case, both algos overlaid)
# ----------------------------------------------------------------------------

def plot_per_case(individuals, outdir):
    print("\n[per_case] generating per-case individual-run comparison graphs...")

    # group individuals by (category, case)
    grouped = defaultdict(dict)  # (category, case) -> {algo: df}
    for item in individuals:
        key = (item["category"], item["case"])
        grouped[key][item["algo"]] = item["df"]

    n_written = 0
    for (category, case), algo_dfs in sorted(grouped.items()):
        od = os.path.join(outdir, "per_case", category)

        # A) avg time per triangulation vs triangulation number (both algos)
        fig, ax = plt.subplots()
        any_data = False
        for algo in ALGO_ORDER:
            if algo not in algo_dfs:
                continue
            df = algo_dfs[algo]
            # average across runs, for each triangulation index
            grp = df.groupby("triangulation")["avgNsPerTri"].mean().reset_index()
            if grp.empty:
                continue
            ax.plot(grp.triangulation, grp.avgNsPerTri / 1000.0, "-", lw=1.2,
                    label=algo, color=ALGO_COLORS[algo])
            any_data = True
        if any_data:
            ax.set_xlabel("Triangulation number (n)")
            ax.set_ylabel("Avg time per triangulation up to n (\u00b5s)")
            ax.set_title(f"[{category}/{case}] Avg Time at Nth Triangulation")
            ax.legend()
            savefig(fig, os.path.join(od, f"{case}_avg_time_per_nth_triangulation.png"))
            n_written += 1
        else:
            plt.close(fig)

        # B) delta time (per-step cost) vs triangulation number (both algos)
        fig, ax = plt.subplots()
        any_data = False
        for algo in ALGO_ORDER:
            if algo not in algo_dfs:
                continue
            df = algo_dfs[algo]
            grp = df.groupby("triangulation")["deltaNs"].mean().reset_index()
            if grp.empty:
                continue
            ax.plot(grp.triangulation, grp.deltaNs / 1000.0, "-", lw=1.0, alpha=0.85,
                    label=algo, color=ALGO_COLORS[algo])
            any_data = True
        if any_data:
            ax.set_xlabel("Triangulation number (n)")
            ax.set_ylabel("Delta time for step n (\u00b5s)")
            ax.set_title(f"[{category}/{case}] Per-Step (Delta) Time vs Triangulation Number")
            ax.legend()
            savefig(fig, os.path.join(od, f"{case}_delta_time_per_step.png"))
        else:
            plt.close(fig)

        # C) cumulative time vs triangulation number, log-log (both algos)
        fig, ax = plt.subplots()
        any_data = False
        for algo in ALGO_ORDER:
            if algo not in algo_dfs:
                continue
            df = algo_dfs[algo]
            grp = df.groupby("triangulation")["cumulativeNs"].mean().reset_index()
            grp = grp[(grp.triangulation > 0) & (grp.cumulativeNs > 0)]
            if grp.empty:
                continue
            ax.plot(grp.triangulation, grp.cumulativeNs, "-", lw=1.2,
                    label=algo, color=ALGO_COLORS[algo])
            any_data = True
        if any_data:
            ax.set_xscale("log")
            ax.set_yscale("log")
            ax.set_xlabel("Triangulation number (log)")
            ax.set_ylabel("Cumulative time (ns, log)")
            ax.set_title(f"[{category}/{case}] Log-Log Cumulative Time vs Triangulation Number")
            ax.legend()
            savefig(fig, os.path.join(od, f"{case}_loglog_cumulative_time.png"))
        else:
            plt.close(fig)

        # D) Bar: average time per triangulation (single summary number per algo) for this case
        fig, ax = plt.subplots(figsize=(5, 5))
        labels, vals, colors = [], [], []
        for algo in ALGO_ORDER:
            if algo not in algo_dfs:
                continue
            df = algo_dfs[algo]
            last_per_run = df.sort_values("triangulation").groupby("run").tail(1)
            if last_per_run.empty:
                continue
            avg = (last_per_run["cumulativeNs"] / last_per_run["triangulation"]).mean()
            labels.append(algo)
            vals.append(avg / 1000.0)
            colors.append(ALGO_COLORS[algo])
        if vals:
            ax.bar(labels, vals, color=colors)
            ax.set_ylabel("Average time per triangulation (\u00b5s)")
            ax.set_title(f"[{category}/{case}] Avg Time per Triangulation (Summary)")
            savefig(fig, os.path.join(od, f"{case}_bar_avg_time_summary.png"))
        else:
            plt.close(fig)

    print(f"  {n_written} per-case 'avg time at nth triangulation' graphs written")


# ----------------------------------------------------------------------------
# Main
# ----------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(description="Generate Biconnected vs Oneconnected benchmark graphs.")
    ap.add_argument("--root", default="./benchmark-results",
                     help="Path to the benchmark-results root folder")
    ap.add_argument("--outdir", default="./graphs_output",
                     help="Directory to write generated graphs into")
    args = ap.parse_args()

    root = args.root
    outdir = args.outdir

    if not os.path.isdir(root):
        print(f"ERROR: root folder not found: {root}")
        sys.exit(1)

    print(f"Loading data from: {root}")
    totals_df, individuals, categories = load_all(root)
    totals_df = add_derived_columns(totals_df)

    print(f"\nLoaded {len(totals_df)} total-summary rows across {len(categories)} categories.")
    print(f"Loaded {len(individuals)} individual per-case run files.")

    if totals_df.empty and not individuals:
        print("\nNo data found. Check --root and folder layout (see script docstring).")
        sys.exit(1)

    os.makedirs(outdir, exist_ok=True)

    plot_overall(totals_df, individuals, outdir)
    plot_per_category(totals_df, individuals, categories, outdir)
    plot_per_case(individuals, outdir)

    print(f"\nAll graphs written under: {os.path.abspath(outdir)}")


if __name__ == "__main__":
    main()