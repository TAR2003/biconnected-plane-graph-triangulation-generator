import networkx as nx
import matplotlib.pyplot as plt
from itertools import combinations

def fan_triangulation(n):
    """Root: all chords from v1 = 0 to 2..n-2"""
    return frozenset((0, i) for i in range(2, n))

def boundary_edges(n):
    return {(i, (i + 1) % n) for i in range(n)}

def all_edges(tri, n):
    return set(tri).union(boundary_edges(n))

def triangles(tri, n):
    """Return all triangles from a triangulation."""
    edges = all_edges(tri, n)
    tris = set()
    for u, v, w in combinations(range(n), 3):
        s = {tuple(sorted((u, v))), tuple(sorted((v, w))), tuple(sorted((u, w)))}
        if s.issubset(edges):
            tris.add(tuple(sorted([u, v, w])))
    return tris

def find_blocking_chords(tri):
    return [e for e in tri if 0 not in e]

def leftmost_blocking_chord(tri):
    chords = find_blocking_chords(tri)
    if not chords:
        return None
    return max(chords, key=lambda e: max(e))

def flip_chords(tri, n):
    """Generate all valid flips (ignore generating checks here)."""
    T = triangles(tri, n)
    result = []
    for t1 in T:
        for t2 in T:
            if t1 == t2: continue
            shared = set(t1) & set(t2)
            if len(shared) == 2:
                quad = list(set(t1 + t2))
                if len(quad) != 4:
                    continue
                u, v = tuple(sorted(shared))
                if (u, v) not in tri:
                    continue
                new_diag = tuple(sorted(set(quad) - {u, v}))
                if new_diag in tri:
                    continue
                new_tri = set(tri)
                new_tri.remove((u, v))
                new_tri.add(new_diag)
                result.append(frozenset(new_tri))
    return result

def is_generating_chord(flipped, new_triangulation):
    """Check if flip is a valid generating chord (from v1 and j ≥ b′)."""
    if 0 not in flipped:
        return False
    j = max(flipped)
    lmb = leftmost_blocking_chord(new_triangulation)
    if lmb is None:
        return True  # root has no blocking chord
    b_prime = max(lmb)
    return j >= b_prime

def build_tree(n):
    root = fan_triangulation(n)
    tree = nx.DiGraph()
    visited = set()

    def dfs(current):
        visited.add(current)
        for child in flip_chords(current, n):
            diff = list(current - child)
            if len(diff) != 1:
                continue
            flipped = diff[0]
            if is_generating_chord(flipped, child):
                tree.add_edge(current, child)
                if child not in visited:
                    dfs(child)

    dfs(root)
    return tree

def label_triangulation(tri):
    return "\n".join(f"{e}" for e in sorted(tri))

def visualize(tree):
    pos = nx.spring_layout(tree, seed=42)
    labels = {node: label_triangulation(node) for node in tree.nodes}
    plt.figure(figsize=(16, 10))
    nx.draw(tree, pos, with_labels=True, labels=labels, node_size=800, font_size=8, arrows=True)
    plt.title("Genealogical Tree of Triangulations T(C)", fontsize=14)
    plt.show()

# Run for n = 6
n = 6
tree = build_tree(n)
print(f"Generated {len(tree.nodes)} triangulations for C_{n}")
visualize(tree)
