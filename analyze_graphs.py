import os
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
            return "C4_quadrilateral_cycle"
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
                return f"W{n}_wheel_{n}_spokes"
    
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
            return f"K{m}_{n}_complete_bipartite"
    
    # Diamond graph (K4 minus one edge)
    if num_vertices == 4 and num_edges == 5:
        degree_vals = list(degrees.values())
        if 2 in degree_vals and degree_vals.count(3) == 2:
            return "diamond_K4_minus_edge"
    
    # Bow-tie (two triangles sharing a vertex)
    if num_vertices == 5 and num_edges == 6:
        bridge_vertex = [v for v in vertices if degrees[v] == 4]
        if len(bridge_vertex) == 1:
            return "bowtie_two_triangles_sharing_vertex"
    
    # Octahedron (bipartite 3,3 graph - 8 triangular faces)
    if num_vertices == 6 and num_edges == 12 and num_faces == 8:
        if all(d == 4 for d in degrees.values()):
            return "octahedron_K3_3"
    
    # Planar subdivision patterns
    if num_vertices == 5 and num_edges == 9:
        return "K5_planar_subdivision"
    
    # Check for common patterns
    triangular_faces = sum(1 for f in faces if len(f) == 3)
    quad_faces = sum(1 for f in faces if len(f) == 4)
    
    if triangular_faces == num_faces and num_faces > 2:
        return f"triangulated_graph_{num_faces}_triangles"
    
    if quad_faces == num_faces:
        return f"quadrilateral_graph_{num_faces}_quads"
    
    # Generic description
    return f"graph_v{num_vertices}_e{num_edges}_f{num_faces}"

def main():
    input_dir = r"c:\Users\TAWKIR\Documents\GitHub\ThesisGraph\src\input"
    
    rename_suggestions = []
    
    for filename in sorted(os.listdir(input_dir)):
        if not filename.endswith('.txt'):
            continue
        
        filepath = os.path.join(input_dir, filename)
        
        try:
            faces, edges, vertices = read_graph_from_file(filepath)
            suggested_name = analyze_graph(faces, edges, vertices)
            
            # Only suggest rename if it's different and more descriptive
            base_name = filename[:-4]  # Remove .txt
            
            rename_suggestions.append({
                'original': filename,
                'suggested': suggested_name + '.txt',
                'vertices': len(vertices),
                'edges': len(edges),
                'faces': len(faces)
            })
            
            print(f"{filename:40s} -> {suggested_name}.txt")
            print(f"  V={len(vertices)}, E={len(edges)}, F={len(faces)}")
            
        except Exception as e:
            print(f"Error processing {filename}: {e}")
    
    print("\n" + "="*80)
    print("Rename commands:")
    print("="*80)
    
    for item in rename_suggestions:
        if item['original'] != item['suggested']:
            old_path = os.path.join(input_dir, item['original'])
            new_path = os.path.join(input_dir, item['suggested'])
            print(f'Rename-Item -Path "{old_path}" -NewName "{item["suggested"]}"')

if __name__ == "__main__":
    main()
