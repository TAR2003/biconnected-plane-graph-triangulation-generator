import os
import shutil
from collections import defaultdict

def read_graph_from_file(filepath):
    """Read graph structure from file and return edges and vertices."""
    with open(filepath, 'r') as f:
        lines = [line.strip() for line in f if line.strip()]
    
    num_faces = int(lines[0])
    faces = []
    idx = 1
    
    for _ in range(num_faces):
        num_vertices = int(lines[idx])
        vertices = list(map(int, lines[idx + 1].split()))
        faces.append(vertices)
        idx += 2
    
    # Build edge list and adjacency info
    edges = set()
    vertices = set()
    for face in faces:
        vertices.update(face)
        for i in range(len(face)):
            v1, v2 = face[i], face[(i + 1) % len(face)]
            edges.add(tuple(sorted([v1, v2])))
    
    return faces, edges, vertices

def analyze_graph(faces, edges, vertices):
    """Analyze graph structure and suggest a name."""
    num_vertices = len(vertices)
    num_edges = len(edges)
    num_faces = len(faces)
    
    # Build adjacency list
    adj = defaultdict(set)
    for v1, v2 in edges:
        adj[v1].add(v2)
        adj[v2].add(v1)
    
    degrees = {v: len(adj[v]) for v in vertices}
    degree_sequence = sorted(degrees.values(), reverse=True)
    
    # Check for specific graph patterns
    
    # Single triangle
    if num_faces == 2 and num_vertices == 3 and num_edges == 3:
        return "K3_triangle"
    
    # K4 (complete graph on 4 vertices)
    if num_vertices == 4 and num_edges == 6 and all(d == 3 for d in degrees.values()):
        return "K4_tetrahedron"
    
    # Cycle graphs
    if all(d == 2 for d in degrees.values()) and num_edges == num_vertices:
        if num_vertices == 3:
            return "C3_triangle_cycle"
        elif num_vertices == 4:
            return "C4_square_cycle"
        elif num_vertices == 5:
            return "C5_pentagon_cycle"
        elif num_vertices == 6:
            return "C6_hexagon_cycle"
        elif num_vertices == 7:
            return "C7_heptagon_cycle"
        elif num_vertices == 8:
            return "C8_octagon_cycle"
        elif num_vertices == 9:
            return "C9_nonagon_cycle"
        else:
            return f"C{num_vertices}_cycle"
    
    # Wheel graphs (hub vertex connected to all cycle vertices)
    hub_candidates = [v for v in vertices if degrees[v] >= num_vertices - 1]
    if hub_candidates and len(hub_candidates) == 1:
        hub = hub_candidates[0]
        rim_vertices = vertices - {hub}
        if degrees[hub] == len(rim_vertices):
            rim_degrees = [degrees[v] for v in rim_vertices]
            if all(d == 3 for d in rim_degrees):  # Connected to hub + 2 neighbors
                n = len(rim_vertices)
                return f"W{n}_wheel"
    
    # Bipartite K_{m,n}
    # Try to find bipartition
    colored = {}
    is_bipartite = True
    for start in vertices:
        if start in colored:
            continue
        queue = [start]
        colored[start] = 0
        while queue and is_bipartite:
            v = queue.pop(0)
            for u in adj[v]:
                if u not in colored:
                    colored[u] = 1 - colored[v]
                    queue.append(u)
                elif colored[u] == colored[v]:
                    is_bipartite = False
                    break
    
    if is_bipartite:
        part0 = {v for v in vertices if colored.get(v) == 0}
        part1 = {v for v in vertices if colored.get(v) == 1}
        
        # Check if it's complete bipartite
        is_complete_bipartite = True
        for v in part0:
            if adj[v] != part1:
                is_complete_bipartite = False
                break
        
        if is_complete_bipartite:
            m, n = len(part0), len(part1)
            if m > n:
                m, n = n, m
            return f"K{m}_{n}_bipartite"
    
    # Diamond graph (K4 minus one edge)
    if num_vertices == 4 and num_edges == 5:
        degree_vals = list(degrees.values())
        if 2 in degree_vals and degree_vals.count(3) == 2:
            return "diamond"
    
    # Bow-tie (two triangles sharing a vertex)
    if num_vertices == 5 and num_edges == 6:
        bridge_vertex = [v for v in vertices if degrees[v] == 4]
        if len(bridge_vertex) == 1:
            return "bowtie"
    
    # Octahedron (actually it's bipartite 3,3 - check the structure)
    if num_vertices == 6 and num_edges == 12 and num_faces == 8:
        if all(d == 4 for d in degrees.values()):
            # This is K3,3 which is the octahedron graph
            return "octahedron"
    
    # Check if it's actually octahedron based on structure (8 triangles)
    if num_vertices == 8 and num_edges == 16 and num_faces == 8:
        triangular_faces = sum(1 for f in faces if len(f) == 3)
        if triangular_faces == 8:
            # Could be two tetrahedra - check connectivity
            return "octahedron_dual_tetrahedra"
    
    # Check for common patterns
    triangular_faces = sum(1 for f in faces if len(f) == 3)
    quad_faces = sum(1 for f in faces if len(f) == 4)
    
    if triangular_faces == num_faces and num_faces > 2:
        return f"triangulated_{num_faces}tri"
    
    if quad_faces == num_faces and num_faces > 1:
        return f"quadrilateral_{num_faces}quad"
    
    # Generic description
    return f"graph_V{num_vertices}E{num_edges}F{num_faces}"

def main():
    input_dir = r"c:\Users\TAWKIR\Documents\GitHub\ThesisGraph\src\input"
    
    # First pass: collect all rename suggestions
    rename_map = {}
    name_counts = defaultdict(int)
    
    for filename in sorted(os.listdir(input_dir)):
        if not filename.endswith('.txt'):
            continue
        
        filepath = os.path.join(input_dir, filename)
        
        try:
            faces, edges, vertices = read_graph_from_file(filepath)
            suggested_name = analyze_graph(faces, edges, vertices)
            
            rename_map[filename] = suggested_name
            name_counts[suggested_name] += 1
            
        except Exception as e:
            print(f"Error processing {filename}: {e}")
    
    # Second pass: handle duplicates by adding numbers
    final_renames = {}
    name_usage = defaultdict(int)
    
    for filename in sorted(rename_map.keys()):
        base_name = rename_map[filename]
        
        if name_counts[base_name] > 1:
            name_usage[base_name] += 1
            new_name = f"{base_name}_{name_usage[base_name]}.txt"
        else:
            new_name = f"{base_name}.txt"
        
        final_renames[filename] = new_name
    
    # Print analysis and generate rename commands
    print("Graph Analysis Results:")
    print("=" * 80)
    
    for filename in sorted(final_renames.keys()):
        if filename != final_renames[filename]:
            print(f"{filename:45s} -> {final_renames[filename]}")
    
    print("\n" + "=" * 80)
    print("PowerShell Rename Commands:")
    print("=" * 80)
    
    for filename in sorted(final_renames.keys()):
        if filename != final_renames[filename]:
            old_path = os.path.join(input_dir, filename)
            new_name = final_renames[filename]
            print(f'Rename-Item -Path "{old_path}" -NewName "{new_name}"')

if __name__ == "__main__":
    main()
