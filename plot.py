#!/usr/bin/env python3
"""
Plot benchmark time and memory results.

Reads:
    benchmark-results/<algo>/*.csv
    benchmark-results-memory/<algo>/*.csv

Writes a Graphs/ tree (replaced on every run):

    Graphs/Algorithmwise/<Algo>/<InputType>/<Category>/     11 charts
    Graphs/Algorithmwise/<Algo>/<InputType>/combined/       11 charts
    Graphs/Algorithmwise/<Algo>/combined/                   11 charts
    Graphs/Comparison/BiconnectedInput/<Category>/          11 charts + traversal comparisons
    Graphs/Comparison/BiconnectedInput/combined/
    Graphs/Comparison/OneconnectedInput/<Category>/
    Graphs/Comparison/OneconnectedInput/combined/
    Graphs/Comparison/Combined/
    Graphs/VGSvsnonVGS/BiconnectedInput/<Category>/          11 charts + VGS comparisons
    Graphs/VGSvsnonVGS/BiconnectedInput/combined/

Category names keep their input-type prefix (Biconnected_plantri and
Oneconnected_plantri stay distinct). Biconnected algorithms are plotted only
on biconnected inputs. The one-connected algorithm is plotted on both.

Time charts use cpuTimeSeconds, Google Benchmark process CPU time in seconds.

    01   points  total CPU time vs triangulations   (log-log)
    02   points  avg time per triangulation vs triangulations (log-log)
    03   points  avg time per triangulation vs vertices (log y)
    04   points  avg time per triangulation vs vertices (log-log)
    05   line    total CPU time vs triangulations   (log-log)
    06   line    avg time per triangulation vs triangulations (log-log)

    On every avg-time chart whose x-axis is not total triangulations, each point
    is coloured red (fewer triangulations) to green (more). The colour bar is the
    count scale. Markers, lines, and outlines still identify the series.

Memory charts (5), peak memory = peakDeltaMiB, per-vertex = peakDeltaBytesPerVertex:
    07 points  peak memory vs triangulations             (log x)
    08 points  peak memory per vertex vs vertices        (log x)
    09 line    mean peak memory per vertex vs vertices   (log x)
    10 points  peak memory vs vertices
    11 points  peak memory vs vertices                   (log x)

Extra charts are added on the algorithm, comparison, and VGS folders described
in the functions below.

One-connected 2D maps (traversal success vs check success, colour = avg time):
    14 points  coloured points
    15 field   1/distance area, with points in their own colour
    16 field   1/distance area only

Biconnected 2D maps (vertices vs triangulations, both log, colour = avg time):
    14 points  coloured points
    15 field   1/distance area, with points in their own colour
    16 field   1/distance area only
Point outlines use the point colour. Green is a low average time, red is high.

Run from anywhere:
    python plot.py
"""

import glob
import os
import shutil
import time
import zlib
from collections import Counter

import matplotlib

matplotlib.use("Agg")
import matplotlib.colors as mcolors
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from matplotlib.lines import Line2D
from scipy.spatial import cKDTree

# ------------------------------------------------------------------------------
# Config
# ------------------------------------------------------------------------------
ROOT = os.path.dirname(os.path.abspath(__file__))
TIME_DIR = os.path.join(ROOT, "benchmark-results")
MEM_DIR = os.path.join(ROOT, "benchmark-results-memory")
OUT_ROOT = os.path.join(ROOT, "Graphs")

ALGO_ORDER = ["BiconnectedWithoutVGS", "BiconnectedWithVGS", "Oneconnected"]
ALGO_SRC = {
    "BiconnectedWithoutVGS": "BiconnectedWithoutVGSAlgo",
    "BiconnectedWithVGS": "BiconnectedWithVGSAlgo",
    "Oneconnected": "OneconnectedAlgo",
}
ALGO_OUT = {
    "BiconnectedWithoutVGS": "BiconnectedWithoutVGS",
    "BiconnectedWithVGS": "BiconnectedWithVGS",
    "Oneconnected": "OneconnectedAlgo",
}
ALGO_LABEL = {
    "BiconnectedWithoutVGS": "Without VGS",
    "BiconnectedWithVGS": "With VGS",
    "Oneconnected": "Oneconnected",
}
ALGO_COLOR = {
    "BiconnectedWithoutVGS": "#1d4ed8",
    "BiconnectedWithVGS": "#ea580c",
    "Oneconnected": "#15803d",
}
ALGO_MARKER = {
    "BiconnectedWithoutVGS": "o",
    "BiconnectedWithVGS": "s",
    "Oneconnected": "^",
}
INPUT_MARKER = {"Biconnected": "o", "Oneconnected": "s"}
INPUT_LS = {"Biconnected": "-", "Oneconnected": "--"}

MARKERS = ["o", "s", "^", "D", "v", "P", "X", "*", "h", "<", ">", "p", "d", "H", "8"]
LINESTYLES = ["-", "--", "-.", ":"]
DPI = 110

# The only timed column is cpuTimeSeconds (process CPU time, seconds).
TIME_METRICS = [
    dict(
        key="total", tag="",
        time="time", avg="avg_tpt",
        time_label="total CPU time", time_title="total time",
        avg_label="avg time per triangulation", avg_title="avg time per triangulation",
        file_time="total_time", file_avg="avg_time",
        equal_label="equal total time",
    ),
]

# num, kind, which (time|avg), x, xlabel, logx, logy, filename tail
_TIME_AXES = [
    ("01", "points", "time", "triangulations", "total triangulations (log scale)", True, True,
     "vs_triangulations_loglog"),
    ("02", "points", "avg", "triangulations", "total triangulations (log scale)", True, True,
     "vs_triangulations_logx"),
    ("03", "points", "avg", "vertices", "vertices", False, True, "vs_vertices"),
    ("04", "points", "avg", "vertices", "vertices (log scale)", True, True, "vs_vertices_logx"),
    ("05", "line", "time", "triangulations", "total triangulations (log scale)", True, True,
     "vs_triangulations_loglog"),
    ("06", "line", "avg", "triangulations", "total triangulations (log scale)", True, True,
     "vs_triangulations_logx"),
]

_TOTAL_TIME_NAMES = {
    ("01", "points", "time"): "01_points_total_time_vs_triangulations_loglog.png",
    ("02", "points", "avg"): "02_points_avg_time_vs_triangulations_logx.png",
    ("03", "points", "avg"): "03_points_avg_time_vs_vertices.png",
    ("04", "points", "avg"): "04_points_avg_time_vs_vertices_logx.png",
    ("05", "line", "time"): "05_line_total_time_vs_triangulations_loglog.png",
    ("06", "line", "avg"): "06_line_avg_time_vs_triangulations_logx.png",
}


def _time_chart_spec(metric, num, kind, which, x, xlabel, logx, logy, tail):
    """One XY spec. Total keeps its old filename; build and generation get a tag."""
    if which == "time":
        y = metric["time"]
        ylabel = f"{metric['time_label']} (s, log scale)"
        title = f"{metric['time_title']} vs total triangulations"
        file_bit = metric["file_time"]
    else:
        y = metric["avg"]
        ylabel = f"{metric['avg_label']} (s, log scale)"
        subject = "total triangulations" if x == "triangulations" else "vertices"
        title = f"{metric['avg_title']} vs {subject}"
        file_bit = metric["file_avg"]
    if metric["key"] == "total":
        name = _TOTAL_TIME_NAMES[(num, kind, which)]
    else:
        name = f"{num}{metric['tag']}_{kind}_{file_bit}_{tail}.png"
    return dict(
        name=name, kind=kind, x=x, y=y,
        xlabel=xlabel, ylabel=ylabel, title=title,
        logx=logx, logy=logy,
    )


def time_plot_specs():
    specs = []
    for metric in TIME_METRICS:
        for num, kind, which, x, xlabel, logx, logy, tail in _TIME_AXES:
            specs.append(_time_chart_spec(
                metric, num, kind, which, x, xlabel, logx, logy, tail,
            ))
    return specs


TIME_PLOTS = time_plot_specs()

MEM_PLOTS = [
    dict(
        name="07_points_peak_memory_vs_triangulations_logx.png",
        kind="points", x="triangulations", y="peak_mib",
        xlabel="total triangulations (log scale)",
        ylabel="peak memory (MiB)",
        title="peak memory vs total triangulations",
        logx=True, logy=False,
    ),
    dict(
        name="08_points_peak_memory_per_vertex_vs_vertices_logx.png",
        kind="points", x="vertices", y="peak_per_vertex",
        xlabel="vertices (log scale)",
        ylabel="peak memory per vertex (bytes)",
        title="peak memory per vertex vs vertices",
        logx=True, logy=False,
    ),
    dict(
        name="09_line_mean_peak_memory_per_vertex_vs_vertices_logx.png",
        kind="line", x="vertices", y="peak_per_vertex",
        xlabel="vertices (log scale)",
        ylabel="mean peak memory per vertex (bytes)",
        title="mean peak memory per vertex vs vertices",
        logx=True, logy=False, mean_by_x=True,
    ),
    dict(
        name="10_points_peak_memory_vs_vertices.png",
        kind="points", x="vertices", y="peak_mib",
        xlabel="vertices",
        ylabel="peak memory (MiB)",
        title="peak memory vs vertices",
        logx=False, logy=False,
    ),
    dict(
        name="11_points_peak_memory_vs_vertices_logx.png",
        kind="points", x="vertices", y="peak_mib",
        xlabel="vertices (log scale)",
        ylabel="peak memory (MiB)",
        title="peak memory vs vertices",
        logx=True, logy=False,
    ),
]


# ------------------------------------------------------------------------------
# Loading
# ------------------------------------------------------------------------------
def input_type_of(category):
    name = str(category)
    if name.startswith("Oneconnected"):
        return "Oneconnected"
    if name.startswith("Biconnected"):
        return "Biconnected"
    return "Other"


def _num(df, col):
    if col not in df.columns:
        return pd.Series(np.nan, index=df.index)
    return pd.to_numeric(df[col], errors="coerce")


TIME_COLUMNS = [
    "algo", "category", "filename", "vertices", "triangulations",
    "time",
    "check_rate", "trav_rate", "total_checks", "successful_checks",
    "input_type", "avg_tpt",
]
MEM_COLUMNS = [
    "algo", "category", "filename", "vertices", "triangulations",
    "peak_mib", "peak_per_vertex", "input_type",
]


def load_time():
    frames = []
    for algo, folder in ALGO_SRC.items():
        for path in sorted(glob.glob(os.path.join(TIME_DIR, folder, "*.csv"))):
            df = pd.read_csv(path)
            if "category" not in df.columns or "filename" not in df.columns:
                print(f"  skip {path}: missing category/filename")
                continue
            out = pd.DataFrame({
                "algo": algo,
                "category": df["category"].astype(str),
                "filename": df["filename"].astype(str),
                "vertices": _num(df, "vertices"),
                "triangulations": _num(df, "triangulations"),
                "time": _num(df, "cpuTimeSeconds"),
                "check_rate": _num(df, "checkSuccessRate"),
                "trav_rate": _num(df, "traversalSuccessRate"),
                "total_checks": _num(df, "totalChecks"),
                "successful_checks": _num(df, "successfulChecks"),
            })
            out["input_type"] = out["category"].map(input_type_of)
            tri = out["triangulations"]
            out["avg_tpt"] = out["time"] / tri
            out = out.replace([np.inf, -np.inf], np.nan)
            out = out.dropna(subset=["vertices", "triangulations", "time"])
            out = out[(out["vertices"] > 0) & (out["triangulations"] > 0) & (out["time"] > 0)]
            frames.append(out)
    if not frames:
        return pd.DataFrame(columns=TIME_COLUMNS)
    return pd.concat(frames, ignore_index=True)


def load_memory():
    frames = []
    for algo, folder in ALGO_SRC.items():
        for path in sorted(glob.glob(os.path.join(MEM_DIR, folder, "*.csv"))):
            df = pd.read_csv(path)
            if "category" not in df.columns or "filename" not in df.columns:
                print(f"  skip {path}: missing category/filename")
                continue
            out = pd.DataFrame({
                "algo": algo,
                "category": df["category"].astype(str),
                "filename": df["filename"].astype(str),
                "vertices": _num(df, "vertices"),
                "triangulations": _num(df, "triangulations"),
                "peak_mib": _num(df, "peakDeltaMiB"),
                "peak_per_vertex": _num(df, "peakDeltaBytesPerVertex"),
            })
            out["input_type"] = out["category"].map(input_type_of)
            out = out.replace([np.inf, -np.inf], np.nan)
            out = out.dropna(subset=["vertices", "triangulations", "peak_mib", "peak_per_vertex"])
            out = out[(out["vertices"] > 0) & (out["triangulations"] > 0)]
            frames.append(out)
    if not frames:
        return pd.DataFrame(columns=MEM_COLUMNS)
    return pd.concat(frames, ignore_index=True)


# ------------------------------------------------------------------------------
# Styles
# ------------------------------------------------------------------------------
def _hex(rgb):
    return mcolors.to_hex(tuple(float(c) for c in list(rgb)[:3]))


def hsv_palette(n, hue0, hue1):
    if n <= 0:
        return []
    colors = []
    for i in range(n):
        span = 0 if n == 1 else (i % 8) / 7
        hue = (hue0 + (hue1 - hue0) * span) % 1.0
        band = i // 8
        sat = 0.88 - 0.18 * (band % 2)
        val = 0.42 + 0.18 * (band % 3)
        colors.append(_hex(mcolors.hsv_to_rgb((hue, sat, val))))
    return colors


def qual_colors(n):
    raw = []
    for name in ("tab20", "tab20b", "tab20c"):
        cmap = plt.colormaps[name]
        raw.extend(cmap(i) for i in range(cmap.N))
    ordered = raw[0::2] + raw[1::2]
    if n <= len(ordered):
        return [_hex(c) for c in ordered[:n]]
    extra = hsv_palette(n - len(ordered), 0.0, 0.9)
    return [_hex(c) for c in ordered] + extra


def build_series(df, mode):
    """Return list of (label, color, marker, linestyle, frame)."""
    if df is None or df.empty:
        return []

    if mode == "single":
        return [("result", "#1f77b4", "o", "-", df)]

    if mode == "by_category":
        cats = sorted(df["category"].unique())
        colors = qual_colors(len(cats))
        series = []
        for i, cat in enumerate(cats):
            sub = df[df["category"] == cat]
            series.append((
                cat,
                colors[i],
                MARKERS[i % len(MARKERS)],
                LINESTYLES[(i // len(MARKERS)) % len(LINESTYLES)],
                sub,
            ))
        return series

    if mode == "by_input_family":
        series = []
        for family, hue0, hue1 in (
            ("Biconnected", 0.50, 0.72),
            ("Oneconnected", 0.00, 0.08),
        ):
            cats = sorted(df.loc[df["input_type"] == family, "category"].unique())
            colors = hsv_palette(len(cats), hue0, hue1)
            for i, cat in enumerate(cats):
                sub = df[df["category"] == cat]
                series.append((
                    cat,
                    colors[i],
                    MARKERS[i % len(MARKERS)],
                    LINESTYLES[(i // len(MARKERS)) % len(LINESTYLES)],
                    sub,
                ))
        return series

    if mode == "by_algo":
        series = []
        for algo in ALGO_ORDER:
            sub = df[df["algo"] == algo]
            if sub.empty:
                continue
            series.append((ALGO_LABEL[algo], ALGO_COLOR[algo], ALGO_MARKER[algo], "-", sub))
        return series

    if mode == "by_algo_category":
        cats = sorted(df["category"].unique())
        series = []
        for algo in ALGO_ORDER:
            for i, cat in enumerate(cats):
                sub = df[(df["algo"] == algo) & (df["category"] == cat)]
                if sub.empty:
                    continue
                series.append((
                    f"{ALGO_LABEL[algo]} | {cat}",
                    ALGO_COLOR[algo],
                    MARKERS[i % len(MARKERS)],
                    LINESTYLES[(i // len(MARKERS)) % len(LINESTYLES)],
                    sub,
                ))
        return series

    if mode == "by_algo_input":
        series = []
        for algo in ALGO_ORDER:
            for inp in ("Biconnected", "Oneconnected"):
                sub = df[(df["algo"] == algo) & (df["input_type"] == inp)]
                if sub.empty:
                    continue
                series.append((
                    f"{ALGO_LABEL[algo]} | {inp} input",
                    ALGO_COLOR[algo],
                    INPUT_MARKER[inp],
                    INPUT_LS[inp],
                    sub,
                ))
        return series

    raise ValueError(mode)


def _proxy(color, marker, ls, label):
    return Line2D(
        [0], [0], color=color, marker=marker, linestyle=ls,
        lw=1.8, ms=6, label=label, markeredgecolor="white", markeredgewidth=0.4,
    )


def add_legend(ax, series, mode, vline_label=None, hline_label=None, swatch="colour"):
    if mode == "single":
        handles = []
    elif mode == "by_algo_category":
        algos = []
        cats = []
        seen_a, seen_c = set(), set()
        for label, color, marker, ls, _sub in series:
            algo_name, cat = label.split(" | ", 1)
            if algo_name not in seen_a:
                seen_a.add(algo_name)
                algos.append(_proxy(color, "o", "-", algo_name))
            if cat not in seen_c:
                seen_c.add(cat)
                cats.append(_proxy("black", marker, ls, cat))
        if algos:
            algo_title = "algorithm (edge)" if swatch == "edge" else "algorithm (colour)"
            leg = ax.legend(handles=algos, title=algo_title, loc="upper left",
                            bbox_to_anchor=(1.01, 1.0), fontsize=8, framealpha=0.92)
            ax.add_artist(leg)
        if cats:
            ax.legend(handles=cats, title="category (symbol)", loc="upper left",
                      bbox_to_anchor=(1.01, 0.72), fontsize=6.5, framealpha=0.92,
                      ncol=1 if len(cats) < 20 else 2)
        if vline_label:
            pass
        return len(series) > 8
    else:
        handles = [_proxy(c, m, ls, lab) for lab, c, m, ls, _ in series]

    if vline_label:
        handles.append(Line2D([0], [0], color="#444444", ls="--", lw=1.2, label=vline_label))
    if hline_label:
        handles.append(Line2D([0], [0], color="#444444", ls="-.", lw=1.2, label=hline_label))
    if not handles:
        return False
    outside = len(handles) > 6
    ncol = 2 if len(handles) > 18 else 1
    if outside:
        ax.legend(handles=handles, fontsize=7 if len(handles) > 10 else 8,
                  framealpha=0.92, loc="upper left", bbox_to_anchor=(1.01, 1.0), ncol=ncol)
    else:
        ax.legend(handles=handles, fontsize=8, framealpha=0.92)
    return outside


def style_ax(ax, title, xlabel, ylabel, logx, logy, rate_x=False, rate_y=False):
    ax.set_title(title, fontsize=11)
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    if logx:
        ax.set_xscale("log")
    if logy:
        ax.set_yscale("log")
    ax.grid(True, which="both", alpha=0.3)
    ax.margins(0.05)
    # Limits last, so margins do not expand a 0–100% axis off the scale.
    if rate_x:
        ax.set_xlim(-3, 103)
    if rate_y:
        ax.set_ylim(-3, 103)


def save_fig(fig, folder, name):
    path = os.path.join(folder, name)
    last_error = None
    for attempt in range(5):
        try:
            os.makedirs(folder, exist_ok=True)
            fig.savefig(path, dpi=DPI, bbox_inches="tight")
            plt.close(fig)
            return
        except FileNotFoundError as exc:
            # Saving onto /mnt/c can briefly report a just-created folder as missing.
            last_error = exc
            time.sleep(0.25 * (attempt + 1))
    plt.close(fig)
    raise last_error


# ------------------------------------------------------------------------------
# Generic XY charts
# ------------------------------------------------------------------------------
# Avg-time y columns. Point colour uses triangulation count only when that
# count is not already an axis.
_AVG_TIME_Y = {"avg_tpt"}
# Low values are red, high values are green.
_COUNT_CMAP = "RdYlGn"


def _count_color_scale(values):
    """Shared red→green scale for one axes. None when there is nothing to map."""
    vals = pd.to_numeric(values, errors="coerce").to_numpy(dtype=float)
    vals = vals[np.isfinite(vals) & (vals > 0)]
    if vals.size == 0:
        return None
    vmin = float(vals.min())
    vmax = float(vals.max())
    if vmin == vmax:
        return mcolors.Normalize(vmin=vmin - 1.0, vmax=vmax + 1.0), False
    use_log = vmin > 0 and vmax / vmin >= 20.0
    if use_log:
        return mcolors.LogNorm(vmin=vmin, vmax=vmax), True
    return mcolors.Normalize(vmin=vmin, vmax=vmax), False


def _wants_count_colors(spec, df):
    if spec.get("y") not in _AVG_TIME_Y or spec.get("x") == "triangulations":
        return False
    return df is not None and "triangulations" in getattr(df, "columns", [])


def _add_count_colorbar(fig, ax, norm, use_log):
    sm = plt.cm.ScalarMappable(norm=norm, cmap=plt.colormaps[_COUNT_CMAP])
    sm.set_array([])
    cbar = fig.colorbar(
        sm, ax=ax, orientation="horizontal", pad=0.22, fraction=0.046, aspect=36,
    )
    scale = "log scale, " if use_log else ""
    cbar.set_label(f"total triangulations ({scale}red = fewer, green = more)")
    return cbar


def _scatter_by_count(ax, xs, ys, counts, norm, marker, size, edge, zorder=3):
    """Colour finite positive counts. Return how many points were drawn that way."""
    counts = np.asarray(counts, dtype=float)
    ok = np.isfinite(counts) & (counts > 0)
    if not ok.any():
        return 0
    ax.scatter(
        np.asarray(xs)[ok], np.asarray(ys)[ok],
        c=counts[ok], cmap=_COUNT_CMAP, norm=norm,
        s=size, marker=marker, linewidths=0.9, edgecolors=edge,
        alpha=0.92, zorder=zorder,
    )
    return int(ok.sum())


def _face_edge(color, alpha=0.62):
    """Translucent fill plus a solid darker outline, so stacked points stay crisp."""
    rgb = mcolors.to_rgb(color)
    face = (rgb[0], rgb[1], rgb[2], alpha)
    edge = tuple(max(0.0, c * 0.32) for c in rgb)
    return face, edge


def _spread(xvals, yvals, logx, logy, label, dodge):
    """Separate points that land on the same spot, and nudge whole series apart."""
    xvals = np.asarray(xvals, dtype=float).copy()
    yvals = np.asarray(yvals, dtype=float).copy()
    if len(xvals) == 0:
        return xvals, yvals
    if dodge:
        if logx:
            xvals = xvals * (10.0 ** dodge)
        else:
            span = float(np.nanmax(xvals) - np.nanmin(xvals))
            xvals = xvals + dodge * (span if span > 0 else 1.0)

    def _key(vals, log_scale):
        if log_scale:
            return np.round(np.log10(np.clip(vals, 1e-30, None)), 3)
        return np.round(vals, 4)

    keys = list(zip(_key(xvals, logx), _key(yvals, logy)))
    counts = Counter(keys)
    mask = np.fromiter((counts[k] > 1 for k in keys), dtype=bool, count=len(keys))
    n_hit = int(mask.sum())
    if n_hit:
        rng = np.random.default_rng(zlib.adler32(str(label).encode("utf-8")) & 0xFFFFFFFF)
        jx = rng.uniform(-1.0, 1.0, n_hit)
        jy = rng.uniform(-1.0, 1.0, n_hit)
        if logx:
            xvals[mask] *= np.power(10.0, jx * 0.014)
        else:
            span = float(np.nanmax(xvals) - np.nanmin(xvals))
            xvals[mask] = xvals[mask] + jx * 0.007 * (span if span > 0 else 1.0)
        if logy:
            yvals[mask] *= np.power(10.0, jy * 0.012)
        else:
            span = float(np.nanmax(yvals) - np.nanmin(yvals))
            yvals[mask] = yvals[mask] + jy * 0.007 * (span if span > 0 else 1.0)
    return xvals, yvals


def plot_xy(df, folder, spec, mode, title_prefix, vline=None, vline_label=None,
            hline=None, hline_label=None):
    x, y = spec["x"], spec["y"]
    if df is None or df.empty or x not in df.columns or y not in df.columns:
        return False
    work = df.dropna(subset=[x, y])
    if spec.get("logx"):
        work = work[work[x] > 0]
    if spec.get("logy"):
        work = work[work[y] > 0]
    if work.empty:
        return False
    series = build_series(work, mode)
    if not series:
        return False

    fig, ax = plt.subplots(figsize=(8.4, 5.6))
    kind = spec["kind"]
    mean_by_x = spec.get("mean_by_x", False)
    logx = spec.get("logx", False)
    logy = spec.get("logy", False)
    count_scale = _count_color_scale(work["triangulations"]) if _wants_count_colors(spec, work) else None
    colored = 0
    nser = sum(1 for _lab, _c, _m, _ls, sub in series if not sub.empty)
    # A small x nudge only when a few series share the axes (algorithm comparisons).
    # Many categories stay on their true x and are told apart by colour.
    for i, (label, color, marker, ls, sub) in enumerate(series):
        if mean_by_x:
            drawn = sub.groupby(x, as_index=False)[y].mean().sort_values(x)
        elif kind == "points":
            drawn = sub.sort_values(y, ascending=False)
        else:
            drawn = sub.sort_values([x, y])
        if drawn.empty:
            continue
        face, edge = _face_edge(color, 0.72 if kind == "points" else 1.0)
        use_counts = count_scale is not None and "triangulations" in drawn.columns
        if kind == "points":
            dodge = 0.0
            if 1 < nser <= 4:
                dodge = (i - (nser - 1) / 2.0) * 0.016
            xs, ys = _spread(drawn[x], drawn[y], logx, logy, label, dodge)
            if use_counts:
                # Outline keeps the series colour when several series share the axes.
                rim = "#333333" if mode == "single" else color
                colored += _scatter_by_count(
                    ax, xs, ys, drawn["triangulations"], count_scale[0], marker, 22, rim,
                )
            else:
                ax.scatter(
                    xs, ys, s=22, marker=marker, facecolors=face, edgecolors=edge,
                    linewidths=0.75, zorder=3,
                )
        else:
            ax.plot(
                drawn[x], drawn[y], color=color,
                marker=None if use_counts else marker, linestyle=ls,
                lw=1.6, ms=5.5, markerfacecolor=color, markeredgecolor=edge,
                markeredgewidth=0.7, zorder=3,
            )
            if use_counts:
                rim = "#333333" if mode == "single" else color
                colored += _scatter_by_count(
                    ax, drawn[x], drawn[y], drawn["triangulations"],
                    count_scale[0], marker, 30, rim, zorder=4,
                )
    if vline is not None:
        ax.axvline(vline, color="#444444", ls="--", lw=1.1, zorder=2)
    if hline is not None:
        ax.axhline(hline, color="#444444", ls="-.", lw=1.1, zorder=2)

    rate_x = x in ("check_rate", "trav_rate")
    rate_y = y in ("check_rate", "trav_rate")
    style_ax(
        ax,
        f"{title_prefix}\n{spec['title']}",
        spec["xlabel"], spec["ylabel"],
        spec.get("logx", False), spec.get("logy", False),
        rate_x=rate_x, rate_y=rate_y,
    )
    if count_scale is not None and colored:
        _add_count_colorbar(fig, ax, count_scale[0], count_scale[1])
    outside = add_legend(
        ax, [] if mode == "single" else series, mode,
        vline_label=vline_label, hline_label=hline_label,
        swatch="edge" if count_scale is not None and colored and kind == "points" else "colour",
    )
    if not outside:
        fig.tight_layout()
    save_fig(fig, folder, spec["name"])
    return True


def plot_suite(df_time, df_mem, folder, mode, title_prefix):
    n = 0
    for spec in TIME_PLOTS:
        n += plot_xy(df_time, folder, spec, mode, title_prefix)
    for spec in MEM_PLOTS:
        n += plot_xy(df_mem, folder, spec, mode, title_prefix)
    return n


# ------------------------------------------------------------------------------
# Extra charts
# ------------------------------------------------------------------------------
def _avg_vs_rate_specs(metric, rate_key, rate_slug, rate_xlabel, rate_title):
    """Points and a line of avg time for one CPU column against a success rate."""
    y = metric["avg"]
    ylabel = f"{metric['avg_label']} (s, log scale)"
    title = f"{metric['avg_title']} vs {rate_title}"
    if metric["key"] == "total":
        point_name = f"12_points_avg_time_vs_{rate_slug}.png"
        line_name = f"13_line_avg_time_vs_{rate_slug}.png"
    else:
        point_name = f"12{metric['tag']}_points_{metric['file_avg']}_vs_{rate_slug}.png"
        line_name = f"13{metric['tag']}_line_{metric['file_avg']}_vs_{rate_slug}.png"
    common = dict(
        x=rate_key, y=y, xlabel=rate_xlabel, ylabel=ylabel, title=title,
        logx=False, logy=True,
    )
    return [
        dict(name=point_name, kind="points", **common),
        dict(name=line_name, kind="line", **common),
    ]


def plot_check_avg(df, folder, mode, title_prefix):
    for metric in TIME_METRICS:
        for spec in _avg_vs_rate_specs(
            metric, "check_rate", "check_success",
            "check success rate (%)", "check success rate",
        ):
            plot_xy(df, folder, spec, mode, title_prefix)


def plot_trav_avg(df, folder, mode, title_prefix):
    for metric in TIME_METRICS:
        for spec in _avg_vs_rate_specs(
            metric, "trav_rate", "traversal_success",
            "traversal success rate (%)", "traversal success rate",
        ):
            plot_xy(df, folder, spec, mode, title_prefix)


def plot_trav_vs_check_colored(df, folder, title_prefix):
    need = ["trav_rate", "check_rate", "avg_tpt", "input_type"]
    if df is None or df.empty or any(c not in df.columns for c in need):
        return
    sub = df.dropna(subset=["trav_rate", "check_rate", "avg_tpt"])
    sub = sub[sub["avg_tpt"] > 0]
    if sub.empty:
        return

    vmin = float(sub["avg_tpt"].min())
    vmax = float(sub["avg_tpt"].max())
    use_log = vmax / vmin >= 20
    if use_log and vmin == vmax:
        use_log = False
    norm = mcolors.LogNorm(vmin=vmin, vmax=vmax) if use_log else mcolors.Normalize(vmin=vmin, vmax=vmax)
    cmap = plt.colormaps["RdYlGn_r"]

    fig, ax = plt.subplots(figsize=(8.6, 5.8))
    present = [it for it in ("Biconnected", "Oneconnected") if (sub["input_type"] == it).any()]
    # Higher check-success points first, so points lower on the chart stay visible.
    present.sort(key=lambda it: float(sub.loc[sub["input_type"] == it, "check_rate"].median()), reverse=True)
    sc = None
    for inp in present:
        part = sub[sub["input_type"] == inp].sort_values("check_rate", ascending=False)
        dodge = 0.0 if len(present) < 2 else (-0.012 if inp == "Biconnected" else 0.012)
        xs, ys = _spread(part["trav_rate"], part["check_rate"], False, False, inp, dodge)
        sc = _scatter_by_time(
            ax, xs, ys, part["avg_tpt"], cmap, norm,
            INPUT_MARKER[inp], 20, label=f"{inp} input",
        )
    cbar = fig.colorbar(sc, ax=ax)
    cbar.set_label(
        "avg time per triangulation (s)" + (" — log scale, green = low, red = high" if use_log
                                            else " — green = low, red = high")
    )
    if len(present) > 1:
        ax.legend(title="input type", fontsize=8, framealpha=0.92)
    style_ax(
        ax,
        f"{title_prefix}\ntraversal success vs check success, coloured by avg time",
        "traversal success rate (%)",
        "check success rate (%)",
        logx=False, logy=False, rate_x=True, rate_y=True,
    )
    fig.tight_layout()
    save_fig(fig, folder, "14_traversal_vs_check_colored_by_avg_time.png")


def _idw_grid(x, y, values, res=130, k=24, power=1.0):
    """Inverse-distance field. Weight of a sample is 1/distance**power.

    Coincident samples must already be averaged. Each cell uses its nearest
    samples, so a point 5 units away outweighs a point 7 units away by 7/5.
    """
    pts = np.column_stack([np.asarray(x, float), np.asarray(y, float)])
    values = np.asarray(values, float)
    n = len(values)
    k = max(1, min(k, n))
    gx = np.linspace(-3, 103, res)
    gy = np.linspace(-3, 103, res)
    xx, yy = np.meshgrid(gx, gy)
    flat = np.column_stack([xx.ravel(), yy.ravel()])
    out = np.empty(flat.shape[0], dtype=float)
    for start in range(0, len(flat), 2000):
        block = flat[start:start + 2000]
        dist = np.sqrt(((block[:, None, :] - pts[None, :, :]) ** 2).sum(axis=2))
        if k < n:
            nearest = np.argpartition(dist, k - 1, axis=1)[:, :k]
            dist = np.take_along_axis(dist, nearest, axis=1)
            picked = values[nearest]
        else:
            picked = np.broadcast_to(values, dist.shape)
        weights = 1.0 / np.maximum(dist, 1e-3) ** power
        out[start:start + len(block)] = (weights * picked).sum(axis=1) / weights.sum(axis=1)
    return out.reshape(xx.shape)


def _scatter_by_time(ax, xs, ys, times, cmap, norm, marker, size, label=None):
    """Point outline matches the average-time colour. No dark ring."""
    return ax.scatter(
        xs, ys, c=np.asarray(times, float), cmap=cmap, norm=norm,
        marker=marker, s=size, linewidths=0.45, edgecolors="face",
        zorder=3, label=label, rasterized=True,
    )


def _time_colorbar(fig, mappable, ax, use_log, estimated=False):
    cbar = fig.colorbar(mappable, ax=ax)
    name = "estimated avg time per triangulation (s)" if estimated else "avg time per triangulation (s)"
    cbar.set_label(
        name + (" — log scale, " if use_log else " — ") + "green = low, red = high"
    )
    return cbar


def _input_present(df):
    known = [it for it in ("Biconnected", "Oneconnected") if (df["input_type"] == it).any()]
    extra = [it for it in sorted(df["input_type"].unique()) if it not in known]
    return known + extra


def plot_trav_vs_check_idw(df, folder, title_prefix):
    """Fill the traversal/check plane from nearby cases, weighted by 1/distance.

    Writes the area with coloured points, then the same area with no points.
    """
    need = ["trav_rate", "check_rate", "avg_tpt", "input_type"]
    if df is None or df.empty or any(c not in df.columns for c in need):
        return
    sub = df.dropna(subset=["trav_rate", "check_rate", "avg_tpt"])
    sub = sub[sub["avg_tpt"] > 0]
    if sub.empty:
        return
    sub = sub.copy()
    sub["trav_rate"] = sub["trav_rate"].round(2)
    sub["check_rate"] = sub["check_rate"].round(2)
    averaged = sub.groupby(["trav_rate", "check_rate"], as_index=False)["avg_tpt"].mean()

    vmin = float(averaged["avg_tpt"].min())
    vmax = float(averaged["avg_tpt"].max())
    use_log = vmin > 0 and vmax / vmin >= 20 and vmin != vmax
    if vmin == vmax:
        vmax = vmin * 1.001 if vmin > 0 else vmin + 1.0
    norm = mcolors.LogNorm(vmin=vmin, vmax=vmax) if use_log else mcolors.Normalize(vmin=vmin, vmax=vmax)
    cmap = plt.colormaps["RdYlGn_r"]
    field = _idw_grid(averaged["trav_rate"], averaged["check_rate"], averaged["avg_tpt"])
    field = np.clip(field, vmin, vmax)
    present = _input_present(sub)

    def _draw(points, name, heading):
        fig, ax = plt.subplots(figsize=(8.6, 5.8))
        image = ax.imshow(
            field, origin="lower", extent=(-3, 103, -3, 103),
            cmap=cmap, norm=norm, aspect="auto", interpolation="bilinear", zorder=1,
        )
        if points:
            for inp in present:
                part = sub[sub["input_type"] == inp]
                _scatter_by_time(
                    ax, part["trav_rate"], part["check_rate"], part["avg_tpt"],
                    cmap, norm, INPUT_MARKER.get(inp, "o"), 16, label=f"{inp} input",
                )
            if len(present) > 1:
                ax.legend(title="input type", fontsize=8, framealpha=0.92)
        _time_colorbar(fig, image, ax, use_log, estimated=True)
        style_ax(
            ax, f"{title_prefix}\n{heading}",
            "traversal success rate (%)", "check success rate (%)",
            logx=False, logy=False, rate_x=True, rate_y=True,
        )
        fig.tight_layout()
        save_fig(fig, folder, name)

    _draw(True, "15_traversal_vs_check_idw_avg_time.png",
          "traversal vs check success, field weighted by 1/distance")
    _draw(False, "16_traversal_vs_check_idw_avg_time_area.png",
          "traversal vs check success, coloured area weighted by 1/distance")


def _log_idw_mesh(x, y, values, res=140, k=24, power=1.0, pad=0.06):
    """1/distance field on log10(x), log10(y). Returns the grid and positive edges."""
    x = np.asarray(x, float)
    y = np.asarray(y, float)
    values = np.asarray(values, float)
    xs = np.log10(x)
    ys = np.log10(y)

    def _span(a, b):
        if b <= a:
            return a - 0.05, b + 0.05
        extra = (b - a) * pad
        return a - extra, b + extra

    x0, x1 = _span(float(xs.min()), float(xs.max()))
    y0, y1 = _span(float(ys.min()), float(ys.max()))
    gx = np.linspace(x0, x1, res)
    gy = np.linspace(y0, y1, res)
    xx, yy = np.meshgrid(gx, gy)
    flat = np.column_stack([xx.ravel(), yy.ravel()])
    tree = cKDTree(np.column_stack([xs, ys]))
    kk = max(1, min(k, len(values)))
    dist, idx = tree.query(flat, k=kk)
    if kk == 1:
        dist = np.asarray(dist)[:, None]
        idx = np.asarray(idx)[:, None]
    weights = 1.0 / np.maximum(dist, 1e-6) ** power
    picked = values[idx]
    field = ((weights * picked).sum(axis=1) / weights.sum(axis=1)).reshape(xx.shape)

    def _edges(centers):
        step = np.diff(centers)
        edges = np.empty(len(centers) + 1)
        edges[1:-1] = (centers[:-1] + centers[1:]) / 2.0
        edges[0] = centers[0] - step[0] / 2.0
        edges[-1] = centers[-1] + step[-1] / 2.0
        return edges

    return field, np.power(10.0, _edges(gx)), np.power(10.0, _edges(gy))


def plot_vertices_vs_triangulations(df, folder, title_prefix):
    """Log vertices vs log triangulations, coloured by avg time per triangulation.

    Three charts: coloured points, 1/distance area with those points, and the
    area alone. Distance is measured in log10 space, matching the axes.
    """
    need = ["vertices", "triangulations", "avg_tpt", "input_type"]
    if df is None or df.empty or any(c not in df.columns for c in need):
        return
    sub = df.dropna(subset=["vertices", "triangulations", "avg_tpt"])
    sub = sub[(sub["vertices"] > 0) & (sub["triangulations"] > 0) & (sub["avg_tpt"] > 0)]
    if sub.empty:
        return

    vmin = float(sub["avg_tpt"].min())
    vmax = float(sub["avg_tpt"].max())
    use_log = vmin > 0 and vmax / vmin >= 20 and vmin != vmax
    if vmin == vmax:
        vmax = vmin * 1.001 if vmin > 0 else vmin + 1.0
    norm = mcolors.LogNorm(vmin=vmin, vmax=vmax) if use_log else mcolors.Normalize(vmin=vmin, vmax=vmax)
    cmap = plt.colormaps["RdYlGn_r"]
    present = _input_present(sub)
    xlabel = "vertices (log scale)"
    ylabel = "total triangulations (log scale)"

    fig, ax = plt.subplots(figsize=(8.6, 5.8))
    sc = None
    for inp in present:
        part = sub[sub["input_type"] == inp].sort_values("avg_tpt", ascending=False)
        dodge = 0.0 if len(present) < 2 else (-0.012 if inp == "Biconnected" else 0.012)
        xs, ys = _spread(part["vertices"], part["triangulations"], True, True, inp, dodge)
        sc = _scatter_by_time(
            ax, xs, ys, part["avg_tpt"], cmap, norm,
            INPUT_MARKER.get(inp, "o"), 18, label=f"{inp} input",
        )
    _time_colorbar(fig, sc, ax, use_log, estimated=False)
    if len(present) > 1:
        ax.legend(title="input type", fontsize=8, framealpha=0.92)
    style_ax(
        ax,
        f"{title_prefix}\nvertices vs triangulations, coloured by avg time",
        xlabel, ylabel, logx=True, logy=True,
    )
    fig.tight_layout()
    save_fig(fig, folder, "14_points_vertices_vs_triangulations_colored_by_avg_time.png")

    averaged = sub.groupby(["vertices", "triangulations"], as_index=False)["avg_tpt"].mean()
    field, xe, ye = _log_idw_mesh(
        averaged["vertices"], averaged["triangulations"], averaged["avg_tpt"],
    )
    field = np.clip(field, vmin, vmax)

    def _draw_field(points, name, heading):
        fig, ax = plt.subplots(figsize=(8.6, 5.8))
        image = ax.pcolormesh(
            xe, ye, field, cmap=cmap, norm=norm, shading="flat", zorder=1,
        )
        if points:
            for inp in present:
                part = sub[sub["input_type"] == inp].sort_values("avg_tpt", ascending=False)
                _scatter_by_time(
                    ax, part["vertices"], part["triangulations"], part["avg_tpt"],
                    cmap, norm, INPUT_MARKER.get(inp, "o"), 16, label=f"{inp} input",
                )
            if len(present) > 1:
                ax.legend(title="input type", fontsize=8, framealpha=0.92)
        _time_colorbar(fig, image, ax, use_log, estimated=True)
        style_ax(ax, f"{title_prefix}\n{heading}", xlabel, ylabel, logx=True, logy=True)
        ax.set_xlim(xe[0], xe[-1])
        ax.set_ylim(ye[0], ye[-1])
        fig.tight_layout()
        save_fig(fig, folder, name)

    _draw_field(
        True, "15_vertices_vs_triangulations_idw_avg_time.png",
        "vertices vs triangulations, field weighted by 1/distance",
    )
    _draw_field(
        False, "16_vertices_vs_triangulations_idw_avg_time_area.png",
        "vertices vs triangulations, coloured area weighted by 1/distance",
    )


def _wide_merge(df):
    """Inner-join the three algorithms on category + filename."""
    keys = ["category", "filename"]
    merged = None
    keep = [
        "input_type", "vertices", "triangulations",
        "time", "avg_tpt",
        "check_rate", "trav_rate",
    ]
    for algo, short in (
        ("Oneconnected", "oc"),
        ("BiconnectedWithoutVGS", "wo"),
        ("BiconnectedWithVGS", "w"),
    ):
        sub = df[df["algo"] == algo][keys + keep].copy()
        sub = sub.rename(columns={c: f"{c}_{short}" for c in keep})
        merged = sub if merged is None else merged.merge(sub, on=keys, how="inner")
    return merged


def plot_traversal_algorithm_comparison(df, folder, mode, title_prefix):
    """X = one-connected traversal success. Y = each algorithm's own time, same x."""
    if df is None or df.empty:
        return
    have = set(df["algo"].unique())
    if not set(ALGO_ORDER).issubset(have):
        return
    matched = _wide_merge(df)
    if matched is None or matched.empty:
        return
    matched = matched.dropna(subset=["trav_rate_oc"])

    # Category comparison uses algorithm colour; category is the marker when several are present.
    draw_mode = "by_algo" if mode == "by_algo" else "by_algo_category"
    shorts = (("oc", "Oneconnected"), ("wo", "BiconnectedWithoutVGS"), ("w", "BiconnectedWithVGS"))
    for metric in TIME_METRICS:
        pieces = []
        for short, algo in shorts:
            part = matched[[
                "category", "filename", "trav_rate_oc", f"triangulations_{short}",
                f"{metric['time']}_{short}", f"{metric['avg']}_{short}",
            ]].copy()
            part = part.rename(columns={
                "trav_rate_oc": "trav_rate",
                f"triangulations_{short}": "triangulations",
                f"{metric['time']}_{short}": "time",
                f"{metric['avg']}_{short}": "avg_tpt",
            })
            part["algo"] = algo
            part["input_type"] = "Biconnected"
            pieces.append(part)
        long = pd.concat(pieces, ignore_index=True)
        tag = metric["tag"]
        if metric["key"] == "total":
            names = (
                "12_points_total_time_vs_traversal_success.png",
                "13_line_total_time_vs_traversal_success.png",
                "14_points_avg_time_vs_traversal_success.png",
                "15_line_avg_time_vs_traversal_success.png",
            )
        else:
            names = (
                f"12{tag}_points_{metric['file_time']}_vs_traversal_success.png",
                f"13{tag}_line_{metric['file_time']}_vs_traversal_success.png",
                f"14{tag}_points_{metric['file_avg']}_vs_traversal_success.png",
                f"15{tag}_line_{metric['file_avg']}_vs_traversal_success.png",
            )
        specs = [
            (names[0], "points", "time", f"{metric['time_label']} (s, log scale)",
             f"{metric['time_title']} vs traversal success rate"),
            (names[1], "line", "time", f"{metric['time_label']} (s, log scale)",
             f"{metric['time_title']} vs traversal success rate"),
            (names[2], "points", "avg_tpt", f"{metric['avg_label']} (s, log scale)",
             f"{metric['avg_title']} vs traversal success rate"),
            (names[3], "line", "avg_tpt", f"{metric['avg_label']} (s, log scale)",
             f"{metric['avg_title']} vs traversal success rate"),
        ]
        for name, kind, y, ylabel, title in specs:
            spec = dict(
                name=name, kind=kind, x="trav_rate", y=y,
                xlabel="traversal success rate of the one-connected algorithm (%)",
                ylabel=ylabel, title=title, logx=False, logy=True,
            )
            plot_xy(long, folder, spec, draw_mode, title_prefix)


def plot_vgs_folder(matched, folder, mode, title_prefix):
    if matched is None or matched.empty:
        return
    # Colour is the algorithm. Category symbols and their legend are omitted so
    # the axes keep the space.
    draw_mode = "by_algo"
    ratio_mode = "single"
    for metric in TIME_METRICS:
        pieces_time = []
        pieces_avg = []
        for short, algo in (("wo", "BiconnectedWithoutVGS"), ("w", "BiconnectedWithVGS")):
            ycol_t = f"{metric['time']}_{short}"
            ycol_a = f"{metric['avg']}_{short}"
            if ycol_t not in matched.columns or ycol_a not in matched.columns:
                continue
            tri_col = f"triangulations_{short}"
            base = matched[["category", "filename", "check_rate_wo", tri_col, ycol_t, ycol_a]].copy()
            t = base.rename(columns={"check_rate_wo": "check_rate", ycol_t: "time"})
            t["algo"] = algo
            t["input_type"] = "Biconnected"
            pieces_time.append(t[["category", "filename", "check_rate", "time", "algo", "input_type"]])
            a = base.rename(columns={
                "check_rate_wo": "check_rate", ycol_a: "avg_tpt", tri_col: "triangulations",
            })
            a["algo"] = algo
            a["input_type"] = "Biconnected"
            pieces_avg.append(a[[
                "category", "filename", "check_rate", "avg_tpt", "triangulations", "algo", "input_type",
            ]])
        if not pieces_time:
            continue
        time_long = pd.concat(pieces_time, ignore_index=True)
        avg_long = pd.concat(pieces_avg, ignore_index=True)
        tag = metric["tag"]
        if metric["key"] == "total":
            names = (
                "01_points_total_time_vs_check_success.png",
                "02_line_total_time_vs_check_success.png",
                "03_points_avg_time_vs_check_success.png",
                "04_line_avg_time_vs_check_success.png",
            )
        else:
            names = (
                f"01{tag}_points_{metric['file_time']}_vs_check_success.png",
                f"02{tag}_line_{metric['file_time']}_vs_check_success.png",
                f"03{tag}_points_{metric['file_avg']}_vs_check_success.png",
                f"04{tag}_line_{metric['file_avg']}_vs_check_success.png",
            )
        both = [
            dict(name=names[0], kind="points", ydf=time_long, y="time",
                 ylabel=f"{metric['time_label']} (s, log scale)",
                 title=f"{metric['time_title']} vs without-VGS check success"),
            dict(name=names[1], kind="line", ydf=time_long, y="time",
                 ylabel=f"{metric['time_label']} (s, log scale)",
                 title=f"{metric['time_title']} vs without-VGS check success"),
            dict(name=names[2], kind="points", ydf=avg_long, y="avg_tpt",
                 ylabel=f"{metric['avg_label']} (s, log scale)",
                 title=f"{metric['avg_title']} vs without-VGS check success"),
            dict(name=names[3], kind="line", ydf=avg_long, y="avg_tpt",
                 ylabel=f"{metric['avg_label']} (s, log scale)",
                 title=f"{metric['avg_title']} vs without-VGS check success"),
        ]
        for spec in both:
            plot_xy(
                spec["ydf"], folder,
                dict(name=spec["name"], kind=spec["kind"], x="check_rate", y=spec["y"],
                     xlabel="check success rate of without VGS (%)",
                     ylabel=spec["ylabel"], title=spec["title"], logx=False, logy=True),
                draw_mode, title_prefix,
            )

        left = f"{metric['time']}_w"
        right = f"{metric['time']}_wo"
        if left not in matched.columns or right not in matched.columns:
            continue
        ratio = matched[(matched[right] > 0) & (matched[left] > 0)].copy()
        if ratio.empty:
            continue
        ratio["time_ratio"] = ratio[left] / ratio[right]
        ratio["wo_over_w"] = ratio[right] / ratio[left]
        ratio["check_rate"] = ratio["check_rate_wo"]
        ratio["input_type"] = "Biconnected"
        phase = metric["time_title"]
        if metric["key"] == "total":
            ratio_names = (
                "05_points_time_ratio_vs_check_success.png",
                "06_line_time_ratio_vs_check_success.png",
                "07_points_without_over_with_time_vs_check_success.png",
                "08_line_without_over_with_time_vs_check_success.png",
            )
        else:
            bit = metric["file_time"]
            ratio_names = (
                f"05{tag}_points_{bit}_ratio_vs_check_success.png",
                f"06{tag}_line_{bit}_ratio_vs_check_success.png",
                f"07{tag}_points_without_over_with_{bit}_vs_check_success.png",
                f"08{tag}_line_without_over_with_{bit}_vs_check_success.png",
            )
        for kind, name in (("points", ratio_names[0]), ("line", ratio_names[1])):
            plot_xy(
                ratio, folder,
                dict(name=name, kind=kind, x="time_ratio", y="check_rate",
                     xlabel=f"{phase} with VGS / {phase} without VGS (log scale)",
                     ylabel="successful checks / total checks, without VGS (%)",
                     title=f"check success vs with-VGS / without-VGS {phase}",
                     logx=True, logy=False),
                ratio_mode, title_prefix, vline=1.0, vline_label=metric["equal_label"],
            )
        # Same check-success x as the other VGS charts. Y is how many times slower
        # without VGS is than with VGS on that case.
        for kind, name in (("points", ratio_names[2]), ("line", ratio_names[3])):
            plot_xy(
                ratio, folder,
                dict(name=name, kind=kind, x="check_rate", y="wo_over_w",
                     xlabel="check success rate of without VGS (%)",
                     ylabel=f"{phase} without VGS / {phase} with VGS (log scale)",
                     title=f"without-VGS / with-VGS {phase} vs check success",
                     logx=False, logy=True),
                ratio_mode, title_prefix, hline=1.0, hline_label=metric["equal_label"],
            )


def vgs_matched(df):
    value_cols = ["time", "avg_tpt"]
    wo = df[df["algo"] == "BiconnectedWithoutVGS"][
        ["category", "filename", "check_rate", "triangulations"] + value_cols
    ].rename(columns={
        "time": "time_wo",
        "avg_tpt": "avg_tpt_wo",
        "check_rate": "check_rate_wo",
        "triangulations": "triangulations_wo",
    })
    w = df[df["algo"] == "BiconnectedWithVGS"][
        ["category", "filename", "triangulations"] + value_cols
    ].rename(columns={
        "time": "time_w",
        "avg_tpt": "avg_tpt_w",
        "triangulations": "triangulations_w",
    })
    merged = wo.merge(w, on=["category", "filename"], how="inner")
    merged = merged.dropna(subset=["check_rate_wo", "time_wo", "time_w", "avg_tpt_wo", "avg_tpt_w"])
    return merged


# ------------------------------------------------------------------------------
# Drivers
# ------------------------------------------------------------------------------
def _with_columns(df, columns):
    """Empty loads have no columns. Give them the schema later filters expect."""
    if df is None or len(df) == 0:
        return pd.DataFrame(columns=list(columns))
    return df


def _cats(df):
    if df is None or df.empty or "category" not in df.columns:
        return []
    return sorted(df["category"].unique())


def _algo_categories(df, algo):
    if df is None or df.empty or "algo" not in df.columns or "category" not in df.columns:
        return set()
    return set(df.loc[df["algo"] == algo, "category"])


def _by_algo(df, algo):
    if df is None or df.empty or "algo" not in df.columns:
        return df.iloc[0:0] if df is not None else pd.DataFrame()
    return df[df["algo"] == algo]


def _plot_time_extras(algo, df, folder, mode, title):
    if algo == "BiconnectedWithoutVGS":
        plot_check_avg(df, folder, mode, title)
    if algo in ("BiconnectedWithoutVGS", "BiconnectedWithVGS"):
        plot_vertices_vs_triangulations(df, folder, title)
    if algo == "Oneconnected":
        plot_trav_avg(df, folder, mode, title)
        plot_trav_vs_check_colored(df, folder, title)
        plot_trav_vs_check_idw(df, folder, title)


def make_algorithmwise(time_df, mem_df):
    time_df = _with_columns(time_df, TIME_COLUMNS)
    mem_df = _with_columns(mem_df, MEM_COLUMNS)
    for algo in ALGO_ORDER:
        t_algo = _by_algo(time_df, algo)
        m_algo = _by_algo(mem_df, algo)
        if t_algo.empty and m_algo.empty:
            continue
        inputs = sorted(set(t_algo["input_type"].unique()) | set(m_algo["input_type"].unique()))
        for inp in inputs:
            t_in = t_algo[t_algo["input_type"] == inp]
            m_in = m_algo[m_algo["input_type"] == inp]
            in_folder = f"{inp}Input"
            for cat in sorted(set(_cats(t_in)) | set(_cats(m_in))):
                folder = os.path.join(OUT_ROOT, "Algorithmwise", ALGO_OUT[algo], in_folder, cat)
                title = f"{ALGO_LABEL[algo]} · {inp} input · {cat}"
                print(f"  {os.path.relpath(folder, OUT_ROOT)}")
                plot_suite(
                    t_in[t_in["category"] == cat],
                    m_in[m_in["category"] == cat],
                    folder, "single", title,
                )
                _plot_time_extras(algo, t_in[t_in["category"] == cat], folder, "single", title)

            folder = os.path.join(OUT_ROOT, "Algorithmwise", ALGO_OUT[algo], in_folder, "combined")
            title = f"{ALGO_LABEL[algo]} · {inp} inputs · all categories"
            print(f"  {os.path.relpath(folder, OUT_ROOT)}")
            plot_suite(t_in, m_in, folder, "by_category", title)
            _plot_time_extras(algo, t_in, folder, "by_category", title)

        folder = os.path.join(OUT_ROOT, "Algorithmwise", ALGO_OUT[algo], "combined")
        title = f"{ALGO_LABEL[algo]} · all inputs"
        both_inputs = set(t_algo["input_type"].unique()) >= {"Biconnected", "Oneconnected"}
        mode = "by_input_family" if both_inputs else "by_category"
        if both_inputs:
            title += "  (blue family = biconnected input, red family = oneconnected input)"
        print(f"  {os.path.relpath(folder, OUT_ROOT)}")
        plot_suite(t_algo, m_algo, folder, mode, title)
        _plot_time_extras(algo, t_algo, folder, mode, title)


def _input_types(df):
    if df is None or df.empty or "input_type" not in df.columns:
        return set()
    return set(df["input_type"].unique())


def make_comparison(time_df, mem_df):
    time_df = _with_columns(time_df, TIME_COLUMNS)
    mem_df = _with_columns(mem_df, MEM_COLUMNS)
    inputs = sorted(_input_types(time_df) | _input_types(mem_df))
    for inp in inputs:
        t_in = time_df[time_df["input_type"] == inp]
        m_in = mem_df[mem_df["input_type"] == inp]
        for cat in sorted(set(_cats(t_in)) | set(_cats(m_in))):
            folder = os.path.join(OUT_ROOT, "Comparison", f"{inp}Input", cat)
            title = f"Comparison · {inp} input · {cat}"
            print(f"  {os.path.relpath(folder, OUT_ROOT)}")
            plot_suite(
                t_in[t_in["category"] == cat],
                m_in[m_in["category"] == cat],
                folder, "by_algo", title,
            )
            if inp == "Biconnected":
                plot_traversal_algorithm_comparison(
                    t_in[t_in["category"] == cat], folder, "by_algo", title,
                )
        folder = os.path.join(OUT_ROOT, "Comparison", f"{inp}Input", "combined")
        title = f"Comparison · {inp} inputs · all categories"
        print(f"  {os.path.relpath(folder, OUT_ROOT)}")
        plot_suite(t_in, m_in, folder, "by_algo", title)
        if inp == "Biconnected":
            plot_traversal_algorithm_comparison(t_in, folder, "by_algo", title)

    folder = os.path.join(OUT_ROOT, "Comparison", "Combined")
    title = "Comparison · all inputs  (colour = algorithm, symbol = input type)"
    print(f"  {os.path.relpath(folder, OUT_ROOT)}")
    plot_suite(time_df, mem_df, folder, "by_algo_input", title)
    # Traversal-rate comparison only exists where all three algorithms ran (biconnected inputs).
    plot_traversal_algorithm_comparison(
        time_df[time_df["input_type"] == "Biconnected"],
        folder, "by_algo",
        "Comparison · all inputs · cases run by all three algorithms",
    )


def make_vgs(time_df, mem_df):
    time_df = _with_columns(time_df, TIME_COLUMNS)
    mem_df = _with_columns(mem_df, MEM_COLUMNS)
    pair = ["BiconnectedWithoutVGS", "BiconnectedWithVGS"]
    t_pair = time_df[time_df["algo"].isin(pair)] if not time_df.empty else time_df
    m_pair = mem_df[mem_df["algo"].isin(pair)] if not mem_df.empty else mem_df
    matched = vgs_matched(time_df) if not time_df.empty else pd.DataFrame()
    cats = sorted(set(_cats(t_pair)) | set(_cats(m_pair)))
    if not cats:
        print("  no with-VGS / without-VGS rows")
        return
    for cat in cats:
        folder = os.path.join(OUT_ROOT, "VGSvsnonVGS", "BiconnectedInput", cat)
        title = f"With VGS vs without VGS · {cat}"
        print(f"  {os.path.relpath(folder, OUT_ROOT)}")
        plot_suite(
            t_pair[t_pair["category"] == cat],
            m_pair[m_pair["category"] == cat],
            folder, "by_algo", title,
        )
        if not matched.empty:
            plot_vgs_folder(matched[matched["category"] == cat], folder, "single", title)
    folder = os.path.join(OUT_ROOT, "VGSvsnonVGS", "BiconnectedInput", "combined")
    title = "With VGS vs without VGS · all biconnected categories"
    print(f"  {os.path.relpath(folder, OUT_ROOT)}")
    plot_suite(t_pair, m_pair, folder, "by_algo", title)
    if not matched.empty:
        plot_vgs_folder(matched, folder, "single", title)


def main():
    print("Loading benchmark CSVs...")
    time_df = load_time()
    mem_df = load_memory()
    if time_df.empty and mem_df.empty:
        print("No benchmark rows found.")
        return
    print(f"  time rows: {len(time_df)}   memory rows: {len(mem_df)}")
    time_df = _with_columns(time_df, TIME_COLUMNS)
    mem_df = _with_columns(mem_df, MEM_COLUMNS)
    for algo in ALGO_ORDER:
        n_t = int((time_df["algo"] == algo).sum()) if "algo" in time_df.columns else 0
        n_m = int((mem_df["algo"] == algo).sum()) if "algo" in mem_df.columns else 0
        cats = sorted(_algo_categories(time_df, algo) | _algo_categories(mem_df, algo))
        print(f"  {ALGO_LABEL[algo]}: {n_t} time, {n_m} memory, {len(cats)} categories")

    if os.path.isdir(OUT_ROOT):
        shutil.rmtree(OUT_ROOT)
    os.makedirs(OUT_ROOT, exist_ok=True)

    print("Algorithm-wise charts...")
    make_algorithmwise(time_df, mem_df)
    print("Comparison charts...")
    make_comparison(time_df, mem_df)
    print("VGS vs without-VGS charts...")
    make_vgs(time_df, mem_df)

    n_png = sum(1 for _root, _dirs, files in os.walk(OUT_ROOT) for f in files if f.endswith(".png"))
    print(f"Done. {n_png} plots in {OUT_ROOT}")


if __name__ == "__main__":
    main()
