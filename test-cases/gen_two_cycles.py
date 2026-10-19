"""
Category 9: two cycles joined by two bridges.

The cycles are vertex-disjoint. Each bridge is a single edge, and the four
endpoints are distinct, so neither edge is a cut-edge and no endpoint is a
cut vertex: the union is 2-vertex-connected (and planar). One bridge would
leave a cut-edge; the second bridge is what makes the graph biconnected.

Cycle orders are drawn uniformly up to the vertex ceiling. There is no
fixed menu of sizes.
"""

import random

import networkx as nx

from common import collect_distinct, validate_graph, vertex_limit


def make_two_cycles_two_bridges(n1, n2, rng):
    """Cycles on 0..n1-1 and n1..n1+n2-1, plus two vertex-disjoint edges."""
    if n1 < 3 or n2 < 3:
        return None
    G = nx.cycle_graph(n1)
    offset = n1
    for i in range(n2):
        G.add_edge(offset + i, offset + (i + 1) % n2)
    a1, a2 = rng.sample(range(n1), 2)
    b1, b2 = rng.sample(range(offset, offset + n2), 2)
    G.add_edge(a1, b1)
    G.add_edge(a2, b2)
    return G


def _split_orders(total, rng):
    n1 = rng.randint(3, total - 3)
    return n1, total - n1


def generate_category(count, constraints, seed=0):
    rng = random.Random(seed)
    lo = max(6, constraints.min_vertices or 6)
    hi = vertex_limit(constraints)
    if hi < lo:
        raise RuntimeError(
            f"two-cycles category needs at least 6 vertices (two triangles "
            f"joined by two edges); vertex ceiling is {hi}."
        )

    def build():
        total = rng.randint(lo, hi)
        n1, n2 = _split_orders(total, rng)
        # Swap so a (7, 40) pair and a (40, 7) pair are not both forced
        # through the same first-cycle orientation before the random bridges.
        if rng.random() < 0.5:
            n1, n2 = n2, n1
        G = make_two_cycles_two_bridges(n1, n2, rng)
        ok, faces, reason = validate_graph(G, constraints)
        if not ok:
            return None
        bridges = [(u, v) for u, v in G.edges if (u < n1) != (v < n1)]
        if len(bridges) != 2:
            return None
        return {
            "graph": G,
            "faces": faces,
            "meta": {"cycle_orders": [n1, n2], "bridges": 2},
        }

    instances = collect_distinct(count, build)
    if len(instances) < count:
        print(
            f"  [warning] two-cycles category: only {len(instances)} distinct "
            f"graphs out of the requested {count} under the current "
            f"constraints."
        )
    return instances
