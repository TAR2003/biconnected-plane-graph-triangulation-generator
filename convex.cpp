#include <iostream>
#include <vector>
#include <set>
#include <algorithm>

using namespace std;

class TriangulationGenerator
{
private:
    int n; // number of vertices
    vector<vector<pair<int, int>>> allTriangulations;

    // Represents a triangulation as a set of chords
    typedef vector<pair<int, int>> Triangulation;

    // Generate all children of current triangulation
    void generateChildren(Triangulation &current, vector<int> &visibleFromRoot)
    {
        // Add current triangulation to results
        allTriangulations.push_back(current);

        if (current.empty())
            return;

        // Find leftmost blocking chord (if any)
        int leftmostBlockingVertex = n;
        pair<int, int> leftmostBlockingChord = {-1, -1};

        for (auto &chord : current)
        {
            // A blocking chord is one where both endpoints are visible from v1
            if (chord.first != 1 &&
                find(visibleFromRoot.begin(), visibleFromRoot.end(), chord.first) != visibleFromRoot.end() &&
                find(visibleFromRoot.begin(), visibleFromRoot.end(), chord.second) != visibleFromRoot.end())
            {
                if (chord.second < leftmostBlockingVertex)
                {
                    leftmostBlockingVertex = chord.second;
                    leftmostBlockingChord = chord;
                }
            }
        }

        // Generate children by flipping generating chords
        vector<pair<int, int>> generatingChords;
        for (auto &chord : current)
        {
            if (chord.first == 1)
            {
                // This is a candidate chord
                if (leftmostBlockingChord.first == -1 || chord.second >= leftmostBlockingChord.first)
                {
                    generatingChords.push_back(chord);
                }
            }
        }

        // For each generating chord, create a child triangulation
        for (auto &genChord : generatingChords)
        {
            Triangulation child = current;
            vector<int> childVisible = visibleFromRoot;

            // Find the quadrilateral containing this chord
            int v1 = genChord.first;
            int vj = genChord.second;

            // Find vertices that form quadrilateral with (v1, vj)
            int vPrev = -1, vNext = -1;

            // Find adjacent vertices in the triangulation
            for (int i = 2; i <= n; i++)
            {
                if (i == vj)
                    continue;
                bool foundPrev = false, foundNext = false;

                for (auto &ch : current)
                {
                    if ((ch.first == v1 && ch.second == i) || (ch.first == i && ch.second == v1))
                    {
                        if (i < vj)
                            vPrev = max(vPrev, i);
                        else
                            vNext = (vNext == -1) ? i : min(vNext, i);
                    }
                    if ((ch.first == vj && ch.second == i) || (ch.first == i && ch.second == vj))
                    {
                        if (i < vj)
                            vPrev = max(vPrev, i);
                        else
                            vNext = (vNext == -1) ? i : min(vNext, i);
                    }
                }
            }

            if (vPrev != -1 && vNext != -1)
            {
                // Remove the generating chord and add the new chord
                child.erase(remove(child.begin(), child.end(), genChord), child.end());
                pair<int, int> newChord = {min(vPrev, vNext), max(vPrev, vNext)};
                child.push_back(newChord);

                // Update visible vertices
                childVisible.erase(remove(childVisible.begin(), childVisible.end(), vj), childVisible.end());

                // Recursively generate children
                generateChildren(child, childVisible);
            }
        }
    }

    // Alternative simpler recursive approach using the structure from the paper
    void generateAllTriangulationsRecursive(int start, int end, vector<pair<int, int>> &current)
    {
        if (end - start <= 2)
        {
            return; // Base case: triangle or less
        }

        // Try all possible triangulations by choosing different vertices to connect
        for (int k = start + 1; k < end; k++)
        {
            vector<pair<int, int>> newTriangulation = current;

            // Add chord from start to k if needed
            if (k - start > 1)
            {
                newTriangulation.push_back({start, k});
            }

            // Add chord from k to end if needed
            if (end - k > 1)
            {
                newTriangulation.push_back({k, end});
            }

            // Recursively triangulate the two sub-polygons
            generateAllTriangulationsRecursive(start, k, newTriangulation);
            generateAllTriangulationsRecursive(k, end, newTriangulation);

            // If this completes a triangulation, add it
            if (end - start == n && newTriangulation.size() == n - 3)
            {
                allTriangulations.push_back(newTriangulation);
            }
        }
    }

public:
    // Simplified approach: generate using divide and conquer
    vector<vector<pair<int, int>>> generateAllTriangulations(int numVertices)
    {
        n = numVertices;
        allTriangulations.clear();

        if (n < 3)
            return allTriangulations;
        if (n == 3)
        {
            allTriangulations.push_back(vector<pair<int, int>>());
            return allTriangulations;
        }

        // Use a recursive backtracking approach
        generateByBacktracking(vector<pair<int, int>>(), set<pair<int, int>>());

        return allTriangulations;
    }

    // Backtracking approach to generate all valid triangulations
    void generateByBacktracking(vector<pair<int, int>> current, set<pair<int, int>> used)
    {
        // Check if we have a complete triangulation
        if (current.size() == n - 3)
        {
            allTriangulations.push_back(current);
            return;
        }

        // Try adding each possible chord
        for (int i = 1; i <= n; i++)
        {
            for (int j = i + 2; j <= n; j++)
            {
                // Skip if this would be an edge of the polygon
                if ((i == 1 && j == n) || j == i + 1)
                    continue;

                pair<int, int> chord = {i, j};

                // Skip if already used
                if (used.count(chord))
                    continue;

                // Check if this chord intersects with any existing chord
                bool intersects = false;
                for (auto &existing : current)
                {
                    if (chordsIntersect(chord, existing))
                    {
                        intersects = true;
                        break;
                    }
                }

                if (!intersects)
                {
                    current.push_back(chord);
                    used.insert(chord);
                    generateByBacktracking(current, used);
                    current.pop_back();
                    used.erase(chord);
                }
            }
        }
    }

    // Check if two chords intersect (excluding endpoints)
    bool chordsIntersect(pair<int, int> c1, pair<int, int> c2)
    {
        int a = c1.first, b = c1.second;
        int c = c2.first, d = c2.second;

        // Chords share an endpoint - they don't intersect internally
        if (a == c || a == d || b == c || b == d)
            return false;

        // For a convex polygon with vertices in order,
        // chords (a,b) and (c,d) intersect if one of {c,d} is between a and b
        // and the other is outside this range
        bool c_between = (a < c && c < b) || (b < c && c < a);
        bool d_between = (a < d && d < b) || (b < d && d < a);

        // Handle wraparound for circular ordering
        if (a > b)
        {
            c_between = c > a || c < b;
            d_between = d > a || d < b;
        }

        return c_between != d_between;
    }

    int countTriangulations()
    {
        return allTriangulations.size();
    }

    void printAllTriangulations()
    {
        cout << "Total triangulations: " << allTriangulations.size() << endl;
        for (int i = 0; i < allTriangulations.size(); i++)
        {
            cout << "Triangulation " << (i + 1) << ": ";
            for (auto &chord : allTriangulations[i])
            {
                cout << "(" << chord.first << "," << chord.second << ") ";
            }
            cout << endl;
        }
    }
};

// Function that takes n and returns count of triangulations
int countConvexPolygonTriangulations(int n)
{
    if (n < 3)
        return 0;
    if (n == 3)
        return 1;

    TriangulationGenerator gen;
    gen.generateAllTriangulations(n);
    return gen.countTriangulations();
}

// Function that generates and returns all triangulations
vector<vector<pair<int, int>>> generateAllConvexPolygonTriangulations(int n)
{
    TriangulationGenerator gen;
    return gen.generateAllTriangulations(n);
}

int main()
{
    // Test with different polygon sizes
    for (int n = 3; n <= 8; n++)
    {
        cout << "n = " << n << ": ";
        int count = countConvexPolygonTriangulations(n);
        cout << count << " triangulations" << endl;

        // Optionally print all triangulations for smaller n
        if (n <= 5)
        {
            TriangulationGenerator gen;
            gen.generateAllTriangulations(n);
            gen.printAllTriangulations();
            cout << endl;
        }
    }

    return 0;
}