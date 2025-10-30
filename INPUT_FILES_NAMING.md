# Graph Input Files - Naming Convention Summary

All graph input files in `src/input/` have been renamed to reflect their graph structure.

## Recognized Graph Types

### Complete Graphs (K_n)
- **K3_triangle.txt** - Complete graph on 3 vertices (triangle)
- **K4_tetrahedron_1.txt** - Complete graph on 4 vertices (all vertices connected)
- **K4_tetrahedron_2.txt** - Another K4 instance

### Bipartite Graphs (K_{m,n})
- **K2_3_bipartite.txt** - Complete bipartite graph with partitions of size 2 and 3

### Cycle Graphs (C_n)
- **C4_square_cycle.txt** - 4-vertex cycle (square)
- **C5_pentagon_cycle.txt** - 5-vertex cycle (pentagon)
- **C6_hexagon_cycle_1.txt** - 6-vertex cycle (hexagon), instance 1
- **C6_hexagon_cycle_2.txt** - 6-vertex cycle (hexagon), instance 2
- **C6_hexagon_cycle_3.txt** - 6-vertex cycle (hexagon), instance 3
- **C7_heptagon_cycle.txt** - 7-vertex cycle (heptagon)
- **C8_octagon_cycle.txt** - 8-vertex cycle (octagon)
- **C9_nonagon_cycle.txt** - 9-vertex cycle (nonagon)

### Wheel Graphs (W_n)
- **W4_wheel.txt** - Wheel graph with 4 rim vertices (1 hub + 4-cycle)
- **W5_wheel_1.txt** - Wheel graph with 5 rim vertices, instance 1
- **W5_wheel_2.txt** - Wheel graph with 5 rim vertices, instance 2

### Diamond Graphs
- **diamond_1.txt** - K4 minus one edge (diamond shape)
- **diamond_2.txt** - K4 minus one edge (diamond shape)
- **diamond_3.txt** - K4 minus one edge (diamond shape)

### Triangulated Graphs
- **triangulated_4tri_1.txt** - Graph with 4 triangular faces, instance 1
- **triangulated_4tri_2.txt** - Graph with 4 triangular faces, instance 2
- **triangulated_9tri.txt** - Graph with 9 triangular faces

### Quadrilateral Graphs
- **quadrilateral_4quad.txt** - Graph with 4 quadrilateral faces
- **quadrilateral_7quad.txt** - Graph with 7 quadrilateral faces
- **quadrilateral_14quad.txt** - Graph with 14 quadrilateral faces

### Special Graphs
- **octahedron_dual_tetrahedra.txt** - Octahedron structure (dual tetrahedra with 8 triangular faces)

### Generic Graphs (format: graph_V{vertices}E{edges}F{faces})
These graphs don't match standard named graph families:

- **graph_V5E6F3.txt** - 5 vertices, 6 edges, 3 faces
- **graph_V5E7F3.txt** - 5 vertices, 7 edges, 3 faces
- **graph_V5E7F4_1.txt** - 5 vertices, 7 edges, 4 faces, instance 1
- **graph_V5E7F4_2.txt** - 5 vertices, 7 edges, 4 faces, instance 2
- **graph_V5E7F4_3.txt** - 5 vertices, 7 edges, 4 faces, instance 3
- **graph_V6E7F3_1.txt** - 6 vertices, 7 edges, 3 faces, instance 1
- **graph_V6E7F3_2.txt** - 6 vertices, 7 edges, 3 faces, instance 2
- **graph_V6E7F3_3.txt** - 6 vertices, 7 edges, 3 faces, instance 3
- **graph_V6E7F3_4.txt** - 6 vertices, 7 edges, 3 faces, instance 4
- **graph_V7E9F3.txt** - 7 vertices, 9 edges, 3 faces
- **graph_V7E11F5.txt** - 7 vertices, 11 edges, 5 faces
- **graph_V7E14F9.txt** - 7 vertices, 14 edges, 9 faces
- **graph_V8E10F4.txt** - 8 vertices, 10 edges, 4 faces
- **graph_V8E15F8.txt** - 8 vertices, 15 edges, 8 faces
- **graph_V9E10F3.txt** - 9 vertices, 10 edges, 3 faces
- **graph_V12E25F12.txt** - 12 vertices, 25 edges, 12 faces

## File Format

Each file contains:
1. First line: Number of faces
2. For each face:
   - Line 1: Number of vertices in the face
   - Line 2: Space-separated list of vertex indices

## Naming Convention Legend

- **Cn** = Cycle graph with n vertices
- **Kn** = Complete graph with n vertices
- **K{m}_{n}** = Complete bipartite graph with partitions of size m and n
- **Wn** = Wheel graph with n rim vertices
- **V{n}E{m}F{k}** = Generic graph with n vertices, m edges, k faces

## Original File Mappings

| Original Name | New Name |
|---------------|----------|
| 11_diamond.txt | diamond_1.txt |
| 12_bow_tie.txt | graph_V5E7F3.txt |
| 13_square_both_diagonals.txt | K4_tetrahedron_1.txt |
| 14_pentagon_all_diagonals_from_one_vertex.txt | graph_V5E7F4_1.txt |
| 15_octahedron.txt | octahedron_dual_tetrahedra.txt |
| 16_grid_3x3_section.txt | quadrilateral_4quad.txt |
| 17_fan_graph.txt | graph_V5E7F4_2.txt |
| 18_double_square.txt | graph_V6E7F3_1.txt |
| 19_triangulated_pentagon.txt | graph_V5E7F4_3.txt |
| 20_complex_wheel_5_spokes.txt | W5_wheel_1.txt |
| 21_nested_triangles.txt | graph_V7E9F3.txt |
| 22_bipyramid_base.txt | W5_wheel_2.txt |
| 23_planar_k5_subdivision.txt | triangulated_9tri.txt |
| 24_chain_of_quadrilaterals.txt | graph_V8E10F4.txt |
| 25_star_with_triangle_center.txt | triangulated_4tri_1.txt |
| 26_hexagon_with_internal_triangle.txt | graph_V6E7F3_2.txt |
| 27_double_fan.txt | graph_V7E14F9.txt |
| 28_triangulated_hexagon.txt | graph_V7E11F5.txt |
| 29_complex_biconnected_structure.txt | graph_V8E15F8.txt |
| 30_maximum_complexity_case.txt | graph_V12E25F12.txt |
| 9_gon.txt | C9_nonagon_cycle.txt |
| heptagon.txt | C7_heptagon_cycle.txt |
| hexagon.txt | C6_hexagon_cycle_1.txt |
| hexcycle.txt | C6_hexagon_cycle_2.txt |
| input.txt | graph_V5E6F3.txt |
| inputbiconnected.txt | graph_V6E7F3_3.txt |
| inputcycle.txt | C6_hexagon_cycle_3.txt |
| k23.txt | K2_3_bipartite.txt |
| k4.txt | K4_tetrahedron_2.txt |
| octagon.txt | C8_octagon_cycle.txt |
| pentagon.txt | C5_pentagon_cycle.txt |
| quadrilateral_with_diagonal.txt | diamond_2.txt |
| share_face_vertices.txt | graph_V9E10F3.txt |
| simple_quadrilateral.txt | C4_square_cycle.txt |
| simple_triangle.txt | K3_triangle.txt |
| supercomplex.txt | quadrilateral_7quad.txt |
| supercomplex2.txt | quadrilateral_14quad.txt |
| trinagle_with_3_ext.txt | triangulated_4tri_2.txt |
| two_quads_sharing_edge.txt | graph_V6E7F3_4.txt |
| two_triangles_sharing_edge.txt | diamond_3.txt |
| wheem_w4.txt | W4_wheel.txt |
