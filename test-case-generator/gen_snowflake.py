"""
Category 7 (new): Snowflake graphs.

A "snowflake graph" here means a MAXIMAL OUTERPLANAR GRAPH: every vertex
lies on the outer face, and every face other than the outer face is a
triangle. Equivalently: take a simple polygon (cycle) on n vertices and
triangulate its interior completely with non-crossing diagonals.

Why the name "snowflake": we build these NOT by picking a fixed
triangulation of a static n-gon, but by repeatedly growing a new triangle
outward from a randomly chosen edge of the CURRENT outer boundary (a new
vertex is attached to both endpoints of that boundary edge). Because the
edge to grow from is chosen randomly across the whole current boundary
(instead of always fanning out from one hub, or always extending in one
direction), the branches sprout unevenly all around the shape, giving the
jagged, radiating, "snowflake-like" silhouette when drawn -- while the
graph stays outerplanar and fully triangulated on the inside by
construction.

Construction (guaranteed correct by construction, independent of any
post-hoc check):
  1. Start with a single triangle (3 vertices, all on the boundary, the
     inner face already a triangle). The boundary is the cycle (0,1,2).
  2. Repeat (n - 3) times:
       - pick a random edge (u, v) currently on the boundary cycle
       - add a new vertex w, connect it to u and v
       - this closes off triangle (u, v, w) as a new inner face and
         replaces boundary edge (u, v) with boundary edges (u, w), (w, v)
  3. The result is a maximal outerplanar graph on n vertices: n - 2
     triangular inner faces + 1 outer face = n - 1 faces total, and the
     largest face is the outer face (the full boundary cycle, size n).

This "grow a random boundary edge" process is the same combinatorial
process as picking a random triangulation of a polygon (each step is
equivalent to adding one ear), so every graph produced is guaranteed to
be a valid maximal outerplanar graph -- no separate validation of the
triangulation structure is needed, only the usual shared constraint /
biconnectivity / planarity pipeline in common.py is run as a safety net.

FACE COUNT: for a snowflake graph on n vertices, faces = (n - 2) inner
triangles + 1 outer face = n - 1. So targeting a face count directly
translates to targeting an exact vertex count: n = target_faces + 1.
The minimum is n = 3 (a single triangle, target_faces = 2: one inner
triangle + one outer face, which are literally the same 3-cycle traced
in two directions -- since our generic face-count constraint counts the
outer face separately, and there is no separate "inner" face when n = 3,
we treat n = 3 as target_faces = 1). To keep behavior simple and
unambiguous we require n >= 4 (target_faces >= 3), which is the smallest
case with a genuine outer boundary distinct from any inner triangle.
"""

import random
import networkx as nx

from common import Constraints, validate_graph, relabel_consecutive, build_face_count_plan, DedupTracker


def _grow_snowflake(n, rng):
    """
    Build one random maximal outerplanar graph on exactly `n` vertices
    using the random-boundary-edge growth process described above.

    Returns (G, boundary_cycle) where boundary_cycle is the list of
    vertices in cyclic order around the outer face, or None on failure
    (should only fail for n < 3).
    """
    if n < 3:
        return None

    G = nx.Graph()
    G.add_nodes_from([0, 1, 2])
    G.add_edges_from([(0, 1), (1, 2), (2, 0)])
    next_id = 3

    # boundary kept as an explicit cyclic list of vertices; boundary edge
    # i is (boundary[i], boundary[i+1 mod len]).
    boundary = [0, 1, 2]

    while len(boundary) < n:
        # pick a random boundary edge index to grow from
        i = rng.randrange(len(boundary))
        u = boundary[i]
        v = boundary[(i + 1) % len(boundary)]

        w = next_id
        next_id += 1
        G.add_node(w)
        G.add_edge(u, w)
        G.add_edge(w, v)

        # splice w into the boundary between u and v
        boundary = boundary[: i + 1] + [w] + boundary[i + 1:]

    return G, boundary


def verify_snowflake_structure(G, boundary, n):
    """
    Independent, definition-level sanity check on the final graph:

      1. G has exactly n vertices and every vertex lies on `boundary`
         (outerplanar-by-construction check).
      2. `boundary`, traced as a cycle, is actually a Hamiltonian cycle
         of G (every consecutive pair is an edge, length == n, no
         repeats).
      3. G has exactly 2n - 3 edges, which is the exact edge count of
         every maximal outerplanar graph on n >= 2 vertices (n boundary
         edges + (n - 3) diagonals). This is the key structural
         fingerprint that the growth process produced a FULL
         triangulation, not just some outerplanar graph.

    Returns True/False.
    """
    if G.number_of_nodes() != n:
        return False
    if len(boundary) != n or len(set(boundary)) != n:
        return False
    if set(boundary) != set(G.nodes()):
        return False

    for i in range(n):
        u = boundary[i]
        v = boundary[(i + 1) % n]
        if not G.has_edge(u, v):
            return False

    expected_edges = 2 * n - 3 if n >= 2 else 0
    if G.number_of_edges() != expected_edges:
        return False

    return True


def generate_one_targeted(target_faces, constraints: Constraints, rng, dedup: DedupTracker, max_attempts=500):
    """
    Generate a single snowflake graph with EXACTLY `target_faces` faces.

    See module docstring: faces = n - 1, so n = target_faces + 1.
    Minimum supported target_faces is 3 (n = 4: one square boundary with
    one diagonal, i.e. 2 inner triangles + 1 outer face).

    Rejects (and retries) any candidate isomorphic to a graph already
    accepted for this category, via `dedup`.
    """
    n = target_faces + 1
    if n < 4:
        return None, None, None  # impossible: need a genuine outer boundary

    for _ in range(max_attempts):
        result = _grow_snowflake(n, rng)
        if result is None:
            continue
        G, boundary = result

        if not verify_snowflake_structure(G, boundary, n):
            continue

        G, _ = relabel_consecutive(G)

        ok, faces, reason = validate_graph(G, constraints)
        if ok and len(faces) == target_faces:
            if dedup.try_add(G):
                return G, faces, {"n_vertices": n, "target_faces": target_faces}
            # isomorphic duplicate -- discard and try a different random growth
    return None, None, None


def generate_category(count, constraints: Constraints, seed=0):
    """
    Generates up to `count` snowflake graphs with target face counts
    spread evenly across the achievable range [3, constraints.max_faces].
    3 is the minimum (see generate_one_targeted); if max_faces < 3, no
    snowflake graph can satisfy the constraint at all and we raise a
    clear error rather than hanging or silently skewing results.

    Also respects constraints.max_vertices: since n = target_faces + 1
    for this category, an effective ceiling on target_faces is imposed
    by max_vertices as well as by max_faces, whichever is stricter.

    DUPLICATE HANDLING: every accepted graph is checked against every
    previously accepted graph via exact isomorphism (common.DedupTracker).
    If a face-count target runs out of distinct graphs before its quota
    is filled, the shortfall is redistributed to other feasible targets.
    Only if every feasible target is simultaneously exhausted do we stop
    early with a clear warning, rather than crash or emit a duplicate.
    """
    rng = random.Random(seed)

    min_faces = 3
    if constraints.max_faces is not None and constraints.max_faces < min_faces:
        raise RuntimeError(
            f"max_faces={constraints.max_faces} is below the mathematical "
            f"minimum of {min_faces} faces for any snowflake graph (the "
            f"smallest genuine case is n=4 vertices: 2 inner triangles + 1 "
            f"outer face = 3 faces). Raise max_faces to at least {min_faces}."
        )

    max_faces = constraints.max_faces if constraints.max_faces is not None else min_faces + 20

    # n = target_faces + 1 must also respect max_vertices, if set
    if constraints.max_vertices is not None:
        max_faces = min(max_faces, constraints.max_vertices - 1)
        if max_faces < min_faces:
            raise RuntimeError(
                f"max_vertices={constraints.max_vertices} is too small to "
                f"reach even the minimum snowflake graph (needs n>=4 "
                f"vertices, i.e. max_vertices>=4)."
            )

    plan = build_face_count_plan(count, min_faces, max_faces, seed=seed)
    feasible_targets = list(range(min_faces, max_faces + 1))

    instances = []
    dedup = DedupTracker()

    from collections import deque
    queue = deque(plan)
    exhausted_targets = set()

    while queue and len(instances) < count:
        target_faces = queue.popleft()
        if target_faces not in exhausted_targets:
            G, faces, meta = generate_one_targeted(target_faces, constraints, rng, dedup)
            if G is not None:
                instances.append({"graph": G, "faces": faces, "meta": meta})
                continue
            exhausted_targets.add(target_faces)

        remaining_targets = [f for f in feasible_targets if f not in exhausted_targets]
        if not remaining_targets:
            break
        fallback_target = remaining_targets[len(instances) % len(remaining_targets)]
        queue.append(fallback_target)

    if len(instances) < count:
        print(
            f"  [warning] Snowflake category: only {len(instances)} distinct "
            f"(non-isomorphic) graphs could be generated out of the "
            f"requested {count} -- all feasible face-count targets "
            f"{sorted(exhausted_targets)} ran out of structurally distinct "
            f"snowflake graphs under the current constraints. Consider "
            f"loosening max_vertices / max_vertices_in_face, or reducing "
            f"the requested count."
        )

    return instances