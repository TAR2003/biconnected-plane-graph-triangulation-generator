"""
Category 8: polygons with a random number of non-crossing chords.

Each graph is its own cycle. The order n is drawn from the full vertex
range, and the number of chords is drawn from 0 (bare cycle) through
n - 3 (fully triangulated). Nothing pins n to a single value, and the
number of files is the requested count.

Every graph in the sequence is outerplanar BY CONSTRUCTION: the original
n-cycle is never touched, and every chord added is a diagonal of the
current (sub-)polygon, so no two chords ever cross and every vertex
remains on the single outer boundary throughout. Each successive graph
is identical to the previous one PLUS exactly one more edge (a strict
superset), so face count strictly increases by exactly 1 each step
(each new chord splits exactly one existing face into two).

FACE COUNT: with n vertices and c chords added (0 <= c <= n-3), the
graph has (c + 2) faces total -- 1 outer face (the original n-cycle)
plus (c + 1) pieces of the interior (the interior starts as a single
polygonal region with the bare cycle, and each chord splits one
interior region into two). This is verified directly from the actual
extracted planar faces at every step (not just asserted).
"""

import networkx as nx

from common import collect_distinct, validate_graph, relabel_consecutive, vertex_limit


def _build_chord_sequence(n, rng, max_chords=None):
    """
    Build the full incremental-triangulation sequence for a fixed
    n-vertex cycle: a list of networkx Graphs
        [G_0 (bare cycle), G_1 (+1 chord), ..., G_{n-3} (fully triangulated)]
    where G_{k+1} = G_k plus exactly one new non-crossing chord.

    Uses a "recursively split a polygon with a random diagonal" process
    (same idea as category 7's _triangulate_polygon), but does ONE split
    at a time, snapshotting the graph after every single chord, via an
    explicit worklist of "regions" (sub-polygon boundaries) still
    needing to be split.
    """
    G = nx.Graph()
    G.add_nodes_from(range(n))
    for i in range(n):
        G.add_edge(i, (i + 1) % n)

    sequence = [G.copy()]

    # each region is a list of vertices (original ids) in cyclic order,
    # representing the boundary of a sub-polygon still awaiting
    # diagonals. A region of size 3 is already a triangle -- done.
    regions = [list(range(n))]

    while True:
        if max_chords is not None and len(sequence) - 1 >= max_chords:
            break
        splittable = [idx for idx, r in enumerate(regions) if len(r) > 3]
        if not splittable:
            break

        idx = rng.choice(splittable)
        region = regions.pop(idx)
        m = len(region)

        i = rng.randrange(m)
        forbidden = {(i - 1) % m, i, (i + 1) % m}
        candidates = [j for j in range(m) if j not in forbidden]
        j = rng.choice(candidates)

        a, b = region[i], region[j]
        G.add_edge(a, b)

        lo, hi = min(i, j), max(i, j)
        part1 = region[lo:hi + 1]
        part2 = region[hi:] + region[:lo + 1]
        regions.append(part1)
        regions.append(part2)

        sequence.append(G.copy())

    return sequence


def verify_sequence_structure(sequence, n):
    """
    Independent, definition-level sanity check on the sequence:

      1. Exactly n - 2 graphs (c = 0 .. n-3 chords, inclusive).
      2. Every graph has exactly n vertices.
      3. Every graph contains the full intact n-cycle (outer boundary
         never modified).
      4. Edge counts strictly increase by exactly 1 each step, starting
         at n (bare cycle) and ending at 2n - 3 (fully triangulated,
         same fingerprint as category 7).
      5. Each graph's edge set is a strict superset of the previous
         graph's edge set (nothing ever removed, only added -- true
         "add a chord" progression, never a different graph entirely).
      6. The FINAL graph has every inner face a triangle (i.e. it is a
         valid category-7-style maximal outerplanar graph): checked via
         actual face extraction, not just the edge-count fingerprint.

    Returns True/False.
    """
    if len(sequence) != n - 2:
        return False

    prev_edges = None
    for c, G in enumerate(sequence):
        if G.number_of_nodes() != n:
            return False
        for i in range(n):
            if not G.has_edge(i, (i + 1) % n):
                return False
        expected_edges = n + c
        if G.number_of_edges() != expected_edges:
            return False
        edge_set = set(frozenset(e) for e in G.edges())
        if prev_edges is not None and not prev_edges.issubset(edge_set):
            return False
        prev_edges = edge_set

    final = sequence[-1]
    if final.number_of_edges() != 2 * n - 3:
        return False

    is_planar, embedding = nx.check_planarity(final, counterexample=False)
    if not is_planar:
        return False
    faces = []
    visited = set()
    for u in embedding:
        for v in embedding[u]:
            he = (u, v)
            if he in visited:
                continue
            faces.append(embedding.traverse_face(u, v, mark_half_edges=visited))
    sizes = sorted(len(f) for f in faces)
    if sizes[:-1] != [3] * (len(sizes) - 1) or sizes[-1] != n:
        return False

    return True


def generate_category(count, constraints, seed=0):
    """`count` outerplanar graphs, each with its own random order and chord count."""
    import random
    rng = random.Random(seed)
    lo = max(4, constraints.min_vertices or 4)
    hi = vertex_limit(constraints)
    if constraints.max_vertices_in_face is not None:
        hi = min(hi, constraints.max_vertices_in_face)
    if hi < lo:
        raise RuntimeError(
            f"chord category: no cycle order fits in [{lo}, {hi}]. "
            f"The outer face of an n-cycle with non-crossing chords has size n."
        )

    def build():
        n = rng.randint(lo, hi)
        max_chords = n - 3
        if constraints.max_faces is not None:
            # c chords produce c + 2 faces.
            max_chords = min(max_chords, constraints.max_faces - 2)
        if max_chords < 0:
            return None
        c = rng.randint(0, max_chords)
        sequence = _build_chord_sequence(n, rng, max_chords=c)
        if not sequence or len(sequence) != c + 1:
            return None
        G, _ = relabel_consecutive(sequence[-1])
        ok, faces, reason = validate_graph(G, constraints)
        if not ok or len(faces) != c + 2:
            return None
        return {
            "graph": G,
            "faces": faces,
            "meta": {"n_vertices": n, "num_chords": c},
        }

    instances = collect_distinct(count, build)
    if len(instances) < count:
        print(
            f"  [warning] chord category: only {len(instances)} distinct "
            f"graphs out of the requested {count}."
        )
    return instances