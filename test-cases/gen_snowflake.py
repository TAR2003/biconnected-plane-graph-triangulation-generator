"""Generate fully triangulated snowflake transformations.

The base graph is a biconnected cycle with a fixed embedding. Every base edge
gets one unique apex vertex on exactly one incident face; the remaining polygon
in each face is fan-triangulated from one of those apices. Thus every output
face, including the outer face, is a triangle.
"""

import random

import networkx as nx

from common import Constraints, DedupTracker, build_face_count_plan, relabel_consecutive, validate_graph


def _base_cycle(n):
    base = nx.cycle_graph(n)
    planar, embedding = nx.check_planarity(base)
    if not planar:  # pragma: no cover
        raise ValueError("base cycle is not planar")
    faces = []
    seen = set()
    for u in embedding:
        for v in embedding[u]:
            if (u, v) not in seen:
                faces.append(embedding.traverse_face(u, v, mark_half_edges=seen))
    return base, faces


def _snowflake_cycle(n, rng):
    graph, faces = _base_cycle(n)
    incident_faces = {}
    for face_index, boundary in enumerate(faces):
        for u, v in zip(boundary, boundary[1:] + boundary[:1]):
            incident_faces.setdefault(frozenset((u, v)), []).append(face_index)

    # Choose the face containing each edge apex, while covering both faces.
    face_for_edge = {}
    for face_index, boundary in enumerate(faces):
        edge = frozenset((boundary[0], boundary[1]))
        face_for_edge.setdefault(edge, face_index)
    for edge, incident in incident_faces.items():
        face_for_edge.setdefault(edge, rng.choice(incident))
    represented = set(face_for_edge.values())
    for face_index, boundary in enumerate(faces):
        if face_index not in represented:
            edge = rng.choice([
                frozenset((u, v))
                for u, v in zip(boundary, boundary[1:] + boundary[:1])
            ])
            face_for_edge[edge] = face_index
            represented = set(face_for_edge.values())

    apex_for_edge = {}
    next_vertex = n
    for edge in face_for_edge:
        u, v = tuple(edge)
        apex_for_edge[edge] = next_vertex
        graph.add_edges_from(((u, next_vertex), (v, next_vertex)))
        next_vertex += 1

    for face_index, boundary in enumerate(faces):
        polygon = []
        for u, v in zip(boundary, boundary[1:] + boundary[:1]):
            polygon.append(u)
            edge = frozenset((u, v))
            if face_for_edge[edge] == face_index:
                polygon.append(apex_for_edge[edge])

        apex_position = next(i for i, vertex in enumerate(polygon) if vertex >= n)
        apex = polygon[apex_position]
        for offset in range(2, len(polygon) - 1):
            graph.add_edge(apex, polygon[(apex_position + offset) % len(polygon)])
    return graph


def _target_to_base_size(target_faces):
    # The transformed n-cycle has 2n vertices, hence 2(2n)-4 = 4n-4 faces.
    if target_faces < 8 or target_faces % 4:
        return None
    return target_faces // 4 + 1


def generate_one_targeted(target_faces, constraints: Constraints, rng, dedup, max_attempts=500):
    n = _target_to_base_size(target_faces)
    if n is None:
        return None, None, None
    for _ in range(max_attempts):
        graph, _ = relabel_consecutive(_snowflake_cycle(n, rng))
        ok, faces, _ = validate_graph(graph, constraints)
        if ok and len(faces) == target_faces and all(len(face) == 3 for face in faces):
            if dedup.try_add(graph):
                return graph, faces, {"base_vertices": n, "target_faces": target_faces}
    return None, None, None


def generate_category(count, constraints: Constraints, seed=0):
    """Generate distinct, fully triangulated snowflake transformations."""
    min_faces = 8
    max_faces = constraints.max_faces if constraints.max_faces is not None else min_faces + 20
    if constraints.max_vertices is not None:
        max_faces = min(max_faces, constraints.max_vertices - 4)
    if constraints.max_vertices_in_face is not None and constraints.max_vertices_in_face < 3:
        raise RuntimeError("max_vertices_in_face must be at least 3 for snowflake triangulations")

    targets = [faces for faces in range(min_faces, max_faces + 1) if faces % 4 == 0]
    if not targets:
        raise RuntimeError("no feasible snowflake target: valid face counts are 8, 12, 16, ...")

    rng = random.Random(seed)
    plan = build_face_count_plan(count, min(targets), max(targets), seed=seed)
    plan = [target for target in plan if target in targets]
    while len(plan) < count:
        plan.append(rng.choice(targets))

    instances = []
    dedup = DedupTracker()
    exhausted = set()
    for target_faces in plan:
        if target_faces in exhausted:
            continue
        graph, faces, meta = generate_one_targeted(target_faces, constraints, rng, dedup)
        if graph is None:
            exhausted.add(target_faces)
            continue
        instances.append({"graph": graph, "faces": faces, "meta": meta})
        if len(instances) == count:
            break

    if len(instances) < count:
        print(f"  [warning] Snowflake category: generated {len(instances)} of {count} distinct graphs")
    return instances