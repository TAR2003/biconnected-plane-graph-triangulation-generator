"""
Category 7: Snowflake graphs.

A "snowflake graph" here means a MAXIMAL OUTERPLANAR GRAPH, built the
direct, definition-first way:

  1. Start with a plain n-cycle C_n (vertices 0..n-1 in cyclic order).
     This cycle is, and will REMAIN, the outer face, untouched, for the
     rest of the construction.
  2. Triangulate the INTERIOR of that polygon completely, by adding
     non-crossing diagonals, until every inner face is a triangle (this
     is the classical "polygon triangulation" problem: any simple
     polygon on n vertices needs exactly n-3 non-crossing diagonals to
     be fully triangulated into exactly n-2 triangles).

So the construction directly mirrors the requirement:
  - outer face: exactly the original n-cycle, never modified -- it is
    the LAST face left after every diagonal is drawn, always has size
    n, and its vertex set/order is exactly the polygon boundary.
  - every inner face: a triangle, EXACTLY 3 vertices, never more,
    never fewer -- guaranteed by the triangulation recursion itself
    (see _triangulate_polygon), not by a post-hoc filter.

TRIANGULATION ALGORITHM (recursive random diagonal splitting):
Given a polygon boundary (list of vertices in cyclic order), if it's
already a triangle (3 vertices), stop -- it's a face, done. Otherwise,
pick two non-adjacent boundary vertices i, j, add the diagonal (i, j).
This diagonal splits the polygon into exactly two smaller polygons that
share only that edge; recurse on both halves independently. Because
i and j are always non-adjacent (skips i-1, i, i+1), every added
diagonal is a genuine chord, and because we always split a SIMPLE
polygon into two smaller SIMPLE polygons that don't overlap, no two
diagonals ever cross -- this is the standard proof that any polygon can
be triangulated by n-3 non-crossing diagonals, and it's structurally
guaranteed here, not checked after the fact.

FACE COUNT: for a snowflake graph on n vertices: (n - 2) inner triangles
+ 1 outer face (the original n-cycle) = n - 1 faces total. So targeting
a face count directly translates to targeting an exact vertex count:
n = target_faces + 1. Minimum supported is n = 4 (target_faces = 3):
one inner diagonal splits a square into 2 triangles, plus the outer
4-cycle = 3 faces. n = 3 is excluded on purpose: a bare triangle has no
genuine "outer face distinct from an inner triangle" -- tracing the
single 3-cycle in each direction gives two faces that are literally the
same triangle counted twice, not a meaningfully separate inner/outer
structure, so n >= 4 is required for a real snowflake graph under this
definition.
"""

import random
import networkx as nx

from common import Constraints, validate_graph, relabel_consecutive, build_face_count_plan, DedupTracker


def _triangulate_polygon(n, rng):
    """
    Build one random maximal outerplanar graph on exactly `n` vertices:
    the n-cycle (0, 1, ..., n-1) plus a full random triangulation of its
    interior. Returns the networkx Graph.
    """
    G = nx.Graph()
    G.add_nodes_from(range(n))
    for i in range(n):
        G.add_edge(i, (i + 1) % n)

    def triangulate(vertices):
        # `vertices`: a sub-polygon's boundary, as a list of original
        # vertex ids in cyclic order. Length >= 3 always.
        m = len(vertices)
        if m == 3:
            return  # already a triangular face; nothing to add

        # pick a random vertex i, and a random non-adjacent vertex j
        # (skip i-1, i, i+1 mod m so the diagonal is a genuine chord,
        # never a boundary edge and never a self-loop)
        i = rng.randrange(m)
        forbidden = {(i - 1) % m, i, (i + 1) % m}
        candidates = [j for j in range(m) if j not in forbidden]
        j = rng.choice(candidates)

        a, b = vertices[i], vertices[j]
        G.add_edge(a, b)

        lo, hi = min(i, j), max(i, j)
        # split into two sub-polygons sharing the new diagonal (a, b)
        part1 = vertices[lo:hi + 1]
        part2 = vertices[hi:] + vertices[:lo + 1]
        triangulate(part1)
        triangulate(part2)

    triangulate(list(range(n)))
    return G


def verify_snowflake_structure(G, n):
    """
    Independent, definition-level sanity check on the final graph:

      1. G has exactly n vertices.
      2. The n-cycle (0, 1, ..., n-1) is present in full and intact
         (every boundary edge (i, i+1 mod n) exists) -- confirms the
         outer face was never touched.
      3. G has exactly 2n - 3 edges: n boundary edges + (n - 3)
         diagonals, which is the exact edge count of every maximal
         outerplanar graph on n >= 3 vertices. This is the key
         structural fingerprint that the interior triangulation is
         COMPLETE (every inner face is a triangle, none left larger).

    Returns True/False.
    """
    if G.number_of_nodes() != n:
        return False

    for i in range(n):
        u, v = i, (i + 1) % n
        if not G.has_edge(u, v):
            return False

    expected_edges = 2 * n - 3
    if G.number_of_edges() != expected_edges:
        return False

    return True


def generate_one_targeted(target_faces, constraints: Constraints, rng, dedup: DedupTracker, max_attempts=500):
    """
    Generate a single snowflake graph with EXACTLY `target_faces` faces.

    See module docstring: faces = n - 1, so n = target_faces + 1.
    Minimum supported target_faces is 3 (n = 4).

    Rejects (and retries) any candidate isomorphic to a graph already
    accepted for this category, via `dedup`.
    """
    n = target_faces + 1
    if n < 4:
        return None, None, None  # excluded: no genuine outer/inner distinction at n=3

    for _ in range(max_attempts):
        G = _triangulate_polygon(n, rng)

        if not verify_snowflake_structure(G, n):
            continue

        G, _ = relabel_consecutive(G)

        ok, faces, reason = validate_graph(G, constraints)
        if ok and len(faces) == target_faces:
            # extra, redundant-by-design check: every face must be either
            # the outer n-cycle or a triangle. Since the graph structure
            # already guarantees this (2n-3 edges + intact boundary cycle
            # allows no other possibility), this is just defense in depth.
            sizes = sorted(len(f) for f in faces)
            if sizes[:-1] != [3] * (len(sizes) - 1) or sizes[-1] != n:
                continue
            if dedup.try_add(G):
                return G, faces, {"n_vertices": n, "target_faces": target_faces}
            # isomorphic duplicate -- discard and try a different random triangulation
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

    Additionally, since the OUTER face is always exactly the full
    n-cycle (size n = target_faces + 1), constraints.max_vertices_in_face
    also caps the reachable range for this category specifically (every
    other, inner face is always size exactly 3, so it never binds on
    those) -- this is checked and folded into the ceiling too, with a
    clear error if it leaves nothing feasible.

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
            f"outer 4-cycle = 3 faces). Raise max_faces to at least {min_faces}."
        )

    max_faces = constraints.max_faces if constraints.max_faces is not None else min_faces + 20

    # n = target_faces + 1 must also respect max_vertices, if set
    if constraints.max_vertices is not None:
        max_faces = min(max_faces, constraints.max_vertices - 1)

    # the outer face has size n = target_faces + 1 exactly, so
    # max_vertices_in_face caps target_faces too (inner faces are always
    # size 3 and never bind here)
    if constraints.max_vertices_in_face is not None:
        if constraints.max_vertices_in_face < 3:
            raise RuntimeError(
                f"max_vertices_in_face={constraints.max_vertices_in_face} is "
                f"below 3, so not even a single triangular inner face is "
                f"allowed -- no snowflake graph is possible."
            )
        max_faces = min(max_faces, constraints.max_vertices_in_face - 1)

    if max_faces < min_faces:
        raise RuntimeError(
            f"After combining max_faces / max_vertices / "
            f"max_vertices_in_face, no face-count target >= {min_faces} "
            f"remains feasible for the snowflake category. Loosen one of "
            f"these constraints (max_vertices_in_face in particular must "
            f"be large enough to hold the full outer cycle, i.e. "
            f">= n = target_faces + 1)."
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