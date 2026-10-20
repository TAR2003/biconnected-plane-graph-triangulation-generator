"""
Category 8: Incremental chord-triangulation sequence.

ONE fixed n-vertex cycle. Generate the full sequence of graphs produced
by adding non-crossing chords ONE AT A TIME to that SAME cycle:

    case 1: the bare n-cycle (0 chords)
    case 2: cycle + 1 chord
    case 3: cycle + 2 chords
    ...
    case (n-2): cycle + (n-3) chords  ==  fully triangulated (every
                inner face is a triangle) -- this is the LAST case;
                once the polygon is fully triangulated there is nothing
                left to add, so generation stops here.

This category does NOT target a requested "count" of graphs. n is fixed
(a single settings value), and the sequence naturally has exactly n - 2
graphs -- that number can be smaller or larger than any nominal `count`
elsewhere in the config, and that's expected: the number of graphs
produced is however many steps it genuinely takes to fully triangulate
that one cycle, no more, no less.

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

from common import Constraints, validate_graph, relabel_consecutive


def _build_chord_sequence(n, rng):
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


def generate_category(count, constraints: Constraints, seed=0):
    """
    Generate ONE chord-addition sequence for a SINGLE fixed cycle size n,
    and return every graph in that sequence (n - 2 graphs total).

    `count` is accepted only for interface consistency with every other
    category's generate_category(count, constraints, seed) signature; it
    is NOT used to pad or truncate output here. This category's natural
    output size is fixed by n alone (n - 2 graphs) -- it may end up
    smaller or larger than `count`, and that mismatch is expected and
    fine, not an error.

    n itself is taken from constraints in the same way category 7 infers
    its ceiling, but here it picks the SINGLE LARGEST n that still keeps
    every graph in the sequence -- including the final, fully
    triangulated one -- within max_faces / max_vertices /
    max_vertices_in_face, since a bigger n means a longer, more
    informative sequence and there is no "count" pressure pushing toward
    smaller n. If constraints leave the choice ambiguous, min_vertices
    (when > 4) is used as a floor.

    Every individual graph in the sequence is still validated against
    the standard common.validate_graph pipeline before being returned,
    as a safety net (construction already guarantees they pass).
    """
    min_n = max(4, constraints.min_vertices if constraints.min_vertices else 4)
    max_n = min_n + 20  # generous default if nothing else constrains it

    if constraints.max_vertices is not None:
        max_n = min(max_n, constraints.max_vertices)

    if constraints.max_vertices_in_face is not None:
        if constraints.max_vertices_in_face < 3:
            raise RuntimeError(
                f"max_vertices_in_face={constraints.max_vertices_in_face} is "
                f"below 3, so not even a single triangular face is allowed "
                f"-- no chord-triangulation sequence is possible."
            )
        # outer face is always size n
        max_n = min(max_n, constraints.max_vertices_in_face)

    if constraints.max_faces is not None:
        # final (fully triangulated) state has n - 1 faces; that's the
        # binding constraint since every earlier step in the sequence
        # has FEWER faces than the final one
        max_n = min(max_n, constraints.max_faces + 1)

    if max_n < min_n:
        raise RuntimeError(
            f"After combining max_faces / max_vertices / "
            f"max_vertices_in_face / min_vertices, no cycle size n is "
            f"feasible for the chord-triangulation-sequence category. "
            f"Loosen one of these constraints (remember the fully "
            f"triangulated endpoint of an n-cycle has n-1 faces and an "
            f"outer face of size n)."
        )

    n = max_n  # pick the largest feasible n for the longest, most useful sequence

    rng = __import__("random").Random(seed)

    sequence = None
    for _ in range(200):
        candidate = _build_chord_sequence(n, rng)
        if verify_sequence_structure(candidate, n):
            sequence = candidate
            break

    if sequence is None:
        raise RuntimeError(
            f"Failed to construct a valid chord-triangulation sequence for "
            f"n={n} after repeated attempts -- this should not happen; "
            f"please report it as a bug."
        )

    instances = []
    for c, G in enumerate(sequence):
        ok, faces, reason = validate_graph(G, constraints)
        if not ok:
            raise RuntimeError(
                f"Sequence step {c} (n={n}, {c} chords) unexpectedly failed "
                f"validation ({reason}) despite passing construction-time "
                f"checks -- please report this as a bug."
            )
        G_relabeled, _ = relabel_consecutive(G)
        instances.append({
            "graph": G_relabeled,
            "faces": faces,
            "meta": {"n_vertices": n, "num_chords": c, "step": f"{c + 1}/{len(sequence)}"},
        })

    print(
        f"  [info] Chord-sequence category: generated {len(instances)} graphs "
        f"(one fixed {n}-cycle, chords 0..{n - 3}) -- this is the natural "
        f"length of the sequence and is independent of the requested "
        f"count={count}."
    )

    return instances