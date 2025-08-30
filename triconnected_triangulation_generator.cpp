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
 * Implementation of Parvez-Rahman-Nakano Algorithm for generating all triangulations
 * of triconnected planar graphs.
 * 
 * Reference: "Generating All Triangulations of Plane Graphs" 
 * by Mohammad Tanvir Parvez, Md. Saidur Rahman, and Shin-ichi Nakano (2011)
 */

// Edge representation
struct Edge {
    int u, v;
    
    Edge(int a, int b) : u(min(a, b)), v(max(a, b)) {}
    
    bool operator<(const Edge& other) const {
        return u < other.u || (u == other.u && v < other.v);
    }
    
    bool operator==(const Edge& other) const {
        return u == other.u && v == other.v;
    }
};

// Triangle representation
struct Triangle {
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
};

// Triangulation representation
class Triangulation {
public:
    set<Triangle> triangles;
    set<Edge> internal_edges;
    set<Edge> boundary_edges;
    set<Edge> all_edges;
    
    Triangulation(const vector<Triangle>& tri_list) {
        for (const Triangle& t : tri_list) {
            triangles.insert(t);
        }
        computeEdges();
    }
    
    void computeEdges() {
        map<Edge, int> edge_count;
        
        // Count occurrences of each edge
        for (const Triangle& t : triangles) {
            for (const Edge& e : t.getEdges()) {
                edge_count[e]++;
                all_edges.insert(e);
            }
        }
        
        // Classify edges as boundary or internal
        for (const auto& pair : edge_count) {
            if (pair.second == 1) {
                boundary_edges.insert(pair.first);
            } else if (pair.second == 2) {
                internal_edges.insert(pair.first);
            }
        }
    }
    
    bool canFlipEdge(const Edge& edge) const {
        if (internal_edges.find(edge) == internal_edges.end()) {
            return false;
        }
        
        // Find triangles sharing this edge
        vector<Triangle> sharing_triangles;
        for (const Triangle& t : triangles) {
            if (t.containsEdge(edge)) {
                sharing_triangles.push_back(t);
            }
        }
        
        return sharing_triangles.size() == 2;
    }
    
    Triangulation flipEdge(const Edge& edge) const {
        if (!canFlipEdge(edge)) {
            return *this;  // Return unchanged if cannot flip
        }
        
        // Find the two triangles sharing this edge
        vector<Triangle> sharing_triangles;
        for (const Triangle& t : triangles) {
            if (t.containsEdge(edge)) {
                sharing_triangles.push_back(t);
            }
        }
        
        assert(sharing_triangles.size() == 2);
        
        Triangle t1 = sharing_triangles[0];
        Triangle t2 = sharing_triangles[1];
        
        // Find the four vertices of the quadrilateral
        set<int> quad_vertices;
        quad_vertices.insert(t1.a); quad_vertices.insert(t1.b); quad_vertices.insert(t1.c);
        quad_vertices.insert(t2.a); quad_vertices.insert(t2.b); quad_vertices.insert(t2.c);
        
        // Remove the edge vertices
        quad_vertices.erase(edge.u);
        quad_vertices.erase(edge.v);
        
        assert(quad_vertices.size() == 2);
        vector<int> opposite_vertices(quad_vertices.begin(), quad_vertices.end());
        
        // Create new triangulation with flipped edge
        vector<Triangle> new_triangles;
        for (const Triangle& t : triangles) {
            if (t != t1 && t != t2) {
                new_triangles.push_back(t);
            }
        }
        
        // Add the two new triangles after flipping
        new_triangles.push_back(Triangle(edge.u, opposite_vertices[0], opposite_vertices[1]));
        new_triangles.push_back(Triangle(edge.v, opposite_vertices[0], opposite_vertices[1]));
        
        return Triangulation(new_triangles);
    }
    
    string getCanonicalForm() const {
        stringstream ss;
        for (const Triangle& t : triangles) {
            ss << "(" << t.a << "," << t.b << "," << t.c << ")";
        }
        return ss.str();
    }
    
    bool operator<(const Triangulation& other) const {
        return getCanonicalForm() < other.getCanonicalForm();
    }
    
    bool operator==(const Triangulation& other) const {
        return triangles == other.triangles;
    }
};

// Main algorithm class
class ParvezRahmanNakanoAlgorithm {
private:
    vector<vector<int>> input_faces;
    set<Triangulation> all_triangulations;
    set<string> visited_canonical_forms;
    int total_triangulations;
    
public:
    ParvezRahmanNakanoAlgorithm(const vector<vector<int>>& faces) 
        : input_faces(faces), total_triangulations(0) {}
    
    Triangulation createInitialTriangulation() {
        vector<Triangle> triangles;
        
        // Create fan triangulation for each face
        for (const vector<int>& face : input_faces) {
            if (face.size() < 3) continue;
            
            if (face.size() == 3) {
                // Already a triangle
                triangles.push_back(Triangle(face[0], face[1], face[2]));
            } else {
                // Fan triangulation: connect first vertex to all non-adjacent vertices
                int first_vertex = face[0];
                for (int i = 1; i < face.size() - 1; ++i) {
                    triangles.push_back(Triangle(first_vertex, face[i], face[i + 1]));
                }
            }
        }
        
        return Triangulation(triangles);
    }
    
    Edge findLeftmostBlockingChord(const Triangulation& triangulation) {
        // Find leftmost blocking chord as per the algorithm
        Edge leftmost(-1, -1);
        bool found = false;
        
        for (const Edge& edge : triangulation.internal_edges) {
            // A blocking chord should satisfy certain conditions based on the algorithm
            if (!found || edge.v > leftmost.v || (edge.v == leftmost.v && edge.u > leftmost.u)) {
                leftmost = edge;
                found = true;
            }
        }
        
        return leftmost;
    }
    
    vector<Edge> findGeneratingChords(const Triangulation& triangulation) {
        vector<Edge> generating_chords;
        
        // According to the algorithm, find chords that can be flipped to generate children
        for (const Edge& edge : triangulation.internal_edges) {
            if (triangulation.canFlipEdge(edge)) {
                generating_chords.push_back(edge);
            }
        }
        
        return generating_chords;
    }
    
    void generateTriangulationsRecursive(const Triangulation& current) {
        string canonical = current.getCanonicalForm();
        
        if (visited_canonical_forms.find(canonical) != visited_canonical_forms.end()) {
            return;  // Already visited
        }
        
        visited_canonical_forms.insert(canonical);
        all_triangulations.insert(current);
        total_triangulations++;
        
        // Find generating chords
        vector<Edge> generating_chords = findGeneratingChords(current);
        
        // Recursively generate children by flipping each generating chord
        for (const Edge& chord : generating_chords) {
            if (current.canFlipEdge(chord)) {
                Triangulation child = current.flipEdge(chord);
                generateTriangulationsRecursive(child);
            }
        }
    }
    
    void generateAllTriangulations() {
        cout << "Starting Parvez-Rahman-Nakano Algorithm..." << endl;
        cout << "Input faces: " << input_faces.size() << endl;
        
        for (int i = 0; i < input_faces.size(); ++i) {
            cout << "Face " << (i + 1) << ": [";
            for (int j = 0; j < input_faces[i].size(); ++j) {
                cout << input_faces[i][j];
                if (j < input_faces[i].size() - 1) cout << ", ";
            }
            cout << "]" << endl;
        }
        
        // Create initial triangulation
        Triangulation root = createInitialTriangulation();
        cout << "\nInitial triangulation created with " << root.triangles.size() << " triangles" << endl;
        
        // Generate all triangulations recursively
        generateTriangulationsRecursive(root);
        
        cout << "\nAlgorithm completed!" << endl;
        cout << "Total triangulations generated: " << total_triangulations << endl;
    }
    
    void writeOutputToFile(const string& filename) {
        ofstream outfile(filename);
        if (!outfile.is_open()) {
            cerr << "Error: Cannot open output file " << filename << endl;
            return;
        }
        
        outfile << "Total triangulations: " << total_triangulations << endl;
        outfile << "===========================================" << endl;
        
        int count = 1;
        for (const Triangulation& tri : all_triangulations) {
            outfile << "Triangulation " << count++ << ":" << endl;
            outfile << "Triangles: ";
            bool first = true;
            for (const Triangle& t : tri.triangles) {
                if (!first) outfile << ", ";
                outfile << "(" << t.a << "," << t.b << "," << t.c << ")";
                first = false;
            }
            outfile << endl;
            
            outfile << "Internal edges: ";
            first = true;
            for (const Edge& e : tri.internal_edges) {
                if (!first) outfile << ", ";
                outfile << "(" << e.u << "," << e.v << ")";
                first = false;
            }
            outfile << endl;
            outfile << "-------------------------------------------" << endl;
        }
        
        outfile.close();
        cout << "Output written to " << filename << endl;
    }
    
    void printStatistics() {
        cout << "\n========== STATISTICS ==========" << endl;
        cout << "Input faces: " << input_faces.size() << endl;
        cout << "Total triangulations: " << total_triangulations << endl;
        cout << "Unique triangulations: " << all_triangulations.size() << endl;
        cout << "=================================" << endl;
    }
};

// Utility functions for input parsing
vector<vector<int>> readInputFile(const string& filename) {
    ifstream infile(filename);
    if (!infile.is_open()) {
        cerr << "Error: Cannot open input file " << filename << endl;
        return {};
    }
    
    vector<vector<int>> faces;
    int num_faces;
    
    if (!(infile >> num_faces)) {
        cerr << "Error: Cannot read number of faces" << endl;
        return {};
    }
    
    cout << "Reading " << num_faces << " faces from input..." << endl;
    
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
        
        cout << "Face " << (i + 1) << " read: [";
        for (int j = 0; j < face.size(); ++j) {
            cout << face[j];
            if (j < face.size() - 1) cout << ", ";
        }
        cout << "]" << endl;
    }
    
    infile.close();
    return faces;
}

int main() {
    cout << "=== Triconnected Planar Graph Triangulation Generator ===" << endl;
    cout << "Based on Parvez-Rahman-Nakano Algorithm (2011)" << endl;
    cout << "=========================================================" << endl;
    
    // Read input from file
    string input_filename = "input.txt";
    vector<vector<int>> faces = readInputFile(input_filename);
    
    if (faces.empty()) {
        cerr << "No valid input found. Please check " << input_filename << endl;
        return 1;
    }
    
    // Validate input
    bool valid = true;
    for (const vector<int>& face : faces) {
        if (face.size() < 3) {
            cerr << "Error: Face with less than 3 vertices found" << endl;
            valid = false;
        }
    }
    
    if (!valid) {
        return 1;
    }
    
    // Create algorithm instance and run
    ParvezRahmanNakanoAlgorithm algorithm(faces);
    
    cout << "\n=== ALGORITHM EXECUTION ===" << endl;
    algorithm.generateAllTriangulations();
    
    // Print statistics
    algorithm.printStatistics();
    
    // Write output to file
    string output_filename = "output.txt";
    algorithm.writeOutputToFile(output_filename);
    
    cout << "\n=== EXECUTION COMPLETED ===" << endl;
    
    return 0;
}
