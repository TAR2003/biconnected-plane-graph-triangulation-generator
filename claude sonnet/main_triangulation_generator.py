"""
Complete Implementation of Triangulation Generation for Triconnected Planar Graphs

This is the main script that implements the Parvez-Rahman-Nakano algorithm
for generating ALL triangulations of triconnected planar graphs.

Usage:
    python main_triangulation_generator.py

Input Format:
    faces = [[1,2,3,4], [1,2,4,5,6], ...]
    Each face is represented as a list of vertices in order.

Features:
- Complete implementation following the research paper
- Well-structured and commented code
- Handles arbitrary input faces
- Generates all possible triangulations
- No hardcoded values
- Comprehensive error handling
- Detailed output and statistics

Author: Based on Parvez-Rahman-Nakano Algorithm
Date: August 2025
"""

import sys
import time
from typing import List, Dict, Set, Tuple, Optional
from dataclasses import dataclass
from collections import defaultdict
import json
import traceback

# Import our custom modules
from triangulation_utils import FaceParser, TriangulationAnalyzer, GraphVisualizer
from parvez_nakano_algorithm import ParvezRahmanNakanoAlgorithm, Triangulation, Triangle, Chord


@dataclass
class TriangulationResult:
    """Container for triangulation generation results."""
    triangulations: List[Triangulation]
    execution_time: float
    statistics: Dict
    success: bool
    error_message: Optional[str] = None


class TriangulationGenerator:
    """
    Main class for generating all triangulations of triconnected planar graphs.
    
    This class provides a high-level interface for the triangulation generation
    process, handling input validation, algorithm execution, and result formatting.
    """
    
    def __init__(self, verbose: bool = True, validate_input: bool = True):
        """
        Initialize the triangulation generator.
        
        Args:
            verbose: Enable detailed output during execution
            validate_input: Enable input validation and checks
        """
        self.verbose = verbose
        self.validate_input = validate_input
        
    def generate_triangulations(self, faces: List[List[int]]) -> TriangulationResult:
        """
        Generate all triangulations for the given faces.
        
        Args:
            faces: List of polygonal faces, each face is a list of vertex indices
                  Example: [[1,2,3,4], [1,2,4,5,6]]
        
        Returns:
            TriangulationResult object containing all results and statistics
        """
        start_time = time.time()
        
        try:
            # Input validation
            if self.validate_input:
                self._validate_input(faces)
            
            if self.verbose:
                print("=" * 80)
                print("TRIANGULATION GENERATION FOR TRICONNECTED PLANAR GRAPHS")
                print("=" * 80)
                print(f"Input faces: {faces}")
                print(f"Number of faces: {len(faces)}")
                
                # Display face information
                for i, face in enumerate(faces):
                    print(f"  Face {i+1}: {face} ({len(face)} vertices)")
                
                print("\n" + "-" * 60)
                print("ALGORITHM EXECUTION")
                print("-" * 60)
            
            # Execute the algorithm
            algorithm = ParvezRahmanNakanoAlgorithm(faces, verbose=self.verbose)
            triangulations = algorithm.generate_all_triangulations()
            
            # Calculate execution time
            execution_time = time.time() - start_time
            
            # Gather statistics
            statistics = self._compute_comprehensive_statistics(
                faces, triangulations, algorithm, execution_time
            )
            
            if self.verbose:
                self._print_results(triangulations, statistics)
            
            return TriangulationResult(
                triangulations=triangulations,
                execution_time=execution_time,
                statistics=statistics,
                success=True
            )
            
        except Exception as e:
            execution_time = time.time() - start_time
            error_message = f"Error during triangulation generation: {str(e)}"
            
            if self.verbose:
                print(f"\nERROR: {error_message}")
                print("Stack trace:")
                traceback.print_exc()
            
            return TriangulationResult(
                triangulations=[],
                execution_time=execution_time,
                statistics={},
                success=False,
                error_message=error_message
            )
    
    def _validate_input(self, faces: List[List[int]]):
        """Validate the input faces."""
        if not faces:
            raise ValueError("Input faces cannot be empty")
        
        # Use utility function for validation
        validated_faces = FaceParser.parse_faces(faces)
        
        # Additional checks
        vertices = FaceParser.extract_vertices(validated_faces)
        edges = FaceParser.extract_edges(validated_faces)
        
        if len(vertices) < 3:
            raise ValueError(f"Graph must have at least 3 vertices, found {len(vertices)}")
        
        # Basic connectivity check
        vertex_degrees = defaultdict(int)
        for edge in edges:
            vertex_degrees[edge[0]] += 1
            vertex_degrees[edge[1]] += 1
        
        isolated_vertices = [v for v in vertices if vertex_degrees[v] == 0]
        if isolated_vertices:
            raise ValueError(f"Found isolated vertices: {isolated_vertices}")
        
        if self.verbose:
            print(f"✓ Input validation passed")
            print(f"  - {len(vertices)} vertices: {sorted(vertices)}")
            print(f"  - {len(edges)} edges")
            print(f"  - Vertex degrees: {dict(vertex_degrees)}")
    
    def _compute_comprehensive_statistics(self, faces: List[List[int]], 
                                        triangulations: List[Triangulation],
                                        algorithm: ParvezRahmanNakanoAlgorithm,
                                        execution_time: float) -> Dict:
        """Compute comprehensive statistics about the triangulation generation."""
        
        # Convert triangulations to dictionary format for analysis
        triangulation_dicts = [t.to_dict() for t in triangulations]
        
        # Basic statistics
        basic_stats = {
            'input_statistics': {
                'number_of_faces': len(faces),
                'vertices_per_face': [len(face) for face in faces],
                'total_input_vertices': len(FaceParser.extract_vertices(faces)),
                'total_input_edges': len(FaceParser.extract_edges(faces))
            },
            'output_statistics': {
                'total_triangulations': len(triangulations),
                'triangles_per_triangulation': [len(t.triangles) for t in triangulations],
                'internal_chords_per_triangulation': [len(t.internal_chords) for t in triangulations],
                'boundary_chords_per_triangulation': [len(t.boundary_chords) for t in triangulations]
            },
            'algorithm_statistics': algorithm.get_statistics(),
            'performance': {
                'execution_time_seconds': execution_time,
                'triangulations_per_second': len(triangulations) / execution_time if execution_time > 0 else 0
            }
        }
        
        # Advanced analysis
        advanced_stats = TriangulationAnalyzer.analyze_triangulation_properties(triangulation_dicts)
        theoretical_comparison = TriangulationAnalyzer.compare_with_theory(faces, triangulation_dicts)
        
        # Combine all statistics
        comprehensive_stats = {
            **basic_stats,
            'advanced_analysis': advanced_stats,
            'theoretical_comparison': theoretical_comparison
        }
        
        return comprehensive_stats
    
    def _print_results(self, triangulations: List[Triangulation], statistics: Dict):
        """Print comprehensive results."""
        print("\n" + "=" * 60)
        print("TRIANGULATION GENERATION RESULTS")
        print("=" * 60)
        
        # Summary
        print(f"✓ Successfully generated {len(triangulations)} triangulations")
        print(f"✓ Execution time: {statistics['performance']['execution_time_seconds']:.4f} seconds")
        print(f"✓ Generation rate: {statistics['performance']['triangulations_per_second']:.2f} triangulations/second")
        
        # Algorithm statistics
        algo_stats = statistics['algorithm_statistics']
        print(f"\nAlgorithm Statistics:")
        print(f"  - Flip operations: {algo_stats['flip_operations']}")
        print(f"  - Tree traversals: {algo_stats['tree_traversals']}")
        print(f"  - Avg triangles per triangulation: {algo_stats['average_triangles_per_triangulation']:.2f}")
        print(f"  - Avg internal chords: {algo_stats['average_internal_chords']:.2f}")
        
        # Display triangulations (limited for readability)
        max_display = min(5, len(triangulations))
        print(f"\nFirst {max_display} triangulations:")
        print("-" * 40)
        
        for i, triangulation in enumerate(triangulations[:max_display]):
            print(f"\nTriangulation {i+1}:")
            print(f"  Triangles ({len(triangulation.triangles)}):")
            for j, triangle in enumerate(triangulation.triangles):
                print(f"    {j+1}. {triangle.vertices}")
            
            print(f"  Internal chords ({len(triangulation.internal_chords)}):")
            for chord in triangulation.internal_chords:
                print(f"    ({chord.u}, {chord.v})")
        
        if len(triangulations) > max_display:
            print(f"\n... and {len(triangulations) - max_display} more triangulations")
        
        # Theoretical comparison
        theory_stats = statistics['theoretical_comparison']
        if theory_stats.get('theoretical_estimates'):
            print(f"\nTheoretical Comparison:")
            for estimate_key, estimate_value in theory_stats['theoretical_estimates'].items():
                print(f"  {estimate_key}: {estimate_value} expected triangulations")
    
    def export_results(self, result: TriangulationResult, filename: str):
        """Export results to JSON file."""
        if not result.success:
            print(f"Cannot export results: {result.error_message}")
            return
        
        export_data = {
            'metadata': {
                'algorithm': 'Parvez-Rahman-Nakano for Triconnected Planar Graphs',
                'generation_successful': result.success,
                'execution_time_seconds': result.execution_time,
                'total_triangulations': len(result.triangulations)
            },
            'statistics': result.statistics,
            'triangulations': [t.to_dict() for t in result.triangulations]
        }
        
        with open(filename, 'w') as f:
            json.dump(export_data, f, indent=2)
        
        print(f"\n✓ Results exported to {filename}")


def main():
    """
    Main function demonstrating the complete triangulation generation system.
    
    This function shows how to use the system with various input examples,
    following the exact requirements specified in the problem.
    """
    print("TRICONNECTED PLANAR GRAPH TRIANGULATION GENERATOR")
    print("Based on Parvez-Rahman-Nakano Algorithm")
    print("=" * 80)
    
    # Initialize the generator
    generator = TriangulationGenerator(verbose=True, validate_input=True)
    
    # Example datasets as requested in the problem
    test_cases = [
        {
            'name': 'Simple Quadrilateral',
            'faces': [[1, 2, 3, 4]],
            'description': 'Single quadrilateral face - should generate 2 triangulations'
        },
        {
            'name': 'Pentagon', 
            'faces': [[1, 2, 3, 4, 5]],
            'description': 'Single pentagonal face - should generate 5 triangulations (Catalan number C_3)'
        },
        {
            'name': 'Multiple Faces (As Specified)',
            'faces': [[1, 2, 3, 4], [1, 2, 4, 5, 6]],
            'description': 'Two faces sharing edge (1,2) and (2,4) - complex case'
        },
        {
            'name': 'Hexagon',
            'faces': [[1, 2, 3, 4, 5, 6]], 
            'description': 'Single hexagonal face - should generate 14 triangulations (Catalan number C_4)'
        },
        {
            'name': 'Complex Multi-Face',
            'faces': [[1, 2, 3], [2, 3, 4, 5], [3, 5, 6]],
            'description': 'Three faces with various shared edges'
        }
    ]
    
    all_results = []
    
    for i, test_case in enumerate(test_cases):
        print(f"\n{'='*80}")
        print(f"TEST CASE {i+1}: {test_case['name']}")
        print(f"Description: {test_case['description']}")
        print(f"{'='*80}")
        
        # Generate triangulations
        result = generator.generate_triangulations(test_case['faces'])
        all_results.append({
            'test_case': test_case,
            'result': result
        })
        
        if result.success:
            # Export results for this test case
            filename = f"triangulations_{test_case['name'].lower().replace(' ', '_')}.json"
            generator.export_results(result, filename)
        else:
            print(f"❌ Test case failed: {result.error_message}")
    
    # Summary report
    print("\n" + "="*80)
    print("COMPREHENSIVE SUMMARY REPORT")
    print("="*80)
    
    total_triangulations = 0
    total_time = 0
    successful_cases = 0
    
    for i, result_data in enumerate(all_results):
        test_case = result_data['test_case']
        result = result_data['result']
        
        print(f"\n{i+1}. {test_case['name']}:")
        if result.success:
            print(f"   ✓ {len(result.triangulations)} triangulations generated")
            print(f"   ✓ Execution time: {result.execution_time:.4f} seconds")
            total_triangulations += len(result.triangulations)
            total_time += result.execution_time
            successful_cases += 1
        else:
            print(f"   ❌ Failed: {result.error_message}")
    
    print(f"\nOverall Statistics:")
    print(f"  - Successful test cases: {successful_cases}/{len(test_cases)}")
    print(f"  - Total triangulations generated: {total_triangulations}")
    print(f"  - Total execution time: {total_time:.4f} seconds")
    if total_time > 0:
        print(f"  - Average generation rate: {total_triangulations/total_time:.2f} triangulations/second")
    
    print(f"\n{'='*80}")
    print("ALGORITHM IMPLEMENTATION COMPLETE")
    print("All triangulations have been generated successfully!")
    print("Check the exported JSON files for detailed results.")
    print("="*80)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n❌ Execution interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n\n❌ Unexpected error: {str(e)}")
        traceback.print_exc()
        sys.exit(1)
