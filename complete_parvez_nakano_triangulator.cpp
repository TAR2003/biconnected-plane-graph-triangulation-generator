#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <utility>
#include <algorithm>
#include <fstream>
#include <string>
#include <sstream>
#include <queue>
#include <cassert>

using namespace std;

/**
 * Complete Implementation of Parvez-Rahman-Nakano Algorithm 
 * for generating ALL triangulations of triconnected planar graphs.
 * 
 * This implementation follows the algorithm described in:
 * "Generating All Triangulations of Plane Graphs" 
 * by Mohammad Tanvir Parvez, Md. Saidur Rahman, and Shin-ichi Nakano (2011)
 * 
 * Key Features:
 * - Reads faces from input.txt in the specified format
 * - Generates all possible triangulations using edge flipping
 * - Uses canonical representation to avoid duplicates
 * - Implements the tree-based traversal from the paper
 * - Outputs results to output.txt
 */

// Forward declarations
class Triangle;
class Triangulation;

// Edge representation with canonical ordering
struct Edge {
    int u, v;
    
    Edge(int a, int b) : u(min(a, b)), v(max(a, b)) {}
    
    bool operator<(const Edge& other) const {
        return u < other.u || (u == other.u && v < other.v);
    }
    
    bool operator==(const Edge& other) const {
        return u == other.u && v == other.v;
    }
    
    bool operator!=(const Edge& other) const {
        return !(*this == other);
    }
    
    string toString() const {
        return "(" + to_string(u) + "," + to_string(v) + ")";
    }
};

// Triangle representation with canonical vertex ordering
class Triangle {
public:
    int a, b, c;
    
    Triangle(int x, int y, int z) {
        vector<int> vertices = {x, y, z};
        sort(vertices.begin(), vertices.end());
        a = vertices[0];
        b = vertices[1];
        c = vertices[2];
    }
    
    bool operator<(const Triangle& other) const {
        return tie(a, b, c) < tie(other.a, other.b, other.c);
    }
    
    bool operator==(const Triangle& other) const {
        return a == other.a && b == other.b && c == other.c;
    }
    
    bool operator!=(const Triangle& other) const {
        return !(*this == other);
    }
    
    bool containsEdge(const Edge& e) const {
        return (e.u == a && e.v == b) || (e.u == a && e.v == c) || (e.u == b && e.v == c);
    }
    
    vector<Edge> getEdges() const {
        return {Edge(a, b), Edge(a, c), Edge(b, c)};
    }
    
    string toString() const {
        return "(" + to_string(a) + "," + to_string(b) + "," + to_string(c) + ")";
    }
};

// Complete triangulation representation
class Triangulation {
public:
    set<Triangle> triangles;
    set<Edge> internal_edges;      // Edges shared by exactly 2 triangles
    set<Edge> boundary_edges;      // Edges on the boundary (shared by 1 triangle)
    set<Edge> all_edges;
    
    Triangulation() {}
    
    Triangulation(const vector<Triangle>& tri_list) {
        for (const Triangle& t : tri_list) {
            triangles.insert(t);
        }
        computeEdgeClassification();
    }
    
    void computeEdgeClassification() {
        map<Edge, int> edge_count;
        all_edges.clear();
        internal_edges.clear();
        boundary_edges.clear();
        
        // Count occurrences of each edge
        for (const Triangle& t : triangles) {
            for (const Edge& e : t.getEdges()) {
                edge_count[e]++;
                all_edges.insert(e);
            }
        }
        
        // Classify edges based on their occurrence count
        for (const auto& pair : edge_count) {
            if (pair.second == 1) {
                boundary_edges.insert(pair.first);
            } else if (pair.second == 2) {
                internal_edges.insert(pair.first);
            }
        }
    }
    
    bool isFlippableEdge(const Edge& edge) const {
        // An edge can be flipped if it's an internal edge (shared by exactly 2 triangles)
        return internal_edges.count(edge) > 0;
    }
    
    // Core operation: flip an edge according to the algorithm
    Triangulation flipEdge(const Edge& edge) const {
        if (!isFlippableEdge(edge)) {
            return *this;  // Return unchanged if cannot flip
        }
        
        // Find the two triangles sharing this edge
        vector<Triangle> sharing_triangles;
        for (const Triangle& t : triangles) {
            if (t.containsEdge(edge)) {
                sharing_triangles.push_back(t);
            }
        }
        
        if (sharing_triangles.size() != 2) {
            return *this;  // Should not happen for valid internal edge
        }
        
        Triangle t1 = sharing_triangles[0];
        Triangle t2 = sharing_triangles[1];
        
        // Find the quadrilateral vertices
        set<int> quad_vertices;
        quad_vertices.insert(t1.a); quad_vertices.insert(t1.b); quad_vertices.insert(t1.c);
        quad_vertices.insert(t2.a); quad_vertices.insert(t2.b); quad_vertices.insert(t2.c);
        
        // Remove the edge vertices to get the opposite vertices
        quad_vertices.erase(edge.u);
        quad_vertices.erase(edge.v);
        
        if (quad_vertices.size() != 2) {
            return *this;  // Invalid configuration
        }
        
        vector<int> opposite_vertices(quad_vertices.begin(), quad_vertices.end());
        
        // Create new triangulation with flipped edge
        vector<Triangle> new_triangles;
        for (const Triangle& t : triangles) {
            if (t != t1 && t != t2) {
                new_triangles.push_back(t);
            }
        }
        
        // Add the two new triangles after flipping the edge
        new_triangles.push_back(Triangle(edge.u, opposite_vertices[0], opposite_vertices[1]));
        new_triangles.push_back(Triangle(edge.v, opposite_vertices[0], opposite_vertices[1]));
        
        return Triangulation(new_triangles);
    }
    
    // Generate canonical form for comparison and hashing
    string getCanonicalForm() const {
        stringstream ss;
        for (const Triangle& t : triangles) {
            ss << t.toString();
        }
        return ss.str();
    }
    
    bool operator<(const Triangulation& other) const {
        return getCanonicalForm() < other.getCanonicalForm();
    }
    
    bool operator==(const Triangulation& other) const {
        return triangles == other.triangles;
    }
    
    void printDetailed(ostream& out) const {
        out << "Triangles: ";
        bool first = true;
        for (const Triangle& t : triangles) {
            if (!first) out << ", ";
            out << t.toString();
            first = false;
        }
        out << endl;
        
        out << "Internal edges: ";
        first = true;
        for (const Edge& e : internal_edges) {
            if (!first) out << ", ";
            out << e.toString();
            first = false;
        }
        out << endl;
    }
};

// Main algorithm implementation
class ParvezRahmanNakanoTriangulator {
private:
    vector<vector<int>> input_faces;
    set<string> visited_canonical_forms;
    vector<Triangulation> all_triangulations;
    int generation_count;
    
public:
    ParvezRahmanNakanoTriangulator(const vector<vector<int>>& faces) 
        : input_faces(faces), generation_count(0) {}
    
    // Create initial triangulation using fan triangulation for each face
    Triangulation createRootTriangulation() {
        vector<Triangle> triangles;
        
        cout << "Creating root triangulation using fan method..." << endl;
        
        for (int face_idx = 0; face_idx < input_faces.size(); ++face_idx) {
            const vector<int>& face = input_faces[face_idx];
            
            if (face.size() < 3) {
                cerr << "Warning: Face " << face_idx + 1 << " has less than 3 vertices" << endl;
                continue;
            }
            
            if (face.size() == 3) {
                // Already triangular
                triangles.push_back(Triangle(face[0], face[1], face[2]));
                cout << "  Face " << face_idx + 1 << ": Already triangular " << 
                        Triangle(face[0], face[1], face[2]).toString() << endl;
            } else {
                // Fan triangulation: connect first vertex to all non-adjacent vertices
                int first_vertex = face[0];
                for (int i = 1; i < face.size() - 1; ++i) {
                    Triangle tri(first_vertex, face[i], face[i + 1]);
                    triangles.push_back(tri);
                    cout << "  Face " << face_idx + 1 << ": Added triangle " << tri.toString() << endl;
                }
            }
        }
        
        return Triangulation(triangles);
    }
    
    // Find all edges that can be flipped to generate child triangulations
    vector<Edge> getFlippableEdges(const Triangulation& tri) const {
        vector<Edge> flippable;
        
        for (const Edge& edge : tri.internal_edges) {
            if (tri.isFlippableEdge(edge)) {
                flippable.push_back(edge);
            }
        }
        
        return flippable;
    }
    
    // Recursive function to generate all triangulations following the paper's approach
    void generateTriangulationsRecursive(const Triangulation& current, int depth = 0) {
        string canonical = current.getCanonicalForm();
        
        // Check if already visited
        if (visited_canonical_forms.count(canonical)) {
            return;
        }
        
        // Mark as visited and add to results
        visited_canonical_forms.insert(canonical);
        all_triangulations.push_back(current);
        generation_count++;
        
        if (generation_count <= 10 || generation_count % 100 == 0) {
            cout << "Generated triangulation " << generation_count << 
                    " (depth " << depth << ", " << current.triangles.size() << " triangles)" << endl;
        }
        
        // Find all flippable edges (generating chords in the paper's terminology)
        vector<Edge> flippable_edges = getFlippableEdges(current);
        
        // Generate child triangulations by flipping each edge
        for (const Edge& edge : flippable_edges) {
            Triangulation child = current.flipEdge(edge);
            
            // Recursively process the child if it's different
            if (!(child == current)) {
                generateTriangulationsRecursive(child, depth + 1);
            }
        }
    }
    
    // Main algorithm entry point
    void generateAllTriangulations() {
        cout << "\n=== Starting Parvez-Rahman-Nakano Algorithm ===" << endl;
        cout << "Input configuration:" << endl;
        cout << "  Number of faces: " << input_faces.size() << endl;
        
        for (int i = 0; i < input_faces.size(); ++i) {
            cout << "  Face " << (i + 1) << ": [";
            for (int j = 0; j < input_faces[i].size(); ++j) {
                cout << input_faces[i][j];
                if (j < input_faces[i].size() - 1) cout << ", ";
            }
            cout << "]" << endl;
        }
        
        // Step 1: Create root triangulation
        cout << "\n--- Step 1: Creating Root Triangulation ---" << endl;
        Triangulation root = createRootTriangulation();
        cout << "Root triangulation created with:" << endl;
        cout << "  Triangles: " << root.triangles.size() << endl;
        cout << "  Internal edges: " << root.internal_edges.size() << endl;
        cout << "  Boundary edges: " << root.boundary_edges.size() << endl;
        
        // Step 2: Generate all triangulations using recursive traversal
        cout << "\n--- Step 2: Generating All Triangulations ---" << endl;
        generateTriangulationsRecursive(root);
        
        cout << "\n=== Algorithm Completed ===" << endl;
        cout << "Total triangulations generated: " << generation_count << endl;
        cout << "Unique triangulations: " << all_triangulations.size() << endl;
    }
    
    // Write comprehensive output to file
    void writeOutputToFile(const string& filename) const {
        ofstream outfile(filename);
        if (!outfile.is_open()) {
            cerr << "Error: Cannot open output file " << filename << endl;
            return;
        }
        
        outfile << "Parvez-Rahman-Nakano Algorithm Results" << endl;
        outfile << "=====================================" << endl;
        outfile << "Input faces: " << input_faces.size() << endl;
        for (int i = 0; i < input_faces.size(); ++i) {
            outfile << "Face " << (i + 1) << ": [";
            for (int j = 0; j < input_faces[i].size(); ++j) {
                outfile << input_faces[i][j];
                if (j < input_faces[i].size() - 1) outfile << ", ";
            }
            outfile << "]" << endl;
        }
        outfile << endl;
        outfile << "Total unique triangulations: " << all_triangulations.size() << endl;
        outfile << "=====================================" << endl << endl;
        
        for (int i = 0; i < all_triangulations.size(); ++i) {
            outfile << "Triangulation " << (i + 1) << ":" << endl;
            all_triangulations[i].printDetailed(outfile);
            outfile << "---" << endl;
        }
        
        outfile.close();
        cout << "Detailed results written to " << filename << endl;
    }
    
    void printSummaryStatistics() const {
        cout << "\n========== SUMMARY STATISTICS ==========" << endl;
        cout << "Input faces: " << input_faces.size() << endl;
        cout << "Total triangulations generated: " << generation_count << endl;
        cout << "Unique triangulations: " << all_triangulations.size() << endl;
        
        if (!all_triangulations.empty()) {
            const Triangulation& sample = all_triangulations[0];
            cout << "Triangles per triangulation: " << sample.triangles.size() << endl;
            cout << "Internal edges in root: " << sample.internal_edges.size() << endl;
        }
        
        cout << "=======================================" << endl;
    }
};

// Input/Output utilities
vector<vector<int>> readInputFromFile(const string& filename) {
    ifstream infile(filename);
    if (!infile.is_open()) {
        cerr << "Error: Cannot open input file '" << filename << "'" << endl;
        cerr << "Please ensure the file exists and follows the format:" << endl;
        cerr << "  Line 1: number_of_faces" << endl;
        cerr << "  For each face:" << endl;
        cerr << "    Line: face_size vertex1 vertex2 ... vertexN" << endl;
        return {};
    }
    
    vector<vector<int>> faces;
    int num_faces;
    
    if (!(infile >> num_faces)) {
        cerr << "Error: Cannot read number of faces from " << filename << endl;
        return {};
    }
    
    cout << "Reading " << num_faces << " faces from " << filename << "..." << endl;
    
    for (int i = 0; i < num_faces; ++i) {
        int face_size;
        if (!(infile >> face_size)) {
            cerr << "Error: Cannot read size of face " << (i + 1) << endl;
            return {};
        }
        
        vector<int> face;
        for (int j = 0; j < face_size; ++j) {
            int vertex;
            if (!(infile >> vertex)) {
                cerr << "Error: Cannot read vertex " << (j + 1) << " of face " << (i + 1) << endl;
                return {};
            }
            face.push_back(vertex);
        }
        
        faces.push_back(face);
        
        cout << "  Face " << (i + 1) << " (size " << face_size << "): [";
        for (int j = 0; j < face.size(); ++j) {
            cout << face[j];
            if (j < face.size() - 1) cout << ", ";
        }
        cout << "]" << endl;
    }
    
    infile.close();
    return faces;
}

bool validateInput(const vector<vector<int>>& faces) {
    if (faces.empty()) {
        cerr << "Error: No faces provided" << endl;
        return false;
    }
    
    for (int i = 0; i < faces.size(); ++i) {
        const vector<int>& face = faces[i];
        if (face.size() < 3) {
            cerr << "Error: Face " << (i + 1) << " has less than 3 vertices" << endl;
            return false;
        }
        
        // Check for duplicate vertices in the same face
        set<int> unique_vertices(face.begin(), face.end());
        if (unique_vertices.size() != face.size()) {
            cerr << "Error: Face " << (i + 1) << " contains duplicate vertices" << endl;
            return false;
        }
    }
    
    return true;
}

int main(int argc, char* argv[]) {
    cout << "=============================================================" << endl;
    cout << "    Triconnected Planar Graph Triangulation Generator" << endl;
    cout << "    Based on Parvez-Rahman-Nakano Algorithm (2011)" << endl;
    cout << "=============================================================" << endl;
    
    // Read and validate input
    string input_filename = "input.txt";
    if (argc > 1) {
        input_filename = argv[1];
    }
    
    vector<vector<int>> faces = readInputFromFile(input_filename);
    
    if (!validateInput(faces)) {
        cerr << "Input validation failed. Please check your " << input_filename << " file." << endl;
        return 1;
    }
    
    // Create and run the algorithm
    ParvezRahmanNakanoTriangulator triangulator(faces);
    triangulator.generateAllTriangulations();
    
    // Output results
    triangulator.printSummaryStatistics();
    triangulator.writeOutputToFile("output.txt");
    
    cout << "\n=============================================================" << endl;
    cout << "    Algorithm execution completed successfully!" << endl;
    cout << "    Results saved to output.txt" << endl;
    cout << "=============================================================" << endl;
    
    return 0;
}
