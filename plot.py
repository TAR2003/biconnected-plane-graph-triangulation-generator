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
from scipy import stats as scipy_stats

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


def linreg(x, y):
    """Simple linear regression wrapper. Returns dict or None if insufficient data."""
    x = np.asarray(x, dtype=float)
    y = np.asarray(y, dtype=float)
    mask = np.isfinite(x) & np.isfinite(y)
    x, y = x[mask], y[mask]
    if len(x) < 2 or np.all(x == x[0]):
        return None
    res = scipy_stats.linregress(x, y)
    return {
        "slope": res.slope, "intercept": res.intercept,
        "r2": res.rvalue ** 2, "x": x, "y": y,
    }


def loglog_reg(x, y):
    """Linear regression in log-log space (fits y = a * x^b). Returns dict w/ slope=b, or None."""
    x = np.asarray(x, dtype=float)
    y = np.asarray(y, dtype=float)
    mask = np.isfinite(x) & np.isfinite(y) & (x > 0) & (y > 0)
    x, y = x[mask], y[mask]
    if len(x) < 2 or np.all(x == x[0]):
        return None
    res = scipy_stats.linregress(np.log10(x), np.log10(y))
    return {
        "slope": res.slope, "intercept": res.intercept,
        "r2": res.rvalue ** 2, "x": x, "y": y,
    }


def rolling_median_iqr(series, window):
    """Return (rolling_median, q25, q75) as pandas Series aligned to input."""
    roll = series.rolling(window, min_periods=max(3, window // 4), center=True)
    med = roll.median()
    q25 = roll.quantile(0.25)
    q75 = roll.quantile(0.75)
    return med, q25, q75


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

    # checks / traversals normalized per triangulation
    tri_safe = df["triangulations"].replace(0, np.nan)
    df["checksPerTriangulation"] = df["totalChecks"] / tri_safe
    df["failedChecksPerTriangulation"] = df["failedChecks"] / tri_safe
    df["traversalsPerTriangulation"] = df["totalTraversalsExtended"] / tri_safe

    # invalid traversal rate
    trav_safe = df["totalTraversalsExtended"].replace(0, np.nan)
    df["invalidTraversalRate"] = 100.0 * df["invalidTraversals"] / trav_safe

    # time per check / per traversal (overhead)
    checks_safe = df["totalChecks"].replace(0, np.nan)
    df["timePerCheck"] = df["timeSeconds"] / checks_safe
    df["timePerTraversal"] = df["timeSeconds"] / trav_safe

    # recomputed successful traversals from stored rate, for stacked breakdowns
    df["successfulTraversalsComputed"] = (
        df["totalTraversalsExtended"] * df["traversalSuccessRate"] / 100.0
    )

    # wall-clock duration from startTime/endTime, compared to reported timeSeconds
    try:
        start = pd.to_datetime(df["startTime"], errors="coerce")
        end = pd.to_datetime(df["endTime"], errors="coerce")
        df["wallDurationSeconds"] = (end - start).dt.total_seconds()
        # avoid div-by-zero / negative noise (many runs complete within the same
        # second, so wallDurationSeconds is often 0 -> ratio undefined, which is fine)
        df["timeSecondsToWallRatio"] = df["timeSeconds"] / df["wallDurationSeconds"].replace(0, np.nan)
    except Exception:
        df["wallDurationSeconds"] = np.nan
        df["timeSecondsToWallRatio"] = np.nan

    # sanity-check / identity-check derived columns
    df["memoryPerVertexRecomputed"] = df["peakMemoryBytes"] / df["vertices"].replace(0, np.nan)
    df["memoryIdentityDiff"] = df["memoryPerVertexRecomputed"] - df["memoryPerVertex"]

    total_checks_safe = df["totalChecks"].replace(0, np.nan)
    df["checkSuccessRateRecomputed"] = 100.0 * df["successfulChecks"] / total_checks_safe
    df["checkSuccessRateIdentityDiff"] = df["checkSuccessRateRecomputed"] - df["checkSuccessRate"]

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

    # 15. Peak memory vs vertices WITH linear fit + R^2 (direct evidence for "linear memory")
    fig, ax = plt.subplots()
    for algo in ALGO_ORDER:
        sub = df[df.algo == algo]
        if sub.empty:
            continue
        ax.scatter(sub.vertices, sub.peakMemoryBytes, color=ALGO_COLORS[algo],
                   alpha=0.5, s=20, label=f"{algo} (data)")
        fit = linreg(sub.vertices, sub.peakMemoryBytes)
        if fit:
            xs = np.linspace(sub.vertices.min(), sub.vertices.max(), 100)
            ys = fit["slope"] * xs + fit["intercept"]
            ax.plot(xs, ys, "--", color=ALGO_COLORS[algo], lw=2,
                    label=f"{algo} fit: y={fit['slope']:.1f}x+{fit['intercept']:.0f}, R\u00b2={fit['r2']:.3f}")
    ax.set_xlabel("Number of vertices")
    ax.set_ylabel("Peak memory (bytes)")
    ax.set_title("Peak Memory vs Vertices — Linear Fit (evidence for O(n) memory)")
    ax.legend(fontsize=8)
    savefig(fig, os.path.join(od, "memory_vs_vertices_linear_fit.png"))

    # 15b. Residuals of the linear memory fit (should look like random scatter around 0)
    fig, ax = plt.subplots()
    for algo in ALGO_ORDER:
        sub = df[df.algo == algo]
        if sub.empty:
            continue
        fit = linreg(sub.vertices, sub.peakMemoryBytes)
        if fit:
            pred = fit["slope"] * fit["x"] + fit["intercept"]
            resid = fit["y"] - pred
            ax.scatter(fit["x"], resid, color=ALGO_COLORS[algo], alpha=0.6, s=20, label=algo)
    ax.axhline(0, color="gray", linestyle="--", lw=1)
    ax.set_xlabel("Number of vertices")
    ax.set_ylabel("Residual (actual - fitted memory, bytes)")
    ax.set_title("Residuals of Linear Memory Fit")
    ax.legend()
    savefig(fig, os.path.join(od, "memory_vs_vertices_residuals.png"))

    # 16. Log-log peak memory vs vertices WITH fitted slope (slope ~1 => linear scaling)
    fig, ax = plt.subplots()
    for algo in ALGO_ORDER:
        sub = df[df.algo == algo]
        if sub.empty:
            continue
        ax.scatter(sub.vertices, sub.peakMemoryBytes, color=ALGO_COLORS[algo],
                   alpha=0.5, s=20, label=f"{algo} (data)")
        fit = loglog_reg(sub.vertices, sub.peakMemoryBytes)
        if fit:
            xs = np.logspace(np.log10(fit["x"].min()), np.log10(fit["x"].max()), 100)
            ys = (10 ** fit["intercept"]) * xs ** fit["slope"]
            ax.plot(xs, ys, "--", color=ALGO_COLORS[algo], lw=2,
                    label=f"{algo} slope={fit['slope']:.2f}, R\u00b2={fit['r2']:.3f}")
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("Vertices (log)")
    ax.set_ylabel("Peak memory (bytes, log)")
    ax.set_title("Log-Log Memory vs Vertices (slope \u2248 1 \u21d2 linear memory)")
    ax.legend(fontsize=8)
    savefig(fig, os.path.join(od, "loglog_memory_vs_vertices_with_slope.png"))

    # 17. Memory per vertex distribution (boxplot) per algo
    fig, ax = plt.subplots()
    data = [df[df.algo == a]["memoryPerVertex"].dropna() for a in ALGO_ORDER]
    bp = ax.boxplot(data, labels=ALGO_ORDER, patch_artist=True)
    for patch, algo in zip(bp["boxes"], ALGO_ORDER):
        patch.set_facecolor(ALGO_COLORS[algo])
        patch.set_alpha(0.6)
    ax.set_ylabel("Memory per vertex (bytes)")
    ax.set_title("Memory-per-Vertex Distribution by Algorithm\n(flat/constant = linear memory)")
    savefig(fig, os.path.join(od, "boxplot_memory_per_vertex.png"))

    # 18. Regression slope summary: time vs triangulations, and time vs vertices
    fig, axes = plt.subplots(1, 2, figsize=(13, 5.5))
    for algo in ALGO_ORDER:
        sub = df[df.algo == algo]
        if sub.empty:
            continue
        fit1 = linreg(sub.triangulations, sub.timeSeconds)
        axes[0].scatter(sub.triangulations, sub.timeSeconds, color=ALGO_COLORS[algo], alpha=0.4, s=15)
        if fit1:
            xs = np.linspace(sub.triangulations.min(), sub.triangulations.max(), 100)
            axes[0].plot(xs, fit1["slope"] * xs + fit1["intercept"], "--", color=ALGO_COLORS[algo], lw=2,
                         label=f"{algo}: slope={fit1['slope']:.2e} s/tri, R\u00b2={fit1['r2']:.3f}")
        fit2 = linreg(sub.vertices, sub.timeSeconds)
        axes[1].scatter(sub.vertices, sub.timeSeconds, color=ALGO_COLORS[algo], alpha=0.4, s=15)
        if fit2:
            xs = np.linspace(sub.vertices.min(), sub.vertices.max(), 100)
            axes[1].plot(xs, fit2["slope"] * xs + fit2["intercept"], "--", color=ALGO_COLORS[algo], lw=2,
                         label=f"{algo}: slope={fit2['slope']:.2e} s/vertex, R\u00b2={fit2['r2']:.3f}")
    axes[0].set_xlabel("Triangulations")
    axes[0].set_ylabel("Total time (s)")
    axes[0].set_title("Time vs Triangulations (fit)")
    axes[0].legend(fontsize=8)
    axes[1].set_xlabel("Vertices")
    axes[1].set_ylabel("Total time (s)")
    axes[1].set_title("Time vs Vertices (fit)")
    axes[1].legend(fontsize=8)
    savefig(fig, os.path.join(od, "regression_slopes_time.png"))

    # 19. Avg time per triangulation vs vertices, LOG-LOG (flat = amortized constant across scales)
    fig, ax = plt.subplots()
    for algo in ALGO_ORDER:
        sub = df[df.algo == algo].sort_values("vertices")
        if sub.empty:
            continue
        grp = sub.groupby("vertices")["avgTimePerTriangulation"].mean().reset_index()
        ax.plot(grp.vertices, grp.avgTimePerTriangulation, "o-", label=algo, color=ALGO_COLORS[algo])
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("Vertices (log)")
    ax.set_ylabel("Avg time per triangulation (s, log)")
    ax.set_title("Log-Log Amortized Time per Triangulation vs Vertices")
    ax.legend()
    savefig(fig, os.path.join(od, "loglog_avg_time_per_triangulation_vs_vertices.png"))

    # 20. Avg time per triangulation vs triangulations (should be flat if time depends on
    #     triangulation count alone, not vertex count)
    fig, ax = plt.subplots()
    for algo in ALGO_ORDER:
        sub = df[df.algo == algo]
        if sub.empty:
            continue
        ax.scatter(sub.triangulations, sub.avgTimePerTriangulation, color=ALGO_COLORS[algo],
                   alpha=0.5, s=20, label=algo)
    ax.set_xscale("log")
    ax.set_xlabel("Triangulations (log)")
    ax.set_ylabel("Avg time per triangulation (s)")
    ax.set_title("Avg Time per Triangulation vs Total Triangulations")
    ax.legend()
    savefig(fig, os.path.join(od, "avg_time_per_triangulation_vs_triangulations.png"))

    # 21. Checks / traversals per triangulation vs vertices
    fig, axes = plt.subplots(1, 2, figsize=(13, 5.5))
    for algo in ALGO_ORDER:
        sub = df[df.algo == algo].sort_values("vertices")
        if sub.empty:
            continue
        grp = sub.groupby("vertices")[["checksPerTriangulation", "traversalsPerTriangulation"]].mean().reset_index()
        axes[0].plot(grp.vertices, grp.checksPerTriangulation, "o-", label=algo, color=ALGO_COLORS[algo])
        axes[1].plot(grp.vertices, grp.traversalsPerTriangulation, "o-", label=algo, color=ALGO_COLORS[algo])
    axes[0].set_xlabel("Vertices")
    axes[0].set_ylabel("Checks per triangulation")
    axes[0].set_title("Checks per Triangulation vs Vertices")
    axes[0].legend()
    axes[1].set_xlabel("Vertices")
    axes[1].set_ylabel("Traversals per triangulation")
    axes[1].set_title("Traversals per Triangulation vs Vertices")
    axes[1].legend()
    savefig(fig, os.path.join(od, "checks_traversals_per_triangulation_vs_vertices.png"))

    # 22. Invalid traversal rate vs vertices
    fig, ax = plt.subplots()
    for algo in ALGO_ORDER:
        sub = df[df.algo == algo].sort_values("vertices")
        if sub.empty:
            continue
        grp = sub.groupby("vertices")["invalidTraversalRate"].mean().reset_index()
        ax.plot(grp.vertices, grp.invalidTraversalRate, "o-", label=algo, color=ALGO_COLORS[algo])
    ax.set_xlabel("Vertices")
    ax.set_ylabel("Invalid traversal rate (%)")
    ax.set_title("Invalid Traversal Rate vs Vertices")
    ax.legend()
    savefig(fig, os.path.join(od, "invalid_traversal_rate_vs_vertices.png"))

    # 23. Time per check and time per traversal vs vertices (stable overhead check)
    fig, axes = plt.subplots(1, 2, figsize=(13, 5.5))
    for algo in ALGO_ORDER:
        sub = df[df.algo == algo].sort_values("vertices")
        if sub.empty:
            continue
        grp = sub.groupby("vertices")[["timePerCheck", "timePerTraversal"]].mean().reset_index()
        axes[0].plot(grp.vertices, grp.timePerCheck, "o-", label=algo, color=ALGO_COLORS[algo])
        axes[1].plot(grp.vertices, grp.timePerTraversal, "o-", label=algo, color=ALGO_COLORS[algo])
    axes[0].set_xlabel("Vertices")
    axes[0].set_ylabel("Time per check (s)")
    axes[0].set_title("Time per Check vs Vertices")
    axes[0].legend()
    axes[1].set_xlabel("Vertices")
    axes[1].set_ylabel("Time per traversal (s)")
    axes[1].set_title("Time per Traversal vs Vertices")
    axes[1].legend()
    savefig(fig, os.path.join(od, "time_per_check_and_traversal_vs_vertices.png"))

    # 24. checkSuccessRate vs traversalSuccessRate scatter (do correctness metrics correlate?)
    fig, ax = plt.subplots()
    for algo in ALGO_ORDER:
        sub = df[df.algo == algo]
        if sub.empty:
            continue
        ax.scatter(sub.checkSuccessRate, sub.traversalSuccessRate, color=ALGO_COLORS[algo],
                   alpha=0.5, s=20, label=algo)
    ax.set_xlabel("Check success rate (%)")
    ax.set_ylabel("Traversal success rate (%)")
    ax.set_title("Check Success Rate vs Traversal Success Rate")
    ax.legend()
    savefig(fig, os.path.join(od, "check_vs_traversal_success_rate.png"))

    # 25. Status distribution per algo
    if "status" in df.columns:
        fig, ax = plt.subplots()
        statuses = sorted(df.status.dropna().unique())
        x = np.arange(len(statuses))
        width = 0.35
        for i, algo in enumerate(ALGO_ORDER):
            counts = [len(df[(df.algo == algo) & (df.status == s)]) for s in statuses]
            ax.bar(x + (i - 0.5) * width, counts, width, label=algo, color=ALGO_COLORS[algo])
        ax.set_xticks(x)
        ax.set_xticklabels(statuses, rotation=20, ha="right")
        ax.set_ylabel("Run count")
        ax.set_title("Run Status Distribution by Algorithm")
        ax.legend()
        savefig(fig, os.path.join(od, "status_distribution.png"))

    # 26. Data-quality identity checks: memoryPerVertex recompute, checkSuccessRate recompute
    fig, axes = plt.subplots(1, 2, figsize=(13, 5.5))
    for algo in ALGO_ORDER:
        sub = df[df.algo == algo]
        if sub.empty:
            continue
        axes[0].scatter(sub.vertices, sub.memoryIdentityDiff, color=ALGO_COLORS[algo],
                        alpha=0.5, s=15, label=algo)
        axes[1].scatter(sub.vertices, sub.checkSuccessRateIdentityDiff, color=ALGO_COLORS[algo],
                        alpha=0.5, s=15, label=algo)
    axes[0].axhline(0, color="gray", linestyle="--", lw=1)
    axes[0].set_xlabel("Vertices")
    axes[0].set_ylabel("Recomputed - stored memoryPerVertex")
    axes[0].set_title("Data-Quality Check: memoryPerVertex")
    axes[0].legend()
    axes[1].axhline(0, color="gray", linestyle="--", lw=1)
    axes[1].set_xlabel("Vertices")
    axes[1].set_ylabel("Recomputed - stored checkSuccessRate (%)")
    axes[1].set_title("Data-Quality Check: checkSuccessRate")
    axes[1].legend()
    savefig(fig, os.path.join(od, "identity_sanity_checks.png"))

    # 27. timeSeconds vs wall-clock duration ratio (CPU time vs wall time sanity check)
    valid_wall = df[df.wallDurationSeconds.notna() & (df.wallDurationSeconds > 0)]
    if not valid_wall.empty:
        fig, ax = plt.subplots()
        for algo in ALGO_ORDER:
            sub = valid_wall[valid_wall.algo == algo]
            if sub.empty:
                continue
            ax.scatter(sub.vertices, sub.timeSecondsToWallRatio, color=ALGO_COLORS[algo],
                       alpha=0.5, s=20, label=algo)
        ax.axhline(1.0, color="gray", linestyle="--", lw=1)
        ax.set_xlabel("Vertices")
        ax.set_ylabel("timeSeconds / wall-clock duration")
        ax.set_title("Reported Time vs Wall-Clock Duration Ratio")
        ax.legend()
        savefig(fig, os.path.join(od, "time_vs_wallclock_ratio.png"))
    else:
        print("  [info] startTime/endTime give zero or invalid duration for all rows "
              "(likely sub-second runs); skipping time_vs_wallclock_ratio.png")

    # 28. Correlation heatmap of numeric columns, per algo
    numeric_cols = ["vertices", "triangulations", "timeSeconds", "peakMemoryBytes",
                     "memoryPerVertex", "totalChecks", "successfulChecks", "failedChecks",
                     "checkSuccessRate", "invalidTraversals", "totalTraversalsExtended",
                     "traversalSuccessRate"]
    numeric_cols = [c for c in numeric_cols if c in df.columns]
    fig, axes = plt.subplots(1, 2, figsize=(16, 7))
    for ax, algo in zip(axes, ALGO_ORDER):
        sub = df[df.algo == algo][numeric_cols]
        if sub.empty or len(sub) < 2:
            ax.set_title(f"{algo} (insufficient data)")
            continue
        corr = sub.corr()
        im = ax.imshow(corr.values, cmap="coolwarm", vmin=-1, vmax=1)
        ax.set_xticks(range(len(numeric_cols)))
        ax.set_xticklabels(numeric_cols, rotation=90, fontsize=7)
        ax.set_yticks(range(len(numeric_cols)))
        ax.set_yticklabels(numeric_cols, fontsize=7)
        ax.set_title(f"Correlation Heatmap — {algo}")
        fig.colorbar(im, ax=ax, fraction=0.046, pad=0.04)
    savefig(fig, os.path.join(od, "correlation_heatmap.png"))

    # 29. Paired scatter: Oneconnected vs Biconnected for matched (category, case) —
    #     time, memory, checks, traversals, with identity line
    pivot_cols = ["timeSeconds", "peakMemoryBytes", "totalChecks", "totalTraversalsExtended"]
    merged = None
    bi = df[df.algo == "Biconnected"].groupby(["category", "case"])[pivot_cols].mean().reset_index()
    one = df[df.algo == "Oneconnected"].groupby(["category", "case"])[pivot_cols].mean().reset_index()
    merged = bi.merge(one, on=["category", "case"], suffixes=("_bi", "_one"))
    if not merged.empty:
        fig, axes = plt.subplots(2, 2, figsize=(12, 11))
        titles = ["Total time (s)", "Peak memory (bytes)", "Total checks", "Total traversals"]
        for ax, col, title in zip(axes.flat, pivot_cols, titles):
            xb, yo = merged[f"{col}_bi"], merged[f"{col}_one"]
            ax.scatter(xb, yo, alpha=0.6, s=25, color="#6A4C93")
            lim = [min(xb.min(), yo.min()), max(xb.max(), yo.max())]
            ax.plot(lim, lim, "--", color="gray", lw=1, label="identity (y=x)")
            ax.set_xlabel(f"Biconnected {title}")
            ax.set_ylabel(f"Oneconnected {title}")
            ax.set_title(title)
            ax.legend(fontsize=8)
        fig.suptitle("Paired Comparison: Oneconnected vs Biconnected (matched cases)", fontweight="bold")
        savefig(fig, os.path.join(od, "paired_scatter_oneconnected_vs_biconnected.png"))

        # 29b. Bland-Altman style difference plots for the same paired metrics
        fig, axes = plt.subplots(2, 2, figsize=(12, 11))
        for ax, col, title in zip(axes.flat, pivot_cols, titles):
            xb, yo = merged[f"{col}_bi"], merged[f"{col}_one"]
            mean = (xb + yo) / 2
            diff = yo - xb
            ax.scatter(mean, diff, alpha=0.6, s=25, color="#F4A261")
            mdiff = diff.mean()
            sdiff = diff.std()
            ax.axhline(mdiff, color="black", lw=1, label=f"mean diff={mdiff:.3g}")
            ax.axhline(mdiff + 1.96 * sdiff, color="gray", linestyle="--", lw=1)
            ax.axhline(mdiff - 1.96 * sdiff, color="gray", linestyle="--", lw=1)
            ax.set_xlabel(f"Mean of Bi/One {title}")
            ax.set_ylabel(f"Oneconnected - Biconnected {title}")
            ax.set_title(title)
            ax.legend(fontsize=8)
        fig.suptitle("Bland\u2013Altman: Agreement Between Oneconnected and Biconnected", fontweight="bold")
        savefig(fig, os.path.join(od, "bland_altman_oneconnected_vs_biconnected.png"))

    # 30. Triangulation count ratio per case (Oneconnected / Biconnected) — should be
    #     ~1 if both algos generate the same set of triangulations
    tri_pivot = df.groupby(["category", "case", "algo"])["triangulations"].mean().unstack()
    if "Biconnected" in tri_pivot.columns and "Oneconnected" in tri_pivot.columns:
        tp = tri_pivot.dropna()
        ratio = tp["Oneconnected"] / tp["Biconnected"]
        fig, ax = plt.subplots(figsize=(max(10, len(ratio) * 0.3), 6))
        colors = ["#E76F51" if abs(r - 1) > 0.01 else "#2E86AB" for r in ratio.values]
        ax.bar(range(len(ratio)), ratio.values, color=colors)
        ax.axhline(1.0, color="gray", linestyle="--", lw=1, label="ratio = 1 (identical count)")
        labels = [f"{c}/{k}" for c, k in ratio.index]
        ax.set_xticks(range(len(ratio)))
        ax.set_xticklabels(labels, rotation=75, ha="right", fontsize=6)
        ax.set_ylabel("Oneconnected triangulations / Biconnected triangulations")
        ax.set_title("Triangulation Count Ratio per Case\n(deviation from 1 = coverage mismatch)")
        ax.legend()
        savefig(fig, os.path.join(od, "triangulation_count_ratio_per_case.png"))

    # 31. Run-to-run variability: timeSeconds vs runIndex, aggregated across all cases
    if "runIndex" in df.columns:
        fig, ax = plt.subplots()
        for algo in ALGO_ORDER:
            sub = df[df.algo == algo]
            if sub.empty:
                continue
            grp = sub.groupby("runIndex")["timeSeconds"].mean().reset_index()
            ax.plot(grp.runIndex, grp.timeSeconds, "o-", label=algo, color=ALGO_COLORS[algo])
        ax.set_xlabel("Run index")
        ax.set_ylabel("Mean total time (s)")
        ax.set_title("Time vs Run Index (warmup / caching effects)")
        ax.legend()
        savefig(fig, os.path.join(od, "time_vs_run_index.png"))

    # 32. Total time and total triangulations and peak memory per category per algo
    #     (grouped bars, in addition to the existing avg-time-per-triangulation bar)
    cats_sorted = sorted(df.category.unique())
    x = np.arange(len(cats_sorted))
    width = 0.35
    for metric, ylabel, fname in [
        ("timeSeconds", "Total time (s)", "bar_total_time_by_category.png"),
        ("triangulations", "Total triangulations", "bar_total_triangulations_by_category.png"),
        ("peakMemoryBytes", "Peak memory (bytes)", "bar_peak_memory_by_category.png"),
    ]:
        fig, ax = plt.subplots(figsize=(12, 6))
        for i, algo in enumerate(ALGO_ORDER):
            means, stds = [], []
            for c in cats_sorted:
                sub = df[(df.algo == algo) & (df.category == c)][metric]
                means.append(sub.mean() if not sub.empty else 0)
                stds.append(sub.std() if not sub.empty else 0)
            ax.bar(x + (i - 0.5) * width, means, width, yerr=stds, capsize=3,
                   label=algo, color=ALGO_COLORS[algo])
        ax.set_xticks(x)
        ax.set_xticklabels(cats_sorted, rotation=40, ha="right")
        ax.set_ylabel(ylabel)
        ax.set_title(f"{ylabel} by Category (mean \u00b1 std)")
        ax.legend()
        savefig(fig, os.path.join(od, fname))

    # 33. Boxplots/violins of avgTimePerTriangulation and memoryPerVertex, overall
    fig, axes = plt.subplots(1, 2, figsize=(12, 5.5))
    data_t = [df[df.algo == a]["avgTimePerTriangulation"].dropna() for a in ALGO_ORDER]
    bp = axes[0].boxplot(data_t, labels=ALGO_ORDER, patch_artist=True)
    for patch, algo in zip(bp["boxes"], ALGO_ORDER):
        patch.set_facecolor(ALGO_COLORS[algo])
        patch.set_alpha(0.6)
    axes[0].set_ylabel("Avg time per triangulation (s)")
    axes[0].set_title("Distribution: Avg Time per Triangulation")

    data_m = [df[df.algo == a]["memoryPerVertex"].dropna() for a in ALGO_ORDER]
    bp2 = axes[1].boxplot(data_m, labels=ALGO_ORDER, patch_artist=True)
    for patch, algo in zip(bp2["boxes"], ALGO_ORDER):
        patch.set_facecolor(ALGO_COLORS[algo])
        patch.set_alpha(0.6)
    axes[1].set_ylabel("Memory per vertex (bytes)")
    axes[1].set_title("Distribution: Memory per Vertex")
    savefig(fig, os.path.join(od, "boxplots_time_memory_distributions.png"))

    # 34. Heatmap of category x metric, normalized per algo (executive summary view)
    metrics_for_heatmap = ["timeSeconds", "avgTimePerTriangulation", "peakMemoryBytes",
                            "checkSuccessRate", "traversalSuccessRate"]
    fig, axes = plt.subplots(1, 2, figsize=(11, max(4, len(cats_sorted) * 0.5)))
    for ax, algo in zip(axes, ALGO_ORDER):
        sub = df[df.algo == algo]
        if sub.empty:
            ax.set_title(f"{algo} (no data)")
            continue
        pivot = sub.groupby("category")[metrics_for_heatmap].mean().reindex(cats_sorted)
        # normalize each column 0-1 for comparability
        normed = (pivot - pivot.min()) / (pivot.max() - pivot.min()).replace(0, 1)
        im = ax.imshow(normed.values, cmap="viridis", aspect="auto")
        ax.set_xticks(range(len(metrics_for_heatmap)))
        ax.set_xticklabels(metrics_for_heatmap, rotation=45, ha="right", fontsize=7)
        ax.set_yticks(range(len(cats_sorted)))
        ax.set_yticklabels(cats_sorted, fontsize=7)
        ax.set_title(f"{algo}: Category \u00d7 Metric (normalized)")
        fig.colorbar(im, ax=ax, fraction=0.046, pad=0.04)
    savefig(fig, os.path.join(od, "heatmap_category_metric.png"))


# ----------------------------------------------------------------------------
# ALGO-SCOPE LOG-LOG GRAPHS
# One plot per algorithm ("all Biconnected inputs" / "all Oneconnected inputs"),
# each showing points from BOTH algorithms (differently colored) so you can see
# how one algorithm's whole input set compares against the other's, all at once.
# ----------------------------------------------------------------------------

def plot_algo_scope_loglog(totals_df, outdir):
    print("\n[algo_scope] generating per-algorithm 'all inputs' log-log overlays...")
    od = os.path.join(outdir, "overall")

    if totals_df.empty:
        print("  [warn] no total data loaded, skipping algo-scope graphs")
        return

    df = totals_df

    # metric pairs we show as log-log scatter: (x, y, xlabel, ylabel, fname_suffix)
    metric_pairs = [
        ("vertices", "timeSeconds", "Vertices", "Total time (s)", "time_vs_vertices"),
        ("vertices", "peakMemoryBytes", "Vertices", "Peak memory (bytes)", "memory_vs_vertices"),
        ("triangulations", "timeSeconds", "Triangulations", "Total time (s)", "time_vs_triangulations"),
        ("vertices", "avgTimePerTriangulation", "Vertices", "Avg time per triangulation (s)", "avgtime_vs_vertices"),
    ]

    # For each "focus" algo, plot ALL points from BOTH algos (different colors),
    # but title/name the file after the focus algo, and draw the focus algo's
    # points on top / with fuller opacity so it reads as "this algo's full input set,
    # with the other algo shown for context".
    for focus_algo in ALGO_ORDER:
        other_algo = [a for a in ALGO_ORDER if a != focus_algo][0]
        focus_od = os.path.join(od, f"{focus_algo.lower()}_all_inputs")

        for xcol, ycol, xlabel, ylabel, suffix in metric_pairs:
            fig, ax = plt.subplots()
            # background: other algo, lower alpha
            sub_other = df[df.algo == other_algo]
            if not sub_other.empty:
                ax.scatter(sub_other[xcol], sub_other[ycol], color=ALGO_COLORS[other_algo],
                           alpha=0.25, s=18, label=f"{other_algo} (context)")
            # foreground: focus algo, full alpha, every input point across every category/case
            sub_focus = df[df.algo == focus_algo]
            if not sub_focus.empty:
                ax.scatter(sub_focus[xcol], sub_focus[ycol], color=ALGO_COLORS[focus_algo],
                           alpha=0.75, s=22, label=f"{focus_algo} (all inputs)")
            ax.set_xscale("log")
            ax.set_yscale("log")
            ax.set_xlabel(f"{xlabel} (log)")
            ax.set_ylabel(f"{ylabel} (log)")
            ax.set_title(f"All {focus_algo} Inputs — {ylabel} vs {xlabel} (log-log)\n"
                         f"({other_algo} shown for context)")
            ax.legend()
            savefig(fig, os.path.join(focus_od, f"{focus_algo.lower()}_all_inputs_loglog_{suffix}.png"))

    # Also produce one "both fully overlaid, equal weight" version per metric pair,
    # living directly under overall/ — this is the direct "overall biconnected inputs
    # and overall oneconnected inputs, same plot, different colors" view.
    for xcol, ycol, xlabel, ylabel, suffix in metric_pairs:
        fig, ax = plt.subplots()
        for algo in ALGO_ORDER:
            sub = df[df.algo == algo]
            if sub.empty:
                continue
            ax.scatter(sub[xcol], sub[ycol], color=ALGO_COLORS[algo], alpha=0.55, s=20, label=algo)
        ax.set_xscale("log")
        ax.set_yscale("log")
        ax.set_xlabel(f"{xlabel} (log)")
        ax.set_ylabel(f"{ylabel} (log)")
        ax.set_title(f"All Inputs, Both Algorithms — {ylabel} vs {xlabel} (log-log)")
        ax.legend()
        savefig(fig, os.path.join(od, f"both_algos_all_inputs_loglog_{suffix}.png"))


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

        # 8. Small multiples: scaling relationships WITHIN this category (per doc section 6)
        #    Using per-run rows (vertices vary within a category across cases), so these
        #    show whether the claims hold when isolated to this category's vertex range.
        scaling_metrics = [
            ("peakMemoryBytes", "Peak memory (bytes)", "memory_vs_vertices", False),
            ("timeSeconds", "Total time (s)", "time_vs_vertices", False),
            ("avgTimePerTriangulation", "Avg time per triangulation (s)", "avgtime_vs_vertices", False),
            ("totalChecks", "Total checks", "checks_vs_vertices", False),
            ("failedChecks", "Failed checks", "failedchecks_vs_vertices", False),
            ("invalidTraversals", "Invalid traversals", "invalidtrav_vs_vertices", False),
        ]
        fig, axes = plt.subplots(2, 3, figsize=(16, 9))
        for ax, (col, ylabel, _, _) in zip(axes.flat, scaling_metrics):
            for algo in ALGO_ORDER:
                sub = cat_df[cat_df.algo == algo].sort_values("vertices")
                if sub.empty:
                    continue
                grp = sub.groupby("vertices")[col].mean().reset_index()
                ax.plot(grp.vertices, grp[col], "o-", label=algo, color=ALGO_COLORS[algo])
            ax.set_xlabel("Vertices")
            ax.set_ylabel(ylabel)
            ax.set_title(ylabel)
            ax.legend(fontsize=8)
        fig.suptitle(f"[{category}] Scaling Relationships vs Vertices (within-category)", fontweight="bold")
        savefig(fig, os.path.join(od, f"{category}_scaling_small_multiples.png"))

        # 9. Log-log peak memory vs vertices, within this category (slope ~1 check per category)
        fig, ax = plt.subplots()
        for algo in ALGO_ORDER:
            sub = cat_df[cat_df.algo == algo]
            if sub.empty:
                continue
            ax.scatter(sub.vertices, sub.peakMemoryBytes, color=ALGO_COLORS[algo], alpha=0.5, s=20)
            fit = loglog_reg(sub.vertices, sub.peakMemoryBytes)
            if fit:
                xs = np.logspace(np.log10(fit["x"].min()), np.log10(max(fit["x"].max(), fit["x"].min()*1.01)), 50)
                ys = (10 ** fit["intercept"]) * xs ** fit["slope"]
                ax.plot(xs, ys, "--", color=ALGO_COLORS[algo], lw=2,
                        label=f"{algo} slope={fit['slope']:.2f}")
        ax.set_xscale("log")
        ax.set_yscale("log")
        ax.set_xlabel("Vertices (log)")
        ax.set_ylabel("Peak memory (bytes, log)")
        ax.set_title(f"[{category}] Log-Log Memory vs Vertices")
        ax.legend(fontsize=8)
        savefig(fig, os.path.join(od, f"{category}_loglog_memory_vs_vertices.png"))

        # 10. Run-to-run variability within this category: boxplot of timeSeconds per case
        fig, ax = plt.subplots(figsize=(max(10, len(cases) * 0.5), 6))
        positions = []
        box_data = []
        box_colors = []
        pos = 0
        tick_pos = []
        tick_labels = []
        for c in cases:
            group_start = pos
            for algo in ALGO_ORDER:
                sub = cat_df[(cat_df.algo == algo) & (cat_df.case == c)]["timeSeconds"].dropna()
                if sub.empty:
                    continue
                box_data.append(sub.values)
                positions.append(pos)
                box_colors.append(ALGO_COLORS[algo])
                pos += 1
            tick_pos.append((group_start + pos - 1) / 2)
            tick_labels.append(c)
            pos += 1  # gap between cases
        if box_data:
            bp = ax.boxplot(box_data, positions=positions, widths=0.7, patch_artist=True)
            for patch, color in zip(bp["boxes"], box_colors):
                patch.set_facecolor(color)
                patch.set_alpha(0.6)
            ax.set_xticks(tick_pos)
            ax.set_xticklabels(tick_labels, rotation=60, ha="right", fontsize=7)
            ax.set_ylabel("Total time per run (s)")
            ax.set_title(f"[{category}] Run-to-Run Time Variability per Case\n(blue=Biconnected, orange=Oneconnected)")
            savefig(fig, os.path.join(od, f"{category}_run_variability_boxplot.png"))
        else:
            plt.close(fig)


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

        # A2) SAME plot as above, but log-log (as requested): makes it clear whether
        #     the curve is truly flat (amortized constant) across orders of magnitude.
        fig, ax = plt.subplots()
        any_data = False
        for algo in ALGO_ORDER:
            if algo not in algo_dfs:
                continue
            df = algo_dfs[algo]
            grp = df.groupby("triangulation")["avgNsPerTri"].mean().reset_index()
            grp = grp[(grp.triangulation > 0) & (grp.avgNsPerTri > 0)]
            if grp.empty:
                continue
            ax.plot(grp.triangulation, grp.avgNsPerTri / 1000.0, "-", lw=1.2,
                    label=algo, color=ALGO_COLORS[algo])
            any_data = True
        if any_data:
            ax.set_xscale("log")
            ax.set_yscale("log")
            ax.set_xlabel("Triangulation number (n, log)")
            ax.set_ylabel("Avg time per triangulation up to n (\u00b5s, log)")
            ax.set_title(f"[{category}/{case}] Log-Log: Avg Time at Nth Triangulation")
            ax.legend()
            savefig(fig, os.path.join(od, f"{case}_loglog_avg_time_per_nth_triangulation.png"))
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

        # B2) delta time with rolling median + shaded IQR band (stability of per-step cost)
        fig, axes = plt.subplots(1, 2, figsize=(14, 5.5), sharey=False)
        any_data = False
        for ax, algo in zip(axes, ALGO_ORDER):
            if algo not in algo_dfs:
                ax.set_title(f"{algo} (no data)")
                continue
            df = algo_dfs[algo]
            grp = df.groupby("triangulation")["deltaNs"].mean().sort_index()
            if grp.empty:
                continue
            window = max(5, len(grp) // 20)
            med, q25, q75 = rolling_median_iqr(grp, window)
            ax.plot(grp.index, grp.values / 1000.0, color=ALGO_COLORS[algo], alpha=0.25, lw=0.6,
                    label="raw delta")
            ax.plot(grp.index, med / 1000.0, color=ALGO_COLORS[algo], lw=1.8, label="rolling median")
            ax.fill_between(grp.index, q25 / 1000.0, q75 / 1000.0, color=ALGO_COLORS[algo], alpha=0.2,
                            label="IQR band")
            ax.set_xlabel("Triangulation number (n)")
            ax.set_ylabel("Delta time (\u00b5s)")
            ax.set_title(algo)
            ax.legend(fontsize=8)
            any_data = True
        if any_data:
            fig.suptitle(f"[{category}/{case}] Delta-Time Stability (rolling median \u00b1 IQR)", fontweight="bold")
            savefig(fig, os.path.join(od, f"{case}_delta_time_rolling_iqr.png"))
        else:
            plt.close(fig)

        # B3) delta time histogram per algo (stable distribution = amortized constant)
        fig, ax = plt.subplots()
        any_data = False
        for algo in ALGO_ORDER:
            if algo not in algo_dfs:
                continue
            df = algo_dfs[algo]
            vals = (df["deltaNs"] / 1000.0).dropna()
            if vals.empty:
                continue
            ax.hist(vals, bins=40, alpha=0.5, label=algo, color=ALGO_COLORS[algo], density=True)
            any_data = True
        if any_data:
            ax.set_xlabel("Delta time per step (\u00b5s)")
            ax.set_ylabel("Density")
            ax.set_title(f"[{category}/{case}] Delta-Time Distribution")
            ax.legend()
            savefig(fig, os.path.join(od, f"{case}_delta_time_histogram.png"))
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

        # C2) Spaghetti plot: every individual run's cumulativeNs vs triangulation,
        #     with the mean overlaid, per algo (run-to-run stability, doc section 5)
        fig, axes = plt.subplots(1, 2, figsize=(14, 5.5), sharey=False)
        any_data = False
        for ax, algo in zip(axes, ALGO_ORDER):
            if algo not in algo_dfs:
                ax.set_title(f"{algo} (no data)")
                continue
            df = algo_dfs[algo]
            for run_id, run_df in df.groupby("run"):
                run_df = run_df.sort_values("triangulation")
                ax.plot(run_df.triangulation, run_df.cumulativeNs / 1000.0,
                        color=ALGO_COLORS[algo], alpha=0.25, lw=0.8)
            mean_line = df.groupby("triangulation")["cumulativeNs"].mean().reset_index()
            ax.plot(mean_line.triangulation, mean_line.cumulativeNs / 1000.0,
                    color="black", lw=1.8, label="mean across runs")
            ax.set_xlabel("Triangulation number (n)")
            ax.set_ylabel("Cumulative time (\u00b5s)")
            ax.set_title(algo)
            ax.legend(fontsize=8)
            any_data = True
        if any_data:
            fig.suptitle(f"[{category}/{case}] Run-to-Run Spaghetti Plot (cumulative time)", fontweight="bold")
            savefig(fig, os.path.join(od, f"{case}_spaghetti_runs_cumulative_time.png"))
        else:
            plt.close(fig)

        # C3) Cumulative time vs n, LINEAR axes, with fitted line (slope + intercept summary)
        fig, ax = plt.subplots()
        any_data = False
        for algo in ALGO_ORDER:
            if algo not in algo_dfs:
                continue
            df = algo_dfs[algo]
            grp = df.groupby("triangulation")["cumulativeNs"].mean().reset_index()
            if grp.empty:
                continue
            ax.plot(grp.triangulation, grp.cumulativeNs, ".", color=ALGO_COLORS[algo],
                    alpha=0.4, ms=3)
            fit = linreg(grp.triangulation, grp.cumulativeNs)
            if fit:
                xs = np.linspace(grp.triangulation.min(), grp.triangulation.max(), 50)
                ys = fit["slope"] * xs + fit["intercept"]
                ax.plot(xs, ys, "-", lw=2, color=ALGO_COLORS[algo],
                        label=f"{algo}: {fit['slope']:.1f} ns/tri, intercept={fit['intercept']:.0f}, "
                              f"R\u00b2={fit['r2']:.3f}")
            any_data = True
        if any_data:
            ax.set_xlabel("Triangulation number (n)")
            ax.set_ylabel("Cumulative time (ns)")
            ax.set_title(f"[{category}/{case}] Cumulative Time vs n (linear fit)")
            ax.legend(fontsize=8)
            savefig(fig, os.path.join(od, f"{case}_cumulative_time_linear_fit.png"))
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
    plot_algo_scope_loglog(totals_df, outdir)
    plot_per_category(totals_df, individuals, categories, outdir)
    plot_per_case(individuals, outdir)

    print(f"\nAll graphs written under: {os.path.abspath(outdir)}")


if __name__ == "__main__":
    main()