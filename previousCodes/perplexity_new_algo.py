from itertools import combinations
from collections import defaultdict

def find_risky_vertices(faces):
    """Find all sets of shared vertices between faces, to detect risky regions."""
    vertex_faces = defaultdict(set)
    for idx, face in enumerate(faces):
        for v in face:
            vertex_faces[v].add(idx)
    edge_shared = defaultdict(set)
    for vfs in vertex_faces.values():
        if len(vfs) > 1:
            for pair in combinations(vfs, 2):
                edge_shared[frozenset(pair)] |= vfs
    risky_sets = []
    for idxs in edge_shared.values():
        if len(idxs) > 1:
            shared_vertices = set()
            for idx in idxs:
                shared_vertices.update(faces[idx])
            risky_sets.append(shared_vertices)
    return risky_sets

def canonical_edge(u, v):
    """Return a sorted tuple for a chord (for set operations and lexicographic comparison)."""
    return tuple(sorted((u, v)))

def triangulate_biconnected(
    faces,
    face_adj,
    generate_triangulations,
    face_order=None
):
    """
    Enumerate all valid triangulations for a biconnected planar graph.
    - faces: list of lists (each face as a list of vertices in order)
    - face_adj: adjacency list of faces (unused here)
    - generate_triangulations: function, e.g., from paper algorithm, for a face. 
      Should return all unique triangulations for a face, each as a list of (i, j) pairs, 0 ≤ i < j < len(face).
    - face_order: optional order to process faces
    """
    risky_sets = find_risky_vertices(faces)
    results = []

    def recurse(
        face_idx,
        established_chords,
        established_risky_chords,
        min_risky_edge_by_face
    ):
        if face_idx >= len(faces):
            results.append(frozenset(established_chords))
            return

        current_face = faces[face_idx]
        shared_risk = None
        for rset in risky_sets:
            if set(current_face).issubset(rset):
                shared_risk = rset
                break

        valid_roots = range(len(current_face))  # Allow all roots in simple case (can be refined)

        for root_idx in valid_roots:
            # For each triangulation for this face rooted at 'root_idx'
            for triang in generate_triangulations(len(current_face), root=root_idx):
                triang_chords = [canonical_edge(current_face[e[0]], current_face[e[1]]) for e in triang]

                # Prune if triangulation introduces a multi-edge (repeats)
                if any(c in established_chords for c in triang_chords):
                    continue

                risky_face_chords = []
                if shared_risk:
                    for c in triang_chords:
                        if set(c).issubset(shared_risk):
                            risky_face_chords.append(c)

                # Apply symmetry-breaking: the MIN risky chord for this face
                # must be greater than the MIN for all previous faces.
                # Only allow the unique ordering.
                if risky_face_chords:
                    min_this = min(risky_face_chords)
                    previous_mins = [v for k,v in min_risky_edge_by_face.items() if v is not None]
                    if previous_mins:
                        max_prev = max(previous_mins)
                        # Only allow strict increase
                        if min_this <= max_prev:
                            continue

                # Continue branching
                next_established_risky = established_risky_chords | set(risky_face_chords)
                next_min = dict(min_risky_edge_by_face)
                if risky_face_chords:
                    next_min[face_idx] = min(risky_face_chords)
                else:
                    next_min[face_idx] = None
                recurse(
                    face_idx + 1,
                    established_chords | set(triang_chords),
                    next_established_risky,
                    next_min
                )

    # Start recursion.
    recurse(
        face_idx=0,
        established_chords=set(),
        established_risky_chords=set(),
        min_risky_edge_by_face={}
    )
    # Return unique triangulations as sets of chords
    return list(results)

# Dummy triangulation generator for a pentagon, adjust for your face triangulator
def pentagon_triangulations(n, root=0):
    # Returns a list of triangulations, each as a list of (i, j)
    # n == 5, indices 0..4
    # All 5 triangulations rooted anywhere
    pentagon_idx = list(range(n))
    triangs = [
        [(0, 2), (0, 3), (2, 4)],   # root 0
        [(1, 3), (1, 4), (3, 0)],   # root 1
        [(2, 4), (2, 0), (4, 1)],   # root 2 (wrap mod 5)
        [(3, 0), (3, 1), (0, 2)],
        [(4, 1), (4, 2), (1, 3)]
    ]
    # Permute indices so root appears at index 0?
    offset = (0 - root) % n
    def rotate_e(e): return ((e[0]+offset)%n, (e[1]+offset)%n)
    return [list(map(rotate_e, t)) for t in triangs]

# Example usage for a biconnected graph with two shared pentagonal faces
faces = [
    [1,2,3,4,5],  # face F1
    [1,2,3,4,5]   # face F2 (shared, for testing risky region)
]
face_adj = {
    0: [1],
    1: [0]
}

results = triangulate_biconnected(
    faces=faces,
    face_adj=face_adj,
    generate_triangulations=pentagon_triangulations
)

print(f"Total valid triangulations: {len(results)}")
for idx, triang in enumerate(results):
    print(f"Triangulation #{idx+1}: {sorted(triang)}")
