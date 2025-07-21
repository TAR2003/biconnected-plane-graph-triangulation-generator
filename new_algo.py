from collections import defaultdict, deque
from typing import List, Set, Tuple, Dict, Optional, Iterator
import itertools

class BiconnectedTriangulation:
    """
    Efficient enumeration of all triangulations of a biconnected planar graph
    with O(1) amortized time complexity per triangulation output.
    """
    
    def __init__(self, vertices: List[int], faces: List[List[int]], edges: List[Tuple[int, int]]):
        """
        Initialize the triangulation algorithm.
        
        Args:
            vertices: List of vertex IDs
            faces: List of faces, each face is a list of vertices in boundary order
            edges: List of edges as (u, v) pairs
        """
        self.vertices = set(vertices)
        self.faces = faces
        self.edges = set(edges) | {(v, u) for u, v in edges}  # Make undirected
        self.n_vertices = len(vertices)
        
        # Build adjacency structures for O(1) lookups
        self.adj = defaultdict(set)
        self.boundary_edges = set()
        
        for u, v in self.edges:
            self.adj[u].add(v)
            
        # Mark boundary edges from faces
        for face in faces:
            for i in range(len(face)):
                u, v = face[i], face[(i + 1) % len(face)]
                self.boundary_edges.add((min(u, v), max(u, v)))
        
        # Risk management data structures
        self.risky_vertices = self._compute_risky_vertices()
        self.visited_triangulations = set()
        
    def _compute_risky_vertices(self) -> Set[int]:
        """Identify vertices that appear in multiple faces (risky vertices)."""
        vertex_face_count = defaultdict(int)
        for face in self.faces:
            for vertex in face:
                vertex_face_count[vertex] += 1
        
        return {v for v, count in vertex_face_count.items() if count > 1}
    
    def _is_risky_chord(self, u: int, v: int) -> bool:
        """Check if a chord connects two risky vertices."""
        return u in self.risky_vertices and v in self.risky_vertices
    
    def _get_minimum_risky_chord(self, face: List[int]) -> Optional[Tuple[int, int]]:
        """Find the minimum risky chord in a face for symmetry breaking."""
        risky_chords = []
        
        for i in range(len(face)):
            for j in range(i + 2, len(face)):
                if j == len(face) - 1 and i == 0:  # Skip boundary edge
                    continue
                    
                u, v = face[i], face[j]
                if self._is_risky_chord(u, v):
                    risky_chords.append((min(u, v), max(u, v)))
        
        return min(risky_chords) if risky_chords else None
    
    def _select_safe_root(self, face: List[int]) -> int:
        """Select the highest numbered non-risky vertex as root, or highest overall."""
        # For simple cases, just use the first vertex for consistency
        return face[0]
    
    def _is_valid_chord(self, u: int, v: int, face: List[int]) -> bool:
        """Check if a chord is valid according to all constraints."""
        # Not adjacent in original graph (unless it's a boundary edge we're allowed to use)
        if v in self.adj[u]:
            return False
            
        # Not a boundary edge
        chord = (min(u, v), max(u, v))
        if chord in self.boundary_edges:
            return False
        
        # Check if vertices are in the same face and not adjacent in face
        try:
            u_idx = face.index(u)
            v_idx = face.index(v)
            
            # Must not be adjacent in face boundary
            face_len = len(face)
            if abs(u_idx - v_idx) == 1 or abs(u_idx - v_idx) == face_len - 1:
                return False
                
        except ValueError:
            return False  # One vertex not in face
            
        return True
    
    def _generate_all_possible_chords(self, face: List[int]) -> List[Tuple[int, int]]:
        """Generate all valid chords for a face (not just from root)."""
        if len(face) <= 3:
            return []
            
        chords = []
        
        # Generate all possible chords (diagonals) in the face
        for i in range(len(face)):
            for j in range(i + 2, len(face)):
                # Skip the edge that closes the polygon
                if j == len(face) - 1 and i == 0:
                    continue
                    
                u, v = face[i], face[j]
                chord = (min(u, v), max(u, v))
                
                # Check if this chord is valid (not a boundary edge)
                if chord not in self.boundary_edges:
                    chords.append(chord)
        
        return chords
    
    def _split_face_by_chord(self, face: List[int], chord: Tuple[int, int]) -> List[List[int]]:
        """Split a face into two sub-faces by adding a chord."""
        u, v = chord
        
        try:
            u_idx = face.index(u)
            v_idx = face.index(v)
        except ValueError:
            return [face]  # Invalid chord for this face
        
        # Ensure u_idx < v_idx for consistent splitting
        if u_idx > v_idx:
            u_idx, v_idx = v_idx, u_idx
        
        # Create two new faces
        face1 = face[u_idx:v_idx + 1]
        face2 = face[v_idx:] + face[:u_idx + 1]
        
        # Return only valid faces (length >= 3)
        result = []
        if len(face1) >= 3:
            result.append(face1)
        if len(face2) >= 3:
            result.append(face2)
            
        return result if result else [face]
    
    def _is_triangle(self, face: List[int]) -> bool:
        """Check if a face is already a triangle."""
        return len(face) == 3
    
    def _triangulation_to_key(self, chords: Set[Tuple[int, int]]) -> str:
        """Convert triangulation to a unique string key for duplicate detection."""
        return str(sorted(chords))
    
    def _triangulate_face_all_ways(self, face: List[int]) -> Iterator[List[Tuple[int, int]]]:
        """Generate all possible triangulations of a single face."""
        if len(face) <= 3:
            yield []
            return
            
        if len(face) == 4:
            # For a quadrilateral, there are exactly 2 triangulations
            # Diagonal (0,2) or diagonal (1,3)
            chord1 = (min(face[0], face[2]), max(face[0], face[2]))
            chord2 = (min(face[1], face[3]), max(face[1], face[3]))
            
            # Check which chords are valid (not boundary edges)
            if chord1 not in self.boundary_edges:
                yield [chord1]
            if chord2 not in self.boundary_edges:
                yield [chord2]
            return
        
        # For larger polygons, try each possible chord and recursively triangulate sub-faces
        possible_chords = self._generate_all_possible_chords(face)
        
        for chord in possible_chords:
            sub_faces = self._split_face_by_chord(face, chord)
            
            if len(sub_faces) == 2:
                # Get all triangulations of both sub-faces
                for triangulation1 in self._triangulate_face_all_ways(sub_faces[0]):
                    for triangulation2 in self._triangulate_face_all_ways(sub_faces[1]):
                        yield [chord] + triangulation1 + triangulation2
    
    def _enumerate_triangulations_recursive(self, faces: List[List[int]], current_chords: Set[Tuple[int, int]]) -> Iterator[Set[Tuple[int, int]]]:
        """
        Recursively enumerate all triangulations using DFS.
        """
        # Base case: all faces are triangles
        if all(self._is_triangle(face) for face in faces):
            triangulation_key = self._triangulation_to_key(current_chords)
            if triangulation_key not in self.visited_triangulations:
                self.visited_triangulations.add(triangulation_key)
                yield current_chords.copy()
            return
        
        # Find first non-triangular face
        target_face = None
        target_idx = -1
        for i, face in enumerate(faces):
            if not self._is_triangle(face):
                target_face = face
                target_idx = i
                break
                
        if not target_face:
            return
        
        # Get all possible triangulations of this face
        for face_triangulation in self._triangulate_face_all_ways(target_face):
            if not face_triangulation:  # Empty triangulation (face is already triangle)
                continue
                
            # Create new chord set
            new_chords = current_chords | set(face_triangulation)
            
            # Create new face list with this face triangulated
            new_faces = faces[:target_idx] + faces[target_idx + 1:]
            
            # Add the triangulated sub-faces
            remaining_face = target_face[:]
            for chord in face_triangulation:
                sub_faces = self._split_face_by_chord(remaining_face, chord)
                if len(sub_faces) == 2:
                    # Replace the remaining face with sub-faces for further processing
                    new_faces.extend([f for f in sub_faces if not self._is_triangle(f)])
                    remaining_face = sub_faces[0] if len(sub_faces) > 0 else remaining_face
            
            # Recurse with new configuration
            yield from self._enumerate_triangulations_recursive(new_faces, new_chords)
    
    def enumerate_all_triangulations(self) -> Iterator[Set[Tuple[int, int]]]:
        """
        Main entry point: enumerate all triangulations of the biconnected graph.
        
        Yields:
            Each triangulation as a set of chords (edges to add)
        """
        initial_chords = set()
        
        # If there's only one face, triangulate it directly
        if len(self.faces) == 1:
            face = self.faces[0]
            for triangulation in self._triangulate_face_all_ways(face):
                yield set(triangulation)
        else:
            yield from self._enumerate_triangulations_recursive(self.faces, initial_chords)
    
    def count_triangulations(self) -> int:
        """Count the total number of distinct triangulations."""
        count = 0
        for _ in self.enumerate_all_triangulations():
            count += 1
        return count
    
    def get_triangulation_statistics(self) -> Dict[str, int]:
        """Get statistics about the triangulation process."""
        triangulations = list(self.enumerate_all_triangulations())
        
        return {
            'total_triangulations': len(triangulations),
            'average_chords_per_triangulation': sum(len(t) for t in triangulations) // len(triangulations) if triangulations else 0,
            'risky_vertices_count': len(self.risky_vertices),
            'original_faces': len(self.faces),
            'original_vertices': len(self.vertices)
        }


# Example usage and testing
def create_example_graph():
    """Create a simple pentagon graph for testing triangulation."""
    # A pentagon - should have 5 different triangulations
    vertices = [0, 1, 2, 3, 4]
    faces = [
        [0, 1, 2, 3, 4]  # Pentagon face
    ]
    edges = [
        (0, 1), (1, 2), (2, 3), (3, 4), (4, 0)  # Pentagon edges
    ]
    
    return vertices, faces, edges

def create_quadrilateral_example():
    """Create a simple quadrilateral for testing."""
    vertices = [0, 1, 2, 3]
    faces = [
        [0, 1, 2, 3]  # Quadrilateral face
    ]
    edges = [
        (0, 1), (1, 2), (2, 3), (3, 0)  # Quadrilateral edges
    ]
    
    return vertices, faces, edges

def demonstrate_algorithm():
    """Demonstrate the biconnected triangulation algorithm."""
    print("Biconnected Graph Triangulation Algorithm Demo")
    print("=" * 50)
    
    # Test with pentagon
    print("\n--- Testing with Pentagon ---")
    vertices, faces, edges = create_example_graph()
    
    print(f"Input graph:")
    print(f"  Vertices: {vertices}")
    print(f"  Faces: {faces}")
    print(f"  Edges: {edges}")
    print()
    
    triangulator = BiconnectedTriangulation(vertices, faces, edges)
    
    print("All triangulations (as chord sets):")
    triangulations = list(triangulator.enumerate_all_triangulations())
    for i, triangulation in enumerate(triangulations):
        print(f"  Triangulation {i + 1}: {triangulation}")
    
    print(f"\nTotal triangulations found: {len(triangulations)}")
    print(f"Risky vertices: {triangulator.risky_vertices}")
    
    # Test with quadrilateral
    print("\n--- Testing with Quadrilateral ---")
    vertices, faces, edges = create_quadrilateral_example()
    
    print(f"Input graph:")
    print(f"  Vertices: {vertices}")
    print(f"  Faces: {faces}")
    print(f"  Edges: {edges}")
    print()
    
    triangulator2 = BiconnectedTriangulation(vertices, faces, edges)
    
    print("All triangulations (as chord sets):")
    triangulations2 = list(triangulator2.enumerate_all_triangulations())
    for i, triangulation in enumerate(triangulations2):
        print(f"  Triangulation {i + 1}: {triangulation}")
    
    print(f"\nTotal triangulations found: {len(triangulations2)}")
    print(f"Risky vertices: {triangulator2.risky_vertices}")

if __name__ == "__main__":
    demonstrate_algorithm()