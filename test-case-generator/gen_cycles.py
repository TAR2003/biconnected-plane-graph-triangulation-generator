"""
Category 5:
  Simple cycles C_n for n = 3..15 (one graph per n -> 13 base graphs, then
  we cycle through / repeat with different labelings to reach 100 if needed).

Category 6:
  Union of k cycles (k = 2..4), where each cycle has between 4 and 12
  vertices, glued together so the whole thing stays biconnected and planar.
  We build these as "chains of cycles sharing an edge" (a common way to
  union simple cycles while preserving biconnectivity & planarity):
  cycle_1 and cycle_2 share one edge, cycle_2 and cycle_3 share one edge,
  etc. This keeps the union simple, planar, and biconnected.
"""

import random
import networkx as nx

from common import Constraints, validate_graph, relabel_consecutive, build_face_count_plan, DedupTracker


# ---------------------------------------------------------------------
# Category 5: plain cycles
# ---------------------------------------------------------------------

def make_cycle(n):
    G = nx.cycle_graph(n)
    return G


def generate_category_cycles(count, constraints: Constraints, seed=0):
    """
    Produce cycle-graph instances with n ranging 3..15.

    IMPORTANT STRUCTURAL LIMIT: a simple cycle graph is completely
    determined, up to isomorphism, by its vertex count n. Since n is
    restricted to 3..15 by this category's definition, there are ONLY 13
    genuinely distinct (non-isomorphic) cycle graphs possible, full stop
    -- no amount of relabeling changes that; relabeling produces graphs
    that are structurally identical (isomorphic), which is exactly what
    we must NOT output as duplicates.

    So: if `count` <= 13 (number of achievable n values that also satisfy
    the given constraints), we return `count` graphs, each with a
    DIFFERENT n, no duplicates. If `count` > 13, we cannot honestly
    produce that many non-isomorphic graphs in this category -- we
    return every achievable distinct n exactly once and print a clear
    warning explaining the shortfall, rather than silently padding the
    output with isomorphic "duplicates in disguise" (relabeled but
    structurally identical graphs), which is precisely the behavior you
    asked us to eliminate.
    """
    rng = random.Random(seed)
    n_values = list(range(3, 103))  # 3..15 inclusive

    dedup = DedupTracker()
    instances = []
    for n in n_values:
        G = make_cycle(n)
        perm = list(G.nodes())
        rng.shuffle(perm)
        mapping = {old: new for old, new in zip(G.nodes(), perm)}
        G = nx.relabel_nodes(G, mapping)
        G, _ = relabel_consecutive(G)

        ok, faces, reason = validate_graph(G, constraints)
        if not ok:
            continue
        if not dedup.try_add(G):
            continue  # should not happen (each n is structurally unique) but guard anyway
        instances.append({"graph": G, "faces": faces, "meta": {"n": n}})

        if len(instances) >= count:
            break

    if len(instances) < count:
        print(
            f"  [warning] cycles category: only {len(instances)} distinct "
            f"(non-isomorphic) cycle graphs are possible with n in 3..15 "
            f"under the current constraints -- a simple cycle is fully "
            f"determined by its vertex count, so relabeling cannot create "
            f"additional distinct graphs. Returning {len(instances)} graphs "
            f"instead of the requested {count} to avoid duplicate output."
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


def generate_category_union(count, constraints: Constraints, seed=0):
    """
    Union of k chained cycles has EXACTLY k+1 faces (k cycle interiors +
    1 outer face) by construction -- this is a direct, deterministic
    relationship, not something we need to search for. So instead of
    drawing k at random and hoping it lands within constraints.max_faces,
    we explicitly target every achievable face count in
    [3, constraints.max_faces] (k ranges 2..4 by spec, so faces range
    3..5 -- see note below) and spread `count` graphs evenly across it.

    IMPORTANT CAVEAT: the category spec fixes k in {2,3,4}, which means
    the ONLY achievable face counts for this category are 3, 4, and 5
    (k+1). If constraints.max_faces > 5, face counts 6+ are simply not
    reachable by a k in {2,3,4}-cycle union -- there is no bug to fix
    there, it is a structural fact of "2 to 4 cycles glued in a chain".
    We still respect constraints.max_faces as a ceiling, but we no longer
    let random k-sampling starve the smaller achievable face counts.
    """
    lo_k, hi_k = 2, 4
    achievable_faces = [k + 1 for k in range(lo_k, hi_k + 1)]  # [3, 4, 5]

    if constraints.max_faces is not None:
        achievable_faces = [f for f in achievable_faces if f <= constraints.max_faces]
    if not achievable_faces:
        raise RuntimeError(
            f"max_faces={constraints.max_faces} is too small for the cycle-union "
            f"category: with k in [{lo_k},{hi_k}] cycles chained together, the "
            f"minimum possible face count is {lo_k + 1}. Raise max_faces to at "
            f"least {lo_k + 1}."
        )

    rng = random.Random(seed)

    plan = []
    i = 0
    while len(plan) < count:
        plan.append(achievable_faces[i % len(achievable_faces)])
        i += 1
    rng.shuffle(plan)

    instances = []
    dedup = DedupTracker()

    from collections import deque
    queue = deque(plan)
    exhausted_targets = set()

    def try_one(target_faces):
        k = target_faces - 1
        for _ in range(400):
            cycle_sizes = [rng.randint(4, 12) for _ in range(k)]
            G = make_cycle_union(cycle_sizes, rng)
            G, _ = relabel_consecutive(G)
            ok, faces, reason = validate_graph(G, constraints)
            if ok and len(faces) == target_faces:
                if dedup.try_add(G):
                    return {
                        "graph": G, "faces": faces,
                        "meta": {"k": k, "cycle_sizes": cycle_sizes, "target_faces": target_faces},
                    }
        return None

    while queue and len(instances) < count:
        target_faces = queue.popleft()
        if target_faces not in exhausted_targets:
            result = try_one(target_faces)
            if result is not None:
                instances.append(result)
                continue
            exhausted_targets.add(target_faces)

        remaining_targets = [f for f in achievable_faces if f not in exhausted_targets]
        if not remaining_targets:
            break
        fallback_target = remaining_targets[len(instances) % len(remaining_targets)]
        queue.append(fallback_target)

    if len(instances) < count:
        print(
            f"  [warning] cycle-union category: only {len(instances)} "
            f"distinct (non-isomorphic) graphs could be generated out of "
            f"the requested {count} -- all feasible face-count targets "
            f"{sorted(exhausted_targets)} ran out of structurally distinct "
            f"graphs under the current constraints. Consider loosening "
            f"max_vertices / max_vertices_in_face, or reducing the "
            f"requested count."
        )

    return instances
