#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <stack>

using namespace std;

// Structure to represent a chord in a triangulation
struct Chord
{
    int u, v;
    Chord(int a = 0, int b = 0) : u(min(a, b)), v(max(a, b)) {}
    bool operator<(const Chord &other) const
    {
        return u < other.u || (u == other.u && v < other.v);
    }
    bool operator==(const Chord &other) const
    {
        return u == other.u && v == other.v;
    }
};

// Class to represent a triangulation of a cycle
class CycleTriangulation
{
public:
    vector<int> vertices;
    set<Chord> chords;
    vector<Chord> generatingSet;
    map<Chord, pair<int, int>> opposites; // Maps chord to its opposite pair

    CycleTriangulation(const vector<int> &verts) : vertices(verts) {}

    // Create root triangulation where all chords are incident to first vertex
    void makeRoot()
    {
        chords.clear();
        generatingSet.clear();
        opposites.clear();

        int n = vertices.size();
        if (n <= 3)
            return;

        int root = vertices[0];
        for (int i = 2; i < n - 1; i++)
        {
            Chord c(root, vertices[i]);
            chords.insert(c);
            generatingSet.push_back(c);

            // Set opposite pair
            opposites[c] = make_pair(vertices[i - 1], vertices[i + 1]);
        }
    }

    // Find leftmost blocking chord
    Chord findLeftmostBlockingChord() const
    {
        int root = vertices[0];
        Chord leftmost(-1, -1);
        int maxIdx = -1;

        for (const auto &c : chords)
        {
            if (c.u != root && c.v != root)
            {
                // This is a blocking chord
                int idx1 = find(vertices.begin(), vertices.end(), c.u) - vertices.begin();
                int idx2 = find(vertices.begin(), vertices.end(), c.v) - vertices.begin();
                int maxCurrent = max(idx1, idx2);

                if (maxCurrent > maxIdx)
                {
                    maxIdx = maxCurrent;
                    leftmost = c;
                }
            }
        }
        return leftmost;
    }

    // Update generating set based on leftmost blocking chord
    void updateGeneratingSet()
    {
        generatingSet.clear();
        Chord leftmost = findLeftmostBlockingChord();

        if (leftmost.u == -1)
        {
            // No blocking chord, all chords are generating
            for (const auto &c : chords)
            {
                generatingSet.push_back(c);
            }
        }
        else
        {
            int root = vertices[0];
            int b = find(vertices.begin(), vertices.end(), leftmost.u) - vertices.begin();

            for (const auto &c : chords)
            {
                if (c.u == root || c.v == root)
                {
                    int other = (c.u == root) ? c.v : c.u;
                    int idx = find(vertices.begin(), vertices.end(), other) - vertices.begin();
                    if (idx >= b)
                    {
                        generatingSet.push_back(c);
                    }
                }
            }
        }
    }

    // Flip a chord to generate new triangulation
    CycleTriangulation flip(const Chord &c) const
    {
        CycleTriangulation result(vertices);
        result.chords = chords;
        result.opposites = opposites;

        // Remove the chord
        result.chords.erase(c);

        // Add the opposite chord
        auto opp = opposites.find(c);
        if (opp != opposites.end())
        {
            Chord newChord(opp->second.first, opp->second.second);
            result.chords.insert(newChord);

            // Update opposites for affected chords
            result.updateOpposites(c, newChord);
        }

        result.updateGeneratingSet();
        return result;
    }

    void updateOpposites(const Chord &oldChord, const Chord &newChord)
    {
        // This is simplified - full implementation would update
        // opposites for chords adjacent to the flipped chord
        opposites.erase(oldChord);

        // Find and update affected opposite pairs
        for (auto &op : opposites)
        {
            if (op.second.first == oldChord.u || op.second.first == oldChord.v)
            {
                if (op.second.first == oldChord.u)
                    op.second.first = newChord.u;
                if (op.second.first == oldChord.v)
                    op.second.first = newChord.v;
            }
            if (op.second.second == oldChord.u || op.second.second == oldChord.v)
            {
                if (op.second.second == oldChord.u)
                    op.second.second = newChord.u;
                if (op.second.second == oldChord.v)
                    op.second.second = newChord.v;
            }
        }
    }

    void print() const
    {
        cout << "{";
        for (const auto &c : chords)
        {
            cout << "(" << c.u << "," << c.v << ")";
        }
        cout << "}";
    }
};

// Class to generate all triangulations of a triconnected plane graph
class TriconnectedTriangulator
{
private:
    vector<vector<int>> faces;
    vector<CycleTriangulation> currentTriangulation;
    int triangulationCount;

public:
    TriconnectedTriangulator(const vector<vector<int>> &f) : faces(f), triangulationCount(0)
    {
        // Initialize with root triangulation for each face
        for (const auto &face : faces)
        {
            CycleTriangulation ct(face);
            ct.makeRoot();
            currentTriangulation.push_back(ct);
        }
    }

    void generateAllTriangulations()
    {
        cout << "Generating all triangulations of triconnected plane graph\n";
        cout << "Number of faces: " << faces.size() << "\n\n";

        // Start with root triangulation
        outputCurrentTriangulation();

        // Generate all triangulations recursively
        for (size_t i = 0; i < faces.size(); i++)
        {
            generateChildTriangulations(i);
        }

        cout << "\nTotal triangulations generated: " << triangulationCount << "\n";
    }

private:
    void generateChildTriangulations(int faceIdx)
    {
        if (faceIdx >= faces.size())
            return;

        // Save current state
        CycleTriangulation saved = currentTriangulation[faceIdx];

        // Generate children by flipping each generating chord
        for (const auto &chord : saved.generatingSet)
        {
            // Flip the chord
            currentTriangulation[faceIdx] = saved.flip(chord);

            // Output the new triangulation
            outputCurrentTriangulation();

            // Recursively generate children for eligible faces
            for (int j = 0; j <= faceIdx; j++)
            {
                generateChildTriangulations(j);
            }

            // Restore state
            currentTriangulation[faceIdx] = saved;
        }
    }

    void outputCurrentTriangulation()
    {
        triangulationCount++;
        cout << "Triangulation " << triangulationCount << ": ";

        for (size_t i = 0; i < currentTriangulation.size(); i++)
        {
            cout << "F" << (i + 1) << ":";
            currentTriangulation[i].print();
            if (i < currentTriangulation.size() - 1)
                cout << " ";
        }
        cout << "\n";
    }
};

// Helper function to parse face input
vector<int> parseFace(const string &input)
{
    vector<int> face;
    int num = 0;
    bool inNumber = false;

    for (char c : input)
    {
        if (c >= '0' && c <= '9')
        {
            num = num * 10 + (c - '0');
            inNumber = true;
        }
        else if (inNumber)
        {
            face.push_back(num);
            num = 0;
            inNumber = false;
        }
    }
    if (inNumber)
    {
        face.push_back(num);
    }

    return face;
}

int main()
{
    cout << "Enter faces of the triconnected plane graph\n";
    cout << "Format: {v1,v2,v3,...} one per line\n";
    cout << "Enter 'done' when finished:\n";

    vector<vector<int>> faces;
    string line;

    while (getline(cin, line))
    {
        if (line == "done")
            break;

        vector<int> face = parseFace(line);
        if (face.size() >= 3)
        {
            faces.push_back(face);
            cout << "Face " << faces.size() << " added with " << face.size() << " vertices\n";
        }
    }

    if (faces.empty())
    {
        cout << "No valid faces entered.\n";
        return 1;
    }

    // Generate all triangulations
    TriconnectedTriangulator triangulator(faces);
    triangulator.generateAllTriangulations();

    return 0;
}