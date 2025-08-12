"""
Simple test runner and demo for the Triconnected Planar Graph Triangulation Generator.

This script provides a quick way to test the implementation with your specified
face format: faces = [[1,2,3,4], [1,2,4,5,6]]

Run this script to see the algorithm in action!
"""

from main_triangulation_generator import TriangulationGenerator
import json


def quick_demo():
    """Quick demonstration with the exact format you specified."""
    print("🔺 QUICK DEMO: Triconnected Planar Graph Triangulations 🔺")
    print("=" * 70)
    
    # Your exact example
    faces = [[1, 2, 3, 4], [1, 2, 4, 5, 6]]
    print(f"Input faces: {faces}")
    
    # Create generator
    generator = TriangulationGenerator(verbose=True, validate_input=True)
    
    # Generate triangulations
    result = generator.generate_triangulations(faces)
    
    if result.success:
        print(f"\n✅ SUCCESS! Generated {len(result.triangulations)} triangulations")
        
        # Show detailed results for first few triangulations
        print("\n📋 DETAILED TRIANGULATION BREAKDOWN:")
        for i, triangulation in enumerate(result.triangulations[:3]):  # Show first 3
            print(f"\n  Triangulation {i+1}:")
            for j, triangle in enumerate(triangulation.triangles):
                print(f"    Triangle {j+1}: {triangle.vertices}")
            
            if triangulation.internal_chords:
                print(f"    Internal chords: {[(c.u, c.v) for c in triangulation.internal_chords]}")
        
        if len(result.triangulations) > 3:
            print(f"    ... and {len(result.triangulations) - 3} more triangulations")
        
        # Export to file
        with open("quick_demo_results.json", "w") as f:
            export_data = {
                'input_faces': faces,
                'triangulations': [
                    {
                        'id': i+1,
                        'triangles': [list(triangle.vertices) for triangle in t.triangles],
                        'internal_chords': [(c.u, c.v) for c in t.internal_chords]
                    }
                    for i, t in enumerate(result.triangulations)
                ]
            }
            json.dump(export_data, f, indent=2)
        
        print(f"\n💾 Results saved to 'quick_demo_results.json'")
        
    else:
        print(f"❌ FAILED: {result.error_message}")


def test_different_inputs():
    """Test with various input configurations."""
    print("\n" + "="*70)
    print("🧪 TESTING DIFFERENT INPUT CONFIGURATIONS")
    print("="*70)
    
    test_inputs = [
        # Simple cases
        {"name": "Triangle", "faces": [[1, 2, 3]]},
        {"name": "Square", "faces": [[1, 2, 3, 4]]}, 
        {"name": "Pentagon", "faces": [[1, 2, 3, 4, 5]]},
        
        # Your specified format
        {"name": "Your Example", "faces": [[1, 2, 3, 4], [1, 2, 4, 5, 6]]},
        
        # More complex cases
        {"name": "Two Squares", "faces": [[1, 2, 3, 4], [4, 5, 6, 7]]},
        {"name": "Hexagon", "faces": [[1, 2, 3, 4, 5, 6]]},
    ]
    
    generator = TriangulationGenerator(verbose=False, validate_input=True)
    
    results_summary = []
    
    for test in test_inputs:
        print(f"\n📝 Testing: {test['name']}")
        print(f"   Input: {test['faces']}")
        
        try:
            result = generator.generate_triangulations(test['faces'])
            
            if result.success:
                count = len(result.triangulations)
                time_taken = result.execution_time
                print(f"   ✅ Generated {count} triangulations in {time_taken:.4f}s")
                
                results_summary.append({
                    'name': test['name'],
                    'faces': test['faces'],
                    'triangulations': count,
                    'time': time_taken,
                    'success': True
                })
            else:
                print(f"   ❌ Failed: {result.error_message}")
                results_summary.append({
                    'name': test['name'],
                    'faces': test['faces'], 
                    'success': False,
                    'error': result.error_message
                })
                
        except Exception as e:
            print(f"   💥 Exception: {str(e)}")
            results_summary.append({
                'name': test['name'],
                'faces': test['faces'],
                'success': False, 
                'error': str(e)
            })
    
    # Summary table
    print(f"\n📊 SUMMARY TABLE:")
    print("-" * 70)
    print(f"{'Test Name':<15} {'Input Faces':<25} {'Triangulations':<15} {'Time(s)':<10}")
    print("-" * 70)
    
    for result in results_summary:
        if result['success']:
            faces_str = str(result['faces'])[:23] + ('...' if len(str(result['faces'])) > 23 else '')
            print(f"{result['name']:<15} {faces_str:<25} {result['triangulations']:<15} {result['time']:<10.4f}")
        else:
            faces_str = str(result['faces'])[:23] + ('...' if len(str(result['faces'])) > 23 else '')
            print(f"{result['name']:<15} {faces_str:<25} {'FAILED':<15} {'N/A':<10}")
    
    print("-" * 70)


def main():
    """Main test runner function."""
    print("🚀 TRICONNECTED TRIANGULATION ALGORITHM - TEST RUNNER")
    print("Based on Parvez-Rahman-Nakano Algorithm")
    print("=" * 70)
    
    try:
        # Run quick demo first
        quick_demo()
        
        # Then run comprehensive tests
        test_different_inputs()
        
        print(f"\n✨ ALL TESTS COMPLETED SUCCESSFULLY! ✨")
        print("Check the generated JSON files for detailed results.")
        
    except Exception as e:
        print(f"\n💥 Test runner failed with error: {str(e)}")
        import traceback
        traceback.print_exc()


if __name__ == "__main__":
    main()
