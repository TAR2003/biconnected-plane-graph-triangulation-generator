"""
Category 7: snowflake graphs of any order.

Start from a triangle. Each round places a new vertex on a random
non-empty subset of the current outer edges (an ear on each chosen edge).
Inner faces stay triangles and the outer walk stays a single cycle. The
order is whatever was asked for: it is not locked to 3 * 2^s.
"""

import random

import networkx as nx

from common import collect_distinct, validate_graph, vertex_limit


def _layered_snowflake(n, rng):
    if n < 3:
        return None
    G = nx.Graph()
    G.add_nodes_from(range(3))
    G.add_edges_from([(0, 1), (1, 2), (2, 0)])
    outer = [0, 1, 2]
    nxt = 3
    while nxt < n:
        room = n - nxt
        k = len(outer)
        take = rng.randint(1, min(k, room))
        chosen = set(rng.sample(range(k), take))
        new_outer = []
        for i in range(k):
            u = outer[i]
            new_outer.append(u)
            if i not in chosen:
                continue
            w = nxt
            nxt += 1
            v = outer[(i + 1) % k]
            G.add_node(w)
            G.add_edge(u, w)
            G.add_edge(v, w)
            new_outer.append(w)
        outer = new_outer
    return G


def generate_category(count, constraints, seed=0):
    rng = random.Random(seed if seed is not None else 0)
    lo = max(3, constraints.min_vertices or 3)
    hi = vertex_limit(constraints)
    if constraints.max_vertices_in_face is not None:
        hi = min(hi, constraints.max_vertices_in_face)
    if constraints.max_faces is not None:
        # F = n - 1 for this construction (n - 2 triangles plus the outer cycle).
        hi = min(hi, constraints.max_faces + 1)
    if hi < lo:
        raise RuntimeError(
            f"snowflake category: no order n fits in [{lo}, {hi}]."
        )

    def build():
        n = rng.randint(lo, hi)
        G = _layered_snowflake(n, rng)
        ok, faces, reason = validate_graph(G, constraints)
        if not ok:
            return None
        return {"graph": G, "faces": faces, "meta": {"n": n}}

    instances = collect_distinct(count, build)
    if len(instances) < count:
        print(
            f"  [warning] snowflake: only {len(instances)} distinct graphs "
            f"fit the constraints (requested {count})."
        )
    return instances
