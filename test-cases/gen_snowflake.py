"""
Category 7: Snowflake graphs.

Stage 0 : triangle 0-1-2.
Stage s : look at every edge (u, v) of the CURRENT outer cycle. Place a new
          vertex w in the outer face, right beside that edge, and join it
          to both u and v. (3 outer edges -> 6 -> 12 -> 24 ...)

The planar embedding is built explicitly while growing the graph, so the
outer face is always the full outer cycle (size n) and every inner face is
a triangle -- exactly as drawn. This embedding is handed to the writer
(instance key "rotation"), instead of letting networkx pick one.

Sizes at stage s:  n = 3*2^s,  m = 2n-3,  faces = n-1 (incl. outer).
"""


def _faces_from_rotation(rot):
    """Trace faces of a rotation system (dict v -> cyclic neighbour list)."""
    pos = {v: {w: i for i, w in enumerate(nb)} for v, nb in rot.items()}
    seen, faces = set(), []
    for u in rot:
        for v in rot[u]:
            if (u, v) in seen:
                continue
            face, a, b = [], u, v
            while (a, b) not in seen:
                seen.add((a, b))
                face.append(a)
                nb = rot[b]
                a, b = b, nb[(pos[b][a] + 1) % len(nb)]
            faces.append(face)
    return faces


def _stages():
    """Yield (stage, graph_edges, rotation_ccw) for stage 0, 1, 2, ..."""
    rot = {0: [1, 2], 1: [2, 0], 2: [0, 1]}   # triangle 0,1,2 counter-clockwise
    edges = [(0, 1), (1, 2), (2, 0)]
    outer = [0, 1, 2]                          # outer cycle, counter-clockwise
    stage = 0
    yield stage, list(edges), {v: list(nb) for v, nb in rot.items()}

    while True:
        stage += 1
        k = len(outer)
        nxt = len(rot)
        apex = []
        for i in range(k):
            u, v = outer[i], outer[(i + 1) % k]
            w = nxt + i
            apex.append(w)
            edges += [(u, w), (v, w)]
            rot[w] = [u, v]
        # at each old outer vertex, the two new apexes go into the outer
        # angle, right after its previous outer neighbour
        for i in range(k):
            u, prev = outer[i], outer[i - 1]
            nb = rot[u]
            j = nb.index(prev) + 1
            nb[j:j] = [apex[i - 1], apex[i]]
        new_outer = []
        for i in range(k):
            new_outer += [outer[i], apex[i]]
        outer = new_outer
        yield stage, list(edges), {v: list(nb) for v, nb in rot.items()}


def generate_category(count, constraints, seed=None):
    """Deterministic: one graph per stage. `seed` is unused."""
    import networkx as nx

    instances = []
    for stage, edges, rot in _stages():
        if len(instances) >= count:
            break

        n = len(rot)
        if n < constraints.min_vertices:
            continue
        if constraints.max_vertices is not None and n > constraints.max_vertices:
            break
        if constraints.max_faces is not None and n - 1 > constraints.max_faces:
            break

        G = nx.Graph()
        G.add_nodes_from(range(n))
        G.add_edges_from(edges)

        faces = _faces_from_rotation(rot)
        # sanity: genus 0  (Euler: V - E + F = 2)
        assert n - G.number_of_edges() + len(faces) == 2, "embedding is not planar"
        if not constraints.check_faces(faces):
            break

        # writer expects clockwise order; ours is counter-clockwise
        rotation_cw = {v: list(reversed(nb)) for v, nb in rot.items()}
        instances.append({
            "graph": G,
            "faces": faces,
            "rotation": rotation_cw,
            "meta": {"stage": stage, "n": n},
        })

    if len(instances) < count:
        print(f"  [warning] snowflake: only {len(instances)} stages fit the "
              f"constraints (requested {count}). Raise max_faces / "
              f"max_vertices_in_face / max_vertices to get more.")
    return instances