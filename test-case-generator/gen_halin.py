"""
Category 4:
  Generate a random tree T with >= 4 vertices such that NO vertex of T has
  degree 2 (this is part of the formal Halin-graph definition), then
  connect the leaves of T with a cycle, in the cyclic order induced by a
  planar embedding of T, to form the Halin graph H = T + C.

  A structural check is run at the end (independent of how the tree was
  built) to guarantee: every non-leaf vertex of T has degree >= 3, and
  removing the outer cycle from H exactly reconstructs T. This closes a
  bug where degree-2 chains in T could slip through and appear as
  "orphan" degree-2 vertices spliced into the outer cycle without being
  attached to the internal tree structure.
"""

import random
import networkx as nx

from common import Constraints, validate_graph, relabel_consecutive, build_face_count_plan, DedupTracker


def _random_tree_with_exact_leaves(target_leaves, rng, max_attempts=400):
    """
    Build a random tree with EXACTLY `target_leaves` leaves, where every
    internal (non-leaf) vertex has degree >= 3 (no degree-2 vertices
    anywhere -- required by the Halin definition).

    Construction: start with `target_leaves` leaf "slots" and repeatedly
    merge groups of >=2 of them under a fresh internal parent, chosen
    randomly, until only one group remains (the root). This is like
    building a random unordered binary-or-wider merge tree bottom-up, and
    it guarantees:
      - exactly `target_leaves` original leaves survive as leaves (degree 1),
      - every newly created internal node has degree = (# things merged)
        + (1 parent edge, except the final root which has no parent) which
        is always >= 3 as long as each merge combines >= 2 items AND we
        don't create a chain of internal nodes with only 1 child each.

    Returns (tree, children_map, root), guaranteed to have exactly
    `target_leaves` leaves and no degree-2 vertices, or None if
    target_leaves < 3 (can't form a cycle) or construction fails.
    """
    if target_leaves < 3:
        return None

    for _ in range(max_attempts):
        G = nx.Graph()
        children = {}
        next_id = 0

        # active "groups": each group is represented by its root node id.
        # Start with target_leaves separate leaf nodes.
        groups = []
        for _ in range(target_leaves):
            G.add_node(next_id)
            children[next_id] = []
            groups.append(next_id)
            next_id += 1

        while len(groups) > 1:
            # Merge between 2 and min(3, remaining) groups under one new
            # internal parent. Using up to 3 keeps trees from being
            # perfectly binary every time (more structural variety) while
            # NEVER merging just 1 (which would create a degree-2 node).
            max_merge = min(3, len(groups))
            num_to_merge = rng.randint(2, max_merge)

            rng.shuffle(groups)
            chosen = groups[:num_to_merge]
            rest = groups[num_to_merge:]

            parent = next_id
            next_id += 1
            G.add_node(parent)
            children[parent] = list(chosen)
            for c in chosen:
                G.add_edge(parent, c)

            rest.append(parent)
            groups = rest

        root = groups[0]

        # Structural validation: every internal (has children) node must
        # end with total degree >= 3. Root has no parent edge, so its
        # degree = number of children; non-root internal nodes have
        # degree = 1 (parent) + number of children.
        ok = True
        for node in G.nodes():
            is_leaf = len(children.get(node, [])) == 0
            if is_leaf:
                continue
            deg = G.degree(node)
            if deg < 3:
                ok = False
                break
        if not ok:
            continue

        # Confirm leaf count matches exactly (guards against any edge case
        # in the merge logic, e.g. root ending up as a leaf itself).
        actual_leaves = [n for n in G.nodes() if len(children.get(n, [])) == 0]
        if len(actual_leaves) != target_leaves:
            continue

        return G, children, root

    return None


def leaf_cyclic_order(children, root):
    """
    DFS in children-order to get the left-to-right (planar) order of
    leaves as they appear around the tree when drawn with this rotation
    system. This order, taken as a cycle, is exactly the construction
    used to build a Halin graph from a tree.
    """
    leaves_in_order = []

    def dfs(u):
        kids = children.get(u, [])
        if not kids:
            leaves_in_order.append(u)
            return
        for c in kids:
            dfs(c)

    dfs(root)
    return leaves_in_order


def verify_halin_structure(H, tree_edges, leaves):
    """
    Independent, definition-level sanity check performed on the final
    graph H (not just trusted from construction):

      1. Removing exactly the cycle edges from H must leave precisely
         the original tree T (same edge set) -- no extra/missing edges.
      2. T must have >= 4 vertices and actually be a tree.
      3. Every non-leaf vertex of T must have degree >= 3 in T.
      4. Every leaf of T must have degree exactly 1 in T, and every
         vertex touched by a cycle edge must be exactly one of the
         declared leaves (rules out "orphan" degree-2 vertices spliced
         into the boundary cycle that aren't attached to the tree body).

    Returns True/False.
    """
    n = len(leaves)
    if n < 3:
        return False

    cycle_edges = set()
    for i in range(n):
        u, v = leaves[i], leaves[(i + 1) % n]
        cycle_edges.add(frozenset((u, v)))

    H_edges = set(frozenset(e) for e in H.edges())
    reconstructed_tree_edges = H_edges - cycle_edges

    if reconstructed_tree_edges != set(frozenset(e) for e in tree_edges):
        return False

    T = nx.Graph()
    T.add_nodes_from(H.nodes())
    for e in reconstructed_tree_edges:
        u, v = tuple(e)
        T.add_edge(u, v)

    if T.number_of_nodes() < 4 or not nx.is_tree(T):
        return False

    leaf_set = set(leaves)
    for node in T.nodes():
        deg = T.degree(node)
        if node in leaf_set:
            if deg != 1:
                return False
        else:
            if deg < 3:
                return False

    touched_by_cycle = set()
    for e in cycle_edges:
        touched_by_cycle |= set(e)
    if touched_by_cycle != leaf_set:
        return False

    return True


def build_halin(target_leaves, rng):
    result = _random_tree_with_exact_leaves(target_leaves, rng)
    if result is None:
        return None
    tree, children, root = result

    leaves = leaf_cyclic_order(children, root)
    if len(leaves) < 3:
        return None

    H = tree.copy()
    tree_edges = list(tree.edges())
    for i in range(len(leaves)):
        u = leaves[i]
        v = leaves[(i + 1) % len(leaves)]
        H.add_edge(u, v)

    if not verify_halin_structure(H, tree_edges, leaves):
        return None

    return H


def generate_one_targeted(target_faces, constraints: Constraints, rng, dedup: DedupTracker, max_attempts=500):
    """
    Generate a single Halin graph with EXACTLY `target_faces` faces.

    KEY RELATIONSHIP (verified empirically and structurally): for any
    Halin graph H = T + C, the number of faces equals (number of leaves
    of T) + 1. So targeting a face count directly translates to targeting
    an exact leaf count: leaves = target_faces - 1.

    The absolute minimum face count for ANY Halin graph is 4 (the
    smallest case: a single internal hub connected to exactly 3 leaves,
    i.e. K4). Face counts 2 and 3 are structurally impossible for Halin
    graphs -- this is a mathematical fact of the definition, not a
    generator limitation, so callers should not request target_faces < 4.

    Rejects (and retries) any candidate isomorphic to a graph already
    accepted for this category, via `dedup`.
    """
    target_leaves = target_faces - 1
    if target_leaves < 3:
        return None, None, None  # impossible: Halin graphs need >=3 leaves

    for _ in range(max_attempts):
        H = build_halin(target_leaves, rng)
        if H is None:
            continue
        H, _ = relabel_consecutive(H)

        ok, faces, reason = validate_graph(H, constraints)
        if ok and len(faces) == target_faces:
            if dedup.try_add(H):
                return H, faces, {"target_leaves": target_leaves, "target_faces": target_faces}
            # isomorphic duplicate -- discard and try a different random tree
    return None, None, None


def generate_category(count, constraints: Constraints, seed=0):
    """
    Generates up to `count` Halin graphs with target face counts spread
    evenly across the achievable range [4, constraints.max_faces]. 4 is
    the mathematical minimum (see generate_one_targeted); if max_faces < 4,
    no Halin graph can satisfy the constraint at all and we raise a clear
    error rather than hanging or silently skewing results.

    DUPLICATE HANDLING: every accepted graph is checked against every
    previously accepted graph via exact isomorphism (common.DedupTracker).
    If a face-count target runs out of distinct graphs before its quota
    is filled, the shortfall is redistributed to other feasible targets.
    Only if every feasible target is simultaneously exhausted do we stop
    early with a clear warning, rather than crash or emit a duplicate.
    """
    rng = random.Random(seed)

    min_faces = 4
    if constraints.max_faces is not None and constraints.max_faces < min_faces:
        raise RuntimeError(
            f"max_faces={constraints.max_faces} is below the mathematical "
            f"minimum of {min_faces} faces for any Halin graph (the smallest "
            f"Halin graph is K4: a 3-leaf star plus its triangle cycle, which "
            f"has 4 faces). Raise max_faces to at least {min_faces}."
        )

    max_faces = constraints.max_faces if constraints.max_faces is not None else min_faces + 20
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
            H, faces, meta = generate_one_targeted(target_faces, constraints, rng, dedup)
            if H is not None:
                instances.append({"graph": H, "faces": faces, "meta": meta})
                continue
            exhausted_targets.add(target_faces)

        remaining_targets = [f for f in feasible_targets if f not in exhausted_targets]
        if not remaining_targets:
            break
        fallback_target = remaining_targets[len(instances) % len(remaining_targets)]
        queue.append(fallback_target)

    if len(instances) < count:
        print(
            f"  [warning] Halin category: only {len(instances)} distinct "
            f"(non-isomorphic) graphs could be generated out of the "
            f"requested {count} -- all feasible face-count targets "
            f"{sorted(exhausted_targets)} ran out of structurally distinct "
            f"Halin graphs under the current constraints. Consider loosening "
            f"max_vertices / max_vertices_in_face, or reducing the requested "
            f"count."
        )

    return instances
