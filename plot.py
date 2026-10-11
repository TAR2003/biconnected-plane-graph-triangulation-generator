#!/usr/bin/env python3
"""
plot.py  -  compare BiconnectedWithoutVGS, BiconnectedWithVGS, and Oneconnected benchmarks.

Layout expected (run from project root; paths are resolved from this file's directory):

    <root>/plot.py
    <root>/benchmark-results/BiconnectedWithoutVGSAlgo/results_time_*.csv
    <root>/benchmark-results/BiconnectedWithVGSAlgo/results_time_*.csv
    <root>/benchmark-results/OneconnectedAlgo/results_time_*.csv

Output:

    <root>/graph-biconnected-out/<category>/*.png
    <root>/graph-biconnected-out/_combined/*.png
    <root>/graph-biconnected-out/_metrics/*.png

Requires: pandas, numpy, matplotlib   (pip install pandas numpy matplotlib)
"""
import glob
import os
import re

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
import numpy as np
import pandas as pd

# ----------------------------------------------------------------------------------
# CONFIG
# ----------------------------------------------------------------------------------
ROOT = os.path.dirname(os.path.abspath(__file__))
RESULTS_DIR = os.path.join(ROOT, "benchmark-results")
OUT_DIR = os.path.join(ROOT, "graph-biconnected-out")
METRICS_DIR = os.path.join(OUT_DIR, "_metrics")

ALGOS = {  # label -> folder name under benchmark-results/
    "Without VGS": "BiconnectedWithoutVGSAlgo",
    "With VGS": "BiconnectedWithVGSAlgo",
    "Oneconnected": "OneconnectedAlgo",
}
ALGO_ORDER = list(ALGOS.keys())
COLORS = {
    "Without VGS": "#1f77b4",
    "With VGS": "#e8590c",
    "Oneconnected": "#2ca02c",
}
ALGO_SHORT = {"Without VGS": "wo", "With VGS": "w", "Oneconnected": "oc"}

TIME_COL = "cpuTimeSeconds"      # or "realTimeSeconds"
LOG_Y_TIME = True
LOG_Y_BARS = True
EXCLUDE_LIMIT_EXCEEDED = False
DPI = 150

MARKERS = ["o", "s", "^", "D", "v", "P", "X", "*", "h", "<", ">", "p", "d", "H", "8"]
LINESTYLES = ["-", "--", "-.", ":"]


# ----------------------------------------------------------------------------------
# LOADING
# ----------------------------------------------------------------------------------
def category_from_filename(path):
    """results_time_Biconnected_4_halin.csv -> 4_halin"""
    name = os.path.splitext(os.path.basename(path))[0]
    name = re.sub(r"^results_time_", "", name)
    name = re.sub(r"^Biconnected_", "", name)
    name = re.sub(r"^Oneconnected_", "", name)
    return name


def load_algo(label, folder):
    frames = []
    for path in sorted(glob.glob(os.path.join(RESULTS_DIR, folder, "*.csv"))):
        try:
            df = pd.read_csv(path, on_bad_lines="skip")
        except Exception as e:  # noqa: BLE001
            print(f"  ! could not read {path}: {e}")
            continue
        for c in ("vertices", TIME_COL, "triangulations"):
            if c not in df.columns:
                print(f"  ! {path} lacks column {c}, skipped")
                break
        else:
            for c in ("vertices", TIME_COL, "triangulations"):
                df[c] = pd.to_numeric(df[c], errors="coerce")
            for c in ("checkSuccessRate", "traversalSuccessRate"):
                if c in df.columns:
                    df[c] = pd.to_numeric(df[c], errors="coerce")
            df = df.dropna(subset=["vertices", TIME_COL, "triangulations", "filename"])
            df = df[(df[TIME_COL] > 0) & (df["triangulations"] > 0) & (df["vertices"] > 0)].copy()
            if EXCLUDE_LIMIT_EXCEEDED and "status" in df.columns:
                df = df[~df["status"].astype(str).str.contains("exceeded", case=False)]
            df["cat"] = category_from_filename(path)
            df["algo"] = label
            df["time"] = df[TIME_COL]
            df["avg_tpt"] = df["time"] / df["triangulations"]
            df["log2v"] = np.log2(df["vertices"])
            keep = ["algo", "cat", "filename", "vertices", "log2v",
                    "time", "triangulations", "avg_tpt"]
            if "checkSuccessRate" in df.columns:
                keep.append("checkSuccessRate")
            if "traversalSuccessRate" in df.columns:
                keep.append("traversalSuccessRate")
            frames.append(df[keep])
    base_cols = ["algo", "cat", "filename", "vertices", "log2v",
                 "time", "triangulations", "avg_tpt"]
    if not frames:
        return pd.DataFrame(columns=base_cols)
    return pd.concat(frames, ignore_index=True)


# ----------------------------------------------------------------------------------
# PLOT HELPERS
# ----------------------------------------------------------------------------------
def xy_plot(ax, df, xcol, ycol, color, label=None, marker="o", ls="-", raw=True):
    if df.empty or ycol not in df.columns:
        return
    sub = df.dropna(subset=[xcol, ycol])
    if sub.empty:
        return
    if raw:
        ax.scatter(sub[xcol], sub[ycol], s=14, color=color, alpha=0.30, marker=marker, linewidths=0)
    g = sub.groupby(xcol, as_index=False)[ycol].mean().sort_values(xcol)
    ax.plot(g[xcol], g[ycol], color=color, marker=marker, ls=ls, lw=1.6, ms=5,
            label=label, markeredgecolor="white", markeredgewidth=0.4)


def style(ax, title, xlabel, ylabel, logx=False, logy=False):
    ax.set_title(title, fontsize=11)
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    if logx:
        ax.set_xscale("log")
    if logy:
        ax.set_yscale("log")
    ax.grid(True, which="both", alpha=0.3)


def save(fig, folder, name):
    os.makedirs(folder, exist_ok=True)
    fig.tight_layout()
    fig.savefig(os.path.join(folder, name), dpi=DPI)
    plt.close(fig)


def grouped_bars(ax, labels, series, ylabel, title, xlabel, rot=90, fs=7):
    """series: dict label -> array-like, one value per label row."""
    labs = [lab for lab in ALGO_ORDER if lab in series and len(series[lab])]
    if not labs:
        return
    x = np.arange(len(labels))
    n = len(labs)
    bw = 0.8 / n
    for i, lab in enumerate(labs):
        offset = (i - (n - 1) / 2) * bw
        ax.bar(x + offset, series[lab], bw, color=COLORS[lab], label=lab)
    ax.set_xticks(x)
    ax.set_xticklabels(labels, rotation=rot, fontsize=fs, ha="center")
    ax.set_xlim(-0.8, len(labels) - 0.2)
    style(ax, title, xlabel, ylabel, logy=LOG_Y_BARS)
    ax.legend()


def per_case_table(all_data, keys):
    m = None
    vcols = []
    for lab, df in all_data.items():
        if df.empty:
            continue
        short = ALGO_SHORT[lab]
        cols = keys + ["vertices", "avg_tpt"]
        if "checkSuccessRate" in df.columns:
            cols.append("checkSuccessRate")
        if "traversalSuccessRate" in df.columns:
            cols.append("traversalSuccessRate")
        a = df[cols].copy()
        ren = {"avg_tpt": short, "vertices": f"v_{short}"}
        if "checkSuccessRate" in a.columns:
            ren["checkSuccessRate"] = f"csr_{short}"
        if "traversalSuccessRate" in a.columns:
            ren["traversalSuccessRate"] = f"tsr_{short}"
        a = a.rename(columns=ren)
        vcols.append(f"v_{short}")
        m = a if m is None else m.merge(a, on=keys, how="outer")
    if m is None or m.empty:
        return m
    m["v"] = m[vcols[0]]
    for c in vcols[1:]:
        m["v"] = m["v"].combine_first(m[c])
    return m.sort_values(["v"] + keys).reset_index(drop=True)


def per_vertex_table(all_data):
    parts = []
    for lab, df in all_data.items():
        if df.empty:
            continue
        short = ALGO_SHORT[lab]
        parts.append(df.groupby("vertices")["avg_tpt"].mean().rename(short))
    if not parts:
        return pd.DataFrame()
    m = pd.concat(parts, axis=1).sort_index().reset_index()
    return m


def biconnected_pairs(all_data, keys):
    """Rows with both Without VGS and With VGS on the same case."""
    d_wo = all_data.get("Without VGS", pd.DataFrame())
    d_w = all_data.get("With VGS", pd.DataFrame())
    if d_wo.empty or d_w.empty:
        return pd.DataFrame()
    a = d_wo[keys + ["avg_tpt"]].rename(columns={"avg_tpt": "wo"})
    if "checkSuccessRate" in d_wo.columns:
        a["csr_wo"] = d_wo["checkSuccessRate"].values
    b = d_w[keys + ["avg_tpt"]].rename(columns={"avg_tpt": "w"})
    m = a.merge(b, on=keys, how="inner")
    m = m[(m["wo"] > 0) & (m["w"] > 0)]
    m["ratio_wo_over_w"] = m["wo"] / m["w"]
    return m


# ----------------------------------------------------------------------------------
# EXISTING PLOT SETS (now three algorithms)
# ----------------------------------------------------------------------------------
def make_category_plots(cat, cat_data):
    out = os.path.join(OUT_DIR, cat)

    # 1) time vs triangulations (log-log)
    fig, ax = plt.subplots(figsize=(8, 5.5))
    for lab in ALGO_ORDER:
        xy_plot(ax, cat_data.get(lab, pd.DataFrame()), "triangulations", "time", COLORS[lab], lab)
    style(ax, f"[{cat}] total time vs total triangulations (log-log)",
          "total triangulations", f"total time ({TIME_COL}, s)", logx=True, logy=True)
    ax.legend()
    save(fig, out, "1_time_vs_triangulations_loglog.png")

    # 2) time vs vertices
    fig, ax = plt.subplots(figsize=(8, 5.5))
    for lab in ALGO_ORDER:
        xy_plot(ax, cat_data.get(lab, pd.DataFrame()), "vertices", "time", COLORS[lab], lab)
    style(ax, f"[{cat}] total time vs vertices", "vertices", f"total time ({TIME_COL}, s)",
          logy=LOG_Y_TIME)
    ax.legend()
    save(fig, out, "2_time_vs_vertices.png")

    # 3) time vs log2(vertices)
    fig, ax = plt.subplots(figsize=(8, 5.5))
    for lab in ALGO_ORDER:
        xy_plot(ax, cat_data.get(lab, pd.DataFrame()), "log2v", "time", COLORS[lab], lab)
    style(ax, f"[{cat}] total time vs log2(vertices)", "log2(vertices)",
          f"total time ({TIME_COL}, s)", logy=LOG_Y_TIME)
    ax.legend()
    save(fig, out, "3_time_vs_log2_vertices.png")

    # 4) avg time per triangulation per case
    t = per_case_table(cat_data, ["filename"])
    if t is not None and not t.empty:
        labels = [f"n={int(v)} | {f.replace('.txt', '')}" for v, f in zip(t["v"], t["filename"])]
        series = {}
        for lab in ALGO_ORDER:
            short = ALGO_SHORT[lab]
            if short in t.columns:
                series[lab] = t[short]
        fig, ax = plt.subplots(figsize=(max(8, 0.35 * len(t) + 3), 6))
        grouped_bars(ax, labels, series, "avg time per triangulation (s)",
                     f"[{cat}] average time per triangulation, per case (sorted by vertex count)",
                     "case (vertex count | file)")
        save(fig, out, "4_avg_time_per_triangulation_per_case_bar.png")

    # 5a/5b) avg time per triangulation vs vertex count
    pv = per_vertex_table(cat_data)
    if not pv.empty:
        labels = [f"{int(v)}\n(log2={np.log2(v):.2f})" for v in pv["vertices"]]
        series = {lab: pv[ALGO_SHORT[lab]] for lab in ALGO_ORDER if ALGO_SHORT[lab] in pv.columns}
        fig, ax = plt.subplots(figsize=(max(8, 0.9 * len(pv) + 3), 6))
        grouped_bars(ax, labels, series, "avg time per triangulation (s)",
                     f"[{cat}] avg time per triangulation vs vertex count (log2 shown)",
                     "vertices (log2 of vertices)", rot=0, fs=8)
        save(fig, out, "5a_avg_time_per_triangulation_vs_log2_vertices_bar.png")

        fig, ax = plt.subplots(figsize=(8, 5.5))
        for lab in ALGO_ORDER:
            xy_plot(ax, cat_data.get(lab, pd.DataFrame()), "log2v", "avg_tpt", COLORS[lab], lab, raw=False)
        style(ax, f"[{cat}] avg time per triangulation vs log2(vertices)", "log2(vertices)",
              "avg time per triangulation (s)", logy=LOG_Y_BARS)
        ax.legend()
        save(fig, out, "5b_avg_time_per_triangulation_vs_log2_vertices_line.png")


def combined_line(all_data, cats, xcol, ycol, title, xlabel, ylabel, fname, logx=False, logy=False):
    fig, ax = plt.subplots(figsize=(11, 7))
    for i, cat in enumerate(cats):
        mk = MARKERS[i % len(MARKERS)]
        ls = LINESTYLES[(i // len(MARKERS) + i) % len(LINESTYLES)]
        for lab in ALGO_ORDER:
            df = all_data.get(lab, pd.DataFrame())
            sub = df[df["cat"] == cat]
            xy_plot(ax, sub, xcol, ycol, COLORS[lab], marker=mk, ls=ls, raw=False)
    style(ax, title, xlabel, ylabel, logx=logx, logy=logy)
    algo_handles = [Line2D([0], [0], color=COLORS[l], lw=3, label=l) for l in ALGO_ORDER]
    cat_handles = [Line2D([0], [0], color="black", marker=MARKERS[i % len(MARKERS)],
                          ls=LINESTYLES[(i // len(MARKERS) + i) % len(LINESTYLES)],
                          lw=1.3, ms=6, label=c) for i, c in enumerate(cats)]
    leg1 = ax.legend(handles=algo_handles, title="Algorithm (colour)", loc="upper left",
                     bbox_to_anchor=(1.01, 1.0))
    ax.add_artist(leg1)
    ax.legend(handles=cat_handles, title="Category (symbol / line)", loc="upper left",
              bbox_to_anchor=(1.01, 0.85), fontsize=8)
    fig.subplots_adjust(right=0.72)
    os.makedirs(os.path.join(OUT_DIR, "_combined"), exist_ok=True)
    fig.savefig(os.path.join(OUT_DIR, "_combined", fname), dpi=DPI, bbox_inches="tight")
    plt.close(fig)


def make_combined_plots(all_data, cats):
    out = os.path.join(OUT_DIR, "_combined")

    combined_line(all_data, cats, "triangulations", "time",
                  "ALL categories: total time vs total triangulations (log-log)",
                  "total triangulations", f"total time ({TIME_COL}, s)",
                  "1_time_vs_triangulations_loglog.png", logx=True, logy=True)
    combined_line(all_data, cats, "vertices", "time",
                  "ALL categories: total time vs vertices", "vertices",
                  f"total time ({TIME_COL}, s)", "2_time_vs_vertices.png", logy=LOG_Y_TIME)
    combined_line(all_data, cats, "log2v", "time",
                  "ALL categories: total time vs log2(vertices)", "log2(vertices)",
                  f"total time ({TIME_COL}, s)", "3_time_vs_log2_vertices.png", logy=LOG_Y_TIME)

    t = per_case_table(all_data, ["cat", "filename"])
    if t is not None and not t.empty:
        labels = [f"n={int(v)} | {c}/{f.replace('.txt', '')}"
                  for v, c, f in zip(t["v"], t["cat"], t["filename"])]
        series = {lab: t[ALGO_SHORT[lab]] for lab in ALGO_ORDER if ALGO_SHORT[lab] in t.columns}
        fig, ax = plt.subplots(figsize=(min(200, max(10, 0.25 * len(t) + 3)), 8))
        grouped_bars(ax, labels, series, "avg time per triangulation (s)",
                     "ALL categories: average time per triangulation, per case (sorted by vertex count)",
                     "case (vertex count | category/file)", fs=5)
        save(fig, out, "4_avg_time_per_triangulation_per_case_bar.png")

    pv = per_vertex_table(all_data)
    if not pv.empty:
        labels = [f"{int(v)}\n(log2={np.log2(v):.2f})" for v in pv["vertices"]]
        series = {lab: pv[ALGO_SHORT[lab]] for lab in ALGO_ORDER if ALGO_SHORT[lab] in pv.columns}
        fig, ax = plt.subplots(figsize=(max(8, 0.9 * len(pv) + 3), 6))
        grouped_bars(ax, labels, series, "mean avg time per triangulation (s)",
                     "ALL categories: avg time per triangulation vs vertex count (mean over categories)",
                     "vertices (log2 of vertices)", rot=0, fs=8)
        save(fig, out, "5a_avg_time_per_triangulation_vs_log2_vertices_bar.png")

    combined_line(all_data, cats, "log2v", "avg_tpt",
                  "ALL categories: avg time per triangulation vs log2(vertices)",
                  "log2(vertices)", "avg time per triangulation (s)",
                  "5b_avg_time_per_triangulation_vs_log2_vertices_line.png", logy=LOG_Y_BARS)


# ----------------------------------------------------------------------------------
# NEW METRIC PLOTS (check / traversal success rates)
# ----------------------------------------------------------------------------------
def _scatter_metric_vs_avg_tpt(ax, df, xcol, color, label, logy=True):
    sub = df.dropna(subset=[xcol, "avg_tpt"])
    if sub.empty:
        return
    ax.scatter(sub[xcol], sub["avg_tpt"], s=22, color=color, alpha=0.45, label=label, edgecolors="none")
    g = sub.groupby(xcol, as_index=False)["avg_tpt"].mean().sort_values(xcol)
    ax.plot(g[xcol], g["avg_tpt"], color=color, lw=2, marker="o", ms=4)


def make_metric_plots(all_data, cats):
    os.makedirs(METRICS_DIR, exist_ok=True)
    pairs = biconnected_pairs(all_data, ["cat", "filename"])
    d_oc = all_data.get("Oneconnected", pd.DataFrame())
    d_w = all_data.get("With VGS", pd.DataFrame())

    # --- check success rate vs avg time (algorithms that report it) ---
    fig, ax = plt.subplots(figsize=(9, 6))
    for lab in ("Without VGS", "Oneconnected"):
        df = all_data.get(lab, pd.DataFrame())
        if "checkSuccessRate" not in df.columns:
            continue
        _scatter_metric_vs_avg_tpt(ax, df, "checkSuccessRate", COLORS[lab], lab)
    style(ax, "Check success rate vs avg time per triangulation",
          "check success rate (%)", "avg time per triangulation (s)", logy=LOG_Y_BARS)
    ax.legend()
    save(fig, METRICS_DIR, "1_check_success_rate_vs_avg_tpt.png")

    # --- same x-axis, ratio vs With VGS on matched biconnected cases ---
    if not pairs.empty and "csr_wo" in pairs.columns:
        fig, ax = plt.subplots(figsize=(9, 6))
        ax.scatter(pairs["csr_wo"], pairs["ratio_wo_over_w"], s=24,
                   color=COLORS["Without VGS"], alpha=0.5, label="Without VGS / With VGS")
        g = pairs.groupby("csr_wo", as_index=False)["ratio_wo_over_w"].mean().sort_values("csr_wo")
        ax.plot(g["csr_wo"], g["ratio_wo_over_w"], color=COLORS["Without VGS"], lw=2, marker="o", ms=4)
        ax.axhline(1.0, color=COLORS["With VGS"], ls="--", lw=1.2, label="parity with With VGS")
        style(ax, "Without VGS: check success rate vs speed ratio to With VGS",
              "check success rate (%, Without VGS)", "avg time per triangulation (Without / With VGS)")
        ax.legend()
        save(fig, METRICS_DIR, "2_check_success_rate_vs_ratio_to_with_vgs_biconnected.png")

    # --- Oneconnected check rate vs ratio to With VGS where same (cat, file) exists ---
    if not d_oc.empty and "checkSuccessRate" in d_oc.columns and not d_w.empty:
        a = d_oc[["cat", "filename", "avg_tpt", "checkSuccessRate"]].rename(
            columns={"avg_tpt": "oc", "checkSuccessRate": "csr_oc"})
        b = d_w[["cat", "filename", "avg_tpt"]].rename(columns={"avg_tpt": "w"})
        m = a.merge(b, on=["cat", "filename"], how="inner")
        m = m[(m["oc"] > 0) & (m["w"] > 0)]
        if not m.empty:
            m["ratio_oc_over_w"] = m["oc"] / m["w"]
            fig, ax = plt.subplots(figsize=(9, 6))
            ax.scatter(m["csr_oc"], m["ratio_oc_over_w"], s=24,
                       color=COLORS["Oneconnected"], alpha=0.5, label="Oneconnected / With VGS")
            g = m.groupby("csr_oc", as_index=False)["ratio_oc_over_w"].mean().sort_values("csr_oc")
            ax.plot(g["csr_oc"], g["ratio_oc_over_w"], color=COLORS["Oneconnected"], lw=2, marker="o", ms=4)
            ax.axhline(1.0, color=COLORS["With VGS"], ls="--", lw=1.2, label="parity with With VGS")
            style(ax, "Oneconnected: check success rate vs speed ratio to With VGS (matched cases)",
                  "check success rate (%, Oneconnected)", "avg time per triangulation (Oneconnected / With VGS)")
            ax.legend()
            save(fig, METRICS_DIR, "3_check_success_rate_vs_ratio_to_with_vgs_oneconnected.png")

    # --- traversal success rate (Oneconnected only) vs avg time ---
    if not d_oc.empty and "traversalSuccessRate" in d_oc.columns:
        fig, ax = plt.subplots(figsize=(9, 6))
        _scatter_metric_vs_avg_tpt(ax, d_oc, "traversalSuccessRate", COLORS["Oneconnected"], "Oneconnected")
        style(ax, "Oneconnected: traversal success rate vs avg time per triangulation",
              "traversal success rate (%)", "avg time per triangulation (s)", logy=LOG_Y_BARS)
        ax.legend()
        save(fig, METRICS_DIR, "4_traversal_success_rate_vs_avg_tpt_oneconnected.png")

        fig, ax = plt.subplots(figsize=(9, 6))
        sub = d_oc.dropna(subset=["traversalSuccessRate", "avg_tpt", "log2v"])
        if not sub.empty:
            sc = ax.scatter(sub["traversalSuccessRate"], sub["avg_tpt"], c=sub["log2v"],
                            cmap="viridis", s=28, alpha=0.55, edgecolors="none")
            plt.colorbar(sc, ax=ax, label="log2(vertices)")
        style(ax, "Oneconnected: traversal success rate vs avg time (colour = log2 vertices)",
              "traversal success rate (%)", "avg time per triangulation (s)", logy=LOG_Y_BARS)
        save(fig, METRICS_DIR, "5_traversal_success_rate_vs_avg_tpt_by_vertices_oneconnected.png")

    # Per-category metric plots where data exists
    for cat in cats:
        cat_out = os.path.join(OUT_DIR, cat)
        sub_data = {lab: df[df["cat"] == cat] for lab, df in all_data.items()}
        d_oc_cat = sub_data.get("Oneconnected", pd.DataFrame())

        if not pairs.empty and "csr_wo" in pairs.columns:
            pc = pairs[pairs["cat"] == cat]
            if not pc.empty:
                fig, ax = plt.subplots(figsize=(8, 5.5))
                ax.scatter(pc["csr_wo"], pc["ratio_wo_over_w"], s=22,
                           color=COLORS["Without VGS"], alpha=0.55)
                ax.axhline(1.0, color=COLORS["With VGS"], ls="--", lw=1)
                style(ax, f"[{cat}] check success vs Without/With VGS time ratio",
                      "check success rate (%, Without VGS)", "avg time ratio (Without / With VGS)")
                save(fig, cat_out, "6_check_success_vs_ratio_to_with_vgs.png")

        if not d_oc_cat.empty and "traversalSuccessRate" in d_oc_cat.columns:
            fig, ax = plt.subplots(figsize=(8, 5.5))
            _scatter_metric_vs_avg_tpt(ax, d_oc_cat, "traversalSuccessRate", COLORS["Oneconnected"], "Oneconnected")
            style(ax, f"[{cat}] traversal success rate vs avg time per triangulation",
                  "traversal success rate (%)", "avg time per triangulation (s)", logy=LOG_Y_BARS)
            save(fig, cat_out, "7_traversal_success_rate_vs_avg_tpt.png")


# ----------------------------------------------------------------------------------
def main():
    all_data = {lab: load_algo(lab, folder) for lab, folder in ALGOS.items()}
    for lab, df in all_data.items():
        print(f"{lab}: {len(df)} rows, {df['cat'].nunique() if not df.empty else 0} categories")

    cat_sets = [set(df["cat"]) for df in all_data.values() if not df.empty]
    cats = sorted(set.union(*cat_sets)) if cat_sets else []
    if not cats:
        print("No data found. Check benchmark-results/ layout.")
        return
    os.makedirs(OUT_DIR, exist_ok=True)

    for cat in cats:
        print(f"plotting category: {cat}")
        cat_data = {lab: df[df["cat"] == cat] for lab, df in all_data.items()}
        present = sum(1 for d in cat_data.values() if not d.empty)
        if present < len(ALGO_ORDER):
            print(f"  (note: {present}/{len(ALGO_ORDER)} algorithms have data for '{cat}')")
        make_category_plots(cat, cat_data)

    print("plotting combined set")
    make_combined_plots(all_data, cats)

    print("plotting check / traversal metric charts")
    make_metric_plots(all_data, cats)

    print(f"Done. Plots saved in: {OUT_DIR}")


if __name__ == "__main__":
    main()
