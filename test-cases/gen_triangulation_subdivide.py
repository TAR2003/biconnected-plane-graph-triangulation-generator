"""
Categories 1, 2, 3:
  Generate a random 2D point set -> Delaunay triangulation -> planar graph.
  Then subdivide each original edge k times (k = 1, 2, or 3), i.e. replace
  edge (u,v) with a path u - x1 - x2 - ... - xk - v of k new internal
  vertices. Subdividing every edge of a planar graph keeps it planar and
  keeps the graph biconnected (subdividing preserves biconnectivity).
"""

import random
import networkx as nx
import numpy as np
from scipy.spatial import Delaunay

from common import collect_distinct, validate_graph, relabel_consecutive, vertex_limit


def random_delaunay_graph(num_points, seed=None):
    """Build a planar graph from the Delaunay triangulation of num_points
    random 2D points. Returns a networkx Graph with integer node labels
    0..num_points-1 and an edge set free of duplicates."""
    rng = np.random.default_rng(seed)
    pts = rng.random((num_points, 2))
    tri = Delaunay(pts)

    G = nx.Graph()
    G.add_nodes_from(range(num_points))
    for simplex in tri.simplices:
        a, b, c = simplex
        G.add_edge(int(a), int(b))
        G.add_edge(int(b), int(c))
        G.add_edge(int(c), int(a))
    return G


def subdivide_all_edges(G, k):
    """Return a new graph where every edge of G is subdivided into a path
    with k intermediate vertices (so each original edge becomes a path of
    k+1 edges)."""
    H = nx.Graph()
    H.add_nodes_from(G.nodes())
    next_id = max(G.nodes()) + 1 if G.number_of_nodes() > 0 else 0

    for u, v in G.edges():
        prev = u
        for _ in range(k):
            new_node = next_id
            next_id += 1
            H.add_edge(prev, new_node)
            prev = new_node
        H.add_edge(prev, v)

    return H


def generate_category(k, count, constraints, seed=0):
    """Subdivided Delaunay graphs on a random point-set order.

    The base point count is drawn uniformly up to the largest m whose
    subdivision still fits in the vertex ceiling (a triangulation has at
    most 3m - 6 edges). Face counts are whatever that drawing produces;
    they are not pinned to a fixed list.
    """
    rng = random.Random(seed)
    hi = vertex_limit(constraints)
    # n <= m + k * (3m - 6) for a maximal planar base on m >= 3 points.
    m_hi = max(3, (hi + 6 * k) // (1 + 3 * k))

    def build():
        base_n = rng.randint(3, m_hi)
        try:
            base_G = random_delaunay_graph(base_n, seed=rng.randint(0, 2**31 - 1))
        except Exception:
            return None
        H, _ = relabel_consecutive(subdivide_all_edges(base_G, k))
        ok, faces, reason = validate_graph(H, constraints)
        if not ok:
            return None
        return {
            "graph": H,
            "faces": faces,
            "meta": {
                "base_points": base_n,
                "subdivisions": k,
                "n": H.number_of_nodes(),
            },
        }

    instances = collect_distinct(count, build)
    if len(instances) < count:
        print(
            f"  [warning] subdivision k={k}: only {len(instances)} distinct "
            f"graphs out of the requested {count} under the current constraints."
        )
    return instances
