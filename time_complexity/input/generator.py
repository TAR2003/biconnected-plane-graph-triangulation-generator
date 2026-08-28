"""
Test case generators for biconnected-algorithm testing.

Categories:
1. Delaunay triangulation, each edge subdivided once
2. Delaunay triangulation, each edge subdivided twice
3. Delaunay triangulation, each edge subdivided three times
4. Random tree + cycle through leaves (Halin graph)
5. Cycles, n = 3..15 (cycled through repeatedly to reach N graphs)
6. Integrated union of k cycles (k=2..4): cycles glued together by sharing
   vertices/edges so the result is ONE connected, biconnected, planar graph
   (not a disjoint union).

All generated graphs are required to be planar, and every generated graph's
face structure (outer face included) must respect the global caps:
    MAX_FACES               -> max number of faces allowed (outer face counted)
    MAX_VERTICES_PER_FACE   -> max number of vertices allowed on any single face
    MAX_TOTAL_VERTICES      -> max number of vertices allowed in the whole graph
Graphs that would exceed any cap are rejected and regenerated with smaller
parameters until they fit.
"""

import os
import random
import networkx as nx
import numpy as np
from scipy.spatial import Delaunay


# =====================================================================
# GLOBAL PARAMETERS (tune these)
# =====================================================================

N = 100                     # number of graphs to generate PER CATEGORY
MAX_FACES = 60               # cap on total number of faces (outer face included)
MAX_VERTICES_PER_FACE = 20   # cap on number of vertices on any single face
MAX_TOTAL_VERTICES = 100      # cap on total number of vertices in any one graph

SEED = 42


# =====================================================================
# Face extraction helpers
# =====================================================================

def get_all_faces(embedding):
    """
    Extract every face of a networkx PlanarEmbedding as a list of vertices
    in order around the face (each face is a list of node ids). This
    includes the outer face like any other face.
    """
    faces = []
    visited_half_edges = set()
    for u in embedding.nodes:
        for v in embedding.neighbors_cw_order(u):
            if (u, v) not in visited_half_edges:
                face = embedding.traverse_face(u, v, mark_half_edges=visited_half_edges)
                faces.append(face)
    return faces


def graph_faces_within_caps(G, max_faces=MAX_FACES, max_vertices_per_face=MAX_VERTICES_PER_FACE,
                             max_total_vertices=MAX_TOTAL_VERTICES):
    """
    Returns (ok, faces) where ok is True iff G is planar, its total vertex
    count fits within max_total_vertices, and its face structure satisfies
    the max_faces / max_vertices_per_face caps.
    faces is the list of faces (each a list of vertex ids) if ok, else None.

    max_total_vertices may be None to skip that check entirely.
    """
    if max_total_vertices is not None and G.number_of_nodes() > max_total_vertices:
        return False, None

    is_planar, embedding = nx.check_planarity(G)
    if not is_planar:
        return False, None

    faces = get_all_faces(embedding)

    if len(faces) > max_faces:
        return False, None

    for face in faces:
        if len(face) > max_vertices_per_face:
            return False, None

    return True, faces


def graph_to_face_txt(G, path, node_labels=None):
    """
    Write G's planar faces to `path` in the required format:

        <number of faces>
        <vertex count for face 1>
        <v1 v2 v3 ... vk>
        <vertex count for face 2>
        <v1 v2 v3 ... vk>
        ...

    node_labels: optional dict mapping G's node ids -> integer labels to print
                 (e.g. to renumber nodes as 1..n). Defaults to the node ids
                 themselves.
    """
    ok, embedding = nx.check_planarity(G)
    if not ok:
        raise ValueError("Graph is not planar; cannot extract faces.")

    faces = get_all_faces(embedding)

    if node_labels is None:
        node_labels = {n: n for n in G.nodes}

    with open(path, "w") as f:
        f.write(f"{len(faces)}\n")
        for face in faces:
            f.write(f"{len(face)}\n")
            f.write(" ".join(str(node_labels[v]) for v in face) + "\n")


# =====================================================================
# 1-3: Delaunay triangulation + edge subdivision
# =====================================================================

def random_delaunay_triangulation(num_points=30, seed=None):
    """Random 2D point set -> Delaunay triangulation -> planar graph (as a nx.Graph)."""
    rng = np.random.default_rng(seed)
    pts = rng.random((num_points, 2))
    tri = Delaunay(pts)

    G = nx.Graph()
    for i, (x, y) in enumerate(pts):
        G.add_node(i, pos=(float(x), float(y)))

    for simplex in tri.simplices:
        a, b, c = simplex
        G.add_edge(int(a), int(b))
        G.add_edge(int(b), int(c))
        G.add_edge(int(c), int(a))

    return G


def subdivide_edges(G, times=1):
    """Subdivide every edge of G `times` times (insert `times` new degree-2 nodes per edge)."""
    H = nx.Graph()
    H.add_nodes_from(G.nodes(data=True))
    next_id = max(G.nodes) + 1 if G.nodes else 0

    for u, v in G.edges():
        prev = u
        for _ in range(times):
            H.add_node(next_id)
            H.add_edge(prev, next_id)
            prev = next_id
            next_id += 1
        H.add_edge(prev, v)

    return H


def gen_delaunay_subdivided(num_points=30, subdivisions=1, seed=None):
    G = random_delaunay_triangulation(num_points=num_points, seed=seed)
    return subdivide_edges(G, times=subdivisions)


def gen_delaunay_subdivided_capped(subdivisions, seed,
                                    max_faces=MAX_FACES,
                                    max_vertices_per_face=MAX_VERTICES_PER_FACE,
                                    max_total_vertices=MAX_TOTAL_VERTICES,
                                    min_points=10, max_points_start=40,
                                    max_attempts=500):
    """
    Repeatedly try random point counts (shrinking the search range on
    failure) until the resulting subdivided Delaunay graph satisfies the
    face caps and the total-vertex cap.

    IMPORTANT (two independent infeasibility sources, both checked up front):

    1) Subdividing every edge `subdivisions` times inflates every face's
       vertex count by roughly (subdivisions + 1)x regardless of how many
       points the triangulation started with (a triangular face becomes a
       face with 3*(subdivisions+1) vertices at minimum). If that already
       exceeds max_vertices_per_face, no point count will ever work.

    2) By Euler's formula, a triangulation of n points with h points on the
       convex hull (h >= 3) has 2n - 1 - h total faces (outer face
       included). The worst case (most faces for a given n) occurs at the
       minimum hull size h=3, giving 2n - 4 faces. So max_faces caps n at
       roughly n <= (max_faces + 4) // 2. If the caller's `min_points` is
       already above that bound, the previous version of this function would
       loop 500 times sampling only infeasible point counts and always fail
       -- it shrunk the *upper* bound on retries but never the *lower* one.
       We now shrink both bounds (down to the true minimum of 3 points),
       so it can always reach a feasible point count if one exists.
    """
    min_possible_face_size = 3 * (subdivisions + 1)
    if min_possible_face_size > max_vertices_per_face:
        raise ValueError(
            f"Infeasible caps: with subdivisions={subdivisions}, every face has at least "
            f"{min_possible_face_size} vertices (3 edges x (subdivisions+1)), which already "
            f"exceeds max_vertices_per_face={max_vertices_per_face}. Increase "
            f"max_vertices_per_face to at least {min_possible_face_size} for this category, "
            f"or lower `subdivisions`."
        )

    # Absolute floor for a triangulation; below this graph_faces_within_caps
    # can never even be evaluated meaningfully.
    absolute_min_points = 3

    rng = random.Random(seed)
    lo = max(absolute_min_points, min(min_points, max_points_start))
    hi = max(lo, max_points_start)
    attempt = 0
    while attempt < max_attempts:
        attempt += 1
        num_points = rng.randint(lo, hi)
        G = gen_delaunay_subdivided(num_points=num_points, subdivisions=subdivisions,
                                     seed=seed * 100000 + attempt)
        ok, _ = graph_faces_within_caps(G, max_faces, max_vertices_per_face, max_total_vertices)
        if ok:
            return G

        # Shrink the search space so future attempts are more likely to fit.
        # First try narrowing the upper bound toward the lower bound; once
        # they meet, start lowering the floor itself (down to the absolute
        # minimum of 3) so a persistently-infeasible min_points doesn't
        # doom every attempt.
        if hi > lo:
            hi = max(lo, hi - 2)
        else:
            lo = max(absolute_min_points, lo - 1)
            hi = lo

    raise RuntimeError(
        f"Could not generate a Delaunay(subdivisions={subdivisions}) graph satisfying "
        f"max_faces={max_faces}, max_vertices_per_face={max_vertices_per_face}, "
        f"max_total_vertices={max_total_vertices} within {max_attempts} attempts "
        f"(seed={seed}). Try relaxing the caps."
    )


# =====================================================================
# 4: Halin graph (random tree + cycle through leaves)
# =====================================================================

def random_halin_graph(num_nodes=20, seed=None):
    """
    Build a Halin graph: a random tree (with every internal node having
    degree >= 3, so leaves are well-defined) plus a cycle connecting the
    leaves in the order they appear from a planar embedding (here we just
    use a random planar-ish ordering via a random tree traversal).
    """
    rng = random.Random(seed)
    T = nx.random_labeled_tree(num_nodes, seed=seed) if hasattr(nx, "random_labeled_tree") \
        else nx.random_tree(num_nodes, seed=seed)

    leaves = [n for n in T.nodes if T.degree(n) == 1]

    if len(leaves) < 3:
        leaves = sorted(T.nodes, key=lambda n: T.degree(n))[:max(3, len(leaves))]

    root = next(iter(T.nodes))
    dfs_order = list(nx.dfs_preorder_nodes(T, source=root))
    leaves_ordered = [n for n in dfs_order if n in leaves]

    H = T.copy()
    for i in range(len(leaves_ordered)):
        u = leaves_ordered[i]
        v = leaves_ordered[(i + 1) % len(leaves_ordered)]
        if u != v:
            H.add_edge(u, v)

    return H


def random_halin_graph_capped(seed, max_faces=MAX_FACES, max_vertices_per_face=MAX_VERTICES_PER_FACE,
                               max_total_vertices=MAX_TOTAL_VERTICES,
                               min_nodes=10, max_nodes_start=40, max_attempts=500):
    # A Halin graph needs at least 3 leaves to form a cycle, which in turn
    # needs at least 4 nodes total (root/internal + 3 leaves) to be well
    # defined as a tree-plus-cycle structure.
    absolute_min_nodes = 4

    rng = random.Random(seed)
    lo = max(absolute_min_nodes, min(min_nodes, max_nodes_start))
    hi = max(lo, max_nodes_start)
    attempt = 0
    while attempt < max_attempts:
        attempt += 1
        num_nodes = rng.randint(lo, hi)
        G = random_halin_graph(num_nodes=num_nodes, seed=seed * 100000 + attempt)
        ok, _ = graph_faces_within_caps(G, max_faces, max_vertices_per_face, max_total_vertices)
        if ok:
            return G

        if hi > lo:
            hi = max(lo, hi - 2)
        else:
            lo = max(absolute_min_nodes, lo - 1)
            hi = lo

    raise RuntimeError(
        f"Could not generate a Halin graph satisfying max_faces={max_faces}, "
        f"max_vertices_per_face={max_vertices_per_face}, max_total_vertices={max_total_vertices} "
        f"within {max_attempts} attempts (seed={seed}). Try relaxing the caps."
    )


# =====================================================================
# 5: Simple cycles, n = 3..15, cycled through repeatedly to reach N graphs
# =====================================================================

def cycle_graph(n):
    return nx.cycle_graph(n)


CYCLE_N_RANGE = list(range(3, 16))  # 3..15 inclusive


def gen_cycles_category(num_graphs, max_faces=MAX_FACES, max_vertices_per_face=MAX_VERTICES_PER_FACE,
                         max_total_vertices=MAX_TOTAL_VERTICES):
    """
    Cycle through n = 3..15 repeatedly (matching how the original code cycled
    through [4, 8, 12]) until num_graphs graphs are produced. Any n that would
    violate the face caps or the total-vertex cap is skipped (a cycle C_n
    always has exactly 2 faces: the inner face with n vertices and the outer
    face with n vertices, and n total vertices, so this only matters if
    max_vertices_per_face < 3, max_faces < 2, or max_total_vertices < 3).
    """
    graphs = []
    valid_ns = [
        n for n in CYCLE_N_RANGE
        if n <= max_vertices_per_face and max_faces >= 2
        and (max_total_vertices is None or n <= max_total_vertices)
    ]
    if not valid_ns:
        raise ValueError(
            "MAX_VERTICES_PER_FACE / MAX_FACES / MAX_TOTAL_VERTICES too small to build "
            "any cycle in range 3..15"
        )
    i = 0
    while len(graphs) < num_graphs:
        n = valid_ns[i % len(valid_ns)]
        graphs.append(cycle_graph(n))
        i += 1
    return graphs


# =====================================================================
# 6: Integrated union of k cycles (glued together, sharing vertices/edges)
#
# Cycles are glued one at a time onto the growing graph by sharing an
# existing EDGE on the current outer boundary (this is the standard way to
# attach a new bounded face to a planar biconnected graph while preserving
# both planarity and biconnectivity: gluing two biconnected planar graphs
# along a shared edge always yields a biconnected planar graph). Some glues
# additionally share more than one vertex/edge with the existing structure
# (a shared path of vertices), matching "share one, two, or more vertices".
# =====================================================================

def _shared_edge_glue(G, next_id, boundary_edges, new_cycle_len, rng):
    """
    Glue a new cycle of length `new_cycle_len` onto G by sharing one existing
    edge (u, v) taken from boundary_edges. The new cycle contributes
    (new_cycle_len - 2) brand-new vertices between u and v, forming a second
    u-v path of length (new_cycle_len - 1) edges, which together with the
    existing (u, v) edge closes a new cycle of length new_cycle_len.

    Returns (new_next_id, list_of_new_boundary_edges_from_this_glue).
    """
    u, v = rng.choice(boundary_edges)

    path_internal = [next_id + k for k in range(new_cycle_len - 2)]
    next_id += (new_cycle_len - 2)

    for node in path_internal:
        G.add_node(node)

    chain = [u] + path_internal + [v]
    new_edges = []
    for a, b in zip(chain, chain[1:]):
        G.add_edge(a, b)
        new_edges.append((a, b))

    return next_id, new_edges


def _shared_path_glue(G, next_id, boundary_edges, new_cycle_len, rng, shared_len=2):
    """
    Glue a new cycle onto G sharing a PATH of `shared_len` consecutive
    existing edges (i.e. shared_len + 1 shared vertices) rather than just one
    edge -- this realizes the "share two or more vertices" case. Falls back
    to a single-edge glue if the boundary doesn't have a long enough
    consecutive run available.
    """
    if len(boundary_edges) < shared_len:
        return _shared_edge_glue(G, next_id, boundary_edges, new_cycle_len, rng)

    start = rng.randrange(len(boundary_edges))
    chosen = [boundary_edges[(start + k) % len(boundary_edges)] for k in range(shared_len)]

    # Only valid if edges chain together u1-u2-u3-...
    ok_chain = all(chosen[k][1] == chosen[k + 1][0] for k in range(len(chosen) - 1))
    if not ok_chain:
        return _shared_edge_glue(G, next_id, boundary_edges, new_cycle_len, rng)

    u = chosen[0][0]
    v = chosen[-1][1]
    shared_vertex_count = shared_len + 1

    remaining = new_cycle_len - shared_vertex_count
    if remaining < 1:
        return _shared_edge_glue(G, next_id, boundary_edges, new_cycle_len, rng)

    path_internal = [next_id + k for k in range(remaining - 1)] if remaining >= 1 else []
    next_id += max(0, remaining - 1)

    for node in path_internal:
        G.add_node(node)

    chain = [u] + path_internal + [v]
    new_edges = []
    for a, b in zip(chain, chain[1:]):
        if not G.has_edge(a, b):
            G.add_edge(a, b)
        new_edges.append((a, b))

    return next_id, new_edges


def integrated_union_of_cycles(k_range=(2, 4), n_range=(4, 12), seed=None):
    """
    Build k cycles GLUED together (sharing vertices/edges) into a single
    connected, biconnected, planar graph -- not a disjoint union.

    Construction:
      1. Start with a base cycle of length n0 (n_range).
      2. For each additional cycle (k-1 more), pick a random existing
         boundary edge (or short boundary path) and glue a new cycle onto it,
         sharing 1 edge (2 vertices) most of the time, occasionally sharing a
         longer path (3+ vertices) to realize "two or more shared vertices".
    Every glue operation preserves biconnectivity and planarity by
    construction, so the final graph is guaranteed biconnected + planar.
    """
    rng = random.Random(seed)
    k = rng.randint(*k_range)

    n0 = rng.randint(*n_range)
    G = nx.cycle_graph(n0)
    next_id = n0

    boundary_edges = list(G.edges())
    # cycle_graph gives directed-feeling pairs but nx.Graph edges are undirected;
    # normalize to a consistent traversal order (both directions available to glue on)
    boundary_edges = boundary_edges + [(b, a) for (a, b) in boundary_edges]

    for _ in range(k - 1):
        new_len = rng.randint(*n_range)
        if new_len < 3:
            new_len = 3

        share_long = rng.random() < 0.3  # 30% chance: share a longer path
        if share_long and len(boundary_edges) >= 2:
            shared_len = rng.randint(2, min(3, len(boundary_edges)))
            next_id, new_edges = _shared_path_glue(G, next_id, boundary_edges, new_len, rng, shared_len)
        else:
            next_id, new_edges = _shared_edge_glue(G, next_id, boundary_edges, new_len, rng)

        boundary_edges.extend(new_edges)
        boundary_edges.extend([(b, a) for (a, b) in new_edges])

    return G


def integrated_union_of_cycles_capped(seed, max_faces=MAX_FACES, max_vertices_per_face=MAX_VERTICES_PER_FACE,
                                       max_total_vertices=MAX_TOTAL_VERTICES,
                                       k_range=(2, 4), n_range_start=(4, 12), max_attempts=500):
    """
    Repeatedly attempt integrated_union_of_cycles with shrinking n_range (and,
    if needed, shrinking k) on failure until the result satisfies the global
    face caps and the total-vertex cap.
    """
    if n_range_start[0] > max_vertices_per_face or 3 > max_vertices_per_face:
        raise ValueError(
            f"Infeasible caps: cannot build any cycle-based face with "
            f"max_vertices_per_face={max_vertices_per_face} (need at least 3)."
        )

    rng = random.Random(seed)
    hi = n_range_start[1]
    lo = min(n_range_start[0], max_vertices_per_face)
    k_hi = k_range[1]
    attempt = 0
    while attempt < max_attempts:
        attempt += 1
        n_range = (lo, max(lo, min(hi, max_vertices_per_face)))
        cur_k_range = (min(k_range[0], k_hi), max(k_range[0], k_hi))
        G = integrated_union_of_cycles(k_range=cur_k_range, n_range=n_range, seed=seed * 100000 + attempt)
        ok, _ = graph_faces_within_caps(G, max_faces, max_vertices_per_face, max_total_vertices)
        if ok and nx.is_connected(G):
            # sanity-check biconnectivity too
            if G.number_of_nodes() < 3 or nx.is_biconnected(G):
                return G

        if hi > lo:
            hi = max(lo, hi - 1)
        elif k_hi > k_range[0]:
            # Narrowing n_range alone wasn't enough (likely because too many
            # glued cycles push the total vertex count over the cap) -- also
            # shrink how many cycles we glue together.
            k_hi = max(k_range[0], k_hi - 1)
        else:
            lo = max(3, lo - 1)
            hi = lo

    raise RuntimeError(
        f"Could not generate an integrated union-of-cycles graph satisfying "
        f"max_faces={max_faces}, max_vertices_per_face={max_vertices_per_face}, "
        f"max_total_vertices={max_total_vertices} within {max_attempts} attempts (seed={seed})."
    )


# =====================================================================
# Batch generation for all 6 categories
# =====================================================================

def generate_all_test_cases(num_graphs=N, seed=SEED,
                             max_faces=MAX_FACES, max_vertices_per_face=MAX_VERTICES_PER_FACE,
                             max_total_vertices=MAX_TOTAL_VERTICES):
    """
    Returns a dict: category_name -> list of nx.Graph (length num_graphs each).
    """
    results = {}

    # 1-3: Delaunay + subdivision
    for subdiv in (1, 2, 3):
        key = f"delaunay_subdivide_{subdiv}x"
        graphs = []
        for i in range(num_graphs):
            graphs.append(
                gen_delaunay_subdivided_capped(
                    subdivisions=subdiv,
                    seed=seed + subdiv * 1000 + i,
                    max_faces=max_faces,
                    max_vertices_per_face=max_vertices_per_face,
                    max_total_vertices=max_total_vertices,
                )
            )
        results[key] = graphs

    # 4: Halin graphs
    graphs = []
    for i in range(num_graphs):
        graphs.append(
            random_halin_graph_capped(
                seed=seed + 4000 + i,
                max_faces=max_faces,
                max_vertices_per_face=max_vertices_per_face,
                max_total_vertices=max_total_vertices,
            )
        )
    results["halin_graph"] = graphs

    # 5: Cycles n=3..15 (cycled through repeatedly for num_graphs)
    results["cycle_3_to_15"] = gen_cycles_category(
        num_graphs, max_faces=max_faces, max_vertices_per_face=max_vertices_per_face,
        max_total_vertices=max_total_vertices,
    )

    # 6: Integrated union of k cycles (glued, sharing vertices)
    graphs = []
    for i in range(num_graphs):
        graphs.append(
            integrated_union_of_cycles_capped(
                seed=seed + 6000 + i,
                max_faces=max_faces,
                max_vertices_per_face=max_vertices_per_face,
                max_total_vertices=max_total_vertices,
            )
        )
    results["integrated_union_of_cycles"] = graphs

    return results


def save_all_test_cases(num_graphs=N, seed=SEED, base_dir=None,
                         max_faces=MAX_FACES, max_vertices_per_face=MAX_VERTICES_PER_FACE,
                         max_total_vertices=MAX_TOTAL_VERTICES):
    """
    Generates all categories and saves each graph's planar faces as a .txt
    file (format described in graph_to_face_txt) inside a category-named
    subfolder next to this script (or under base_dir if given).

    Folder layout:
        base_dir/delaunay_subdivide_1x/graph_000.txt ... graph_{N-1:03d}.txt
        base_dir/delaunay_subdivide_2x/...
        base_dir/delaunay_subdivide_3x/...
        base_dir/halin_graph/...
        base_dir/cycle_3_to_15/...
        base_dir/integrated_union_of_cycles/...
    """
    if base_dir is None:
        base_dir = os.path.dirname(os.path.abspath(__file__))

    data = generate_all_test_cases(
        num_graphs=num_graphs, seed=seed,
        max_faces=max_faces, max_vertices_per_face=max_vertices_per_face,
        max_total_vertices=max_total_vertices,
    )

    for name, graphs in data.items():
        folder = os.path.join(base_dir, name)
        os.makedirs(folder, exist_ok=True)
        for i, g in enumerate(graphs):
            path = os.path.join(folder, f"graph_{i:03d}.txt")
            relabeled = {old: new for new, old in enumerate(sorted(g.nodes), start=1)}
            graph_to_face_txt(g, path, node_labels=relabeled)
        print(f"saved {len(graphs)} graphs to {folder}")


if __name__ == "__main__":
    save_all_test_cases(num_graphs=N, seed=SEED)