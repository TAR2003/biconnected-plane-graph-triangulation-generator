#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <map>

using namespace std;

/**
 * Implementation of "Generating All Triangulations of Plane Graphs"
 * by Parvez, Rahman, and Nakano (JGAA 2011)
 * 
 * Algorithm: Generates all triangulations of a cycle C of n labeled vertices
 * Time Complexity: O(1) per triangulation
 * Space Complexity: O(n)
 * 
 * Key Concepts from the Paper:
 * - Triangulation: decomposition of a cycle into triangles by non-intersecting chords
 * - Root vertex: v1, all chords in root triangulation are incident to v1
 * - Blocking chord: chord (vb, vb') where both endpoints are adjacent to v1
 * - Leftmost blocking chord: blocking chord with highest vb' value
 * - Generating chord: chord (v1, vj) that can be flipped to generate children
 * - Flipping: edge flip operation that transforms one triangulation to another
 * - Genealogical tree: tree structure where each node is a triangulation
 */

// Structure to represent an edge (chord)
// Edges are stored with u < v convention for consistency
struct Edge {
    int u, v;  // u < v by convention
    
    Edge(int a, int b) {
        if (a < b) {
            u = a;
            v = b;
        } else {
            u = b;
            v = a;
        }
    }
    
    bool operator<(const Edge& other) const {
        if (u != other.u) return u < other.u;
        return v < other.v;
    }
    
    bool operator==(const Edge& other) const {
        return u == other.u && v == other.v;
    }
};

/**
 * Class to represent a triangulation of a cycle
 * 
 * Data Structures (as per Section 4.3 of the paper):
 * - L: List of all chords in the triangulation
 * - GS: Generating set - chords that can be flipped to generate children
 * - O: Opposite pairs - for each generating chord (v1,vj), stores (vo, vo')
 *      where <v1, vo, vj, vo'> forms a quadrilateral
 */
class Triangulation {
public:
    int n;  // Number of vertices in the cycle (v1, v2, ..., vn)
    vector<Edge> L;  // List of chords (n-3 chords for a triangulation)
    vector<Edge> GS;  // Generating set of chords
    vector<pair<int, int>> O;  // Opposite pairs corresponding to GS
    
    Triangulation(int vertices) : n(vertices) {}
    
    // Copy constructor for creating child triangulations
    Triangulation(const Triangulation& t) : n(t.n), L(t.L), GS(t.GS), O(t.O) {}
    
    // Print the triangulation (outputs the chord list)
    void print() const {
        cout << "Chords: ";
        for (const auto& e : L) {
            cout << "(" << e.u << "," << e.v << ") ";
        }
        cout << endl;
    }
};

// Global counter for triangulations
int triangulation_count = 0;

/**
 * Find the leftmost blocking chord in a triangulation (Section 4.1)
 * 
 * Definition: A chord (vb, vb') is a BLOCKING CHORD if:
 *  1. Neither vb nor vb' equals v1 (not incident to root vertex)
 *  2. Both vb and vb' are adjacent to v1 (either via chord or cycle boundary)
 * 
 * The LEFTMOST blocking chord is the one with the HIGHEST vb' value
 * This is used to determine the generating set of a triangulation
 * 
 * Returns: pair (vb, vb') of the leftmost blocking chord, or (-1, -1) if none exists
 */
pair<int, int> findLeftmostBlockingChord(const Triangulation& T) {
    int max_vb_prime = -1;
    pair<int, int> blocking_chord = {-1, -1};
    
    // Find all chords not incident to vertex 1
    for (const auto& chord : T.L) {
        // A chord (vb, vb') is blocking if neither endpoint is vertex 1
        if (chord.u != 1 && chord.v != 1) {
            // Check if both endpoints are adjacent to v1
            bool u_adjacent = false, v_adjacent = false;
            
            for (const auto& c : T.L) {
                if ((c.u == 1 && c.v == chord.u) || (c.u == chord.u && c.v == 1)) {
                    u_adjacent = true;
                }
                if ((c.u == 1 && c.v == chord.v) || (c.u == chord.v && c.v == 1)) {
                    v_adjacent = true;
                }
            }
            
            // Check adjacency on the cycle boundary
            if (chord.u == 2 || chord.u == T.n) u_adjacent = true;
            if (chord.v == 2 || chord.v == T.n) v_adjacent = true;
            
            if (u_adjacent && v_adjacent) {
                if (chord.v > max_vb_prime) {
                    max_vb_prime = chord.v;
                    blocking_chord = {chord.u, chord.v};
                }
            }
        }
    }
    
    return blocking_chord;
}

/**
 * Initialize the root triangulation for a cycle (Section 4.3)
 * 
 * The root triangulation has the following properties:
 * - All chords are incident to vertex v1 (full vision from v1)
 * - Chords: (v1,v3), (v1,v4), ..., (v1,vn-1)
 * - All chords are generating chords in the root
 * - For each chord (v1,vj), the opposite pair is (vj-1, vj+1)
 * 
 * Returns: Root triangulation with n-3 chords
 */
Triangulation initializeRoot(int n) {
    Triangulation root(n);
    
    // Add all chords from v1 to other vertices (except v2 which is already connected)
    for (int j = 3; j <= n - 1; j++) {
        root.L.push_back(Edge(1, j));
    }
    
    // For the root, all chords are generating chords
    root.GS = root.L;
    
    // Initialize opposite pairs for each generating chord
    // For chord (v1, vj), opposite pair is (vj-1, vj+1)
    for (const auto& chord : root.GS) {
        int j = chord.v;
        int vo = j - 1;
        int vo_prime = j + 1;
        root.O.push_back({vo, vo_prime});
    }
    
    return root;
}

// Find the opposite pair for a chord (v1, vj) in triangulation T
pair<int, int> findOppositePair(const Triangulation& T, const Edge& chord) {
    // Find index of the chord in GS
    for (size_t i = 0; i < T.GS.size(); i++) {
        if (T.GS[i] == chord) {
            return T.O[i];
        }
    }
    return {-1, -1};
}

/**
 * Flip a generating chord and create a new child triangulation (Section 4.2)
 * 
 * Flipping Operation:
 * 1. Given chord (v1, vj) with opposite pair (vo, vo')
 * 2. Remove chord (v1, vj) from triangulation
 * 3. Add chord (vo, vo') creating a blocking chord
 * 4. Update the generating set based on the new leftmost blocking chord
 * 5. Recompute opposite pairs for the new generating set
 * 
 * Time Complexity: O(1) per flip (as per Theorem 2)
 * 
 * Returns: New triangulation after flipping
 */
Triangulation flipChord(const Triangulation& T, const Edge& chord_to_flip) {
    Triangulation T_new(T);
    
    // Find the opposite pair for this chord
    pair<int, int> opp = findOppositePair(T, chord_to_flip);
    int vo = opp.first;
    int vo_prime = opp.second;
    
    // Remove the old chord from L
    T_new.L.erase(remove(T_new.L.begin(), T_new.L.end(), chord_to_flip), T_new.L.end());
    
    // Add the new blocking chord (vo, vo_prime)
    T_new.L.push_back(Edge(vo, vo_prime));
    
    // Update the generating set
    // The leftmost blocking chord determines which chords are generating
    pair<int, int> blocking = findLeftmostBlockingChord(T_new);
    
    T_new.GS.clear();
    T_new.O.clear();
    
    if (blocking.first == -1) {
        // No blocking chord, all chords incident to v1 are generating
        for (const auto& c : T_new.L) {
            if (c.u == 1) {
                T_new.GS.push_back(c);
            }
        }
    } else {
        // Only chords (v1, vj) where vj >= blocking.first are generating
        int b = blocking.first;
        for (const auto& c : T_new.L) {
            if (c.u == 1 && c.v >= b) {
                T_new.GS.push_back(c);
            }
        }
    }
    
    // Compute opposite pairs for the new generating set
    for (const auto& gen_chord : T_new.GS) {
        int j = gen_chord.v;
        
        // Find vertices forming quadrilateral with v1 and vj
        // We need to find vo and vo' such that <v1, vo, vj, vo'> is a quadrilateral
        
        int vo_val = -1, vo_prime_val = -1;
        
        // Look through all chords to find the quadrilateral
        for (const auto& c : T_new.L) {
            if (c.v == j && c.u != 1) {
                vo_val = c.u;
            }
            if (c.u == j && c.v != 1 && c.v > j) {
                vo_prime_val = c.v;
            }
        }
        
        // Check boundary connections
        if (vo_val == -1) {
            // Check if vj-1 is visible from v1
            if (j > 2) {
                bool found = false;
                for (const auto& c : T_new.L) {
                    if ((c.u == 1 && c.v == j-1) || (c.u == j-1 && c.v == 1)) {
                        found = true;
                        break;
                    }
                }
                if (!found && j == 3) found = true;  // v2 is always adjacent to v1 on boundary
                if (found) vo_val = j - 1;
            }
        }
        
        if (vo_prime_val == -1) {
            // Check if vj+1 is visible from v1
            if (j < T_new.n) {
                bool found = false;
                for (const auto& c : T_new.L) {
                    if ((c.u == 1 && c.v == j+1) || (c.u == j+1 && c.v == 1)) {
                        found = true;
                        break;
                    }
                }
                if (!found && j == T_new.n - 1) found = true;  // vn is always adjacent to v1 on boundary
                if (found) vo_prime_val = j + 1;
            }
        }
        
        T_new.O.push_back({vo_val, vo_prime_val});
    }
    
    return T_new;
}

/**
 * Recursive function to generate all child triangulations (Section 4.4)
 * 
 * This implements the DFS traversal of the genealogical tree T(C)
 * 
 * Algorithm from paper (Procedure find-all-child-triangulations-cycle):
 * 1. Output current triangulation T
 * 2. If T has no generating chords, return (leaf node)
 * 3. For each generating chord e in T:
 *    a. Flip e to create child T'
 *    b. Recursively generate all children of T'
 * 
 * The genealogical tree structure ensures:
 * - No duplications (unique parent-child relationship via leftmost blocking chord)
 * - No omissions (Lemma 3: every triangulation has unique path from root)
 */
void findAllChildTriangulationsCycle(const Triangulation& T) {
    // Output current triangulation
    triangulation_count++;
    T.print();
    
    // If no generating chords, this is a leaf node in the tree
    if (T.GS.empty()) {
        return;
    }
    
    // Generate all children by flipping each generating chord
    // By Lemma 6, flipping a generating chord preserves parent-child relationship
    for (const auto& gen_chord : T.GS) {
        Triangulation T_child = flipChord(T, gen_chord);
        findAllChildTriangulationsCycle(T_child);
    }
}

// Main algorithm to generate all triangulations of a cycle
void findAllTriangulationsCycle(int n) {
    cout << "Generating all triangulations of a cycle with " << n << " vertices\n";
    cout << "=================================================================\n\n";
    
    triangulation_count = 0;
    
    // Initialize and output root
    Triangulation root = initializeRoot(n);
    cout << "Root triangulation:\n";
    triangulation_count++;
    root.print();
    cout << endl;
    
    // Generate all children of root
    // For root, we flip each generating chord
    for (const auto& gen_chord : root.GS) {
        Triangulation T_child = flipChord(root, gen_chord);
        findAllChildTriangulationsCycle(T_child);
    }
    
    cout << "\n=================================================================\n";
    cout << "Total triangulations generated: " << triangulation_count << "\n";
    
    // Calculate expected Catalan number
    // C(n-2) where C(k) is the k-th Catalan number
    // For n=6: C(4) = 14
    // For n=7: C(5) = 42
    int k = n - 2;
    long long catalan = 1;
    for (int i = 0; i < k; i++) {
        catalan = catalan * (2 * k - i) / (i + 1);
    }
    catalan = catalan / (k + 1);
    
    cout << "Expected (Catalan number C(" << k << ")): " << catalan << "\n";
    
    if (triangulation_count == catalan) {
        cout << "✓ CORRECT! Count matches Catalan number.\n";
    } else {
        cout << "✗ ERROR! Count does not match Catalan number.\n";
    }
}

int main() {
    cout << "Triangulation Generation Algorithm\n";
    cout << "Based on: Parvez, Rahman, Nakano 2011\n";
    cout << "Paper: Generating All Triangulations of Plane Graphs\n\n";
    
    // Test for n=6 (should generate 14 triangulations)
    findAllTriangulationsCycle(6);
    
    cout << "\n\n";
    
    // Test for n=7 (should generate 42 triangulations)
    findAllTriangulationsCycle(7);
    
    return 0;
}
