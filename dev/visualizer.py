#!/usr/bin/env python3
"""
Visualize planar graphs from adjacency-list text files (recursive folders).

Input format (per file):
    N
    k a1 a2 ... ak      (one line per vertex, in vertex order)

Pipeline per connected component:
  1. Strict planarity test + FPP straight-line grid drawing (crossing-free).
  2. Normalise to a unit square (affine map: keeps it crossing-free).
  3. Clearance maximisation: vertices are nudged so that the smallest
     distance among  {vertex-vertex, vertex-edge, edge-edge}  grows.
     (For non-crossing segments, edge-edge distance is always attained at an
     endpoint, so vertex-vertex + vertex-edge distances cover all three.)
     A move is accepted only if it strictly improves that vertex's local
     clearance AND creates no crossing, so the drawing is never made worse.
  4. Final crossing check; falls back to the plain FPP drawing if it fails.
  5. Node size is derived from the achieved clearance, so nodes never
     touch other nodes or edges.

Usage:
    python draw_planar.py --input new-input --output output-images
"""

import argparse
import math
import sys
import time
from pathlib import Path

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.collections import LineCollection
import networkx as nx


# ----------------------------------------------------------------------
# Parsing
# ----------------------------------------------------------------------
def parse_graph(path: Path) -> nx.Graph:
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
            raise ValueError(f"vertex row {i}: declared {k} neighbours, found {len(nbrs)}")
        adj.append(nbrs)

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
            if u != v:
                G.add_edge(u, v)
    return G


# ----------------------------------------------------------------------
# Geometry (vectorised)
# ----------------------------------------------------------------------
def pt_seg_dist(p, A, B):
    """Distance from point p to each segment A[k]-B[k]."""
    AB = B - A
    L2 = np.maximum((AB * AB).sum(1), 1e-18)
    t = np.clip(((p - A) * AB).sum(1) / L2, 0.0, 1.0)
    D = p - (A + t[:, None] * AB)
    return np.sqrt((D * D).sum(1))


class Drawing:
    """Vertex positions P (n x 2) and edge list E (m x 2) with fast local tests."""

    def __init__(self, P, E):
        self.P = np.array(P, dtype=float)
        self.E = np.array(E, dtype=int).reshape(-1, 2)
        n, m = len(self.P), len(self.E)
        nbr = [[] for _ in range(n)]
        for a, b in self.E:
            nbr[a].append(b)
            nbr[b].append(a)
        self.nbr = [np.array(x, dtype=int) for x in nbr]
        ea, eb = self.E[:, 0], self.E[:, 1]
        self.non_inc = [np.nonzero((ea != i) & (eb != i))[0] for i in range(n)]

    def local_clear(self, i, p):
        """Smallest distance involving vertex i if it were placed at p."""
        P, E = self.P, self.E
        d = np.sqrt(((P - p) ** 2).sum(1))
        d[i] = np.inf
        c = d.min()
        ne = self.non_inc[i]
        if len(ne):                                   # i vs. other edges
            c = min(c, pt_seg_dist(p, P[E[ne, 0]], P[E[ne, 1]]).min())
        nb = self.nbr[i]
        if len(nb):                                   # i's edges vs. other vertices
            ab = P[nb] - p
            L2 = np.maximum((ab * ab).sum(1), 1e-18)
            t = np.clip(((P[None] - p) * ab[:, None]).sum(2) / L2[:, None], 0.0, 1.0)
            diff = P[None] - (p + t[:, :, None] * ab[:, None])
            dd = np.sqrt((diff * diff).sum(2))
            dd[:, i] = np.inf
            dd[np.arange(len(nb)), nb] = np.inf
            c = min(c, dd.min())
        return c

    def crosses(self, i, p):
        """Would i's edges, with i at p, properly cross any other edge?"""
        P, E = self.P, self.E
        ne = self.non_inc[i]
        if not len(ne):
            return False
        C, D = P[E[ne, 0]], P[E[ne, 1]]
        DC = D - C
        d1 = DC[:, 0] * (p[1] - C[:, 1]) - DC[:, 1] * (p[0] - C[:, 0])
        for w in self.nbr[i]:
            q = P[w]
            keep = (E[ne, 0] != w) & (E[ne, 1] != w)
            if not keep.any():
                continue
            Ck, Dk, DCk = C[keep], D[keep], DC[keep]
            d1k = d1[keep]
            d2 = DCk[:, 0] * (q[1] - Ck[:, 1]) - DCk[:, 1] * (q[0] - Ck[:, 0])
            pq = q - p
            d3 = pq[0] * (Ck[:, 1] - p[1]) - pq[1] * (Ck[:, 0] - p[0])
            d4 = pq[0] * (Dk[:, 1] - p[1]) - pq[1] * (Dk[:, 0] - p[0])
            if np.any((d1k * d2 < 0) & (d3 * d4 < 0)):
                return True
        return False

    def clearance(self):
        return min(self.local_clear(i, self.P[i]) for i in range(len(self.P)))

    def has_crossing(self):
        P, E = self.P, self.E
        m = len(E)
        for k in range(m - 1):
            a, b = E[k]
            rest = E[k + 1:]
            ok = (rest[:, 0] != a) & (rest[:, 0] != b) & (rest[:, 1] != a) & (rest[:, 1] != b)
            rest = rest[ok]
            if not len(rest):
                continue
            C, D = P[rest[:, 0]], P[rest[:, 1]]
            ab = P[b] - P[a]
            d1 = ab[0] * (C[:, 1] - P[a][1]) - ab[1] * (C[:, 0] - P[a][0])
            d2 = ab[0] * (D[:, 1] - P[a][1]) - ab[1] * (D[:, 0] - P[a][0])
            cd = D - C
            d3 = cd[:, 0] * (P[a][1] - C[:, 1]) - cd[:, 1] * (P[a][0] - C[:, 0])
            d4 = cd[:, 0] * (P[b][1] - C[:, 1]) - cd[:, 1] * (P[b][0] - C[:, 0])
            if np.any((d1 * d2 < 0) & (d3 * d4 < 0)):
                return True
        return False


# ----------------------------------------------------------------------
# Layout of one component
# ----------------------------------------------------------------------
def refine(dr: Drawing, iters: int, time_limit: float, seed: int = 0):
    """Greedy max-min spreading. Never accepts a crossing or a worse local clearance."""
    n = len(dr.P)
    rng = np.random.default_rng(seed)
    t0 = time.time()
    for it in range(iters):
        step = max(0.08 * (0.9 ** it), 0.002)
        cl = np.array([dr.local_clear(i, dr.P[i]) for i in range(n)])
        gmin = cl.min()
        order = [i for i in np.argsort(cl) if cl[i] < 3.0 * gmin]
        moved = 0
        for i in order:
            if time.time() - t0 > time_limit:
                return
            base = dr.local_clear(i, dr.P[i])
            phase = rng.random() * 2 * math.pi
            cands = []
            for k in range(6):
                a = phase + k * math.pi / 3
                r = step * (0.5 + rng.random())
                q = dr.P[i] + r * np.array([math.cos(a), math.sin(a)])
                cands.append(np.clip(q, 0.0, 1.0))
            scored = sorted(((dr.local_clear(i, q), j) for j, q in enumerate(cands)),
                            key=lambda t: -t[0])
            for c, j in scored:
                if c <= base * (1 + 1e-9):
                    break
                if not dr.crosses(i, cands[j]):
                    dr.P[i] = cands[j]
                    moved += 1
                    break
        if moved == 0 and step <= 0.004:
            break


def layout_component(H: nx.Graph, iters: int, time_limit: float):
    nodes = list(H.nodes)
    n = len(nodes)
    if n == 1:
        return {nodes[0]: np.array([0.0, 0.0])}
    if n == 2:
        return {nodes[0]: np.array([0.0, 0.0]), nodes[1]: np.array([1.0, 0.0])}

    _, emb = nx.check_planarity(H)
    raw = nx.combinatorial_embedding_to_pos(emb, fully_triangulate=False)
    P = np.array([raw[v] for v in nodes], dtype=float)
    P -= P.min(axis=0)
    span = P.max(axis=0)
    span[span == 0] = 1.0
    P /= span                                          # unit square (affine: safe)

    idx = {v: i for i, v in enumerate(nodes)}
    E = [(idx[u], idx[v]) for u, v in H.edges()]
    dr = Drawing(P, E)
    original = dr.P.copy()

    if iters > 0:
        refine(dr, iters, time_limit)
        if dr.has_crossing():                          # safety net, should not happen
            dr.P = original

    s = max(1.0, math.sqrt(n) / 3.0)                   # bigger graphs get more room
    return {v: dr.P[idx[v]] * s for v in nodes}


def planar_positions(G: nx.Graph, iters: int, time_limit: float):
    is_planar, _ = nx.check_planarity(G)
    if not is_planar:
        return None
    pos = {}
    x_cursor = 0.0
    comps = sorted(nx.connected_components(G), key=len, reverse=True)
    for comp in comps:
        H = G.subgraph(comp).copy()
        p = layout_component(H, iters, time_limit)
        xs = [c[0] for c in p.values()]
        lo, hi = min(xs), max(xs)
        for v, c in p.items():
            pos[v] = np.array([c[0] - lo + x_cursor, c[1]])
        x_cursor += (hi - lo) + 0.6
    return pos


def drawing_clearance(G: nx.Graph, pos: dict) -> float:
    nodes = list(G.nodes())
    idx = {v: i for i, v in enumerate(nodes)}
    P = np.array([pos[v] for v in nodes])
    E = [(idx[u], idx[v]) for u, v in G.edges()]
    if len(nodes) < 2:
        return 1.0
    if not E:
        d = np.sqrt(((P[:, None] - P[None]) ** 2).sum(2))
        np.fill_diagonal(d, np.inf)
        return float(d.min())
    return float(Drawing(P, E).clearance())


# ----------------------------------------------------------------------
# Drawing
# ----------------------------------------------------------------------
def draw_graph(G: nx.Graph, pos: dict, out_path: Path, title: str):
    n = G.number_of_nodes()
    nodes = list(G.nodes())
    pts = np.array([pos[v] for v in nodes])
    xs, ys = pts[:, 0], pts[:, 1]
    w = max(np.ptp(xs), 1e-6)
    h = max(np.ptp(ys), 1e-6)

    clr = max(drawing_clearance(G, pos), 1e-9)
    radius = 0.36 * clr                                # node never touches anything
    pad = max(0.06 * max(w, h), 2 * radius)
    W, H = w + 2 * pad, h + 2 * pad

    # choose a figure big enough for ~26pt nodes, within sane limits
    ppu_wanted = 26.0 / (2 * radius)
    fig_w = float(np.clip(ppu_wanted * W / 72, 5, 50))
    fig_h = float(np.clip(ppu_wanted * H / 72, 5, 50))
    fig, ax = plt.subplots(figsize=(fig_w, fig_h))

    ppu = 0.88 * min(fig_w * 72 / W, fig_h * 72 / H)   # effective points per unit
    diam = max(2 * radius * ppu, 2.0)
    lw = float(np.clip(diam * 0.07, 0.5, 2.0))

    segs = [(pos[u], pos[v]) for u, v in G.edges()]
    ax.add_collection(LineCollection(segs, colors="#444444", linewidths=lw, zorder=1))
    ax.scatter(xs, ys, s=diam ** 2, c="#8ecae6", edgecolors="black",
               linewidths=max(0.4, min(1.0, diam * 0.04)), zorder=2)

    digits = len(str(max(n - 1, 1)))
    if diam >= 11:
        fs = min(14.0, 0.65 * diam, 0.8 * diam / (0.6 * digits))
        for v, (x, y) in zip(nodes, pts):
            ax.text(x, y, str(v), ha="center", va="center", fontsize=fs, zorder=3)

    ax.set_xlim(xs.min() - pad, xs.max() + pad)
    ax.set_ylim(ys.min() - pad, ys.max() + pad)
    ax.set_aspect("equal")
    ax.axis("off")
    ax.set_title(f"{title}   (n={n}, m={G.number_of_edges()})", fontsize=10)
    fig.tight_layout()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(out_path, dpi=150 if max(fig_w, fig_h) <= 20 else 100)
    plt.close(fig)


# ----------------------------------------------------------------------
# Main
# ----------------------------------------------------------------------
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--input", default="new-input", help="root input folder")
    ap.add_argument("--output", default="output-images", help="root output folder")
    ap.add_argument("--ext", default=".txt")
    ap.add_argument("--format", default="png", choices=["png", "svg", "pdf"])
    ap.add_argument("--iters", type=int, default=40,
                    help="spacing-optimisation passes (0 = plain FPP)")
    ap.add_argument("--time-limit", type=float, default=20.0,
                    help="max seconds of spacing optimisation per component")
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
            pos = planar_positions(G, args.iters, args.time_limit)
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