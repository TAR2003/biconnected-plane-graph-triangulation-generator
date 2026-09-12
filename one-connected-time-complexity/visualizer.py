import networkx as nx
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import sys
import os
import math
import numpy as np
from itertools import combinations


def parse_faces(text):
    lines = [l.strip() for l in text.strip().splitlines() if l.strip() != ""]
    idx = 0
    num_faces = int(lines[idx]); idx += 1

    faces = []
    for _ in range(num_faces):
        n = int(lines[idx]); idx += 1
        verts = list(map(int, lines[idx].split())); idx += 1
        assert len(verts) == n, f"Expected {n} vertices, got {len(verts)}: {verts}"
        faces.append(verts)
    return faces


def build_graph(faces):
    G = nx.Graph()
    for face in faces:
        n = len(face)
        if n == 1:
            G.add_node(face[0])
            continue
        for i in range(n):
            u = face[i]
            v = face[(i + 1) % n]
            if u != v:
                G.add_edge(u, v)
            else:
                G.add_node(u)
    return G


def polygon_centroid(points):
    """Standard polygon centroid (works for simple, non-self-intersecting polygons)."""
    n = len(points)
    if n < 3:
        xs = [p[0] for p in points]
        ys = [p[1] for p in points]
        return sum(xs) / n, sum(ys) / n

    A = 0.0
    Cx = 0.0
    Cy = 0.0
    for i in range(n):
        x0, y0 = points[i]
        x1, y1 = points[(i + 1) % n]
        cross = x0 * y1 - x1 * y0
        A += cross
        Cx += (x0 + x1) * cross
        Cy += (y0 + y1) * cross
    A *= 0.5
    if abs(A) < 1e-9:
        xs = [p[0] for p in points]
        ys = [p[1] for p in points]
        return sum(xs) / n, sum(ys) / n
    Cx /= (6 * A)
    Cy /= (6 * A)
    return Cx, Cy


# ---------------------------------------------------------------------------
# Adaptive sizing: figure size, node size, font size, and outer-circle radius
# all scale with the number of vertices (and, for radius, with how many
# vertices end up "free"/interior) so dense graphs get more room instead of
# being squeezed onto the same fixed canvas.
# ---------------------------------------------------------------------------

def contract_degree2_chains(G):
    """
    Find every maximal path of degree-2 vertices (a "subdivision chain")
    between two vertices of degree != 2, and contract each chain down to
    a single direct edge between its two endpoints.

    Returns:
      - H: the contracted graph (a topological minor of G). H is a
        MultiGraph because two distinct chains can share the same pair of
        endpoints (e.g. two different paths across a triangulation both
        running between the same two branch vertices) -- using a
        MultiGraph means neither chain is silently dropped.
      - chains: dict mapping a unique chain key -> (u, v, interior_path)
        where interior_path is the ordered list of degree-2 vertices
        removed between endpoint u and endpoint v (in path order from u
        to v). Each physical chain appears EXACTLY ONCE in this dict
        (not once per traversal direction).

    If the whole graph is degree-2 (a pure cycle, e.g. category 5), every
    vertex would be "interior" by this definition -- there'd be no fixed
    endpoints to contract towards. In that case this function returns the
    graph unchanged with no chains, since there is nothing degree-2-heavy
    to simplify (a plain cycle already draws fine as a regular polygon).
    """
    degrees = dict(G.degree())
    non_deg2 = {v for v, d in degrees.items() if d != 2}

    if not non_deg2:
        # Pure cycle (or isolated components) -- nothing to contract.
        return G.copy(), {}

    H = nx.MultiGraph()
    H.add_nodes_from(non_deg2)

    chains = {}
    # Track visited *directed half-edges* out of degree-2 territory, not
    # visited nodes -- a node-visited set is what silently dropped chains
    # that happen to share endpoints with another chain. Each degree-2
    # chain is only ever entered from its two ends, so marking BOTH the
    # (start -> first_interior) step AND the (end -> last_interior) step as
    # consumed as soon as we walk the chain -- from whichever end we reach
    # it first -- is what prevents us from walking (and recording) the very
    # same physical chain a second time when we later iterate to its other
    # endpoint. Previously only the entry step actually used was recorded
    # birectionally in a way that still let the *other* direction slip
    # through, which duplicated every chain (and every parallel edge in H)
    # -- collapsing 2-branch-vertex "theta" graphs into a degenerate
    # multigraph that could only be laid out as a straight line.
    consumed_entry_steps = set()
    chain_id = 0

    for start in sorted(non_deg2):
        for first_step in sorted(G.neighbors(start)):
            if degrees[first_step] != 2:
                # direct edge between two non-degree-2 vertices, no chain
                if first_step in non_deg2 and start < first_step:
                    H.add_edge(start, first_step)
                continue

            entry_step = (start, first_step)
            if entry_step in consumed_entry_steps:
                continue

            # Walk the degree-2 chain until we hit a non-degree-2 vertex
            path = [first_step]
            prev, cur = start, first_step
            while True:
                nxt_candidates = [n for n in G.neighbors(cur) if n != prev]
                if not nxt_candidates:
                    # Dead end (shouldn't happen for a true degree-2 interior
                    # node, since it always has exactly 2 neighbors -- but
                    # guard defensively rather than crash).
                    end = cur
                    path.pop()
                    break
                nxt = nxt_candidates[0]
                if degrees[nxt] != 2:
                    end = nxt
                    break
                path.append(nxt)
                prev, cur = cur, nxt

            # Mark both directions of this chain's entry steps as consumed
            # so we don't walk (or record) it again from the `end` side
            # later in the outer loop. The reverse entry step is
            # (end, last_interior_vertex_before_end) -- that's always
            # path[-1] (the last element of the walked interior path),
            # NOT path[-2]. Using path[-2] here was the bug: for a 3+ long
            # chain it marked the wrong neighbor as consumed, so walking
            # from `end` back toward `start` was never actually blocked,
            # and the same physical chain got recorded a second time
            # (reversed) -- duplicating parallel edges in H and causing
            # overlapping vertex placements downstream.
            consumed_entry_steps.add((start, first_step))
            consumed_entry_steps.add((end, path[-1]))

            H.add_edge(start, end)
            chains[chain_id] = (start, end, path)
            chain_id += 1

    return H, chains


def expand_chain_positions(pos, chains):
    """
    Given Tutte positions `pos` for the contracted graph's endpoint
    vertices, place every interior degree-2 chain vertex along the
    segment between its two endpoints.

    IMPORTANT: multiple distinct chains can connect the SAME pair of
    endpoints (H is a MultiGraph precisely because this happens --
    e.g. two different paths across a triangulation both running between
    the same two branch vertices). If every such parallel chain were
    placed on the exact straight segment u->v, they would all land on
    identical (t-fraction-of-the-way) points and overlap each other
    node-for-node -- this was the cause of vertices like "1/6" and
    "2/11" being drawn stacked on top of each other. To fix this, when
    more than one chain shares an endpoint pair, only ONE of them (the
    longest, arbitrarily) is drawn on the straight segment; every other
    parallel chain is bowed outward along a perpendicular arc so it
    occupies its own distinct curve between the same two endpoints,
    guaranteeing every interior vertex gets a unique position.
    """
    full_pos = dict(pos)

    # Group chains by their (unordered) endpoint pair so we can detect
    # when 2+ chains are parallel (share both endpoints).
    groups = {}
    for chain_id, (u, v, interior) in chains.items():
        if u not in pos or v not in pos:
            continue
        key = frozenset((u, v))
        groups.setdefault(key, []).append((chain_id, u, v, interior))

    for key, group in groups.items():
        # Longest chain first -- it gets the undisturbed straight segment;
        # the rest bow outward on alternating sides at increasing offsets.
        group.sort(key=lambda item: len(item[3]), reverse=True)

        u_ref, v_ref = tuple(key)
        p_u_ref = np.array(pos[u_ref])
        p_v_ref = np.array(pos[v_ref])
        seg_vec = p_v_ref - p_u_ref
        seg_len = np.linalg.norm(seg_vec)
        if seg_len < 1e-12:
            perp = np.array([0.0, 0.0])
        else:
            perp = np.array([-seg_vec[1], seg_vec[0]]) / seg_len  # unit perpendicular

        for rank, (chain_id, u, v, interior) in enumerate(group):
            p_u = np.array(pos[u])
            p_v = np.array(pos[v])
            k = len(interior)

            if rank == 0:
                # Primary chain for this endpoint pair: straight line, as before.
                for i, node in enumerate(interior, start=1):
                    t = i / (k + 1)
                    point = p_u + t * (p_v - p_u)
                    full_pos[node] = (float(point[0]), float(point[1]))
            else:
                # Parallel chain: bow it out along the perpendicular so its
                # interior vertices trace a distinct arc rather than
                # colliding with the primary chain's straight line or with
                # any other parallel chain. Alternate sides (+/-); the bow
                # step is normalized by how many parallel chains share this
                # endpoint pair (num_extra), NOT by an unbounded rank *
                # constant -- with many parallel chains (e.g. 8 single-node
                # chains between the same 2 hubs) an unbounded per-rank
                # step pushed later chains far enough to cross the outer
                # polygon boundary entirely. Capping the total fan-out
                # width to a fixed fraction of the segment length,
                # subdivided evenly across however many parallel chains
                # actually exist, keeps every bow safely inside regardless
                # of how many parallel chains there are.
                num_extra = len(group) - 1  # chains needing a bow (excludes rank 0)
                side = 1 if (rank % 2 == 1) else -1
                bow_rank = (rank + 1) // 2
                max_bow_rank = (num_extra + 1) // 2
                # Total fan width capped at 35% of the segment length, split
                # evenly across however many bow "slots" (max_bow_rank) are
                # needed, so adding more parallel chains packs them closer
                # together instead of pushing the outermost one further out.
                bow_step = (0.35 * seg_len) / max(max_bow_rank, 1)
                bow_amount = bow_step * bow_rank * side
                for i, node in enumerate(interior, start=1):
                    t = i / (k + 1)
                    base_point = p_u + t * (p_v - p_u)
                    # Bow magnitude follows a sine arc profile (0 at the
                    # endpoints, max at the chain's midpoint) so it meets
                    # the fixed endpoints smoothly with no kink.
                    arc_t = i / (k + 1)
                    bow = math.sin(arc_t * math.pi) * bow_amount
                    point = base_point + perp * bow
                    full_pos[node] = (float(point[0]), float(point[1]))

    return full_pos


def compute_style(num_nodes, num_free_nodes):
    """
    Returns a dict of matplotlib style parameters scaled to graph size.

    - figsize grows with sqrt(num_nodes) so area scales roughly linearly
      with vertex count (keeps density-per-inch roughly constant instead
      of squeezing everything into a fixed 13x13 canvas).
    - node_size and font_size shrink as num_nodes grows, but are clamped
      so tiny graphs (a 4-cycle) don't get comically huge markers and
      huge graphs don't get illegibly tiny ones.
    - outer_radius grows with the number of free (interior) vertices,
      since Tutte's barycentric embedding packs interior vertices into
      the space bounded by the fixed unit-circle outer face -- more
      interior vertices means that space needs to be proportionally
      bigger or they collapse together near the center.
    """
    n = max(num_nodes, 1)

    # Figure size: sqrt scaling keeps canvas *area* roughly proportional to
    # vertex count. Base case (n=~10-15) still lands close to the old 13x13.
    side = 9.0 + 2.6 * math.sqrt(n)
    side = min(side, 42.0)  # cap so extremely large graphs don't blow up matplotlib/memory
    figsize = (side, side)

    # Node size: shrink with n, but clamp to a sane range.
    node_size = max(60, min(1100, 14000.0 / n))

    # Font size: shrink with n, clamp to a legible minimum.
    font_size = max(6, min(20, 140.0 / math.sqrt(n)))

    edge_width = max(0.5, min(2.0, 10.0 / math.sqrt(n)))

    # Outer radius: base radius of 1, scaled up as interior vertex count grows,
    # so Tutte's barycentric placement has proportionally more room and
    # interior vertices don't collapse into a tight clump near the center.
    outer_radius = 1.0 + 0.35 * math.sqrt(max(num_free_nodes, 0))

    return {
        "figsize": figsize,
        "node_size": node_size,
        "font_size": font_size,
        "edge_width": edge_width,
        "outer_radius": outer_radius,
    }


def segments_properly_intersect(p1, p2, p3, p4):
    """True if segment p1-p2 and segment p3-p4 cross at an interior point
    (not just touching at a shared endpoint). Standard orientation test."""
    def orient(a, b, c):
        val = (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0])
        if val > 1e-12:
            return 1
        if val < -1e-12:
            return -1
        return 0

    o1 = orient(p1, p2, p3)
    o2 = orient(p1, p2, p4)
    o3 = orient(p3, p4, p1)
    o4 = orient(p3, p4, p2)

    return (o1 != o2 and o1 != 0 and o2 != 0) and (o3 != o4 and o3 != 0 and o4 != 0)


def has_crossing(pos, edges):
    """Check whether any two non-adjacent edges cross, given a straight-line
    placement `pos`. Used as a safety net after repulsion-relaxation."""
    edge_list = [(u, v) for u, v in edges if u in pos and v in pos]
    n = len(edge_list)
    for i in range(n):
        u1, v1 = edge_list[i]
        for j in range(i + 1, n):
            u2, v2 = edge_list[j]
            if len({u1, v1, u2, v2}) < 4:
                continue  # shares an endpoint -- not a crossing
            if segments_properly_intersect(pos[u1], pos[v1], pos[u2], pos[v2]):
                return True
    return False


def relax_free_nodes(pos, fixed_nodes, G, iterations=60, min_dist=None, max_step_frac=0.15):
    """
    Spread out interior (non-fixed) vertices that ended up clustered too
    close together after Tutte's barycentric placement, WITHOUT moving the
    fixed outer-face vertices.

    Vectorized short-range repulsion: at each iteration every free node is
    pushed away from any other node closer than `min_dist`, then blended
    slightly back toward its neighbor-average (to keep it near its
    Tutte-consistent position). Each per-node move is capped at
    `max_step_frac` of that node's shortest incident edge length in the
    original Tutte layout, so a single step is too small to swing an edge
    across another one -- keeping the perturbation crossing-safe in
    practice. A crossing check still runs afterward as a final safety net
    (see `has_crossing`); the caller falls back to pure Tutte if it fails.
    """
    free_nodes = [v for v in pos if v not in fixed_nodes]
    if not free_nodes:
        return pos

    node_list = list(pos.keys())
    n = len(node_list)
    idx_of = {v: i for i, v in enumerate(node_list)}
    coords = np.array([pos[v] for v in node_list])  # (n, 2)

    xs, ys = coords[:, 0], coords[:, 1]
    extent = max(xs.max() - xs.min(), ys.max() - ys.min(), 1e-6)

    if min_dist is None:
        min_dist = 0.6 * extent / math.sqrt(max(n, 1))

    # Per-free-node cap: a fraction of its shortest incident edge length,
    # so repulsion can never move a node far enough to plausibly cross a
    # neighboring edge in one step.
    step_cap = {}
    for v in free_nodes:
        neighbors = list(G.neighbors(v))
        lens = [np.linalg.norm(coords[idx_of[v]] - coords[idx_of[u]])
                for u in neighbors if u in idx_of]
        shortest = min(lens) if lens else extent / math.sqrt(n)
        step_cap[v] = max_step_frac * shortest

    free_idx = np.array([idx_of[v] for v in free_nodes])
    is_free = np.zeros(n, dtype=bool)
    is_free[free_idx] = True

    for _ in range(iterations):
        # Vectorized pairwise displacement: (n, n, 2)
        diff = coords[:, None, :] - coords[None, :, :]
        dist = np.linalg.norm(diff, axis=2)
        np.fill_diagonal(dist, np.inf)

        close_mask = dist < min_dist
        if not close_mask.any():
            break

        safe_dist = np.where(dist < 1e-9, 1e-9, dist)
        strength = np.where(close_mask, (min_dist - dist) / min_dist, 0.0)
        unit = diff / safe_dist[:, :, None]
        repulsion = (unit * strength[:, :, None]).sum(axis=1)  # (n, 2)

        new_coords = coords.copy()
        for i in free_idx:
            v = node_list[i]
            neighbors = [u for u in G.neighbors(v) if u in idx_of]
            if neighbors:
                avg = coords[[idx_of[u] for u in neighbors]].mean(axis=0)
            else:
                avg = coords[i]

            move = 0.1 * (avg - coords[i]) + 0.4 * repulsion[i]
            move_norm = np.linalg.norm(move)
            cap = step_cap[v]
            if move_norm > cap and move_norm > 1e-12:
                move = move * (cap / move_norm)

            new_coords[i] = coords[i] + move

        coords = new_coords

    return {v: (float(coords[idx_of[v]][0]), float(coords[idx_of[v]][1])) for v in node_list}


def diagnose_compatibility(G, faces, is_planar, embedding):
    """
    Returns a list of short warning strings describing any problems found:
      - non-planarity, with which faces (by index) contributed the edges
        involved in the crossing conflict
    (Isolated-vertex warnings are added separately during layout, once we
    know they couldn't be placed via the neighbor-averaging solve.)
    """
    warnings = []

    # 2) Non-planarity: find a Kuratowski subgraph (K5 or K3,3 minor/subdivision)
    #    and report which input faces contributed its edges.
    if not is_planar:
        try:
            _, culprit = nx.check_planarity(G, counterexample=True)
            culprit_edges = set(culprit.edges())
        except Exception:
            culprit_edges = set()

        involved_faces = set()
        for fi, face in enumerate(faces, start=1):
            n = len(face)
            face_edges = {tuple(sorted((face[i], face[(i + 1) % n]))) for i in range(n)}
            for (u, v) in culprit_edges:
                if tuple(sorted((u, v))) in face_edges:
                    involved_faces.add(fi)

        if involved_faces:
            warnings.append(
                f"Graph is NOT planar - faces {sorted(involved_faces)} together "
                f"require edges to cross (incompatible layout)."
            )
        else:
            warnings.append("Graph is NOT planar - some combination of faces requires crossing edges.")

    return warnings


def _build_theta_outer_face(chains, hub_u, hub_v):
    """
    Special case for "generalized theta graphs": once degree-2 chains are
    contracted, if the ENTIRE graph collapses to just two branch/hub
    vertices connected by two-or-more parallel chains, the contracted
    multigraph H has no face of 3+ distinct vertices for Tutte's method to
    use as an outer polygon (every face the planar embedding reports is
    just the 2-cycle [hub_u, hub_v]). Left unhandled, the "outer face"
    degenerates to 2 points, which collapses the whole drawing onto a
    single straight line -- exactly the "just a line" symptom.

    Fix: manually build a genuine outer polygon by using the two LONGEST
    chains between the hubs as the two arcs of the outer boundary (walking
    one chain from hub_u->hub_v, then the other chain back from
    hub_v->hub_u), and route every other chain as an interior path that
    starts and ends on the hubs but bulges into the interior.

    Returns (outer, chosen_chain_ids):
      - outer: a list of vertices (outer polygon, in order) suitable for
        passing straight to `try_outer_face`, built at the ORIGINAL
        (uncontracted) vertex level -- i.e. hub_u, then the interior
        vertices of one chain in order, then hub_v, then the interior
        vertices of another chain in reverse order, back to hub_u.
      - chosen_chain_ids: the set of chain ids consumed to build the
        boundary, so the caller can exclude them from further chain
        processing (their interior vertices are now fixed boundary points,
        not free/interpolated ones).
    Returns (None, None) if fewer than 2 parallel chains exist between the
    two hubs (shouldn't happen when this is called, but guarded).
    """
    # chains is {chain_id: (u, v, interior_path)}, already deduplicated so
    # each physical chain appears exactly once, oriented start->end.
    relevant = []
    for cid, (u, v, interior) in chains.items():
        if {u, v} == {hub_u, hub_v}:
            relevant.append((cid, u, v, interior))

    if len(relevant) < 2:
        return None, None

    # Prefer the two chains with the most interior vertices, so the outer
    # boundary has the most "room" and the remaining (shorter) chains are
    # left as interior chords -- purely a cosmetic choice, any 2 works.
    relevant.sort(key=lambda t: (len(t[3]), t[0]), reverse=True)
    (cid1, u1, v1, path1), (cid2, u2, v2, path2) = relevant[0], relevant[1]

    # Orient both chains starting from hub_u.
    if u1 != hub_u:
        u1, v1, path1 = v1, u1, list(reversed(path1))
    if u2 != hub_u:
        u2, v2, path2 = v2, u2, list(reversed(path2))

    outer = [hub_u] + path1 + [hub_v] + list(reversed(path2))
    return outer, {cid1, cid2}


def layout_component(G_comp, faces):
    """
    Compute a straight-line planar (or spring, if non-planar) layout for a
    single CONNECTED component, using the Tutte/chain-contraction pipeline.

    `faces` is the full list of input faces (from the original file) --
    used only for scoring outer-face choices (min_face_area), matched
    against whichever of a face's vertices happen to fall in this
    component; faces entirely outside this component contribute nothing
    and are harmless to pass in unfiltered.

    Returns: (pos, unplaced, outer_verts, warnings, is_planar)
      - pos: dict node -> (x, y) for every node in G_comp
      - unplaced: list of degree-0 nodes placed off to the side
      - outer_verts: the list of vertices on the chosen outer face
        (empty list if the non-planar/spring-layout path was used)
      - warnings: list of warning strings specific to this component
      - is_planar: bool
    """
    warnings = []
    is_planar, embedding = nx.check_planarity(G_comp)

    if not is_planar:
        pos = nx.spring_layout(G_comp, seed=42, k=0.9)
        warnings.append("This component is NOT planar -- drawn with a "
                         "force-directed layout instead (edges may cross).")
        warnings.extend(diagnose_compatibility(G_comp, faces, is_planar, embedding))
        return pos, [], [], warnings, False

    num_nodes = G_comp.number_of_nodes()

    H, chains = contract_degree2_chains(G_comp)

    covered = set(H.nodes())
    for _cid, (_u, _v, _path) in chains.items():
        covered.update(_path)
    missing = set(G_comp.nodes()) - covered
    if missing:
        raise AssertionError(
            f"contract_degree2_chains dropped {len(missing)} vertex(es): "
            f"{sorted(missing)[:10]}{'...' if len(missing) > 10 else ''}"
        )

    H_is_planar, H_embedding = nx.check_planarity(H)
    if not H_is_planar:
        H, chains = G_comp, {}
        H_embedding = embedding
    layout_graph = H
    num_layout_nodes = layout_graph.number_of_nodes()

    visited_half_edges = set()
    planar_faces = []
    for u in H_embedding:
        for v in H_embedding[u]:
            if (u, v) not in visited_half_edges:
                face = H_embedding.traverse_face(u, v, mark_half_edges=visited_half_edges)
                planar_faces.append(face)

    # --- Theta-graph fallback -------------------------------------------
    # If contraction collapsed everything to just 2 branch vertices (a
    # "generalized theta graph": 2 hubs joined by >=2 parallel chains),
    # every face the embedding reports is the degenerate 2-vertex face
    # [hub_u, hub_v] -- there's no face with 3+ distinct vertices to use as
    # an outer polygon. Detect that here and synthesize a real outer face
    # by walking two of the original (uncontracted) chains as the two arcs
    # of the boundary. This is what previously caused these graphs to
    # render as a single straight line.
    theta_outer_face_original_verts = None
    theta_chosen_chain_ids = None
    if num_layout_nodes == 2 and all(len(f) <= 2 for f in planar_faces):
        hub_u, hub_v = list(layout_graph.nodes())
        theta_outer_face_original_verts, theta_chosen_chain_ids = _build_theta_outer_face(
            chains, hub_u, hub_v
        )

    if theta_outer_face_original_verts is not None:
        # Build the outer face directly on the ORIGINAL graph (not the
        # contracted H), since the polygon now includes interior chain
        # vertices as real, distinct outer-boundary points.
        #
        # Crucially, keep EVERY OTHER parallel chain (beyond the 2 used for
        # the boundary) in `chains` rather than discarding them -- with 3+
        # parallel chains between the same 2 hubs (e.g. several single-edge
        # subdivision chains all running straight from hub_u to hub_v),
        # dropping them here made every one of those interior vertices a
        # "free" node solved by Tutte's linear system with the SAME two
        # fixed neighbors (the hubs) and nothing else to differentiate
        # them -- so they all solved to the exact same point (the
        # hub_u/hub_v barycenter) and were drawn stacked on top of each
        # other. Keeping them as chains routes them through
        # expand_chain_positions instead, which explicitly bows parallel
        # chains apart so each gets a distinct position.
        remaining_chains = {
            cid: c for cid, c in chains.items() if cid not in theta_chosen_chain_ids
        }
        layout_graph = H  # stay in the contracted graph; only the boundary differs
        chains = remaining_chains
        num_layout_nodes = layout_graph.number_of_nodes()
        planar_faces = [theta_outer_face_original_verts]

    def try_outer_face(outer_verts, outer_radius):
        m = len(outer_verts)
        if m < 3:
            return None, None, None
        fixed_pos = {}
        for i, v in enumerate(outer_verts):
            angle = 2 * math.pi * i / m
            fixed_pos[v] = (outer_radius * math.cos(angle), outer_radius * math.sin(angle))

        nodes_list = list(layout_graph.nodes())
        free_nodes = [v for v in nodes_list if v not in fixed_pos]
        idx_map = {v: i for i, v in enumerate(free_nodes)}
        n_free = len(free_nodes)

        pos = dict(fixed_pos)
        unplaced = []

        if n_free > 0:
            A = np.zeros((n_free, n_free))
            bx = np.zeros(n_free)
            by = np.zeros(n_free)

            for v in free_nodes:
                i = idx_map[v]
                incident = list(layout_graph.edges(v))
                neighbor_list = [b if a == v else a for a, b in incident]
                deg = len(neighbor_list)
                A[i, i] = 1.0
                if deg == 0:
                    unplaced.append(v)
                    continue
                for u in neighbor_list:
                    if u in fixed_pos:
                        bx[i] += fixed_pos[u][0] / deg
                        by[i] += fixed_pos[u][1] / deg
                    else:
                        j = idx_map[u]
                        A[i, j] -= 1.0 / deg

            try:
                xs = np.linalg.solve(A, bx)
                ys = np.linalg.solve(A, by)
            except np.linalg.LinAlgError:
                return None, None, None

            for v in free_nodes:
                i = idx_map[v]
                pos[v] = (xs[i], ys[i])

        return pos, unplaced, outer_verts

    def polygon_area(pts):
        n = len(pts)
        a = 0.0
        for i in range(n):
            x0, y0 = pts[i]
            x1, y1 = pts[(i + 1) % n]
            a += x0 * y1 - x1 * y0
        return abs(a) / 2.0

    def min_face_area(pos, faces_):
        areas = []
        for face in faces_:
            pts = []
            for v in face:
                if v in pos:
                    pts.append(pos[v])
            if len(pts) >= 3:
                areas.append(polygon_area(pts))
        return min(areas) if areas else 0.0

    def min_pairwise_distance(pos):
        pts = np.array(list(pos.values()))
        if len(pts) < 2:
            return float("inf")
        diff = pts[:, None, :] - pts[None, :, :]
        dist = np.linalg.norm(diff, axis=2)
        np.fill_diagonal(dist, np.inf)
        return float(dist.min())

    best = None
    best_score = (-1, -1)
    best_free_count = 0
    for candidate in planar_faces:
        if len(candidate) < 3:
            continue
        trial_free_count = num_layout_nodes - len(candidate)
        trial_radius = compute_style(num_layout_nodes, trial_free_count)["outer_radius"]
        pos_try, unplaced_try, outer_try = try_outer_face(candidate, trial_radius)
        if pos_try is None:
            continue
        area_score = min_face_area(pos_try, faces)
        spread_score = min_pairwise_distance(pos_try)
        score = (1 if area_score > 1e-9 else 0, spread_score)
        if score > best_score:
            best_score = score
            best = (pos_try, unplaced_try, outer_try)
            best_free_count = trial_free_count

    if best is None:
        # Fall back to the largest available face. If even that has fewer
        # than 3 distinct vertices (shouldn't normally happen now that the
        # theta-graph case is handled above, but kept as a safety net for
        # any other degenerate topology), fall back further to a plain
        # spring layout rather than silently drawing a collapsed line.
        outer_face = max(planar_faces, key=len) if planar_faces else []
        if len(outer_face) < 3:
            pos = nx.spring_layout(G_comp, seed=42, k=0.9)
            warnings.append("Could not find a usable outer face for a proper planar "
                             "layout of this component -- drawn with a force-directed "
                             "layout instead.")
            return pos, [], [], warnings, True
        fallback_free_count = num_layout_nodes - len(outer_face)
        fallback_radius = compute_style(num_layout_nodes, fallback_free_count)["outer_radius"]
        pos, unplaced, outer_verts = try_outer_face(outer_face, fallback_radius)
    else:
        pos, unplaced, outer_verts = best

    pos = expand_chain_positions(pos, chains)

    if unplaced:
        outer_radius = compute_style(num_nodes, num_nodes - len(outer_verts))["outer_radius"]
        spread = 0.5 * outer_radius
        for k, v in enumerate(unplaced):
            pos[v] = (-outer_radius + k * spread, -outer_radius * 1.3)
        warnings.append(f"Vertex(es) {unplaced} appear in the input but have no edges to any other vertex.")

    return pos, unplaced, outer_verts, warnings, True


def visualize(G, faces, outpath):
    """Render graph for the given faces/G to outpath. outpath must be provided
    (caller controls where each input.txt's output goes).

    Handles disconnected graphs (e.g. category 6: union of several disjoint
    cycles) by laying out each connected component independently and then
    tiling the components side by side in a grid, so components never
    overlap each other -- a shared single Tutte solve across disconnected
    pieces has no coupling between them and otherwise collapses every
    component but one onto a single point.
    """
    num_nodes = G.number_of_nodes()
    components = list(nx.connected_components(G))

    all_warnings = []
    all_pos = {}
    all_unplaced = []
    any_non_planar = False

    if len(components) == 1:
        comp_nodes = components[0]
        G_comp = G.subgraph(comp_nodes).copy()
        pos, unplaced, outer_verts, warns, is_planar = layout_component(G_comp, faces)
        all_pos.update(pos)
        all_unplaced.extend(unplaced)
        all_warnings.extend(warns)
        any_non_planar = not is_planar
    else:
        # Multiple components: lay each out independently in its own local
        # coordinate frame, then translate each into a non-overlapping cell
        # of a roughly-square grid sized by that component's own bounding box.
        all_warnings.append(
            f"Input graph has {len(components)} disconnected components; "
            f"each is laid out independently and tiled below."
        )

        components = sorted(components, key=len, reverse=True)
        grid_cols = math.ceil(math.sqrt(len(components)))

        component_layouts = []
        for comp_nodes in components:
            G_comp = G.subgraph(comp_nodes).copy()
            pos, unplaced, outer_verts, warns, is_planar = layout_component(G_comp, faces)
            any_non_planar = any_non_planar or (not is_planar)
            all_warnings.extend(warns)
            component_layouts.append((pos, unplaced))

        # Compute each component's bounding box extent for grid cell sizing.
        extents = []
        for pos, unplaced in component_layouts:
            if not pos:
                extents.append(1.0)
                continue
            xs = [p[0] for p in pos.values()]
            ys = [p[1] for p in pos.values()]
            extents.append(max(max(xs) - min(xs), max(ys) - min(ys), 1.0))

        gap = 0.6 * (sum(extents) / len(extents))  # gap scales with typical component size
        cell_size = max(extents) + gap

        for idx, (pos, unplaced) in enumerate(component_layouts):
            row = idx // grid_cols
            col = idx % grid_cols
            xs = [p[0] for p in pos.values()]
            ys = [p[1] for p in pos.values()]
            cx = (max(xs) + min(xs)) / 2 if xs else 0.0
            cy = (max(ys) + min(ys)) / 2 if ys else 0.0

            offset_x = col * cell_size
            offset_y = -row * cell_size

            for node, (x, y) in pos.items():
                all_pos[node] = (x - cx + offset_x, y - cy + offset_y)
            all_unplaced.extend(unplaced)

    style = compute_style(num_nodes, num_nodes)

    plt.figure(figsize=style["figsize"])

    if any_non_planar:
        title = "Graph contains a NON-PLANAR component (drawn with force-directed layout for that part)"
    else:
        title = f"Planar graph  |  {G.number_of_nodes()} vertices, {G.number_of_edges()} edges"

    nx.draw_networkx_edges(G, all_pos, width=style["edge_width"], edge_color="#333333")
    node_artist = nx.draw_networkx_nodes(G, all_pos, node_size=style["node_size"], node_color="#4C72B0",
                                          edgecolors="black", linewidths=max(0.6, style["edge_width"] * 0.7))
    node_artist.set_zorder(2)

    label_offset = max(6, style["font_size"] * 0.6)
    for v, (x, y) in all_pos.items():
        color = "darkred" if v in all_unplaced else "black"
        plt.annotate(str(v), (x, y), xytext=(label_offset, label_offset), textcoords="offset points",
                     fontsize=style["font_size"], fontweight="bold", color=color, zorder=4)

    plt.title(title, fontsize=max(12, min(18, style["font_size"] * 0.9)))

    if all_warnings:
        warn_text = "\n".join(f"\u26a0 {w}" for w in all_warnings)
        plt.gcf().text(0.02, 0.02, warn_text, fontsize=11, color="darkred",
                        bbox=dict(boxstyle="round,pad=0.5", facecolor="#fff3f3", edgecolor="darkred"))

    plt.margins(0.18)
    plt.axis("off")
    plt.tight_layout()
    plt.savefig(outpath, dpi=150)
    plt.close()
    print(f"Saved to {outpath}")
    print(f"Vertices ({G.number_of_nodes()}):", sorted(G.nodes()))
    print(f"Edges ({G.number_of_edges()}):", sorted(G.edges()))
    if all_warnings:
        print("Warnings:")
        for w in all_warnings:
            print(" -", w)


def process_file(txt_path, out_path):
    """Parse one input .txt and render its graph to out_path.
    Returns True on success, False on failure (prints the error either way)."""
    try:
        with open(txt_path) as f:
            text = f.read()
        faces = parse_faces(text)
        G = build_graph(faces)
        os.makedirs(os.path.dirname(out_path), exist_ok=True)
        visualize(G, faces, out_path)
        return True
    except Exception as e:
        print(f"[ERROR] Failed on {txt_path}: {e}")
        return False


def process_all(input_root, output_root):
    """
    Walks input_root/<category>/*.txt (e.g. input/big, input/small, input/medium,
    or any other subfolders that exist) and generates a PNG for every .txt file
    found, writing it to output_root/<category>/<name>.png, mirroring the
    input folder structure. Works for any depth of subfolders, not just one level.
    """
    if not os.path.isdir(input_root):
        print(f"[ERROR] Input folder not found: {input_root}")
        return

    txt_files = []
    for dirpath, _dirnames, filenames in os.walk(input_root):
        for fn in filenames:
            if fn.lower().endswith(".txt"):
                txt_files.append(os.path.join(dirpath, fn))

    if not txt_files:
        print(f"[WARN] No .txt files found under {input_root}")
        return

    txt_files.sort()
    print(f"Found {len(txt_files)} input file(s) under {input_root}\n")

    n_ok, n_fail = 0, 0
    for txt_path in txt_files:
        rel = os.path.relpath(txt_path, input_root)          # e.g. big/input1.txt
        rel_png = os.path.splitext(rel)[0] + ".png"           # e.g. big/input1.png
        out_path = os.path.join(output_root, rel_png)

        print(f"--- Processing {rel} ---")
        ok = process_file(txt_path, out_path)
        n_ok += ok
        n_fail += not ok
        print()

    print(f"Done. {n_ok} succeeded, {n_fail} failed. Outputs in: {output_root}")


if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.abspath(__file__))

    # Allow overriding via command line: python graph_batch.py [input_dir] [output_dir]
    input_root = sys.argv[1] if len(sys.argv) > 1 else os.path.join(script_dir, "input")
    output_root = sys.argv[2] if len(sys.argv) > 2 else os.path.join(script_dir, "output-images")

    process_all(input_root, output_root)