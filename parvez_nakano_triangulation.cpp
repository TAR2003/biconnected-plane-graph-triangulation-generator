#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <numeric>
#include <string>
#include <set>
#include <sstream>

// A chord is represented by a pair of vertex indices.
using Chord = std::pair<int, int>;

// A triangulation is a set of chords. Using a set provides a canonical representation.
using Triangulation = std::set<Chord>;

// A set to keep track of visited triangulations to avoid redundant computations.
std::set<Triangulation> visited;

// Forward declaration of the main generation function.
void generate(int n, Triangulation t);

// Helper function to print a single triangulation in a readable format.
void printTriangulation(int n, const Triangulation& t) {
    std::stringstream ss;
    ss << "n=" << n << ", chords: {";
    bool first = true;
    for (const auto& chord : t) {
        if (!first) {
            ss << ", ";
        }
        ss << "(" << chord.first << "," << chord.second << ")";
        first = false;
    }
    ss << "}";
    std::cout << ss.str() << std::endl;
}

// Checks if two vertices are neighbors (either by a boundary edge or a chord).
bool is_neighbor(int u, int v, int n, const Triangulation& t) {
    if ((u + 1) % n == v || (v + 1) % n == u) {
        return true; // Boundary edge
    }
    if (u > v) std::swap(u, v);
    return t.count({u, v}); // Chord
}

// For a given chord (u, v), finds the other two vertices of the quadrilateral
// in which the chord is a diagonal.
std::pair<int, int> findOppositeQuadVertices(int u, int v, int n, const Triangulation& t) {
    std::vector<int> common_neighbors;
    for (int i = 0; i < n; ++i) {
        if (i == u || i == v) continue;
        if (is_neighbor(u, i, n, t) && is_neighbor(v, i, n, t)) {
            common_neighbors.push_back(i);
        }
    }
    if (common_neighbors.size() == 2) {
        return {common_neighbors[0], common_neighbors[1]};
    }
    // This case should not be reached in a valid triangulation of a convex polygon.
    return {-1, -1};
}

// The main recursive function to generate triangulations.
void generate(int n, Triangulation t) {
    if (visited.count(t)) {
        return;
    }
    visited.insert(t);

    printTriangulation(n, t);

    // 1. Find the Leftmost Blocking Chord (LBC).
    // A blocking chord does not involve the root vertex (0).
    // The "leftmost" is defined by maximizing the second vertex index, then the first.
    Chord lbc = {-1, -1};
    bool is_root = true;
    for (const auto& chord : t) {
        if (chord.first != 0) {
            is_root = false;
            if (lbc.first == -1 || chord.second > lbc.second || (chord.second == lbc.second && chord.first > lbc.first)) {
                lbc = chord;
            }
        }
    }

    // 2. Find generating chords and recurse for children.
    if (is_root) {
        // Case 1: The current triangulation is the root of the tree.
        // All its chords are generating. For a root (fan) triangulation, flipping
        // chord (0, j) results in a new chord (j-1, j+1).
        for (int j = n - 2; j >= 2; --j) {
            Chord to_flip = {0, j};
            Chord new_chord = {j - 1, j + 1};
            if (new_chord.first > new_chord.second) std::swap(new_chord.first, new_chord.second);
            
            Triangulation next_t = t;
            next_t.erase(to_flip);
            next_t.insert(new_chord);
            generate(n, next_t);
        }
    } else {
        // Case 2: Not a root triangulation.
        // A chord (0, j) is generating if j is greater than or equal to the
        // first vertex of the LBC.
        int b = lbc.first;
        
        for (const auto& chord_to_flip : t) {
            if (chord_to_flip.first == 0) {
                int j = chord_to_flip.second;
                if (j >= b) {
                    // This is a generating chord, flip it to create a child.
                    std::pair<int, int> quad_verts = findOppositeQuadVertices(0, j, n, t);
                    if (quad_verts.first != -1) {
                        Chord new_chord = {quad_verts.first, quad_verts.second};
                        if (new_chord.first > new_chord.second) std::swap(new_chord.first, new_chord.second);

                        Triangulation next_t = t;
                        next_t.erase(chord_to_flip);
                        next_t.insert(new_chord);
                        generate(n, next_t);
                    }
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    int n;
    if (argc > 1) {
        n = std::stoi(argv[1]);
    } else {
        std::cout << "Enter the number of vertices (n): ";
        std::cin >> n;
    }

    if (n < 3) {
        std::cout << "Number of vertices must be at least 3." << std::endl;
        return 1;
    }
    if (n == 3) {
        printTriangulation(n, {});
        return 0;
    }

    // The root of the generation tree is the "fan" triangulation, where all
    // chords are incident to vertex 0.
    Triangulation root_t;
    for (int j = 2; j < n - 1; ++j) {
        root_t.insert({0, j});
    }

    generate(n, root_t);

    return 0;
}
