#include <bits/stdc++.h>
using namespace std;

// Direct implementation using combinatorial enumeration
vector<vector<pair<int, int>>> triangulationsForCycle(int n) {
    vector<vector<pair<int, int>>> result;
    
    if (n < 3) return result;
    if (n == 3) {
        result.push_back({});
        return result;
    }
    
    // Generate all possible diagonals (non-adjacent vertex pairs)
    vector<pair<int, int>> all_diagonals;
    for (int i = 1; i <= n; i++) {
        for (int j = i + 2; j <= n; j++) {
            if (!(i == 1 && j == n)) { // Exclude the edge (1, n) which is a boundary edge
                all_diagonals.push_back({i, j});
            }
        }
    }
    
    // We need exactly n-3 diagonals for a triangulation
    int needed_diagonals = n - 3;
    
    // Check if a set of diagonals forms a valid triangulation
    auto is_valid_triangulation = [&](const vector<pair<int, int>>& diagonals) -> bool {
        if (diagonals.size() != needed_diagonals) return false;
        
        // Check if diagonals don't intersect
        for (int i = 0; i < diagonals.size(); i++) {
            for (int j = i + 1; j < diagonals.size(); j++) {
                int a1 = diagonals[i].first, b1 = diagonals[i].second;
                int a2 = diagonals[j].first, b2 = diagonals[j].second;
                
                // Two diagonals (a1,b1) and (a2,b2) intersect if they cross
                // They cross if one diagonal has endpoints on opposite sides of the other
                if ((a1 < a2 && a2 < b1 && b1 < b2) || (a2 < a1 && a1 < b2 && b2 < b1)) {
                    return false;
                }
            }
        }
        
        // Additional check: ensure the diagonals actually triangulate the polygon
        // This is a more complex check, but for now we'll use the non-crossing property
        return true;
    };
    
    // Generate all combinations of needed_diagonals from all_diagonals
    function<void(int, vector<pair<int, int>>&)> generate_combinations = [&](int start, vector<pair<int, int>>& current) {
        if (current.size() == needed_diagonals) {
            if (is_valid_triangulation(current)) {
                vector<pair<int, int>> triangulation = current;
                sort(triangulation.begin(), triangulation.end());
                result.push_back(triangulation);
            }
            return;
        }
        
        if (start >= all_diagonals.size()) return;
        if (current.size() + (all_diagonals.size() - start) < needed_diagonals) return;
        
        // Include current diagonal
        current.push_back(all_diagonals[start]);
        generate_combinations(start + 1, current);
        current.pop_back();
        
        // Exclude current diagonal  
        generate_combinations(start + 1, current);
    };
    
    vector<pair<int, int>> current;
    generate_combinations(0, current);
    
    // Remove duplicates
    sort(result.begin(), result.end());
    result.erase(unique(result.begin(), result.end()), result.end());
    
    return result;
}

// Test function
int main() {
    cout << "Testing triangulations for cycles:" << endl;
    
    // Test for n = 4 (quadrilateral)
    cout << "\nFor n = 4 (vertices 1, 2, 3, 4):" << endl;
    auto result4 = triangulationsForCycle(4);
    cout << "Number of triangulations: " << result4.size() << endl;
    for (int i = 0; i < result4.size(); i++) {
        cout << "Triangulation " << i + 1 << ": {";
        for (const auto &chord : result4[i]) {
            cout << "(" << chord.first << "," << chord.second << ")";
            if (&chord != &result4[i].back()) cout << ", ";
        }
        cout << "}" << endl;
    }
    
    // Test for n = 5 (pentagon)
    cout << "\nFor n = 5 (vertices 1, 2, 3, 4, 5):" << endl;
    auto result5 = triangulationsForCycle(5);
    cout << "Number of triangulations: " << result5.size() << endl;
    for (int i = 0; i < result5.size(); i++) {
        cout << "Triangulation " << i + 1 << ": {";
        for (const auto &chord : result5[i]) {
            cout << "(" << chord.first << "," << chord.second << ")";
            if (&chord != &result5[i].back()) cout << ", ";
        }
        cout << "}" << endl;
    }
    
    // Test for n = 6 (hexagon)
    cout << "\nFor n = 6 (vertices 1, 2, 3, 4, 5, 6):" << endl;
    auto result6 = triangulationsForCycle(6);
    cout << "Number of triangulations: " << result6.size() << endl;
    for (int i = 0; i < result6.size(); i++) {
        cout << "Triangulation " << i + 1 << ": {";
        for (const auto &chord : result6[i]) {
            cout << "(" << chord.first << "," << chord.second << ")";
            if (&chord != &result6[i].back()) cout << ", ";
        }
        cout << "}" << endl;
    }
    
    return 0;
}
