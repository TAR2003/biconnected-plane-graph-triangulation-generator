#!/usr/bin/env python3
"""
Draw plane graphs from adjacency-list files, RESPECTING the rotation system.

Input format (per file):
    N
    k a1 a2 ... ak      (one line per vertex; neighbours in CCW cyclic order)

The neighbour order in each row IS the planar embedding (rotation system).
Pipeline per connected component:
  1. Build the combinatorial embedding from the file and verify it is a valid
     planar (genus-0) embedding.  (If it is not, a WARNING is printed and an
     arbitrary planar embedding is used instead.)
  2. Trace all faces (half-edge tracing) and choose the OUTER face by rule.
  3. Force that face to be unbounded: put a dummy vertex z inside it, joined
     to its corners, triangulate, make a triangle at z the outer triangle,
     run the FPP grid algorithm, then delete z and all dummy edges.
  4. Verify the drawing really has the file's rotation at every vertex
     (mirror it if it came out reflected).
  5. Spread vertices apart (max-min clearance).  Every move must keep: no
     crossings, identical rotation system, identical outer face.
  6. Final full verification; falls back to the plain FPP drawing on failure.

Outer face rule (--outer):
    largest        biggest face                          (default)
    with:V         biggest face that CONTAINS vertex V   (V on the outside)
    without:V      biggest face NOT containing vertex V  (V enclosed)

Usage:
    python draw_planar.py --input new-input --output output-images --outer with:0
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
from networkx.algorithms import planar_drawing as _pd


# ----------------------------------------------------------------------
# Parsing  ->  (G, rot)   rot[v] = neighbours of v in the order of the file
# ----------------------------------------------------------------------
def parse_graph(path: Path, clockwise: bool = False):
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

    rot, G, clean = {}, nx.Graph(), True
    G.add_nodes_from(range(n))
    for u, nbrs in enumerate(adj):
        row = []
        for v in nbrs:
            v -= offset
            if not (0 <= v < n):
                raise ValueError(f"vertex {u}: neighbour {v + offset} out of range")
            if v == u or v in row:
                clean = False          # self-loop / repeated neighbour
                continue
            row.append(v)
            G.add_edge(u, v)
        rot[u] = row[::-1] if clockwise else row
    for u in rot:                      # adjacency must be symmetric
        for v in rot[u]:
            if u not in rot[v]:
                clean = False
    return G, (rot if clean else None)


# ----------------------------------------------------------------------
# Combinatorial embedding, faces, outer face
# ----------------------------------------------------------------------
def embedding_from_rotation(nodes, rot):
    emb = nx.PlanarEmbedding()
    for v in nodes:
        emb.add_node(v)
        prev = None
        for w in rot[v]:
            if prev is None:
                emb.add_half_edge(v, w)
            else:
                emb.add_half_edge(v, w, ccw=prev)     # w is CCW-after prev at v
            prev = w
    emb.check_structure()                             # raises if not genus 0
    return emb


def rotation_from_embedding(emb):
    return {v: list(emb.neighbors_cw_order(v))[::-1] for v in emb.nodes}


def all_faces(emb):
    seen, faces = set(), []
    for u, v in emb.edges():
        if (u, v) not in seen:
            faces.append(emb.traverse_face(u, v, mark_half_edges=seen))
    return faces


def choose_outer(faces, rule: str):
    key = lambda f: (len(f), len(set(f)))
    if rule.startswith(("with:", "without:")):
        kind, val = rule.split(":", 1)
        v = int(val)
        pool = [f for f in faces if (v in f) == (kind == "with")]
        if pool:
            return max(pool, key=key)
        # requested vertex not in this component / no such face: fall back
    return max(faces, key=key)


# ----------------------------------------------------------------------
# FPP grid drawing with a prescribed outer face
# ----------------------------------------------------------------------
def fpp_with_outer_face(emb, F):
    Z = -1
    while Z in emb:
        Z -= 1
    A = nx.PlanarEmbedding(emb)
    k = len(F)
    seen, prev = set(), None
    for i in range(k):                    # corner i: F[i-1] -> F[i] -> F[i+1]
        w, v = F[i], F[i - 1]
        if w in seen:
            continue
        seen.add(w)
        A.add_half_edge(w, Z, cw=v)       # Z sits inside the face corner
        if prev is None:
            A.add_half_edge(Z, w)
        else:
            A.add_half_edge(Z, w, ccw=prev)
        prev = w
    A.check_structure()

    T, _ = _pd.triangulate_embedding(A, fully_triangulate=True)
    v2 = next(iter(T[Z]))
    v3 = T[v2][Z]["ccw"]
    assert T.has_edge(Z, v3)

    original = _pd.triangulate_embedding
    _pd.triangulate_embedding = lambda e, fully_triangulate=False: (T, [Z, v2, v3])
    try:
        pos = _pd.combinatorial_embedding_to_pos(A)
    finally:
        _pd.triangulate_embedding = original
    return {v: np.array(p, dtype=float) for v, p in pos.items() if v != Z}


# ----------------------------------------------------------------------
# Geometry + drawing object with embedding-preserving local tests
# ----------------------------------------------------------------------
def pt_seg_dist(p, A, B):
    AB = B - A
    L2 = np.maximum((AB * AB).sum(1), 1e-18)
    t = np.clip(((p - A) * AB).sum(1) / L2, 0.0, 1.0)
    D = p - (A + t[:, None] * AB)
    return np.sqrt((D * D).sum(1))


class Drawing:
    def __init__(self, P, E, rot=None, outer=None):
        self.P = np.array(P, dtype=float)
        self.E = np.array(E, dtype=int).reshape(-1, 2)
        n = len(self.P)
        if rot is None:
            nbr = [[] for _ in range(n)]
            for a, b in self.E:
                nbr[a].append(b)
                nbr[b].append(a)
            rot = nbr
        self.nbr = [np.array(x, dtype=int) for x in rot]
        ea, eb = self.E[:, 0], self.E[:, 1]
        self.non_inc = [np.nonzero((ea != i) & (eb != i))[0] for i in range(n)]
        self.outer = None if outer is None else np.array(outer, dtype=int)
        self.outer_set = set() if outer is None else set(outer)

    # ---- clearance ---------------------------------------------------
    def local_clear(self, i, p):
        P, E = self.P, self.E
        d = np.sqrt(((P - p) ** 2).sum(1))
        d[i] = np.inf
        c = d.min()
        ne = self.non_inc[i]
        if len(ne):
            c = min(c, pt_seg_dist(p, P[E[ne, 0]], P[E[ne, 1]]).min())
        nb = self.nbr[i]
        if len(nb):
            ab = P[nb] - p
            L2 = np.maximum((ab * ab).sum(1), 1e-18)
            t = np.clip(((P[None] - p) * ab[:, None]).sum(2) / L2[:, None], 0.0, 1.0)
            diff = P[None] - (p + t[:, :, None] * ab[:, None])
            dd = np.sqrt((diff * diff).sum(2))
            dd[:, i] = np.inf
            dd[np.arange(len(nb)), nb] = np.inf
            c = min(c, dd.min())
        return c

    def clearance(self):
        return min(self.local_clear(i, self.P[i]) for i in range(len(self.P)))

    # ---- crossings ---------------------------------------------------
    def crosses(self, i, p):
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
            d2 = DCk[:, 0] * (q[1] - Ck[:, 1]) - DCk[:, 1] * (q[0] - Ck[:, 0])
            pq = q - p
            d3 = pq[0] * (Ck[:, 1] - p[1]) - pq[1] * (Ck[:, 0] - p[0])
            d4 = pq[0] * (Dk[:, 1] - p[1]) - pq[1] * (Dk[:, 0] - p[0])
            if np.any((d1[keep] * d2 < 0) & (d3 * d4 < 0)):
                return True
        return False

    def has_crossing(self):
        P, E = self.P, self.E
        for k in range(len(E) - 1):
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

    # ---- embedding preservation --------------------------------------
    @staticmethod
    def _ccw_ok(c, coords):
        """neighbours (in given order) appear in CCW order around c ?"""
        v = coords - c
        a = np.arctan2(v[:, 1], v[:, 0])
        d = (np.roll(a, -1) - a) % (2 * np.pi)
        return abs(d.sum() - 2 * np.pi) < 1e-7

    def rotation_ok(self, i, p):
        """Rotation at i and at each neighbour stays as in the file."""
        P = self.P
        nb = self.nbr[i]
        if len(nb) >= 3 and not self._ccw_ok(p, P[nb]):
            return False
        for w in nb:
            nw = self.nbr[w]
            if len(nw) >= 3:
                coords = P[nw].copy()
                coords[nw == i] = p
                if not self._ccw_ok(P[w], coords):
                    return False
        return True

    def outer_area(self, i=None, p=None):
        pts = self.P[self.outer].copy()
        if i is not None:
            pts[self.outer == i] = p
        x, y = pts[:, 0], pts[:, 1]
        return 0.5 * float(np.sum(x * np.roll(y, -1) - np.roll(x, -1) * y))

    def outer_ok(self, i=None, p=None):
        return self.outer is None or self.outer_area(i, p) <= 1e-12

    def rotation_valid(self):
        return all(len(self.nbr[i]) < 3 or self._ccw_ok(self.P[i], self.P[self.nbr[i]])
                   for i in range(len(self.P)))

    def fully_valid(self):
        return self.rotation_valid() and self.outer_ok() and not self.has_crossing()


# ----------------------------------------------------------------------
# Spacing optimisation (embedding-preserving)
# ----------------------------------------------------------------------
def refine(dr: Drawing, iters: int, time_limit: float, seed: int = 0):
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
                q = cands[j]
                if dr.crosses(i, q) or not dr.rotation_ok(i, q):
                    continue
                if i in dr.outer_set and not dr.outer_ok(i, q):
                    continue
                dr.P[i] = q
                moved += 1
                break
        if moved == 0 and step <= 0.004:
            break


# ----------------------------------------------------------------------
# One connected component
# ----------------------------------------------------------------------
def layout_component(nodes, rot, outer_rule, iters, time_limit):
    """Returns (pos dict, info string)."""
    n = len(nodes)
    if n == 1:
        return {nodes[0]: np.array([0.0, 0.0])}, ""
    if n == 2:
        return {nodes[0]: np.array([0.0, 0.0]), nodes[1]: np.array([1.0, 0.0])}, ""

    note = ""
    try:
        if rot is None:
            raise nx.NetworkXException("adjacency lists are not a clean symmetric simple graph")
        emb = embedding_from_rotation(nodes, rot)
    except nx.NetworkXException as e:
        H = nx.Graph()
        H.add_nodes_from(nodes)
        for v in nodes:
            for w in (rot or {}).get(v, []):
                H.add_edge(v, w)
        if rot is None:
            raise
        ok, emb = nx.check_planarity(H)
        if not ok:
            raise ValueError("component is not planar")
        rot = rotation_from_embedding(emb)
        note = "embedding NOT from file"

    faces = all_faces(emb)
    F = choose_outer(faces, outer_rule)
    raw = fpp_with_outer_face(emb, F)

    idx = {v: i for i, v in enumerate(nodes)}
    P = np.array([raw[v] for v in nodes], dtype=float)
    P -= P.min(axis=0)
    span = P.max(axis=0)
    span[span == 0] = 1.0
    P /= span
    E = [(idx[v], idx[w]) for v in nodes for w in rot[v] if idx[v] < idx[w]]
    rot_i = [[idx[w] for w in rot[v]] for v in nodes]
    F_i = [idx[v] for v in F]

    dr = Drawing(P, E, rot_i, F_i)
    if not dr.fully_valid():                      # maybe it came out mirrored
        dr.P[:, 0] = 1.0 - dr.P[:, 0]
        if not dr.fully_valid():
            raise RuntimeError("FPP drawing does not realise the embedding")
    original = dr.P.copy()

    if iters > 0:
        refine(dr, iters, time_limit)
        if not dr.fully_valid():
            dr.P = original

    s = max(1.0, math.sqrt(n) / 3.0)
    return {v: dr.P[idx[v]] * s for v in nodes}, note


def planar_positions(G, rot, outer_rule, iters, time_limit):
    if not nx.check_planarity(G)[0]:
        return None, []
    pos, notes, x_cursor = {}, [], 0.0
    for comp in sorted(nx.connected_components(G), key=len, reverse=True):
        nodes = sorted(comp)
        sub_rot = None if rot is None else {v: rot[v] for v in nodes}
        p, note = layout_component(nodes, sub_rot, outer_rule, iters, time_limit)
        if note:
            notes.append(note)
        xs = [c[0] for c in p.values()]
        lo, hi = min(xs), max(xs)
        for v, c in p.items():
            pos[v] = np.array([c[0] - lo + x_cursor, c[1]])
        x_cursor += (hi - lo) + 0.6
    return pos, notes


def drawing_clearance(G, pos):
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
def draw_graph(G, pos, out_path: Path, title: str):
    n = G.number_of_nodes()
    nodes = list(G.nodes())
    pts = np.array([pos[v] for v in nodes])
    xs, ys = pts[:, 0], pts[:, 1]
    w = max(np.ptp(xs), 1e-6)
    h = max(np.ptp(ys), 1e-6)

    clr = max(drawing_clearance(G, pos), 1e-9)
    radius = 0.36 * clr
    pad = max(0.06 * max(w, h), 2 * radius)
    W, H = w + 2 * pad, h + 2 * pad

    ppu_wanted = 26.0 / (2 * radius)
    fig_w = float(np.clip(ppu_wanted * W / 72, 5, 50))
    fig_h = float(np.clip(ppu_wanted * H / 72, 5, 50))
    fig, ax = plt.subplots(figsize=(fig_w, fig_h))

    ppu = 0.88 * min(fig_w * 72 / W, fig_h * 72 / H)
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
    ap.add_argument("--input", default="new-input")
    ap.add_argument("--output", default="output-images")
    ap.add_argument("--ext", default=".txt")
    ap.add_argument("--format", default="png", choices=["png", "svg", "pdf"])
    ap.add_argument("--outer", default="largest",
                    help="outer face rule: largest | with:V | without:V")
    ap.add_argument("--clockwise", action="store_true",
                    help="neighbour lists are in CLOCKWISE order (default CCW)")
    ap.add_argument("--iters", type=int, default=40,
                    help="spacing passes (0 = plain FPP)")
    ap.add_argument("--time-limit", type=float, default=20.0,
                    help="max seconds of spacing work per component")
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
            G, rot = parse_graph(f, args.clockwise)
            pos, notes = planar_positions(G, rot, args.outer, args.iters, args.time_limit)
            if pos is None:
                print(f"[{i}/{len(files)}] NON-PLANAR, skipped: {rel}")
                nonplanar += 1
                continue
            title = str(rel) + ("   [WARNING: " + "; ".join(sorted(set(notes))) + "]" if notes else "")
            if notes:
                print(f"[{i}/{len(files)}] WARNING {rel}: {'; '.join(sorted(set(notes)))}")
            draw_graph(G, pos, out_file, title)
            ok += 1
            print(f"[{i}/{len(files)}] drawn: {rel}")
        except Exception as e:
            failed += 1
            print(f"[{i}/{len(files)}] ERROR in {rel}: {e}")

    print(f"\nDone. drawn={ok}, non-planar={nonplanar}, errors={failed}")


if __name__ == "__main__":
    main()