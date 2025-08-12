"""
Implementation of Parvez-Rahman-Nakano Algorithm for generating all triangulations
of triconnected planar graphs.

This implementation follows the algorithm described in the paper:
"Generating All Triangulations of Plane Graphs" by Mohammad Tanvir Parvez, 
Md. Saidur Rahman, and Shin-ichi Nakano (2011).

Author: Implementation based on the paper
Date: August 2025
"""

import sys
from typing import List, Set, Tuple, Dict, Optional
from copy import deepcopy
import json


class Edge:
    """Represents an edge in the graph with two endpoints."""
    
    def __init__(self, u: int, v: int):
        """Initialize edge with vertices u and v (smaller vertex first)."""
        self.u = min(u, v)
        self.v = max(u, v)
    
    def __eq__(self, other) -> bool:
        return isinstance(other, Edge) and self.u == other.u and self.v == other.v
    
    def __hash__(self) -> int:
        return hash((self.u, self.v))
    
    def __str__(self) -> str:
        return f"({self.u}, {self.v})"
    
    def __repr__(self) -> str:
        return f"Edge({self.u}, {self.v})"
    
    def to_tuple(self) -> Tuple[int, int]:
        """Return edge as a tuple."""
        return (self.u, self.v)


class Face:
    """Represents a face of the planar graph."""
    
    def __init__(self, vertices: List[int]):
        """Initialize face with a list of vertices in order."""
        if len(vertices) < 3:
            raise ValueError("A face must have at least 3 vertices")
        self.vertices = vertices.copy()
        self.size = len(vertices)
    
    def __str__(self) -> str:
        return f"Face({self.vertices})"
    
    def __repr__(self) -> str:
        return f"Face({self.vertices})"
    
    def get_edges(self) -> List[Edge]:
        """Get all edges of this face."""
        edges = []
        n = len(self.vertices)
        for i in range(n):
            u = self.vertices[i]
            v = self.vertices[(i + 1) % n]
            edges.append(Edge(u, v))
        return edges
    
    def is_triangle(self) -> bool:
        """Check if this face is a triangle."""
        return self.size == 3
    
    def contains_edge(self, edge: Edge) -> bool:
        """Check if this face contains the given edge."""
        return edge in self.get_edges()


class TriconnectedTriangulator:
    """
    Main class for generating all triangulations of triconnected planar graphs.
    
    The algorithm works by:
    1. Starting with an initial triangulation
    2. Finding flippable edges (chords that can be flipped to create new triangulations)
    3. Recursively generating all possible triangulations through edge flipping
    4. Using a tree structure to avoid duplicates and ensure completeness
    """
    
    def __init__(self, faces: List[List[int]], verbose: bool = False):
        """
        Initialize the triangulator with the input faces.
        
        Args:
            faces: List of faces, where each face is a list of vertex indices
            verbose: Enable verbose output for debugging
        """
        self.original_faces = [Face(face) for face in faces]
        self.verbose = verbose
        self.all_triangulations = []
        self.visited_triangulations = set()
        
        # Statistics
        self.flip_count = 0
        self.triangulation_count = 0
        
        # Extract all vertices and edges from faces
        self._extract_graph_info()
        
        if self.verbose:
            print(f"Initialized with {len(self.original_faces)} faces")
            print(f"Total vertices: {len(self.vertices)}")
            print(f"Total edges: {len(self.all_edges)}")
    
    def _extract_graph_info(self):
        """Extract vertices and edges from the input faces."""
        self.vertices = set()
        self.all_edges = set()
        
        for face in self.original_faces:
            for vertex in face.vertices:
                self.vertices.add(vertex)
            
            for edge in face.get_edges():
                self.all_edges.add(edge)
        
        self.vertices = sorted(list(self.vertices))
        self.vertex_count = len(self.vertices)
    
    def _faces_to_key(self, faces: List[Face]) -> str:
        """Convert a list of faces to a unique string key for duplicate detection."""
        face_tuples = []
        for face in faces:
            # Normalize face by starting with the minimum vertex
            vertices = face.vertices
            min_idx = vertices.index(min(vertices))
            normalized = vertices[min_idx:] + vertices[:min_idx]
            face_tuples.append(tuple(normalized))
        
        # Sort faces to ensure consistent ordering
        face_tuples.sort()
        return str(face_tuples)
    
    def _is_flippable_edge(self, edge: Edge, faces: List[Face]) -> bool:
        """
        Check if an edge can be flipped to create a new valid triangulation.
        
        An edge is flippable if:
        1. It's shared by exactly two triangular faces
        2. Flipping it doesn't create invalid faces
        3. The resulting graph remains planar and triconnected
        """
        # Find faces that contain this edge
        containing_faces = [face for face in faces if face.contains_edge(edge)]
        
        # Must be shared by exactly two faces
        if len(containing_faces) != 2:
            return False
        
        # Both faces must be triangles for the edge to be flippable
        if not all(face.is_triangle() for face in containing_faces):
            return False
        
        return True
    
    def _flip_edge(self, edge: Edge, faces: List[Face]) -> Optional[List[Face]]:
        """
        Flip the given edge and return the new set of faces.
        
        When flipping an edge between two triangles, we remove the edge
        and add a new edge connecting the two vertices not on the original edge.
        """
        # Find the two triangular faces sharing this edge
        face1 = face2 = None
        face1_idx = face2_idx = -1
        
        for i, face in enumerate(faces):
            if face.contains_edge(edge):
                if face1 is None:
                    face1 = face
                    face1_idx = i
                else:
                    face2 = face
                    face2_idx = i
                    break
        
        if face1 is None or face2 is None:
            return None
        
        # Find the vertices not on the edge (the vertices to be connected by new edge)
        edge_vertices = {edge.u, edge.v}
        other1 = None
        other2 = None
        
        for v in face1.vertices:
            if v not in edge_vertices:
                other1 = v
                break
        
        for v in face2.vertices:
            if v not in edge_vertices:
                other2 = v
                break
        
        if other1 is None or other2 is None:
            return None
        
        # Create new faces after flipping
        new_faces = faces.copy()
        
        # Remove old faces
        new_faces = [f for i, f in enumerate(new_faces) if i not in {face1_idx, face2_idx}]
        
        # Create new triangular faces
        # The quadrilateral formed by the two triangles is split differently
        vertices = [edge.u, edge.v, other1, other2]
        
        # Create two new triangles
        triangle1 = Face([edge.u, other1, other2])
        triangle2 = Face([edge.v, other1, other2])
        
        new_faces.extend([triangle1, triangle2])
        
        self.flip_count += 1
        
        return new_faces
    
    def _get_flippable_edges(self, faces: List[Face]) -> List[Edge]:
        """Get all edges that can be flipped in the current triangulation."""
        flippable_edges = []
        
        # Get all edges from all faces
        all_face_edges = set()
        for face in faces:
            for edge in face.get_edges():
                all_face_edges.add(edge)
        
        # Check which edges are flippable
        for edge in all_face_edges:
            if self._is_flippable_edge(edge, faces):
                flippable_edges.append(edge)
        
        return flippable_edges
    
    def _generate_triangulations_recursive(self, current_faces: List[Face], depth: int = 0):
        """
        Recursively generate all possible triangulations using edge flipping.
        
        This is the core of the algorithm that explores the tree of all possible
        triangulations by systematically flipping edges.
        """
        # Create a unique key for this triangulation to avoid duplicates
        current_key = self._faces_to_key(current_faces)
        
        if current_key in self.visited_triangulations:
            return
        
        self.visited_triangulations.add(current_key)
        
        # Store this triangulation
        triangulation_copy = deepcopy(current_faces)
        self.all_triangulations.append(triangulation_copy)
        self.triangulation_count += 1
        
        if self.verbose:
            print(f"{'  ' * depth}Triangulation {self.triangulation_count}: {len(current_faces)} faces")
        
        # Find all flippable edges in current triangulation
        flippable_edges = self._get_flippable_edges(current_faces)
        
        if self.verbose and flippable_edges:
            print(f"{'  ' * depth}Flippable edges: {[str(e) for e in flippable_edges]}")
        
        # Recursively explore all possible flips
        for edge in flippable_edges:
            new_faces = self._flip_edge(edge, current_faces)
            
            if new_faces is not None:
                if self.verbose:
                    print(f"{'  ' * depth}Flipping edge {edge}")
                
                # Recursively generate triangulations from this new state
                self._generate_triangulations_recursive(new_faces, depth + 1)
    
    def _create_initial_triangulation(self) -> List[Face]:
        """
        Create an initial triangulation from the input faces.
        
        If the input is not already triangulated, we triangulate each non-triangular face
        by adding chords to split it into triangles.
        """
        triangulated_faces = []
        
        for face in self.original_faces:
            if face.is_triangle():
                triangulated_faces.append(face)
            else:
                # Triangulate non-triangular face by fan triangulation
                # Connect first vertex to all non-adjacent vertices
                vertices = face.vertices
                first_vertex = vertices[0]
                
                for i in range(2, len(vertices) - 1):
                    triangle = Face([first_vertex, vertices[i], vertices[i + 1]])
                    triangulated_faces.append(triangle)
                
                # Add the triangle with the last edge
                triangle = Face([first_vertex, vertices[1], vertices[-1]])
                triangulated_faces.append(triangle)
        
        return triangulated_faces
    
    def generate_all_triangulations(self) -> List[List[Face]]:
        """
        Generate all possible triangulations of the triconnected planar graph.
        
        Returns:
            List of triangulations, where each triangulation is a list of Face objects.
        """
        if self.verbose:
            print("Starting triangulation generation...")
        
        # Reset counters and storage
        self.all_triangulations = []
        self.visited_triangulations = set()
        self.flip_count = 0
        self.triangulation_count = 0
        
        # Create initial triangulation
        initial_triangulation = self._create_initial_triangulation()
        
        if self.verbose:
            print(f"Initial triangulation has {len(initial_triangulation)} faces")
        
        # Generate all triangulations recursively
        self._generate_triangulations_recursive(initial_triangulation)
        
        if self.verbose:
            print(f"\nGeneration complete!")
            print(f"Total triangulations found: {len(self.all_triangulations)}")
            print(f"Total edge flips performed: {self.flip_count}")
        
        return self.all_triangulations
    
    def print_triangulations(self, max_display: int = 10):
        """Print all generated triangulations (limited to max_display for readability)."""
        print(f"\nAll Triangulations of the Triconnected Planar Graph:")
        print(f"Total triangulations: {len(self.all_triangulations)}")
        print("-" * 60)
        
        for i, triangulation in enumerate(self.all_triangulations[:max_display]):
            print(f"\nTriangulation {i + 1}:")
            for j, face in enumerate(triangulation):
                print(f"  Face {j + 1}: {face.vertices}")
        
        if len(self.all_triangulations) > max_display:
            print(f"\n... and {len(self.all_triangulations) - max_display} more triangulations")
    
    def export_to_json(self, filename: str):
        """Export all triangulations to a JSON file."""
        export_data = {
            "metadata": {
                "total_triangulations": len(self.all_triangulations),
                "vertex_count": self.vertex_count,
                "total_flips": self.flip_count,
                "original_faces": [[v for v in face.vertices] for face in self.original_faces]
            },
            "triangulations": []
        }
        
        for i, triangulation in enumerate(self.all_triangulations):
            triangulation_data = {
                "id": i + 1,
                "faces": [[v for v in face.vertices] for face in triangulation]
            }
            export_data["triangulations"].append(triangulation_data)
        
        with open(filename, 'w') as f:
            json.dump(export_data, f, indent=2)
        
        print(f"Exported {len(self.all_triangulations)} triangulations to {filename}")
    
    def get_statistics(self) -> Dict:
        """Get statistics about the triangulation generation process."""
        return {
            "total_triangulations": len(self.all_triangulations),
            "vertex_count": self.vertex_count,
            "edge_count": len(self.all_edges),
            "original_faces": len(self.original_faces),
            "total_flips": self.flip_count,
            "average_faces_per_triangulation": sum(len(t) for t in self.all_triangulations) / len(self.all_triangulations) if self.all_triangulations else 0
        }


def main():
    """
    Main function demonstrating the usage of the TriconnectedTriangulator.
    
    Example usage with different types of input faces.
    """
    print("=== Triconnected Planar Graph Triangulation Generator ===")
    print("Based on Parvez-Rahman-Nakano Algorithm\n")
    
    # Example 1: Simple quadrilateral (can be triangulated in 2 ways)
    print("Example 1: Quadrilateral")
    faces1 = [[1, 2, 3, 4]]
    
    triangulator1 = TriconnectedTriangulator(faces1, verbose=True)
    triangulations1 = triangulator1.generate_all_triangulations()
    triangulator1.print_triangulations()
    
    print("\nStatistics:")
    stats1 = triangulator1.get_statistics()
    for key, value in stats1.items():
        print(f"  {key}: {value}")
    
    # Example 2: Pentagon
    print("\n" + "="*60)
    print("Example 2: Pentagon")
    faces2 = [[1, 2, 3, 4, 5]]
    
    triangulator2 = TriconnectedTriangulator(faces2, verbose=True)
    triangulations2 = triangulator2.generate_all_triangulations()
    triangulator2.print_triangulations()
    
    print("\nStatistics:")
    stats2 = triangulator2.get_statistics()
    for key, value in stats2.items():
        print(f"  {key}: {value}")
    
    # Example 3: Multiple faces (more complex graph)
    print("\n" + "="*60)
    print("Example 3: Multiple Faces")
    faces3 = [[1, 2, 3, 4], [1, 2, 4, 5, 6]]
    
    triangulator3 = TriconnectedTriangulator(faces3, verbose=True)
    triangulations3 = triangulator3.generate_all_triangulations()
    triangulator3.print_triangulations()
    
    # Export results
    triangulator3.export_to_json("triangulations_output.json")
    
    print("\nStatistics:")
    stats3 = triangulator3.get_statistics()
    for key, value in stats3.items():
        print(f"  {key}: {value}")


if __name__ == "__main__":
    main()
