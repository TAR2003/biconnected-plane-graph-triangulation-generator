#!/usr/bin/env python3
"""
plot.py  -  compare BiconnectedWithoutVGS vs BiconnectedWithVGS benchmark results.

Layout expected (run this script from the project root, or anywhere - it uses its own location as root):

    <root>/plot.py
    <root>/benchmark-results/BiconnectedWithoutVGSAlgo/results_time_*.csv
    <root>/benchmark-results/BiconnectedWithVGSAlgo/results_time_*.csv

Output:

    <root>/graph-biconnected-out/<category>/*.png     (one set per category)
    <root>/graph-biconnected-out/_combined/*.png      (all categories together)

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

ALGOS = {  # label -> folder name
    "Without VGS": "BiconnectedWithoutVGSAlgo",
    "With VGS": "BiconnectedWithVGSAlgo",
}
COLORS = {"Without VGS": "#1f77b4", "With VGS": "#e8590c"}

TIME_COL = "cpuTimeSeconds"      # or "realTimeSeconds"
LOG_Y_TIME = True                # log y-axis for the time-vs-vertices plots (times span many decades)
LOG_Y_BARS = True                # log y-axis for the "average time per triangulation" bar charts
EXCLUDE_LIMIT_EXCEEDED = False   # True -> drop rows whose status is "Triangulation Limit exceeded"
DPI = 150

MARKERS = ["o", "s", "^", "D", "v", "P", "X", "*", "h", "<", ">", "p", "d", "H", "8"]
LINESTYLES = ["-", "--", "-.", ":"]


# ----------------------------------------------------------------------------------
# LOADING
# ----------------------------------------------------------------------------------
def category_from_filename(path):
    """results_time_Biconnected_4_halin.csv -> 4_halin ; results_time_9_general_biconnected.csv -> 9_general_biconnected"""
    name = os.path.splitext(os.path.basename(path))[0]
    name = re.sub(r"^results_time_", "", name)
    name = re.sub(r"^Biconnected_", "", name)
    return name


def load_algo(label, folder):
    frames = []
    for path in sorted(glob.glob(os.path.join(RESULTS_DIR, folder, "*.csv"))):
        try:
            df = pd.read_csv(path, on_bad_lines="skip")
        except Exception as e:  # noqa
            print(f"  ! could not read {path}: {e}")
            continue
        for c in ("vertices", TIME_COL, "triangulations"):
            if c not in df.columns:
                print(f"  ! {path} lacks column {c}, skipped")
                break
        else:
            for c in ("vertices", TIME_COL, "triangulations"):
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
            frames.append(df[["algo", "cat", "filename", "vertices", "log2v",
                              "time", "triangulations", "avg_tpt"]])
    if not frames:
        return pd.DataFrame(columns=["algo", "cat", "filename", "vertices", "log2v",
                                     "time", "triangulations", "avg_tpt"])
    return pd.concat(frames, ignore_index=True)


# ----------------------------------------------------------------------------------
# PLOT HELPERS
# ----------------------------------------------------------------------------------
def xy_plot(ax, df, xcol, ycol, color, label=None, marker="o", ls="-", raw=True):
    """Raw points (faint) + line through per-x mean."""
    if df.empty:
        return
    if raw:
        ax.scatter(df[xcol], df[ycol], s=14, color=color, alpha=0.30, marker=marker, linewidths=0)
    g = df.groupby(xcol, as_index=False)[ycol].mean().sort_values(xcol)
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


def grouped_bars(ax, labels, wo, w, ylabel, title, xlabel, rot=90, fs=7):
    x = np.arange(len(labels))
    bw = 0.4
    ax.bar(x - bw / 2, wo, bw, color=COLORS["Without VGS"], label="Without VGS")
    ax.bar(x + bw / 2, w, bw, color=COLORS["With VGS"], label="With VGS")
    ax.set_xticks(x)
    ax.set_xticklabels(labels, rotation=rot, fontsize=fs, ha="center")
    ax.set_xlim(-0.8, len(labels) - 0.2)
    style(ax, title, xlabel, ylabel, logy=LOG_Y_BARS)
    ax.legend()


def per_case_table(d_wo, d_w, keys):
    """Pair cases of both algorithms (outer join), sorted by vertex count."""
    a = d_wo[keys + ["vertices", "avg_tpt"]].rename(columns={"avg_tpt": "wo", "vertices": "v_wo"})
    b = d_w[keys + ["vertices", "avg_tpt"]].rename(columns={"avg_tpt": "w", "vertices": "v_w"})
    m = a.merge(b, on=keys, how="outer")
    m["v"] = m["v_wo"].combine_first(m["v_w"])
    return m.sort_values(["v"] + keys).reset_index(drop=True)


def per_vertex_table(d_wo, d_w):
    a = d_wo.groupby("vertices")["avg_tpt"].mean().rename("wo")
    b = d_w.groupby("vertices")["avg_tpt"].mean().rename("w")
    m = pd.concat([a, b], axis=1).sort_index().reset_index()
    return m


# ----------------------------------------------------------------------------------
# PLOT SETS
# ----------------------------------------------------------------------------------
def make_category_plots(cat, d_wo, d_w):
    out = os.path.join(OUT_DIR, cat)
    data = {"Without VGS": d_wo, "With VGS": d_w}

    # 1) time vs triangulations (log-log)
    fig, ax = plt.subplots(figsize=(8, 5.5))
    for lab, d in data.items():
        xy_plot(ax, d, "triangulations", "time", COLORS[lab], lab)
    style(ax, f"[{cat}] total time vs total triangulations (log-log)",
          "total triangulations", f"total time ({TIME_COL}, s)", logx=True, logy=True)
    ax.legend()
    save(fig, out, "1_time_vs_triangulations_loglog.png")

    # 2) time vs vertices (linear x)
    fig, ax = plt.subplots(figsize=(8, 5.5))
    for lab, d in data.items():
        xy_plot(ax, d, "vertices", "time", COLORS[lab], lab)
    style(ax, f"[{cat}] total time vs vertices", "vertices", f"total time ({TIME_COL}, s)",
          logy=LOG_Y_TIME)
    ax.legend()
    save(fig, out, "2_time_vs_vertices.png")

    # 3) time vs log2(vertices)
    fig, ax = plt.subplots(figsize=(8, 5.5))
    for lab, d in data.items():
        xy_plot(ax, d, "log2v", "time", COLORS[lab], lab)
    style(ax, f"[{cat}] total time vs log2(vertices)", "log2(vertices)",
          f"total time ({TIME_COL}, s)", logy=LOG_Y_TIME)
    ax.legend()
    save(fig, out, "3_time_vs_log2_vertices.png")

    # 4) avg time per triangulation per case (bars, sorted by vertices)
    t = per_case_table(d_wo, d_w, ["filename"])
    if not t.empty:
        labels = [f"n={int(v)} | {f.replace('.txt', '')}" for v, f in zip(t["v"], t["filename"])]
        fig, ax = plt.subplots(figsize=(max(8, 0.35 * len(t) + 3), 6))
        grouped_bars(ax, labels, t["wo"], t["w"], "avg time per triangulation (s)",
                     f"[{cat}] average time per triangulation, per case (sorted by vertex count)",
                     "case (vertex count | file)")
        save(fig, out, "4_avg_time_per_triangulation_per_case_bar.png")

    # 5a/5b) avg time per triangulation vs log2(vertices): bars + line
    pv = per_vertex_table(d_wo, d_w)
    if not pv.empty:
        labels = [f"{int(v)}\n(log2={np.log2(v):.2f})" for v in pv["vertices"]]
        fig, ax = plt.subplots(figsize=(max(8, 0.9 * len(pv) + 3), 6))
        grouped_bars(ax, labels, pv["wo"], pv["w"], "avg time per triangulation (s)",
                     f"[{cat}] avg time per triangulation vs vertex count (log2 shown)",
                     "vertices (log2 of vertices)", rot=0, fs=8)
        save(fig, out, "5a_avg_time_per_triangulation_vs_log2_vertices_bar.png")

        fig, ax = plt.subplots(figsize=(8, 5.5))
        for lab, d in data.items():
            xy_plot(ax, d, "log2v", "avg_tpt", COLORS[lab], lab, raw=False)
        style(ax, f"[{cat}] avg time per triangulation vs log2(vertices)", "log2(vertices)",
              "avg time per triangulation (s)", logy=LOG_Y_BARS)
        ax.legend()
        save(fig, out, "5b_avg_time_per_triangulation_vs_log2_vertices_line.png")


def combined_line(all_data, cats, xcol, ycol, title, xlabel, ylabel, fname, logx=False, logy=False):
    fig, ax = plt.subplots(figsize=(11, 7))
    for i, cat in enumerate(cats):
        mk = MARKERS[i % len(MARKERS)]
        ls = LINESTYLES[(i // len(MARKERS) + i) % len(LINESTYLES)]
        for lab, df in all_data.items():
            sub = df[df["cat"] == cat]
            xy_plot(ax, sub, xcol, ycol, COLORS[lab], marker=mk, ls=ls, raw=False)
    style(ax, title, xlabel, ylabel, logx=logx, logy=logy)
    algo_handles = [Line2D([0], [0], color=COLORS[l], lw=3, label=l) for l in all_data]
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
    d_wo, d_w = all_data["Without VGS"], all_data["With VGS"]

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

    # 4) per-case bars across every category
    t = per_case_table(d_wo, d_w, ["cat", "filename"])
    if not t.empty:
        labels = [f"n={int(v)} | {c}/{f.replace('.txt', '')}"
                  for v, c, f in zip(t["v"], t["cat"], t["filename"])]
        fig, ax = plt.subplots(figsize=(min(200, max(10, 0.25 * len(t) + 3)), 8))
        grouped_bars(ax, labels, t["wo"], t["w"], "avg time per triangulation (s)",
                     "ALL categories: average time per triangulation, per case (sorted by vertex count)",
                     "case (vertex count | category/file)", fs=5)
        save(fig, out, "4_avg_time_per_triangulation_per_case_bar.png")

    # 5a) bars: mean over everything with the same vertex count
    pv = per_vertex_table(d_wo, d_w)
    if not pv.empty:
        labels = [f"{int(v)}\n(log2={np.log2(v):.2f})" for v in pv["vertices"]]
        fig, ax = plt.subplots(figsize=(max(8, 0.9 * len(pv) + 3), 6))
        grouped_bars(ax, labels, pv["wo"], pv["w"], "mean avg time per triangulation (s)",
                     "ALL categories: avg time per triangulation vs vertex count (mean over categories)",
                     "vertices (log2 of vertices)", rot=0, fs=8)
        save(fig, out, "5a_avg_time_per_triangulation_vs_log2_vertices_bar.png")

    # 5b) line per category
    combined_line(all_data, cats, "log2v", "avg_tpt",
                  "ALL categories: avg time per triangulation vs log2(vertices)",
                  "log2(vertices)", "avg time per triangulation (s)",
                  "5b_avg_time_per_triangulation_vs_log2_vertices_line.png", logy=LOG_Y_BARS)


# ----------------------------------------------------------------------------------
def main():
    all_data = {lab: load_algo(lab, folder) for lab, folder in ALGOS.items()}
    for lab, df in all_data.items():
        print(f"{lab}: {len(df)} rows, {df['cat'].nunique()} categories")

    cats = sorted(set(all_data["Without VGS"]["cat"]) | set(all_data["With VGS"]["cat"]))
    if not cats:
        print("No data found. Check the folder layout.")
        return
    os.makedirs(OUT_DIR, exist_ok=True)

    for cat in cats:
        print(f"plotting category: {cat}")
        d_wo = all_data["Without VGS"].query("cat == @cat")
        d_w = all_data["With VGS"].query("cat == @cat")
        if d_wo.empty or d_w.empty:
            print(f"  (note: only one algorithm has data for '{cat}')")
        make_category_plots(cat, d_wo, d_w)

    print("plotting combined set")
    make_combined_plots(all_data, cats)
    print(f"Done. Plots saved in: {OUT_DIR}")


if __name__ == "__main__":
    main()