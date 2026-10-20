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

from common import Constraints, validate_graph, relabel_consecutive, build_face_count_plan, DedupTracker


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


def _min_outer_face_size(target_faces):
    """
    For a Delaunay-style triangulation with EXACTLY `target_faces` total
    faces (inner triangles + 1 outer face), this returns the smallest
    possible size of the outer (largest, non-triangular in general) face,
    derived from the exact relation between point count m, convex-hull
    size h, and total face count F:

        F = (2m - 2 - h) + 1 = 2m - 1 - h        (T triangles + 1 outer face)

    Minimizing the outer face size h for a fixed F means minimizing m
    (since h = 2m - 1 - F grows with m), subject to h staying in the
    valid range [3, m]. Solving gives:

        m_min = ceil((F + 4) / 2)
        h_min = 2*m_min - 1 - F   (this comes out to 3 if F is even, 4 if F is odd)

    This lets us predict, BEFORE searching, whether a given target face
    count can even be reached without exceeding max_vertices_in_face once
    the outer face is stretched by subdivision (each edge, including the
    outer face's boundary edges, becomes k+1 edges long, so the smallest
    possible outer face size after subdividing k times is h_min * (k+1)).
    """
    import math
    m_min = math.ceil((target_faces + 4) / 2)
    h_min = 2 * m_min - 1 - target_faces
    return h_min


def _base_point_count_for_faces(target_faces):
    """
    Returns a list of candidate base point-counts m that are mathematically
    CAPABLE of producing a triangulation with exactly `target_faces` total
    faces (for at least some convex-hull configuration), using the exact
    relation F = 2m - 1 - h (h = convex hull size, 3 <= h <= m).

    For a given target F: h = 2m - 1 - F, and we need 3 <= h <= m, i.e.
        3 <= 2m - 1 - F  =>  m >= (F + 4) / 2
        2m - 1 - F <= m  =>  m <= F + 1
    """
    import math
    m_lo = math.ceil((target_faces + 4) / 2)
    m_hi = target_faces + 1
    candidates = [m for m in range(max(3, m_lo), max(3, m_hi) + 1)]
    return candidates if candidates else [max(3, target_faces)]


def min_achievable_max_face_size(target_faces, k):
    """
    The smallest possible value of `max face size` achievable for a
    subdivided (k times) triangulation with exactly `target_faces` faces.
    Every face's boundary walk length gets multiplied by (k+1) after
    subdividing every edge k times, so this is simply
    _min_outer_face_size(target_faces) * (k + 1) (the outer face is
    always the largest face in a triangulation, so it's the binding
    constraint against max_vertices_in_face).
    """
    return _min_outer_face_size(target_faces) * (k + 1)


def generate_one_targeted(k, target_faces, constraints: Constraints, rng, dedup: DedupTracker, max_attempts=800):
    """
    Try to generate a single valid subdivided-triangulation instance that
    has EXACTLY `target_faces` faces (subdividing edges doesn't change
    face count, so we just need the BASE triangulation to have exactly
    target_faces faces). Rather than scanning a wide blind range of point
    counts, we precompute a small set of point counts that are
    mathematically capable of producing the target face count, and sample
    from that set -- this makes the search fast even for small targets.

    Rejects (and retries) any candidate that is isomorphic to a graph
    already accepted for this category, via `dedup`, so the final output
    never contains two structurally identical graphs.
    """
    candidate_ms = _base_point_count_for_faces(target_faces)

    for attempt in range(max_attempts):
        base_n = rng.choice(candidate_ms)
        seed = rng.randint(0, 2**31 - 1)
        base_G = random_delaunay_graph(base_n, seed=seed)

        H = subdivide_all_edges(base_G, k)
        H, _ = relabel_consecutive(H)

        ok, faces, reason = validate_graph(H, constraints)
        if ok and len(faces) == target_faces:
            if dedup.try_add(H):
                return H, faces, {"base_points": base_n, "subdivisions": k, "seed": seed}
            # else: isomorphic duplicate of a graph we already accepted --
            # discard and keep trying with a different random point set.

    return None, None, None


def generate_category(k, count, constraints: Constraints, seed=0):
    """
    Generates up to `count` graphs for subdivision level k, with target
    face counts spread EVENLY across the achievable range
    [2, constraints.max_faces]. This guarantees small face counts
    (2, 3, 4, ...) get a fair share instead of being crowded out by chance.

    NOTE: subdividing every edge of a planar graph does NOT change its
    face count -- so the achievable face-count range here is exactly the
    achievable face-count range of the BASE triangulation.

    IMPORTANT FEASIBILITY CHECK: for a triangulation with F total faces,
    the outer face has a hard mathematical minimum size (3 if F is even,
    4 if F is odd -- see _min_outer_face_size). After subdividing k
    times, that minimum grows to _min_outer_face_size(F) * (k+1). If this
    exceeds constraints.max_vertices_in_face, then NO graph with that
    face count can ever satisfy the constraints -- this is a genuine
    mathematical impossibility, not a search failure. We detect this
    BEFORE searching and skip that target with a clear warning instead of
    burning through attempts and crashing mid-run.

    DUPLICATE HANDLING: every accepted graph is checked against every
    previously accepted graph in this category via exact graph
    isomorphism (see common.DedupTracker) so the output never contains
    two structurally identical graphs. If a particular face-count target
    runs out of distinct (non-isomorphic) graphs before its quota is
    filled, the shortfall is automatically redistributed to other
    face-count targets that still have room. Only if EVERY target is
    simultaneously exhausted do we stop early, returning fewer than
    `count` graphs with a clear warning -- never a crash, and never a
    silent duplicate.
    """
    rng = random.Random(seed)

    min_faces = 2
    max_faces = constraints.max_faces if constraints.max_faces is not None else 30
    plan = build_face_count_plan(count, min_faces, max_faces, seed=seed)

    # Pre-filter: drop any face-count target that is mathematically
    # impossible under the current max_vertices_in_face constraint, and
    # redistribute those slots across the remaining achievable targets so
    # we still produce `count` graphs (rather than silently returning fewer).
    if constraints.max_vertices_in_face is not None:
        feasible_targets = [
            f for f in range(min_faces, max_faces + 1)
            if min_achievable_max_face_size(f, k) <= constraints.max_vertices_in_face
        ]
        infeasible_targets = sorted(
            set(range(min_faces, max_faces + 1)) - set(feasible_targets)
        )
        if infeasible_targets:
            print(
                f"  [warning] subdivision k={k}: face counts "
                f"{infeasible_targets} are mathematically unreachable with "
                f"max_vertices_in_face={constraints.max_vertices_in_face} "
                f"(the outer face necessarily grows to at least "
                f"{min_achievable_max_face_size(infeasible_targets[0], k)} "
                f"vertices or more after subdividing each edge {k} time(s)). "
                f"Skipping these targets and redistributing across the "
                f"{len(feasible_targets)} remaining achievable face counts: "
                f"{feasible_targets}."
            )
        if not feasible_targets:
            raise RuntimeError(
                f"No achievable face count in [{min_faces}, {max_faces}] is "
                f"reachable for subdivision k={k} with "
                f"max_vertices_in_face={constraints.max_vertices_in_face}. "
                f"Raise max_vertices_in_face to at least "
                f"{min_achievable_max_face_size(min_faces, k)} to allow the "
                f"smallest possible triangulation through."
            )
        # remap plan: any infeasible target gets replaced by a feasible one,
        # cycling through feasible_targets to keep the distribution even.
        new_plan = []
        j = 0
        for target in plan:
            if target in feasible_targets:
                new_plan.append(target)
            else:
                new_plan.append(feasible_targets[j % len(feasible_targets)])
                j += 1
        plan = new_plan
    else:
        feasible_targets = list(range(min_faces, max_faces + 1))

    instances = []
    dedup = DedupTracker()

    # Use a work-queue so that when a target runs dry (no more distinct
    # graphs available), we can pull the next request from a DIFFERENT
    # target instead of giving up on the whole category.
    from collections import deque
    queue = deque(plan)
    exhausted_targets = set()

    while queue and len(instances) < count:
        target_faces = queue.popleft()
        if target_faces in exhausted_targets:
            # already known to be dry -- redirect to a fresh target below
            pass
        else:
            H, faces, meta = generate_one_targeted(k, target_faces, constraints, rng, dedup)
            if H is not None:
                instances.append({"graph": H, "faces": faces, "meta": meta})
                continue
            # This target came up empty (likely exhausted the distinct
            # graphs available at this size/constraint combination).
            exhausted_targets.add(target_faces)

        # Redirect this slot to whichever feasible target isn't exhausted yet.
        remaining_targets = [f for f in feasible_targets if f not in exhausted_targets]
        if not remaining_targets:
            break  # every feasible target is dry -- stop early, don't crash
        fallback_target = remaining_targets[len(instances) % len(remaining_targets)]
        queue.append(fallback_target)

    if len(instances) < count:
        print(
            f"  [warning] subdivision k={k}: only {len(instances)} distinct "
            f"(non-isomorphic) graphs could be generated out of the "
            f"requested {count} -- all feasible face-count targets "
            f"{sorted(exhausted_targets)} ran out of structurally distinct "
            f"graphs under the current constraints. Consider loosening "
            f"max_vertices / max_vertices_in_face for this category, or "
            f"reducing the requested count."
        )

    return instances
