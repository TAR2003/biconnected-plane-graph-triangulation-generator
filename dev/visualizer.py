#!/usr/bin/env python3
"""
Visualize planar graphs from adjacency-list text files.

Input format (per file):
    N
    k a1 a2 ... ak      (one line per vertex, in vertex order)

Usage:
    python draw_planar.py --input input --output output
"""

import argparse
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")  # headless, no display needed
import matplotlib.pyplot as plt
import networkx as nx


def parse_graph(path: Path) -> nx.Graph:
    """Parse one adjacency-list file into a simple undirected graph."""
    tokens = []
    with open(path, "r") as f:
        for line in f:
            line = line.split("#")[0].split("//")[0].strip()
            if line:
                tokens.append(line.split())

    if not tokens:
        raise ValueError("empty file")

    n = int(tokens[0][0])
    rows = tokens[1:]
    if len(rows) < n:
        raise ValueError(f"expected {n} adjacency rows, found {len(rows)}")
    rows = rows[:n]

    adj = []
    for i, row in enumerate(rows):
        nums = list(map(int, row))
        k, nbrs = nums[0], nums[1:]
        if len(nbrs) != k:
            raise ValueError(
                f"vertex row {i}: declared {k} neighbours, found {len(nbrs)}"
            )
        adj.append(nbrs)

    # Auto-detect indexing: if any neighbour == n, or none == 0, it's 1-based.
    all_ids = [v for nbrs in adj for v in nbrs]
    if all_ids:
        if max(all_ids) >= n:
            offset = 1
        elif min(all_ids) == 0:
            offset = 0
        else:
            offset = 1 if min(all_ids) >= 1 and max(all_ids) == n else 0
    else:
        offset = 0

    G = nx.Graph()
    G.add_nodes_from(range(n))
    for u, nbrs in enumerate(adj):
        for v in nbrs:
            v -= offset
            if not (0 <= v < n):
                raise ValueError(f"vertex {u}: neighbour {v + offset} out of range")
            if u == v:
                continue  # ignore self-loops
            G.add_edge(u, v)
    return G


def planar_positions(G: nx.Graph):
    """
    Return (positions, embedding) for a straight-line crossing-free drawing.
    Works on disconnected graphs by drawing each component separately and
    placing them side by side.
    """
    is_planar, _ = nx.check_planarity(G)
    if not is_planar:
        return None

    pos = {}
    x_shift = 0.0
    for comp in nx.connected_components(G):
        H = G.subgraph(comp).copy()
        if H.number_of_nodes() == 1:
            v = next(iter(H.nodes))
            pos[v] = (x_shift, 0.0)
            x_shift += 2.0
            continue
        if H.number_of_nodes() == 2:
            a, b = list(H.nodes)
            pos[a] = (x_shift, 0.0)
            pos[b] = (x_shift + 1.0, 0.0)
            x_shift += 3.0
            continue

        _, emb = nx.check_planarity(H)
        # The FPP algorithm needs a triangulated embedding; networkx's
        # combinatorial_embedding_to_pos triangulates internally.
        p = nx.combinatorial_embedding_to_pos(emb, fully_triangulate=False)
        xs = [c[0] for c in p.values()]
        lo, hi = min(xs), max(xs)
        for v, (x, y) in p.items():
            pos[v] = (x - lo + x_shift, y)
        x_shift += (hi - lo) + 3.0
    return pos


def draw_graph(G: nx.Graph, pos: dict, out_path: Path, title: str):
    n = G.number_of_nodes()
    xs = [p[0] for p in pos.values()]
    ys = [p[1] for p in pos.values()]
    w = max(xs) - min(xs) + 1
    h = max(ys) - min(ys) + 1

    # Scale the figure with graph extent, within sane bounds
    scale = 0.55
    fig_w = min(max(6, w * scale), 40)
    fig_h = min(max(6, h * scale), 40)

    fig, ax = plt.subplots(figsize=(fig_w, fig_h))
    node_size = max(120, min(900, 9000 / max(n, 1)))
    font_size = max(5, min(11, 200 / max(n, 1) + 4))

    nx.draw_networkx_edges(G, pos, ax=ax, width=1.2, edge_color="#444444")
    nx.draw_networkx_nodes(
        G, pos, ax=ax, node_size=node_size,
        node_color="#8ecae6", edgecolors="black", linewidths=1.0,
    )
    nx.draw_networkx_labels(G, pos, ax=ax, font_size=font_size)

    ax.set_title(f"{title}   (n={n}, m={G.number_of_edges()})", fontsize=10)
    ax.set_aspect("equal")
    ax.axis("off")
    fig.tight_layout()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(out_path, dpi=150)
    plt.close(fig)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--input", default="new-input", help="root input folder")
    ap.add_argument("--output", default="output-images", help="root output folder")
    ap.add_argument("--ext", default=".txt", help="input file extension")
    ap.add_argument("--format", default="png", choices=["png", "svg", "pdf"])
    args = ap.parse_args()

    in_root = Path(args.input).resolve()
    out_root = Path(args.output).resolve()
    if not in_root.is_dir():
        sys.exit(f"Input folder not found: {in_root}")

    files = sorted(p for p in in_root.rglob(f"*{args.ext}") if p.is_file())
    print(f"Found {len(files)} file(s) under {in_root}")

    ok = nonplanar = failed = 0
    for i, f in enumerate(files, 1):
        rel = f.relative_to(in_root)
        out_file = (out_root / rel).with_suffix("." + args.format)
        try:
            G = parse_graph(f)
            pos = planar_positions(G)
            if pos is None:
                print(f"[{i}/{len(files)}] NON-PLANAR, skipped: {rel}")
                nonplanar += 1
                continue
            draw_graph(G, pos, out_file, str(rel))
            ok += 1
            print(f"[{i}/{len(files)}] drawn: {rel}")
        except Exception as e:
            failed += 1
            print(f"[{i}/{len(files)}] ERROR in {rel}: {e}")

    print(f"\nDone. drawn={ok}, non-planar={nonplanar}, errors={failed}")


if __name__ == "__main__":
    main()