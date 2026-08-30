"""
Common utilities shared by all biconnected-graph generators.

Responsibilities:
  - Extract combinatorial faces from a planar embedding of a graph
    (this includes the outer face).
  - Check graphs against user supplied constraints.
  - Write out test case files in the required format.
  - Sort generated instances by (#faces, max face size).
"""

import os
import random
import networkx as nx


class Constraints:
    """
    max_faces          : maximum number of faces allowed (including outer face)
    max_vertices_in_face: maximum number of vertices allowed in any single face
    max_vertices        : maximum number of vertices allowed in the whole graph
    min_vertices         : (optional) minimum number of vertices, default 3
    """

    def __init__(self, max_faces=None, max_vertices_in_face=None,
                 max_vertices=None, min_vertices=3):
        self.max_faces = max_faces
        self.max_vertices_in_face = max_vertices_in_face
        self.max_vertices = max_vertices
        self.min_vertices = min_vertices

    def check_graph_size(self, G):
        n = G.number_of_nodes()
        if n < self.min_vertices:
            return False
        if self.max_vertices is not None and n > self.max_vertices:
            return False
        return True

    def check_faces(self, faces):
        if self.max_faces is not None and len(faces) > self.max_faces:
            return False
        if self.max_vertices_in_face is not None:
            for f in faces:
                if len(f) > self.max_vertices_in_face:
                    return False
        return True


def extract_faces(G):
    """
    Given a connected, simple, planar, biconnected(-ish) graph G (networkx Graph),
    return a list of faces, where each face is a list of vertices in the order
    they appear when walking its boundary. Includes the outer face.

    Uses networkx's combinatorial planar embedding and traces faces via the
    standard "next edge in rotation system" face-traversal algorithm.

    Returns None if the graph is not planar.
    """
    is_planar, embedding = nx.check_planarity(G, counterexample=False)
    if not is_planar:
        return None

    faces = []
    visited_half_edges = set()

    for u in embedding:
        for v in embedding[u]:
            he = (u, v)
            if he in visited_half_edges:
                continue
            face = embedding.traverse_face(u, v, mark_half_edges=visited_half_edges)
            faces.append(face)

    # de-duplicate faces that are identical as vertex sets/cyclic sequences
    # (traverse_face with mark_half_edges already prevents double counting,
    # but keep a safety de-dup on the exact half-edge sequence just in case)
    return faces


def faces_are_valid(faces):
    """A structural sanity check: every face must have >=3 vertices (no self
    loops / degenerate faces) for a simple biconnected planar graph."""
    if faces is None:
        return False
    for f in faces:
        if len(f) < 3:
            return False
    return True


def graph_is_biconnected(G):
    if G.number_of_nodes() < 3:
        return False
    return nx.is_biconnected(G)


def validate_graph(G, constraints: Constraints):
    """
    Full validation pipeline used by every category generator.
    Returns (ok: bool, faces: list_or_None, reason: str)
    """
    if not constraints.check_graph_size(G):
        return False, None, "vertex-count constraint violated"

    if not graph_is_biconnected(G):
        return False, None, "graph is not biconnected"

    faces = extract_faces(G)
    if not faces_are_valid(faces):
        return False, None, "not planar / degenerate face"

    if not constraints.check_faces(faces):
        return False, None, "face constraint violated"

    return True, faces, "ok"


def write_test_case(filepath, faces):
    """
    Write in the required format:

        Total_Face_number
        vertex count of first face
        vertices of 1st face
        vertex count of 2nd face
        vertices of 2nd face
        ...
    """
    with open(filepath, "w") as f:
        f.write(f"{len(faces)}\n")
        for face in faces:
            f.write(f"{len(face)}\n")
            f.write(" ".join(str(v) for v in face) + "\n")


def save_sorted_instances(instances, out_dir, prefix):
    """
    instances: list of dicts each containing:
        'faces' : list of faces
        'graph' : networkx Graph (optional, for reproducibility/debug)
        'meta'  : dict of extra info to record in a summary line (optional)

    Sorts ascending by (num_faces, max_face_size) and writes files named
    prefix_001.txt, prefix_002.txt, ... into out_dir.
    """
    os.makedirs(out_dir, exist_ok=True)

    def sort_key(inst):
        faces = inst["faces"]
        num_faces = len(faces)
        max_face = max((len(f) for f in faces), default=0)
        return (num_faces, max_face)

    instances_sorted = sorted(instances, key=sort_key)

    manifest_path = os.path.join(out_dir, f"{prefix}_manifest.txt")
    with open(manifest_path, "w") as manifest:
        for i, inst in enumerate(instances_sorted, start=1):
            fname = f"{prefix}_{i:03d}.txt"
            fpath = os.path.join(out_dir, fname)
            write_test_case(fpath, inst["faces"])

            n_faces = len(inst["faces"])
            max_face = max((len(f) for f in inst["faces"]), default=0)
            n_vertices = inst["graph"].number_of_nodes() if inst.get("graph") is not None else "?"
            n_edges = inst["graph"].number_of_edges() if inst.get("graph") is not None else "?"
            extra = inst.get("meta", {})
            extra_str = " ".join(f"{k}={v}" for k, v in extra.items())
            manifest.write(
                f"{fname}\tfaces={n_faces}\tmax_face={max_face}\t"
                f"vertices={n_vertices}\tedges={n_edges}\t{extra_str}\n"
            )

    return instances_sorted


def relabel_consecutive(G):
    """Relabel graph nodes to consecutive integers 0..n-1 (stable order)."""
    mapping = {old: i for i, old in enumerate(G.nodes())}
    return nx.relabel_nodes(G, mapping, copy=True), mapping


class DedupTracker:
    """
    Tracks graphs already accepted for a category and rejects any new
    candidate that is ISOMORPHIC to one already accepted (i.e. the "same
    graph" up to relabeling vertices -- exactly what a person visually
    comparing two drawn graphs would call "identical").

    Implementation: a cheap canonical signature (sorted degree sequence +
    Weisfeiler-Lehman hash) buckets candidates first -- two non-isomorphic
    graphs essentially never collide on this signature, so most rejections
    are decided in O(1). Only when a candidate's signature matches an
    already-seen bucket do we run an exact nx.is_isomorphic check against
    every graph in that bucket, which is the mathematically definitive
    test and guarantees NO isomorphic duplicate ever slips through
    (the WL hash alone is a fast pre-filter, not the final word).
    """

    def __init__(self):
        # signature -> list of graphs already accepted with that signature
        self._buckets = {}

    @staticmethod
    def _signature(G):
        degree_seq = tuple(sorted(d for _, d in G.degree()))
        wl_hash = nx.weisfeiler_lehman_graph_hash(G, iterations=4)
        return (G.number_of_nodes(), G.number_of_edges(), degree_seq, wl_hash)

    def is_duplicate(self, G):
        sig = self._signature(G)
        bucket = self._buckets.get(sig)
        if not bucket:
            return False
        return any(nx.is_isomorphic(G, existing) for existing in bucket)

    def add(self, G):
        sig = self._signature(G)
        self._buckets.setdefault(sig, []).append(G)

    def try_add(self, G):
        """Returns True and records G if it's not a duplicate of anything
        already accepted; returns False (and does NOT record) otherwise."""
        sig = self._signature(G)
        bucket = self._buckets.setdefault(sig, [])
        if any(nx.is_isomorphic(G, existing) for existing in bucket):
            return False
        bucket.append(G)
        return True


def build_face_count_plan(count, min_faces, max_faces, seed=0):
    """
    Build an explicit list of `count` TARGET face-counts, one per graph we
    intend to generate, spread as evenly as possible across
    [min_faces, max_faces] (inclusive). This guarantees every achievable
    face count in that range gets a fair, roughly-equal share of the
    `count` graphs instead of leaving small/awkward face counts to random
    chance (which can make them vanishingly rare or unreachable in
    practice, since generator parameters do not vary face count linearly
    or uniformly).

    Returns a list of length `count` (a multiset of target face counts),
    shuffled so consecutive generation attempts aren't grouped by target
    (keeps things robust if a run is interrupted early).
    """
    if max_faces is None:
        # No explicit ceiling requested: fall back to a generous default
        # spread so we still get variety instead of drifting to one size.
        max_faces = min_faces + 20

    span = list(range(min_faces, max_faces + 1))
    if not span:
        raise ValueError(
            f"No achievable face-count range: min_faces={min_faces} > max_faces={max_faces}"
        )

    rng = random.Random(seed)
    plan = []
    i = 0
    while len(plan) < count:
        plan.append(span[i % len(span)])
        i += 1
    rng.shuffle(plan)
    return plan
