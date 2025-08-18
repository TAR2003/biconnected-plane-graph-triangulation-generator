"""
Utility functions for handling face input and graph operations
for the triconnected planar graph triangulation algorithms.

This module provides helper functions to:
- Parse and validate input faces
- Convert between different representations
- Visualize results
- Perform graph theory operations
"""

from typing import List, Set, Tuple, Dict, Optional
import json
import matplotlib.pyplot as plt
import networkx as nx
from collections import defaultdict, Counter


class FaceParser:
    """Helper class for parsing and validating input faces."""
    
    @staticmethod
    def parse_faces(faces_input: List[List[int]]) -> List[List[int]]:
        """
        Parse and validate the input faces.
        
        Args:
            faces_input: List of faces where each face is [v1, v2, v3, ...]
            
        Returns:
            Validated list of faces
            
        Raises:
            ValueError: If input is invalid
        """
        if not faces_input:
            raise ValueError("Input faces cannot be empty")
        
        validated_faces = []
        
        for i, face in enumerate(faces_input):
            if len(face) < 3:
                raise ValueError(f"Face {i+1} has less than 3 vertices: {face}")
            
            # Remove duplicates while preserving order
            cleaned_face = []
            seen = set()
            for vertex in face:
                if vertex not in seen:
                    cleaned_face.append(vertex)
                    seen.add(vertex)
            
            if len(cleaned_face) < 3:
                raise ValueError(f"Face {i+1} has less than 3 unique vertices after cleaning: {face}")
            
            validated_faces.append(cleaned_face)
        
        return validated_faces
    
    @staticmethod
    def extract_vertices(faces: List[List[int]]) -> Set[int]:
        """Extract all unique vertices from faces."""
        vertices = set()
        for face in faces:
            vertices.update(face)
        return vertices
    
    @staticmethod
    def extract_edges(faces: List[List[int]]) -> Set[Tuple[int, int]]:
        """Extract all edges from faces."""
        edges = set()
        for face in faces:
            n = len(face)
            for i in range(n):
                u, v = face[i], face[(i + 1) % n]
                edges.add((min(u, v), max(u, v)))
        return edges
    
    @staticmethod
    def validate_planarity(faces: List[List[int]]) -> bool:
        """
        Basic planarity check using Euler's formula: V - E + F = 2
        This is a necessary but not sufficient condition.
        """
        vertices = FaceParser.extract_vertices(faces)
        edges = FaceParser.extract_edges(faces)
        
        V = len(vertices)
        E = len(edges)
        F = len(faces)
        
        euler_characteristic = V - E + F
        
        # For a connected planar graph, V - E + F = 2
        return euler_characteristic == 2


class GraphVisualizer:
    """Utility class for visualizing graphs and triangulations."""
    
    @staticmethod
    def create_networkx_graph(faces: List[List[int]]) -> nx.Graph:
        """Create a NetworkX graph from faces."""
        G = nx.Graph()
        
        # Add all vertices
        vertices = FaceParser.extract_vertices(faces)
        G.add_nodes_from(vertices)
        
        # Add all edges
        edges = FaceParser.extract_edges(faces)
        G.add_edges_from(edges)
        
        return G
    
    @staticmethod
    def plot_triangulation(triangulation_data: Dict, title: str = "Triangulation", 
                          save_path: Optional[str] = None):
        """
        Plot a triangulation using matplotlib and networkx.
        
        Args:
            triangulation_data: Dictionary with 'triangles' key containing triangles
            title: Plot title
            save_path: Path to save the plot (optional)
        """
        # Extract triangles and create graph
        triangles = triangulation_data.get('triangles', [])
        
        G = nx.Graph()
        for triangle in triangles:
            if len(triangle) == 3:
                v1, v2, v3 = triangle
                G.add_edge(v1, v2)
                G.add_edge(v2, v3)
                G.add_edge(v3, v1)
        
        # Create layout
        try:
            pos = nx.planar_layout(G)
        except:
            pos = nx.spring_layout(G, seed=42)
        
        # Plot
        plt.figure(figsize=(10, 8))
        nx.draw(G, pos, with_labels=True, node_color='lightblue', 
                node_size=500, font_size=12, font_weight='bold')
        
        # Highlight triangular faces
        for triangle in triangles:
            if len(triangle) == 3:
                v1, v2, v3 = triangle
                triangle_pos = [pos[v1], pos[v2], pos[v3], pos[v1]]
                xs, ys = zip(*triangle_pos)
                plt.fill(xs, ys, alpha=0.2, color='yellow')
        
        plt.title(title)
        plt.axis('equal')
        
        if save_path:
            plt.savefig(save_path, dpi=300, bbox_inches='tight')
        
        plt.show()
    
    @staticmethod
    def plot_all_triangulations(triangulations: List[Dict], max_plots: int = 6):
        """Plot multiple triangulations in a grid."""
        n_plots = min(len(triangulations), max_plots)
        if n_plots == 0:
            return
        
        # Calculate grid dimensions
        cols = min(3, n_plots)
        rows = (n_plots + cols - 1) // cols
        
        fig, axes = plt.subplots(rows, cols, figsize=(5*cols, 5*rows))
        if rows == 1 and cols == 1:
            axes = [axes]
        elif rows == 1:
            axes = [axes]
        else:
            axes = axes.flatten()
        
        for i in range(n_plots):
            triangulation = triangulations[i]
            ax = axes[i]
            
            # Create graph for this triangulation
            triangles = triangulation.get('triangles', [])
            G = nx.Graph()
            for triangle in triangles:
                if len(triangle) == 3:
                    v1, v2, v3 = triangle
                    G.add_edge(v1, v2)
                    G.add_edge(v2, v3)
                    G.add_edge(v3, v1)
            
            # Layout
            try:
                pos = nx.planar_layout(G)
            except:
                pos = nx.spring_layout(G, seed=42)
            
            # Draw
            nx.draw(G, pos, ax=ax, with_labels=True, node_color='lightblue',
                   node_size=300, font_size=10, font_weight='bold')
            
            ax.set_title(f'Triangulation {i+1}')
            ax.axis('equal')
        
        # Hide unused subplots
        for i in range(n_plots, len(axes)):
            axes[i].set_visible(False)
        
        plt.tight_layout()
        plt.show()


class TriangulationAnalyzer:
    """Analyze and compare different triangulations."""
    
    @staticmethod
    def count_catalan_triangulations(n: int) -> int:
        """
        Calculate the expected number of triangulations for a convex n-gon
        using the Catalan number formula: C_{n-2} = (1/(n-1)) * C(2n-4, n-2)
        """
        if n < 3:
            return 0
        if n == 3:
            return 1
        
        # Calculate (n-2)th Catalan number
        k = n - 2
        catalan = 1
        for i in range(k):
            catalan = catalan * (2 * (2 * i + 1)) // (i + 2)
        
        return catalan
    
    @staticmethod
    def analyze_triangulation_properties(triangulations: List[Dict]) -> Dict:
        """
        Analyze properties of the generated triangulations.
        
        Returns:
            Dictionary with various statistics
        """
        if not triangulations:
            return {'error': 'No triangulations provided'}
        
        stats = {
            'total_triangulations': len(triangulations),
            'triangle_counts': [],
            'vertex_degrees': defaultdict(list),
            'common_vertices': set(),
            'all_vertices': set()
        }
        
        for triangulation in triangulations:
            triangles = triangulation.get('triangles', [])
            stats['triangle_counts'].append(len(triangles))
            
            # Analyze vertex degrees
            vertex_degree = defaultdict(int)
            vertices_in_triangulation = set()
            
            for triangle in triangles:
                for vertex in triangle:
                    vertices_in_triangulation.add(vertex)
                    vertex_degree[vertex] += 1
            
            stats['all_vertices'].update(vertices_in_triangulation)
            
            if not stats['common_vertices']:
                stats['common_vertices'] = vertices_in_triangulation
            else:
                stats['common_vertices'] &= vertices_in_triangulation
            
            for vertex, degree in vertex_degree.items():
                stats['vertex_degrees'][vertex].append(degree)
        
        # Calculate summary statistics
        stats['avg_triangles'] = sum(stats['triangle_counts']) / len(stats['triangle_counts'])
        stats['min_triangles'] = min(stats['triangle_counts'])
        stats['max_triangles'] = max(stats['triangle_counts'])
        stats['total_vertices'] = len(stats['all_vertices'])
        stats['common_vertex_count'] = len(stats['common_vertices'])
        
        return stats
    
    @staticmethod
    def compare_with_theory(faces: List[List[int]], triangulations: List[Dict]) -> Dict:
        """
        Compare the generated triangulations with theoretical expectations.
        """
        comparison = {
            'input_faces': len(faces),
            'generated_count': len(triangulations),
            'theoretical_estimates': {}
        }
        
        # For simple polygons, we can estimate using Catalan numbers
        for i, face in enumerate(faces):
            if len(face) >= 3:
                expected = TriangulationAnalyzer.count_catalan_triangulations(len(face))
                comparison['theoretical_estimates'][f'face_{i+1}_{len(face)}_vertices'] = expected
        
        return comparison


def demo_utilities():
    """Demonstrate the utility functions."""
    print("=== Utility Functions Demo ===")
    
    # Example faces
    test_faces = [[1, 2, 3, 4], [1, 2, 4, 5, 6]]
    
    print(f"Input faces: {test_faces}")
    
    # Parse and validate
    try:
        validated_faces = FaceParser.parse_faces(test_faces)
        print(f"Validated faces: {validated_faces}")
        
        vertices = FaceParser.extract_vertices(validated_faces)
        print(f"Vertices: {sorted(vertices)}")
        
        edges = FaceParser.extract_edges(validated_faces)
        print(f"Edges: {sorted(edges)}")
        
        is_planar = FaceParser.validate_planarity(validated_faces)
        print(f"Passes basic planarity check: {is_planar}")
        
        # Theoretical analysis
        for i, face in enumerate(validated_faces):
            expected_triangulations = TriangulationAnalyzer.count_catalan_triangulations(len(face))
            print(f"Face {i+1} ({len(face)} vertices): Expected {expected_triangulations} triangulations")
        
    except ValueError as e:
        print(f"Validation error: {e}")


if __name__ == "__main__":
    demo_utilities()
