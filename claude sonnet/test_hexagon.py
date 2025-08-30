"""
Test for hexagon (6-vertex polygon) triangulation
"""

from main_triangulation_generator import TriangulationGenerator

def test_hexagon():
    # Hexagon with vertices 0,1,2,3,4,5
    faces = [[0, 1, 2, 3, 4, 5]]
    
    print(f"Testing hexagon with vertices: {faces[0]}")
    
    generator = TriangulationGenerator(verbose=True, validate_input=True)
    result = generator.generate_triangulations(faces)
    
    if result.success:
        print(f"\n✅ SUCCESS! Generated {len(result.triangulations)} triangulations")
        print("Expected: 14 triangulations for a hexagon")
        
        # Show first few triangulations
        print("\nFirst few triangulations:")
        for i, triangulation in enumerate(result.triangulations[:5]):
            print(f"\nTriangulation {i+1}:")
            triangles_str = ', '.join([f"{t.vertices}" for t in triangulation.triangles])
            print(f"  Triangles: {triangles_str}")
            if triangulation.internal_chords:
                chords_str = ', '.join([f"({c.u},{c.v})" for c in triangulation.internal_chords])
                print(f"  Internal chords: {chords_str}")
    else:
        print(f"❌ FAILED: {result.error}")

if __name__ == "__main__":
    test_hexagon()
