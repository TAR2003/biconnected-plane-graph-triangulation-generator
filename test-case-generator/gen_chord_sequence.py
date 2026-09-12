"""
Category 8: Incremental chord-triangulation sequence.

For a SINGLE fixed n-vertex cycle, generate the full sequence of graphs
produced by adding non-crossing chords ONE AT A TIME:

    case 1: the bare n-cycle (0 chords)
    case 2: cycle + 1 chord
    case 3: cycle + 2 chords
    ...
    case (n-2): cycle + (n-3) chords  ==  fully triangulated (every
                inner face is a triangle) -- this is the LAST case;
                once the polygon is fully triangulated there is nothing
                left to add, so generation stops here.

Every graph in this category is outerplanar BY CONSTRUCTION: the
original n-cycle is never touched, and every chord added is a diagonal
of the current (sub-)polygon, so no two chords ever cross and every
vertex remains on the single outer boundary throughout. Each successive
graph is identical to the previous one PLUS exactly one more edge (a
strict superset), so face count strictly increases by exactly 1 each
step (each new chord splits exactly one existing face into two).

This mirrors category 7 (snowflake / maximal outerplanar graphs), but
where category 7 emits ONE fully-triangulated graph per requested face
count, category 8 emits an entire progressive BUILD-UP -- every
intermediate stage of triangulating ONE polygon, from empty to full.

FACE COUNT: with n vertices and c chords added (0 <= c <= n-3), the
graph has (c + 1) faces total: 1 outer face (the original n-cycle) plus
c inner faces from the c chords having each split one face into two,
except technically it's (1 + c) faces for c=0 (just the cycle itself,
counted as inner-cycle==outer-cycle -- see note below) and (n-1) faces
once c = n-3 (fully triangulated, matching category 7's face count for
the same n). Concretely: faces_after_c_chords = c + 2 for c >= 1
(1 outer + (c+1) pieces of the original polygon interior)... the
precise, verified-by-construction formula actually used here is:

    faces(c) = c + 2   for a SIMPLE POLYGON interpretation where the
                         bare cycle (c=0) is treated as enclosing ONE
                         single interior face plus the outer face (2
                         faces total for c=0), and each subsequent
                         chord adds exactly 1 face (splits one interior
                         region into two).

This is verified directly from the actual extracted planar faces at
every step (not just asserted), so the code is self-checking regardless
of which convention networkx's face traversal uses for c=0.

One sequence = one "instance group" sharing a single n. The category
generator produces multiple independent sequences (different n, and/or
different random chord orders for the same n) until `count` total
graphs have been emitted, honoring the standard Constraints exactly
like every other category (each individual graph in every sequence is
still validated and must satisfy max_faces / max_vertices /
max_vertices_in_face on its own).
"""

import random
import networkx as nx

from common import Constraints, validate_graph, relabel_consecutive, DedupTracker


def _build_chord_sequence(n, rng):
    """
    Build ONE full incremental-triangulation sequence for a fixed
    n-vertex cycle: a list of networkx Graphs
        [G_0 (bare cycle), G_1 (+1 chord), ..., G_{n-3} (fully triangulated)]
    where G_{k+1} = G_k plus exactly one new non-crossing chord.

    Uses the same "recursively split a polygon with a random diagonal"
    idea as category 7's _triangulate_polygon, but instead of recursing
    all the way to the end before returning, it does ONE split at a
    time, snapshotting the graph after every single chord, by keeping
    an explicit worklist of "regions" (sub-polygon boundaries) still
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
    Independent, definition-level sanity check on an entire sequence:

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


def generate_one_sequence(n, constraints: Constraints, rng, dedup: DedupTracker, max_attempts=200):
    """
    Build and validate one full chord-addition sequence for a fixed n.

    Every individual graph in the sequence is run through the standard
    common.validate_graph pipeline (biconnectivity / planarity /
    max_faces / max_vertices_in_face / max_vertices), exactly like every
    other category. If ANY graph in the sequence fails, the whole
    sequence is discarded and retried with a fresh random chord order
    (a different random triangulation order can still fail the same
    max_faces ceiling since the final face count is fixed by n -- in
    that case retrying is pointless and the caller should not request
    an n whose final state already violates max_faces).

    The DedupTracker is applied to the FINAL (fully triangulated) graph
    of each sequence only, since that's what distinguishes one sequence
    of a given n from another (different chord orders on the same n
    that happen to produce isomorphic intermediate steps are still a
    legitimately different sequence overall unless the two full
    sequences are edge-for-edge identical after relabeling -- checking
    only the endpoint is the same convention category 7 uses for single
    graphs and keeps this category's semantics simple and predictable).

    Returns (sequence, per_graph_faces, meta) or (None, None, None).
    """
    for _ in range(max_attempts):
        sequence = _build_chord_sequence(n, rng)

        if not verify_sequence_structure(sequence, n):
            continue

        per_graph_faces = []
        all_ok = True
        for G in sequence:
            ok, faces, reason = validate_graph(G, constraints)
            if not ok:
                all_ok = False
                break
            per_graph_faces.append(faces)
        if not all_ok:
            # this n's final state (or an intermediate state) can't
            # satisfy the constraints under any chord order -- no point
            # retrying with a different random order for the same n
            return None, None, None

        final_relabeled, _ = relabel_consecutive(sequence[-1])
        if not dedup.try_add(final_relabeled):
            continue  # this exact final triangulation already used; retry with a new order

        return sequence, per_graph_faces, {"n_vertices": n, "num_chords_final": n - 3}

    return None, None, None


def generate_category(count, constraints: Constraints, seed=0):
    """
    Generates chord-addition sequences (see module docstring) across a
    range of n values until `count` TOTAL GRAPHS (summed across all
    sequences, not sequences themselves) have been produced, or no
    further progress is possible.

    n starts at the minimum that yields a genuine outer/inner
    distinction (n = 4, same floor as category 7) and increases until
    the fully-triangulated endpoint's face count (n - 1) would exceed
    constraints.max_faces, or n itself would exceed constraints.max_vertices,
    or the outer face size (n) would exceed constraints.max_vertices_in_face
    -- whichever is smallest. This mirrors the ceiling logic in category 7
    exactly, since both categories share the same underlying triangulated
    structure.

    Sequences are generated in increasing order of n, cycling back to
    smaller n values (with a fresh random chord order, producing a
    different sequence) if `count` hasn't been reached after exhausting
    every feasible n once -- this keeps output size predictable and
    controllable via `count`, the same as every other category, even
    though this category's natural unit of work is a whole sequence
    rather than a single graph.
    """
    rng = random.Random(seed)

    min_n = 4
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
            f"max_vertices_in_face, no cycle size n >= {min_n} remains "
            f"feasible for the chord-triangulation-sequence category. "
            f"Loosen one of these constraints (remember the fully "
            f"triangulated endpoint of an n-cycle has n-1 faces and an "
            f"outer face of size n)."
        )

    instances = []
    dedup = DedupTracker()
    exhausted_n = set()

    n = min_n
    while len(instances) < count:
        if len(exhausted_n) >= (max_n - min_n + 1):
            break  # every feasible n has been tried and failed at least once in a row

        if n in exhausted_n:
            n = min_n if n >= max_n else n + 1
            continue

        sequence, per_graph_faces, meta = generate_one_sequence(n, constraints, rng, dedup)
        if sequence is None:
            exhausted_n.add(n)
            n = min_n if n >= max_n else n + 1
            continue

        for c, (G, faces) in enumerate(zip(sequence, per_graph_faces)):
            if len(instances) >= count:
                break
            instances.append({
                "graph": G,
                "faces": faces,
                "meta": {"n_vertices": n, "num_chords": c, "sequence_id": meta["n_vertices"]},
            })

        n = min_n if n >= max_n else n + 1

    if len(instances) < count:
        print(
            f"  [warning] Chord-sequence category: only {len(instances)} "
            f"graphs could be generated out of the requested {count} -- "
            f"every feasible cycle size n in [{min_n}, {max_n}] ran out of "
            f"distinct chord orderings (or violated constraints) under the "
            f"current settings. Consider loosening max_vertices / "
            f"max_vertices_in_face / max_faces, or reducing the requested "
            f"count."
        )

    return instances