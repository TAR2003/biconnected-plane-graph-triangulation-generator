#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <set>
#include <map>
#include <functional>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

/**
 * Complete Implementation of Sections 3 & 4:
 * "Generating All Triangulations of Plane Graphs"
 * Parvez, Rahman, Nakano (JGAA 2011)
 * 
 * This program:
 * 1. Reads triconnected plane graphs from input/*.txt
 * 2. Generates ALL triangulations of each graph
 * 3. Outputs full chord lists to output/*.txt
 * 
 * Algorithm: For each face, generate all triangulations (Section 4),
 * then combine them to get all triangulations of the graph (Section 3)
 */

// ============================================================================
// SECTION 4: Cycle Triangulation Generator (Core Algorithm)
// ============================================================================

/**
 * State for a single flip operation (for undo)
 */
struct FlipState {
    int j;              // Flipped chord (1, j)
    int prev_j;         // Previous neighbor in linked list
    int next_j;         // Next neighbor in linked list
    int old_b;          // Previous leftmost blocking chord endpoint
    bool old_has_block; // Whether parent had blocking chord
    int i;              // Neighbor > j
    int k;              // Neighbor < j
    int old_o1_i;       // Saved opposite pair for (1,i)
    int old_o2_k;       // Saved opposite pair for (1,k)
};

/**
 * Generates all triangulations of a single cycle (labeled vertices)
 * Section 4 of the paper: O(1) time per triangulation, O(n) space
 */
class CycleTriangulator {
public:
    explicit CycleTriangulator(int n_) : n(n_) {
        reset();
    }

    /**
     * Generate all triangulations and call callback for each
     * @param callback Function called with vector of ALL chords for each triangulation
     */
    void generate_all(function<void(const vector<pair<int,int>>&)> callback) {
        this->callback = callback;
        count = 0;
        
        // Output root triangulation
        output_current();
        
        // Generate all children via DFS (Section 4.4)
        for (int j = lastPresent; j >= firstPresent; j = prevIdx[j]) {
            flip_push(j);
            output_current();
            enumerate_children();
            undo_pop();
        }
    }

    long long get_count() const { return count; }

private:
    int n;                          // Number of vertices in cycle
    vector<char> present;           // present[j] = 1 if chord (1,j) exists
    vector<int> nextIdx, prevIdx;   // Doubly-linked list of present chords
    vector<int> o1, o2;             // Opposite pairs for candidate chords
    
    int firstPresent, lastPresent;  // Linked list boundaries
    bool has_block;                 // Whether blocking chord exists
    int b;                          // Leftmost blocking chord lower endpoint
    
    long long count;
    vector<FlipState> stk;          // Flip stack for backtracking
    function<void(const vector<pair<int,int>>&)> callback;
    set<pair<int,int>> all_chords;  // Track ALL chords including blocking ones

    void reset() {
        present.assign(n + 1, 0);
        nextIdx.assign(n + 1, 0);
        prevIdx.assign(n + 1, 0);
        o1.assign(n + 1, 0);
        o2.assign(n + 1, 0);

        // Root: all chords (1,j) for j = 3..n-1
        firstPresent = 3;
        lastPresent = n - 1;
        all_chords.clear();
        for (int j = 3; j <= n - 1; ++j) {
            present[j] = 1;
            nextIdx[j] = (j == n - 1 ? 0 : j + 1);
            prevIdx[j] = (j == 3 ? 0 : j - 1);
            o1[j] = j - 1;  // Opposite pair in root: (j-1, j+1)
            o2[j] = j + 1;
            all_chords.insert({1, j});
        }
        has_block = false;
        b = 3;
        stk.clear();
        count = 0;
    }

    /**
     * Output current triangulation using the complete chord set
     */
    void output_current() {
        ++count;
        vector<pair<int,int>> chords(all_chords.begin(), all_chords.end());
        callback(chords);
    }

    /**
     * Flip chord (1,j) in O(1) time (Section 4.2, Figures 13-14)
     */
    void flip_push(int j) {
        FlipState st{};
        st.j = j;
        st.prev_j = prevIdx[j];
        st.next_j = nextIdx[j];
        st.old_b = b;
        st.old_has_block = has_block;

        int i = nextIdx[j];  // Neighbor > j
        int k = prevIdx[j];  // Neighbor < j
        st.i = i;
        st.k = k;

        // Opposite pair before flip
        const int vo = o1[j];
        const int vop = o2[j];

        // Save affected opposite pairs
        if (i) st.old_o1_i = o1[i];
        if (k) st.old_o2_k = o2[k];

        // Remove (1,j) from linked list
        present[j] = 0;
        if (k) nextIdx[k] = i; else firstPresent = i;
        if (i) prevIdx[i] = k; else lastPresent = k;

        // Update chord set: remove (1,j), add blocking chord (vo,vop)
        all_chords.erase({1, j});
        all_chords.insert({min(vo,vop), max(vo,vop)});

        // Update opposite pairs (only 2 affected, Figures 13-14)
        if (i) o1[i] = vo;
        if (k) o2[k] = vop;

        // New leftmost blocking chord (Lemma 5)
        has_block = true;
        b = vo;

        stk.push_back(st);
    }

    /**
     * Undo flip in O(1) time
     */
    void undo_pop() {
        auto st = stk.back();
        stk.pop_back();

        int j = st.j;
        int i = st.i;
        int k = st.k;

        // Get opposite pair to restore chord set
        int vo = o1[j];
        int vop = o2[j];

        // Restore opposite pairs
        if (i) o1[i] = st.old_o1_i;
        if (k) o2[k] = st.old_o2_k;

        // Reinsert (1,j) into linked list
        present[j] = 1;
        prevIdx[j] = k;
        nextIdx[j] = i;
        if (k) nextIdx[k] = j; else firstPresent = j;
        if (i) prevIdx[i] = j; else lastPresent = j;

        // Restore chord set: remove blocking chord, add back (1,j)
        all_chords.erase({min(vo,vop), max(vo,vop)});
        all_chords.insert({1, j});

        // Restore blocking state
        has_block = st.old_has_block;
        b = st.old_b;
    }

    /**
     * Recursively enumerate children (Section 4.4)
     * Generating chords are (1,j) where j >= b among present chords
     */
    void enumerate_children() {
        if (!has_block) return;

        // Iterate over generating chords: j >= b
        for (int j = lastPresent; j && j >= b; j = prevIdx[j]) {
            flip_push(j);
            output_current();
            enumerate_children();
            undo_pop();
        }
    }
};

// ============================================================================
// SECTION 3: Triconnected Plane Graph Triangulation
// ============================================================================

/**
 * Represents a face of the plane graph as a cycle of vertices
 */
struct Face {
    int id;              // Face identifier
    vector<int> vertices; // Vertices on face boundary (in order)
    
    int size() const { return vertices.size(); }
};

/**
 * Represents a triconnected plane graph
 */
struct PlaneGraph {
    vector<Face> faces;
    set<int> all_vertices;  // All unique vertices in the graph
    
    void add_face(const Face& f) {
        faces.push_back(f);
        for (int v : f.vertices) {
            all_vertices.insert(v);
        }
    }
};

/**
 * A complete triangulation of a plane graph
 * Stores which triangulation index was chosen for each face
 */
struct GraphTriangulation {
    map<int, int> face_triangulation_index;  // face_id -> chosen triangulation
    map<int, vector<pair<int,int>>> face_chords; // face_id -> list of chords
};

/**
 * Main class for generating all triangulations of a triconnected plane graph
 * Section 3 of the paper
 */
class PlaneGraphTriangulator {
public:
    explicit PlaneGraphTriangulator(const PlaneGraph& g) : graph(g) {}

    /**
     * Generate all triangulations of the plane graph
     * Returns vector of all triangulations (each is a list of all chords)
     */
    vector<vector<pair<int,int>>> generate_all() {
        all_triangulations.clear();
        
        // Step 1: Generate all triangulations for each face (Section 4)
        for (const Face& face : graph.faces) {
            generate_face_triangulations(face);
        }
        
        // Step 2: Combine triangulations (Section 3)
        // Use Cartesian product: for each combination of face triangulations,
        // we get one triangulation of the entire graph
        GraphTriangulation current;
        combine_triangulations(0, current);
        
        return all_triangulations;
    }

private:
    PlaneGraph graph;
    // For each face, store all its possible triangulations
    map<int, vector<vector<pair<int,int>>>> face_triangulations;
    vector<vector<pair<int,int>>> all_triangulations;

    /**
     * Generate all triangulations for a single face (treat as cycle)
     */
    void generate_face_triangulations(const Face& face) {
        if (face.size() < 3) {
            // Degenerate case: face with < 3 vertices has no triangulation
            face_triangulations[face.id] = {{}};
            return;
        }
        
        if (face.size() == 3) {
            // Triangle: already triangulated, no chords needed
            face_triangulations[face.id] = {{}};
            return;
        }

        // For face with n vertices, we need to map them to 1..n for the algorithm
        // The algorithm assumes vertices are labeled 1, 2, ..., n
        map<int, int> vertex_to_index;  // Original vertex -> 1..n
        map<int, int> index_to_vertex;  // 1..n -> Original vertex
        
        for (size_t i = 0; i < face.vertices.size(); ++i) {
            vertex_to_index[face.vertices[i]] = i + 1;
            index_to_vertex[i + 1] = face.vertices[i];
        }

        int n = face.size();
        CycleTriangulator ct(n);
        
        vector<vector<pair<int,int>>> triangulations;
        
        // Generate all triangulations of this cycle
        ct.generate_all([&](const vector<pair<int,int>>& chords) {
            // Convert back from 1..n to original vertex labels
            vector<pair<int,int>> original_chords;
            for (const auto& chord : chords) {
                int u = index_to_vertex[chord.first];
                int v = index_to_vertex[chord.second];
                original_chords.push_back({min(u,v), max(u,v)});
            }
            triangulations.push_back(original_chords);
        });
        
        face_triangulations[face.id] = triangulations;
    }

    /**
     * Recursively combine triangulations from all faces
     * This generates the Cartesian product of all face triangulations
     */
    void combine_triangulations(int face_idx, GraphTriangulation& current) {
        if (face_idx == (int)graph.faces.size()) {
            // Base case: all faces processed, output this combination
            vector<pair<int,int>> all_chords;
            for (const auto& p : current.face_chords) {
                for (const auto& chord : p.second) {
                    all_chords.push_back(chord);
                }
            }
            // Remove duplicates (edges shared between faces)
            sort(all_chords.begin(), all_chords.end());
            all_chords.erase(unique(all_chords.begin(), all_chords.end()), all_chords.end());
            
            all_triangulations.push_back(all_chords);
            return;
        }

        // Recursive case: try each triangulation of current face
        const Face& face = graph.faces[face_idx];
        const auto& triangulations = face_triangulations[face.id];
        
        for (size_t i = 0; i < triangulations.size(); ++i) {
            current.face_triangulation_index[face.id] = i;
            current.face_chords[face.id] = triangulations[i];
            combine_triangulations(face_idx + 1, current);
        }
    }
};

// ============================================================================
// File I/O Functions
// ============================================================================

/**
 * Read a plane graph from input file
 * Format:
 *   num_faces
 *   num_vertices_face1
 *   v1 v2 v3 ...
 *   num_vertices_face2
 *   v1 v2 v3 ...
 *   ...
 */
PlaneGraph read_graph_from_file(const string& filename) {
    ifstream infile(filename);
    if (!infile.is_open()) {
        cerr << "Error: Cannot open file " << filename << endl;
        exit(1);
    }

    PlaneGraph graph;
    int num_faces;
    infile >> num_faces;

    for (int i = 0; i < num_faces; ++i) {
        Face face;
        face.id = i;
        
        int num_vertices;
        infile >> num_vertices;
        
        for (int j = 0; j < num_vertices; ++j) {
            int v;
            infile >> v;
            face.vertices.push_back(v);
        }
        
        graph.add_face(face);
    }

    infile.close();
    return graph;
}

/**
 * Write all triangulations to output file
 * Format:
 *   total_count  // Total number of triangulations
 *   chord1_u chord1_v chord2_u chord2_v ...  // First triangulation
 *   chord1_u chord1_v chord2_u chord2_v ...  // Second triangulation
 *   ...
 */
void write_triangulations_to_file(const string& filename, 
                                   const vector<vector<pair<int,int>>>& triangulations) {
    ofstream outfile(filename);
    if (!outfile.is_open()) {
        cerr << "Error: Cannot open output file " << filename << endl;
        exit(1);
    }

    // Write total count
    outfile << triangulations.size() << " // Total triangulations" << endl;

    // Write each triangulation
    for (const auto& tri : triangulations) {
        if (tri.empty()) {
            outfile << "// No chords (already triangulated)" << endl;
        } else {
            for (const auto& chord : tri) {
                outfile << "(" << chord.first << "," << chord.second << ") ";
            }
            outfile << endl;
        }
    }

    outfile.close();
}

// ============================================================================
// Main Program
// ============================================================================

int main() {
    cout << "==================================================================" << endl;
    cout << "Triconnected Plane Graph Triangulation Generator" << endl;
    cout << "Based on: Parvez, Rahman, Nakano (JGAA 2011)" << endl;
    cout << "Sections 3 & 4: Complete Implementation" << endl;
    cout << "==================================================================" << endl;
    cout << endl;

    // Process all input files
    string input_dir = "input";
    string output_dir = "output";

    int files_processed = 0;

    // Iterate through all .txt files in input directory
    for (const auto& entry : fs::directory_iterator(input_dir)) {
        if (entry.path().extension() == ".txt") {
            string input_file = entry.path().string();
            string filename = entry.path().stem().string();
            string output_file = output_dir + "/" + filename + "_output.txt";

            cout << "Processing: " << filename << ".txt" << endl;

            // Read graph from file
            PlaneGraph graph = read_graph_from_file(input_file);
            
            cout << "  Faces: " << graph.faces.size() << endl;
            cout << "  Vertices: " << graph.all_vertices.size() << endl;

            // Generate all triangulations
            PlaneGraphTriangulator generator(graph);
            auto triangulations = generator.generate_all();

            cout << "  Triangulations generated: " << triangulations.size() << endl;

            // Write to output file
            write_triangulations_to_file(output_file, triangulations);
            
            cout << "  Output written to: " << output_file << endl;
            cout << endl;

            files_processed++;
        }
    }

    if (files_processed == 0) {
        cout << "No input files found in 'input/' directory." << endl;
        cout << "Please create .txt files with plane graph descriptions." << endl;
    } else {
        cout << "==================================================================" << endl;
        cout << "Total files processed: " << files_processed << endl;
        cout << "All outputs written to 'output/' directory." << endl;
        cout << "==================================================================" << endl;
    }

    return 0;
}
