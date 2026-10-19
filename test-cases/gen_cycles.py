"""
Category 5: simple cycles C_n. Order n is drawn from the whole allowed
range (any n >= 3 that fits the vertex ceiling). A cycle is one isomorphism
class per n, so each order is emitted at most once.

Category 6: a chain of k >= 2 cycles sharing an edge between consecutive
cycles. k and the cycle orders are random; the only ceiling is the vertex
limit (and any face limit the caller actually set). The chain stays simple,
planar, and biconnected.
"""

import random
import networkx as nx

from common import collect_distinct, validate_graph, relabel_consecutive, vertex_limit


# ---------------------------------------------------------------------
# Category 5: plain cycles
# ---------------------------------------------------------------------

def make_cycle(n):
    G = nx.cycle_graph(n)
    return G


def generate_category_cycles(count, constraints, seed=0):
    """One cycle per distinct order, orders drawn from the full vertex range."""
    rng = random.Random(seed)
    lo = max(3, constraints.min_vertices or 3)
    hi = vertex_limit(constraints)
    if constraints.max_vertices_in_face is not None:
        hi = min(hi, constraints.max_vertices_in_face)
    if hi < lo:
        raise RuntimeError(
            f"cycles category: no order n fits in [{lo}, {hi}] under the "
            f"current vertex / face-size limits."
        )

    orders = list(range(lo, hi + 1))
    rng.shuffle(orders)
    instances = []
    for n in orders:
        if len(instances) >= count:
            break
        G, _ = relabel_consecutive(make_cycle(n))
        ok, faces, reason = validate_graph(G, constraints)
        if not ok:
            continue
        instances.append({"graph": G, "faces": faces, "meta": {"n": n}})

    if len(instances) < count:
        print(
            f"  [warning] cycles category: only {len(instances)} distinct "
            f"cycle orders fit the current constraints (requested {count}). "
            f"A cycle is one isomorphism class per order."
        )
    return instances


# ---------------------------------------------------------------------
# Category 6: union of k cycles (chained, sharing an edge between
# consecutive cycles) -> stays simple, planar and biconnected.
# ---------------------------------------------------------------------

def make_cycle_union(cycle_sizes, rng):
    """
    Build a biconnected planar union of len(cycle_sizes) cycles by chaining
    them: cycle i and cycle i+1 share exactly one edge. Sharing a full edge
    (2 vertices) between consecutive biconnected planar pieces preserves
    both planarity and biconnectivity (this is essentially a "book" / chain
    of cycles glued along edges, a standard biconnected planar construction).
    """
    G = nx.Graph()
    next_id = 0

    # First cycle
    first_n = cycle_sizes[0]
    first_nodes = list(range(next_id, next_id + first_n))
    next_id += first_n
    for i in range(first_n):
        G.add_edge(first_nodes[i], first_nodes[(i + 1) % first_n])

    shared_edge = (first_nodes[0], first_nodes[1])

    for n in cycle_sizes[1:]:
        # new cycle re-uses `shared_edge` as one of its edges, and adds
        # (n - 2) brand-new vertices to complete the cycle.
        u, v = shared_edge
        new_count = n - 2
        new_nodes = list(range(next_id, next_id + new_count))
        next_id += new_count

        path_nodes = [u] + new_nodes + [v]
        for i in range(len(path_nodes) - 1):
            G.add_edge(path_nodes[i], path_nodes[i + 1])
        # edge (u, v) already exists (shared_edge), completing the cycle.

        # pick a fresh shared edge from this new cycle for the next link,
        # preferring an edge not equal to the incoming shared edge so
        # chains don't degenerate.
        if new_nodes:
            shared_edge = (u, new_nodes[0])
        else:
            shared_edge = (u, v)

    return G


def _random_cycle_sizes(total, rng, constraints):
    """Partition `total` vertices into k >= 2 cycles sharing consecutive edges.

    Minimum is a chain of triangles: k cycles use k + 2 vertices, so k can
    be any integer from 2 up to total - 2 (further capped by max_faces,
    since a chain of k cycles has k + 1 faces).
    """
    max_k = total - 2
    if constraints.max_faces is not None:
        max_k = min(max_k, constraints.max_faces - 1)
    if max_k < 2:
        return None
    k = rng.randint(2, max_k)
    minimum = k + 2
    if total < minimum:
        return None
    extras = [0] * k
    for _ in range(total - minimum):
        extras[rng.randrange(k)] += 1
    return [3 + e for e in extras]


def generate_category_union(count, constraints, seed=0):
    """Random-length chains of cycles of random orders."""
    rng = random.Random(seed)
    lo = max(4, constraints.min_vertices or 4)
    hi = vertex_limit(constraints)
    if hi < lo:
        raise RuntimeError(
            f"cycle-union category needs at least 4 vertices; ceiling is {hi}."
        )

    def build():
        sizes = _random_cycle_sizes(rng.randint(lo, hi), rng, constraints)
        if not sizes:
            return None
        G, _ = relabel_consecutive(make_cycle_union(sizes, rng))
        ok, faces, reason = validate_graph(G, constraints)
        if not ok:
            return None
        return {
            "graph": G,
            "faces": faces,
            "meta": {"k": len(sizes), "cycle_sizes": sizes},
        }

    instances = collect_distinct(count, build)
    if len(instances) < count:
        print(
            f"  [warning] cycle-union category: only {len(instances)} "
            f"distinct graphs out of the requested {count} under the "
            f"current constraints."
        )
    return instances
