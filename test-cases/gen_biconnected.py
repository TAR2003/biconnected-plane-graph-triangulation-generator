#!/usr/bin/env python3
"""Biconnected plane-graph generator.  Every graph is validated: simple, planar
(Euler V-E+F=2 on the emitted rotation system) and 2-connected.
Usage: python gen_biconnected.py --out input/Biconnected --sizes 10 100 1000 --count 20 --seed 1
       python gen_biconnected.py --list
"""
import math
import networkx as nx
import numpy as np
from planar_common import (rot_from_graph, rot_from_points, filter_rot, dual_rot, run)

try:
    from scipy.spatial import Delaunay
except ImportError:  # Delaunay families are skipped without scipy
    Delaunay = None


def _divisors(n, lo=2):
    return [d for d in range(lo, n // 2 + 1) if n % d == 0]


# ------------------------------------------------------------ structured families
def g_cycle(n, rng):
    return None if n < 3 else {i: [(i - 1) % n, (i + 1) % n] for i in range(n)}

def g_wheel(n, rng):
    return None if n < 4 else rot_from_graph(nx.wheel_graph(n))

def g_bipyramid(n, rng):                       # cycle + 2 apexes (two huge-degree vertices)
    if n < 5: return None
    k = n - 2
    G = nx.cycle_graph(k)
    for a in (k, k + 1):
        G.add_edges_from((a, i) for i in range(k))
    return rot_from_graph(G)

def g_prism(n, rng):
    return None if n < 6 or n % 2 else rot_from_graph(nx.circular_ladder_graph(n // 2))

def g_grid(n, rng):                            # random aspect ratio: ladders ... squares
    ds = _divisors(n)
    if not ds: return None
    r = rng.choice(ds)
    return rot_from_graph(nx.convert_node_labels_to_integers(nx.grid_2d_graph(r, n // r)))

def g_cylinder(n, rng):                        # nested cycles + spokes + random diagonals
    ds = _divisors(n, 3)
    ds = [m for m in ds if n // m >= 2]
    if not ds: return None
    m = rng.choice(ds); k = n // m; p = rng.random()
    G = nx.Graph()
    for l in range(k):
        for i in range(m):
            G.add_edge(l * m + i, l * m + (i + 1) % m)
            if l + 1 < k:
                G.add_edge(l * m + i, (l + 1) * m + i)
                if rng.random() < p:
                    G.add_edge(l * m + i, (l + 1) * m + (i + 1) % m)
    return rot_from_graph(G)

def g_outerplanar(n, rng):                     # polygon + random non-crossing chords
    if n < 3: return None
    G = nx.cycle_graph(n); chords, st = [], [(0, n - 1)]
    while st:
        lo, hi = st.pop()
        if hi - lo < 2: continue
        k = rng.randint(lo + 1, hi - 1)
        chords += [(lo, k), (k, hi)]; st += [(lo, k), (k, hi)]
    keep = rng.random()
    G.add_edges_from(c for c in chords if rng.random() < keep)
    return rot_from_graph(G)

def g_series_parallel(n, rng):                 # subdivisions + parallel ears from a triangle
    if n < 3: return None
    edges = [(0, 1), (1, 2), (0, 2)]; p = rng.random()
    for w in range(3, n):
        j = rng.randrange(len(edges)); u, v = edges[j]
        if rng.random() < p:                   # subdivide
            edges[j] = (u, w); edges.append((w, v))
        else:                                  # parallel path of length 2
            edges += [(u, w), (w, v)]
    return rot_from_graph(nx.Graph(edges))


# ------------------------------------------------------------ triangulations
def _stacked(n, rng):
    G = nx.Graph([(0, 1), (1, 2), (0, 2)]); fs = [(0, 1, 2), (0, 1, 2)]
    bias = rng.random() ** 2                   # high bias -> deep, path-like Apollonian nets
    for w in range(3, n):
        i = len(fs) - 1 - rng.randrange(min(3, len(fs))) if rng.random() < bias else rng.randrange(len(fs))
        a, b, c = fs[i]
        G.add_edges_from([(w, a), (w, b), (w, c)])
        fs[i] = (a, b, w); fs += [(b, c, w), (a, c, w)]
    return G

def g_stacked(n, rng):
    return None if n < 3 else rot_from_graph(_stacked(n, rng))

def g_stacked_dual(n, rng):                    # cubic 3-connected graphs
    if n < 4 or n % 2: return None
    return dual_rot(rot_from_graph(_stacked((n + 4) // 2, rng)))


# ------------------------------------------------------------ geometric (Delaunay)
def _points(n, rng, mode):
    g = np.random.default_rng(rng.getrandbits(64))
    if mode == "uniform":
        return g.random((n, 2))
    if mode == "clustered":
        k = max(1, rng.randint(1, max(1, n // 15)))
        c = g.random((k, 2)); return c[g.integers(0, k, n)] + g.normal(0, rng.choice([1e-3, 1e-2, 5e-2]), (n, 2))
    s = math.ceil(math.sqrt(n))                # jittered lattice: many near-degenerate 4-faces
    idx = np.arange(n); pts = np.stack([idx % s, idx // s], 1).astype(float)
    return pts + g.normal(0, rng.choice([1e-4, 0.05, 0.2, 0.4]), (n, 2))

def _delaunay(n, rng, mode):
    if Delaunay is None or n < 3: return None
    pts = _points(n, rng, mode)
    try:
        tri = Delaunay(pts)
    except Exception:
        raise ValueError("degenerate point set")
    G = nx.Graph()
    for a, b, c in tri.simplices.tolist():
        G.add_edges_from([(a, b), (b, c), (a, c)])
    if G.number_of_nodes() != n: raise ValueError("unused points")
    return G, pts

def _dt_family(mode):
    def f(n, rng):
        r = _delaunay(n, rng, mode)
        return None if r is None else rot_from_points(*r)
    return f

def _sparsify(G, rng):
    """Delete random edges while staying biconnected (merges triangles into big faces)."""
    m = G.number_of_edges(); target = G.number_of_nodes() + int(rng.random() * (m - G.number_of_nodes()))
    es = list(G.edges); rng.shuffle(es)
    for u, v in es:
        if m <= target: break
        if G.degree[u] <= 2 or G.degree[v] <= 2: continue
        G.remove_edge(u, v)
        if nx.is_biconnected(G): m -= 1
        else: G.add_edge(u, v)
    return G

def g_sparse_delaunay(n, rng):
    r = _delaunay(n, rng, rng.choice(["uniform", "clustered", "lattice"]))
    if r is None: return None
    G, pts = r; rot0 = rot_from_points(G, pts)
    return filter_rot(rot0, _sparsify(G.copy(), rng))

def g_sparse_stacked(n, rng):
    if n < 3: return None
    G = _stacked(n, rng); rot0 = rot_from_graph(G)
    return filter_rot(rot0, _sparsify(G, rng))


BICONNECTED = {
    "cycle": g_cycle, "wheel": g_wheel, "bipyramid": g_bipyramid, "prism": g_prism,
    "grid": g_grid, "cylinder": g_cylinder, "outerplanar": g_outerplanar,
    "series_parallel": g_series_parallel, "stacked_triangulation": g_stacked,
    "stacked_dual_cubic": g_stacked_dual,
    "delaunay_uniform": _dt_family("uniform"), "delaunay_clustered": _dt_family("clustered"),
    "delaunay_lattice": _dt_family("lattice"),
    "sparse_delaunay": g_sparse_delaunay, "sparse_stacked": g_sparse_stacked,
}

# Edit these values to configure runs in code; matching command-line options override them.
GENERATION_CONFIG = {
    "out": "inputs/biconnected",
    "sizes": [3, 4, 5, 6, 7, 8, 10, 12, 16, 20, 30, 50, 100, 200, 500, 1000],
    "count": 10,
    "seed": 12345,
    "families": None,
}

if __name__ == "__main__":
    run("biconnected", BICONNECTED, defaults=GENERATION_CONFIG)
