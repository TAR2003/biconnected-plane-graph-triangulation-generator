#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <memory>

using namespace std;

// Structure to represent a chord (always stored with lower vertex first)
struct Chord
{
    int u;
    int v;

    Chord(int a, int b) : u(min(a, b)), v(max(a, b)) {}

    bool operator<(const Chord &other) const
    {
        if (u != other.u)
            return u < other.u;
        return v < other.v;
    }

    bool operator==(const Chord &other) const
    {
        return u == other.u && v == other.v;
    }
};

// Structure to represent a triangulation (set of chords)
using Triangulation = set<Chord>;

// Class to represent a face (cycle) of the graph
class Face
{
public:
    vector<int> vertices;
    int rootVertex;

    Face(const vector<int> &v) : vertices(v)
    {
        // Choose the first vertex as root by default
        if (!vertices.empty())
        {
            rootVertex = vertices[0];
        }
    }

    // Get the index of a vertex in the face
    int getIndex(int vertex) const
    {
        for (size_t i = 0; i < vertices.size(); ++i)
        {
            if (vertices[i] == vertex)
                return i;
        }
        return -1;
    }
};

// Class to generate all triangulations of a cycle (face)
class CycleTriangulationGenerator
{
private:
    Face cycle;
    Triangulation rootTriangulation;
    vector<Chord> rootGeneratingSet;

    // For a given triangulation, find its generating set (chords that can be flipped to produce children)
    vector<Chord> findGeneratingSet(const Triangulation &triangulation)
    {
        vector<Chord> generatingSet;
        Chord leftmostBlocking(-1, -1);

        // Find the leftmost blocking chord (if any)
        for (const auto &chord : triangulation)
        {
            if (chord.u != cycle.rootVertex && chord.v != cycle.rootVertex)
            {
                // This is a blocking chord (not incident to root)
                if (leftmostBlocking.u == -1 || chord.v > leftmostBlocking.v)
                {
                    leftmostBlocking = chord;
                }
            }
        }

        if (leftmostBlocking.u == -1)
        {
            // No blocking chords - all chords are generating (root case)
            for (const auto &chord : triangulation)
            {
                generatingSet.push_back(chord);
            }
        }
        else
        {
            // Only chords (v1, vj) where j >= b are generating
            for (const auto &chord : triangulation)
            {
                if (chord.u == cycle.rootVertex && chord.v >= leftmostBlocking.v)
                {
                    generatingSet.push_back(chord);
                }
            }
        }

        return generatingSet;
    }

    // Flip a chord in a triangulation to produce a new triangulation
    Triangulation flipChord(const Triangulation &triangulation, const Chord &chord)
    {
        Triangulation newTriangulation = triangulation;

        // Remove the chord to flip
        newTriangulation.erase(chord);

        // Find the quadrilateral formed by the chord and its adjacent triangles
        // We need to find the opposite chord to add
        Chord opposite = findOppositeChord(triangulation, chord);

        // Add the opposite chord
        newTriangulation.insert(opposite);

        return newTriangulation;
    }

    // Find the opposite chord in the quadrilateral formed by flipping the given chord
    Chord findOppositeChord(const Triangulation &triangulation, const Chord &chord)
    {
        // Get the positions of the chord vertices in the cycle
        int u_pos = cycle.getIndex(chord.u);
        int v_pos = cycle.getIndex(chord.v);

        // The chord divides the cycle into two parts. We need to find the vertices adjacent to chord in the triangulation.
        // For a triangulation, the vertices adjacent to chord.u and chord.v will form the quadrilateral.

        // Find vertex before u in the cycle that's connected to u
        int before_u = -1;
        for (int i = u_pos - 1;; i--)
        {
            if (i < 0)
                i = cycle.vertices.size() - 1;
            if (i == v_pos)
                break;
            Chord c(cycle.vertices[u_pos], cycle.vertices[i]);
            if (triangulation.count(c))
            {
                before_u = cycle.vertices[i];
                break;
            }
        }

        // Find vertex after v in the cycle that's connected to v
        int after_v = -1;
        for (int i = v_pos + 1;; i++)
        {
            if (i >= static_cast<int>(cycle.vertices.size()))
                i = 0;
            if (i == u_pos)
                break;
            Chord c(cycle.vertices[v_pos], cycle.vertices[i]);
            if (triangulation.count(c))
            {
                after_v = cycle.vertices[i];
                break;
            }
        }

        if (before_u != -1 && after_v != -1)
        {
            return Chord(before_u, after_v);
        }

        // If we didn't find the opposite chord, this shouldn't happen for a valid triangulation
        throw runtime_error("Could not find opposite chord for flip operation");
    }

    // Recursive function to generate all child triangulations
    void generateChildren(const Triangulation &triangulation, vector<Triangulation> &allTriangulations)
    {
        allTriangulations.push_back(triangulation);

        vector<Chord> generatingSet = findGeneratingSet(triangulation);

        for (const auto &chord : generatingSet)
        {
            Triangulation child = flipChord(triangulation, chord);
            generateChildren(child, allTriangulations);
        }
    }

public:
    CycleTriangulationGenerator(const Face &f) : cycle(f)
    {
        // Create root triangulation - all chords incident to root vertex
        for (size_t i = 2; i < cycle.vertices.size(); ++i)
        {
            rootTriangulation.insert(Chord(cycle.rootVertex, cycle.vertices[i]));
        }

        // The generating set for root is all its chords
        for (const auto &chord : rootTriangulation)
        {
            rootGeneratingSet.push_back(chord);
        }
    }

    // Generate all triangulations for this cycle
    vector<Triangulation> generateAll()
    {
        vector<Triangulation> allTriangulations;
        generateChildren(rootTriangulation, allTriangulations);
        return allTriangulations;
    }
};

// Class to represent the plane graph and generate all its triangulations
class PlaneGraphTriangulations
{
private:
    vector<Face> faces;
    map<int, vector<int>> adjacencyList;

    // Build adjacency list from faces
    void buildAdjacencyList()
    {
        for (const auto &face : faces)
        {
            for (size_t i = 0; i < face.vertices.size(); ++i)
            {
                int u = face.vertices[i];
                int v = face.vertices[(i + 1) % face.vertices.size()];

                adjacencyList[u].push_back(v);
                adjacencyList[v].push_back(u);
            }
        }

        // Remove duplicates from adjacency lists
        for (auto &entry : adjacencyList)
        {
            sort(entry.second.begin(), entry.second.end());
            entry.second.erase(unique(entry.second.begin(), entry.second.end()), entry.second.end());
        }
    }

    // Find all triangulations of the graph by combining face triangulations
    vector<Triangulation> combineFaceTriangulations(const vector<vector<Triangulation>> &faceTriangulations)
    {
        vector<Triangulation> allTriangulations;
        vector<size_t> indices(faceTriangulations.size(), 0);

        bool done = false;
        while (!done)
        {
            // Combine current triangulations from each face
            Triangulation combined;
            for (size_t i = 0; i < faceTriangulations.size(); ++i)
            {
                const Triangulation &t = faceTriangulations[i][indices[i]];
                combined.insert(t.begin(), t.end());
            }

            // Add original edges
            for (const auto &face : faces)
            {
                for (size_t i = 0; i < face.vertices.size(); ++i)
                {
                    int u = face.vertices[i];
                    int v = face.vertices[(i + 1) % face.vertices.size()];
                    combined.insert(Chord(u, v));
                }
            }

            allTriangulations.push_back(combined);

            // Move to next combination
            size_t i = 0;
            while (i < faceTriangulations.size())
            {
                indices[i]++;
                if (indices[i] < faceTriangulations[i].size())
                {
                    break;
                }
                indices[i] = 0;
                i++;
            }

            if (i == faceTriangulations.size())
            {
                done = true;
            }
        }

        return allTriangulations;
    }

public:
    PlaneGraphTriangulations(const vector<vector<int>> &inputFaces)
    {
        for (const auto &faceVertices : inputFaces)
        {
            faces.emplace_back(faceVertices);
        }
        buildAdjacencyList();
    }

    // Generate all triangulations of the graph
    vector<Triangulation> generateAllTriangulations()
    {
        // First generate all triangulations for each face
        vector<vector<Triangulation>> faceTriangulations;
        for (const auto &face : faces)
        {
            CycleTriangulationGenerator generator(face);
            faceTriangulations.push_back(generator.generateAll());
        }

        // Then combine them to get all graph triangulations
        return combineFaceTriangulations(faceTriangulations);
    }
};

// Function to read input from a file
vector<vector<int>> readInputFromFile(const string &filename)
{
    ifstream infile(filename);
    vector<vector<int>> faces;
    string line;

    while (getline(infile, line))
    {
        istringstream iss(line);
        vector<int> face;
        int vertex;

        while (iss >> vertex)
        {
            face.push_back(vertex);
        }

        if (!face.empty())
        {
            faces.push_back(face);
        }
    }

    return faces;
}

// Function to write output to a file
void writeOutputToFile(const string &filename, const vector<Triangulation> &triangulations)
{
    ofstream outfile(filename);

    outfile << "Total triangulations: " << triangulations.size() << "\n\n";

    for (size_t i = 0; i < triangulations.size(); ++i)
    {
        outfile << "Triangulation #" << i + 1 << ":\n";
        outfile << "Chords: ";

        for (const auto &chord : triangulations[i])
        {
            outfile << "(" << chord.u << "," << chord.v << ") ";
        }

        outfile << "\n\n";
    }
}

int main()
{
    // Example usage
    string inputFilename = "input.txt";
    string outputFilename = "output.txt";

    // Read input
    vector<vector<int>> inputFaces = readInputFromFile(inputFilename);

    if (inputFaces.empty())
    {
        cout << "No input found or empty input file. Using default example.\n";
        // Default example if no input file
        inputFaces = {{1, 2, 3, 4}, {4, 5, 6, 7}};
    }

    // Generate triangulations
    PlaneGraphTriangulations graph(inputFaces);
    vector<Triangulation> allTriangulations = graph.generateAllTriangulations();

    // Write output
    writeOutputToFile(outputFilename, allTriangulations);

    cout << "Generated " << allTriangulations.size() << " triangulations.\n";
    cout << "Results written to " << outputFilename << "\n";

    return 0;
}