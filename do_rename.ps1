$inputDir = "c:\Users\TAWKIR\Documents\GitHub\ThesisGraph\src\input"

$renames = @{
    "11_diamond.txt" = "diamond_1.txt"
    "12_bow_tie.txt" = "graph_V5E7F3.txt"
    "13_square_both_diagonals.txt" = "K4_tetrahedron_1.txt"
    "14_pentagon_all_diagonals_from_one_vertex.txt" = "graph_V5E7F4_1.txt"
    "15_octahedron.txt" = "octahedron_dual_tetrahedra.txt"
    "16_grid_3x3_section.txt" = "quadrilateral_4quad.txt"
    "17_fan_graph.txt" = "graph_V5E7F4_2.txt"
    "18_double_square.txt" = "graph_V6E7F3_1.txt"
    "19_triangulated_pentagon.txt" = "graph_V5E7F4_3.txt"
    "20_complex_wheel_5_spokes.txt" = "W5_wheel_1.txt"
    "21_nested_triangles.txt" = "graph_V7E9F3.txt"
    "22_bipyramid_base.txt" = "W5_wheel_2.txt"
    "23_planar_k5_subdivision.txt" = "triangulated_9tri.txt"
    "24_chain_of_quadrilaterals.txt" = "graph_V8E10F4.txt"
    "25_star_with_triangle_center.txt" = "triangulated_4tri_1.txt"
    "26_hexagon_with_internal_triangle.txt" = "graph_V6E7F3_2.txt"
    "27_double_fan.txt" = "graph_V7E14F9.txt"
    "28_triangulated_hexagon.txt" = "graph_V7E11F5.txt"
    "29_complex_biconnected_structure.txt" = "graph_V8E15F8.txt"
    "30_maximum_complexity_case.txt" = "graph_V12E25F12.txt"
    "9_gon.txt" = "C9_nonagon_cycle.txt"
    "heptagon.txt" = "C7_heptagon_cycle.txt"
    "hexagon.txt" = "C6_hexagon_cycle_1.txt"
    "hexcycle.txt" = "C6_hexagon_cycle_2.txt"
    "input.txt" = "graph_V5E6F3.txt"
    "inputbiconnected.txt" = "graph_V6E7F3_3.txt"
    "inputcycle.txt" = "C6_hexagon_cycle_3.txt"
    "k23.txt" = "K2_3_bipartite.txt"
    "k4.txt" = "K4_tetrahedron_2.txt"
    "octagon.txt" = "C8_octagon_cycle.txt"
    "pentagon.txt" = "C5_pentagon_cycle.txt"
    "quadrilateral_with_diagonal.txt" = "diamond_2.txt"
    "share_face_vertices.txt" = "graph_V9E10F3.txt"
    "simple_quadrilateral.txt" = "C4_square_cycle.txt"
    "simple_triangle.txt" = "K3_triangle.txt"
    "supercomplex.txt" = "quadrilateral_7quad.txt"
    "supercomplex2.txt" = "quadrilateral_14quad.txt"
    "trinagle_with_3_ext.txt" = "triangulated_4tri_2.txt"
    "two_quads_sharing_edge.txt" = "graph_V6E7F3_4.txt"
    "two_triangles_sharing_edge.txt" = "diamond_3.txt"
    "wheem_w4.txt" = "W4_wheel.txt"
}

$successCount = 0
$errorCount = 0

foreach ($oldName in $renames.Keys) {
    $newName = $renames[$oldName]
    $oldPath = Join-Path $inputDir $oldName
    
    if (Test-Path $oldPath) {
        try {
            Rename-Item -Path $oldPath -NewName $newName -ErrorAction Stop
            Write-Host "Renamed: $oldName -> $newName" -ForegroundColor Green
            $successCount++
        }
        catch {
            Write-Host "Error renaming $oldName" -ForegroundColor Red
            $errorCount++
        }
    }
    else {
        Write-Host "Not found: $oldName" -ForegroundColor Yellow
        $errorCount++
    }
}

Write-Host ""
Write-Host "Summary: $successCount renamed, $errorCount skipped/errors"
