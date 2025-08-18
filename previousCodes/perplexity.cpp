#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <map>
#include <set>
#include <list>
#include <cassert>

/**
 * Implementation of the algorithm from "Generating All Triangulations of Plane Graphs"
 * by Parvez, Rahman, and Nakano (2011)
 *
 * This implementation follows the paper's approach:
 * 1. For each face (cycle) of the triconnected plane graph
 * 2. Generate all triangulations of each cycle
 * 3. Combine triangulations to get all graph triangulations
 */

struct Edge
{
    int u, v;
    Edge(int u = 0, int v = 0) : u(u), v(v)
    {
        // Ensure first element is always smaller (as per requirement)
        if (this->u > this->v)
            std::swap(this->u, this->v);
    }

    bool operator<(const Edge &other) const
    {
        if (u != other.u)
            return u < other.u;
        return v < other.v;
    }

    bool operator==(const Edge &other) const
    {
        return u == other.u && v == other.v;
    }
};

/**
 * Class to represent a triangulation of a cycle
 * Based on Section 4 of the paper
 */
class CycleTriangulation
{
private:
    std::vector<int> cycle;                          // Vertices of the cycle in order
    std::vector<Edge> chords;                        // List L: chords of triangulation
    std::vector<Edge> generating_set;                // Set GS: generating chords
    std::vector<std::pair<int, int>> opposite_pairs; // Set O: opposite pairs
    int root_vertex;                                 // Root vertex (v1 in paper)

public:
    CycleTriangulation(const std::vector<int> &vertices) : cycle(vertices)
    {
        root_vertex = vertices[0]; // Choose first vertex as root
        initialize_root_triangulation();
    }

    /**
     * Initialize root triangulation where all chords are incident to root vertex
     * Section 4.3 of the paper
     */
    void initialize_root_triangulation()
    {
        int n = cycle.size();
        chords.clear();
        generating_set.clear();
        opposite_pairs.clear();

        // Root triangulation: all chords from v1 to other vertices
        // L = {(v1, v_{n-1}), (v1, v_{n-2}), ..., (v1, v3)}
        for (int j = n - 1; j >= 2; j--)
        {
            Edge chord(root_vertex, cycle[j]);
            chords.push_back(chord);
            generating_set.push_back(chord);

            // Opposite pair: (v_{j-1}, v_{j+1})
            int vo = cycle[j - 1];
            int vo_prime = cycle[(j + 1) % n];
            opposite_pairs.push_back({vo, vo_prime});
        }
    }

    /**
     * Get all triangulations of this cycle
     * Implements Algorithm find-all-triangulations-cycle from Section 4.4
     */
    void generate_all_triangulations(std::vector<std::vector<Edge>> &all_triangulations)
    {
        std::vector<Edge> current_triangulation = chords;
        all_triangulations.push_back(current_triangulation);

        // For root triangulation, all chords are generating (Case 1)
        for (int i = generating_set.size() - 1; i >= 0; i--)
        {
            Edge generating_chord = generating_set[i];
            generate_children_recursive(generating_chord, all_triangulations);
        }
    }

private:
    /**
     * Recursive function to generate all child triangulations
     * Implements find-all-child-triangulations-cycle from the paper
     */
    void generate_children_recursive(const Edge &chord_to_flip,
                                     std::vector<std::vector<Edge>> &all_triangulations)
    {
        // Create child triangulation by flipping the generating chord
        std::vector<Edge> child_chords = chords;
        std::vector<Edge> child_generating_set;

        // Find and flip the chord
        auto it = std::find(child_chords.begin(), child_chords.end(), chord_to_flip);
        if (it != child_chords.end())
        {
            // Remove the flipped chord and add its opposite
            child_chords.erase(it);

            // Find opposite pair for this chord
            size_t chord_index = std::distance(generating_set.begin(),
                                               std::find(generating_set.begin(),
                                                         generating_set.end(), chord_to_flip));

            if (chord_index < opposite_pairs.size())
            {
                Edge new_chord(opposite_pairs[chord_index].first,
                               opposite_pairs[chord_index].second);
                child_chords.push_back(new_chord);
            }

            all_triangulations.push_back(child_chords);

            // Update generating set for child (simplified for this implementation)
            update_generating_set_for_child(child_chords, child_generating_set);

            // Recursively generate children of this child
            for (const Edge &gen_chord : child_generating_set)
            {
                // Save current state
                auto saved_chords = chords;
                auto saved_gen_set = generating_set;

                // Set child as current
                chords = child_chords;
                generating_set = child_generating_set;

                // Generate children
                generate_children_recursive(gen_chord, all_triangulations);

                // Restore state
                chords = saved_chords;
                generating_set = saved_gen_set;
            }
        }
    }

    /**
     * Update generating set for child triangulation
     * Based on Section 4.2 of the paper
     */
    void update_generating_set_for_child(const std::vector<Edge> &child_chords,
                                         std::vector<Edge> &child_generating_set)
    {
        child_generating_set.clear();

        // Find leftmost blocking chord
        Edge leftmost_blocking(-1, -1);
        bool has_blocking = false;

        for (const Edge &chord : child_chords)
        {
            // Check if chord is blocking (both endpoints adjacent to root)
            if (is_blocking_chord(chord))
            {
                if (!has_blocking || chord.v < leftmost_blocking.v)
                {
                    leftmost_blocking = chord;
                    has_blocking = true;
                }
            }
        }

        // Generating chords are those (v1, vj) where j >= b
        // where b is the index of leftmost blocking chord
        int threshold = has_blocking ? leftmost_blocking.v : 0;

        for (const Edge &chord : child_chords)
        {
            if (chord.u == root_vertex && chord.v >= threshold)
            {
                child_generating_set.push_back(chord);
            }
        }
    }

    /**
     * Check if a chord is blocking (both endpoints adjacent to root vertex)
     */
    bool is_blocking_chord(const Edge &chord) const
    {
        return chord.u != root_vertex && chord.v != root_vertex;
    }
};

/**
 * Main class for generating all triangulations of a triconnected plane graph
 * Implements Algorithm find-all-triangulations from Section 3.2
 */
class TriconnectedGraphTriangulator
{
private:
    std::vector<std::vector<int>> faces;                             // Input faces of the graph
    std::vector<std::vector<std::vector<Edge>>> face_triangulations; // All triangulations per face

public:
    TriconnectedGraphTriangulator(const std::vector<std::vector<int>> &input_faces)
        : faces(input_faces) {}

    /**
     * Generate all triangulations of the triconnected plane graph
     * Returns vector of chord sets, each representing one triangulation
     */
    std::vector<std::vector<Edge>> generate_all_triangulations()
    {
        std::vector<std::vector<Edge>> all_graph_triangulations;

        // Step 1: Generate all triangulations for each face (cycle)
        face_triangulations.resize(faces.size());

        for (size_t i = 0; i < faces.size(); i++)
        {
            CycleTriangulation cycle_triangulator(faces[i]);
            cycle_triangulator.generate_all_triangulations(face_triangulations[i]);
        }

        // Step 2: Combine triangulations from all faces using Cartesian product
        // This implements the core idea from Section 3 of the paper
        generate_combinations(0, std::vector<Edge>(), all_graph_triangulations);

        return all_graph_triangulations;
    }

private:
    /**
     * Generate all combinations of face triangulations
     * Implements the combination strategy from the paper
     */
    void generate_combinations(size_t face_index,
                               std::vector<Edge> current_combination,
                               std::vector<std::vector<Edge>> &result)
    {
        if (face_index == faces.size())
        {
            // All faces processed, add this combination
            result.push_back(current_combination);
            return;
        }

        // Try each triangulation of the current face
        for (const auto &face_triangulation : face_triangulations[face_index])
        {
            std::vector<Edge> new_combination = current_combination;

            // Add chords from this face triangulation
            for (const Edge &chord : face_triangulation)
            {
                new_combination.push_back(chord);
            }

            // Recursively process next face
            generate_combinations(face_index + 1, new_combination, result);
        }
    }
};

/**
 * Utility functions for input/output
 */
class FileHandler
{
public:
    /**
     * Read input from file
     * Expected format:
     * First line: number of faces
     * Following lines: vertices of each face
     */
    static std::vector<std::vector<int>> read_input(const std::string &filename)
    {
        std::ifstream file(filename);
        std::vector<std::vector<int>> faces;

        if (!file.is_open())
        {
            std::cerr << "Error: Cannot open input file " << filename << std::endl;
            return faces;
        }

        int num_faces;
        file >> num_faces;

        for (int i = 0; i < num_faces; i++)
        {
            int face_size;
            file >> face_size;

            std::vector<int> face(face_size);
            for (int j = 0; j < face_size; j++)
            {
                file >> face[j];
            }
            faces.push_back(face);
        }

        file.close();
        return faces;
    }

    /**
     * Write output to file
     * Format: Each line contains one triangulation as list of chord pairs
     */
    static void write_output(const std::string &filename,
                             const std::vector<std::vector<Edge>> &triangulations)
    {
        std::ofstream file(filename);

        if (!file.is_open())
        {
            std::cerr << "Error: Cannot open output file " << filename << std::endl;
            return;
        }

        file << "Total triangulations found: " << triangulations.size() << std::endl;
        file << std::endl;

        for (size_t i = 0; i < triangulations.size(); i++)
        {
            file << "Triangulation " << (i + 1) << ": ";

            for (size_t j = 0; j < triangulations[i].size(); j++)
            {
                const Edge &chord = triangulations[i][j];
                file << "(" << chord.u << "," << chord.v << ")";
                if (j < triangulations[i].size() - 1)
                    file << ", ";
            }
            file << std::endl;
        }

        file.close();
    }

    /**
     * Print results to console
     */
    static void print_results(const std::vector<std::vector<Edge>> &triangulations)
    {
        std::cout << "\n=== TRIANGULATION RESULTS ===" << std::endl;
        std::cout << "Total triangulations found: " << triangulations.size() << std::endl;
        std::cout << std::endl;

        for (size_t i = 0; i < triangulations.size(); i++)
        {
            std::cout << "Triangulation " << (i + 1) << ": ";

            for (size_t j = 0; j < triangulations[i].size(); j++)
            {
                const Edge &chord = triangulations[i][j];
                std::cout << "(" << chord.u << "," << chord.v << ")";
                if (j < triangulations[i].size() - 1)
                    std::cout << ", ";
            }
            std::cout << std::endl;
        }
    }
};

/**
 * Main function demonstrating the algorithm
 */
int main()
{
    std::cout << "=== Triconnected Plane Graph Triangulation Generator ===" << std::endl;
    std::cout << "Based on Parvez, Rahman, and Nakano (2011) algorithm" << std::endl;
    std::cout << std::endl;

    // Example 1: Direct input as shown in the problem statement
    std::cout << "Example 1: Direct input {{1,2,3,4}, {4,5,6,7}}" << std::endl;
    std::vector<std::vector<int>> example_faces = {{1, 2, 3, 4}, {4, 5, 6, 7}};

    TriconnectedGraphTriangulator triangulator(example_faces);
    auto triangulations = triangulator.generate_all_triangulations();

    FileHandler::print_results(triangulations);
    FileHandler::write_output("output_example1.txt", triangulations);

    std::cout << "\nResults written to output_example1.txt" << std::endl;

    // Example 2: Read from input file
    std::cout << "\n"
              << std::string(50, '=') << std::endl;
    std::cout << "Example 2: Reading from input file" << std::endl;

    // Create sample input file
    std::ofstream sample_input("input.txt");
    sample_input << "2" << std::endl;         // 2 faces
    sample_input << "4 1 2 3 4" << std::endl; // Face 1: vertices 1,2,3,4
    sample_input << "4 4 5 6 7" << std::endl; // Face 2: vertices 4,5,6,7
    sample_input.close();

    auto file_faces = FileHandler::read_input("input.txt");

    if (!file_faces.empty())
    {
        TriconnectedGraphTriangulator file_triangulator(file_faces);
        auto file_triangulations = file_triangulator.generate_all_triangulations();

        FileHandler::print_results(file_triangulations);
        FileHandler::write_output("output.txt", file_triangulations);

        std::cout << "\nResults written to output.txt" << std::endl;
    }

    // Example 3: Triangle face
    std::cout << "\n"
              << std::string(50, '=') << std::endl;
    std::cout << "Example 3: Single triangular face {1,2,3}" << std::endl;

    std::vector<std::vector<int>> triangle_face = {{1, 2, 3}};
    TriconnectedGraphTriangulator triangle_triangulator(triangle_face);
    auto triangle_triangulations = triangle_triangulator.generate_all_triangulations();

    FileHandler::print_results(triangle_triangulations);

    return 0;
}
