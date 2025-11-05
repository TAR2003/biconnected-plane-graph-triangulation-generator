#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>

using namespace std;

struct Chord
{
    int u, v;

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

class Triangulation
{
private:
    int n;                              // number of vertices
    set<Chord> chords;                  // current chords in triangulation
    // set<set<Chord>> seenTriangulations; // track visited triangulations
    int count;

    // Check if edge is a boundary edge
    bool isBoundaryEdge(int u, int v)
    {
        return (abs(u - v) == 1) || (u == 1 && v == n) || (v == 1 && u == n);
    }

    // Create initial fan triangulation from v1
    void createRootTriangulation()
    {
        chords.clear();
        for (int j = 3; j <= n - 1; j++)
        {
            chords.insert(Chord(1, j));
        }
    }

    // Find the quadrilateral containing chord (u,v) and return the other diagonal
    Chord findFlipChord(const Chord &chord)
    {
        int u = chord.u;
        int v = chord.v;

        // Find the two triangles sharing this chord
        // We need to find two other vertices that form the quadrilateral
        int a = -1, b = -1;

        // For each potential vertex
        for (int i = 1; i <= n; i++)
        {
            if (i == u || i == v)
                continue;

            // Check if vertex i forms a triangle with edge (u,v)
            bool hasUI = chords.count(Chord(u, i)) || isBoundaryEdge(u, i);
            bool hasIV = chords.count(Chord(i, v)) || isBoundaryEdge(i, v);

            if (hasUI && hasIV)
            {
                if (a == -1)
                    a = i;
                else if (b == -1)
                    b = i;
            }
        }

        if (a != -1 && b != -1)
        {
            return Chord(a, b);
        }

        return Chord(-1, -1); // Invalid flip
    }

    // Check if a chord can be flipped (is a generating chord)
    bool canFlipChord(const Chord &chord)
    {
        // Boundary edges cannot be flipped
        if (isBoundaryEdge(chord.u, chord.v))
            return false;

        // Check if the flip would create a valid triangulation
        Chord newChord = findFlipChord(chord);
        if (newChord.u == -1)
            return false;

        // Check if newChord already exists (would be invalid)
        if (chords.count(newChord))
            return false;

        // Check if it's a boundary edge
        if (isBoundaryEdge(newChord.u, newChord.v))
            return false;

        return true;
    }

    // Check if triangulation has any generating chords
    bool hasGeneratingChords()
    {
        for (const auto &chord : chords)
        {
            if (canFlipChord(chord))
                return true;
        }
        return false;
    }

    // Find leftmost blocking chord (chord from v1 with smallest second vertex)
    Chord findLeftmostBlockingChord()
    {
        Chord leftmost(1, n + 1);

        for (const auto &chord : chords)
        {
            if (chord.u == 1)
            { // Chords from v1
                if (chord.v < leftmost.v)
                {
                    leftmost = chord;
                }
            }
        }

        return leftmost;
    }

    // Output current triangulation
    void outputTriangulation()
    {
        // Check for duplicates
        // if (seenTriangulations.count(chords))
        // {
        //     return; // Skip duplicate
        // }

        // seenTriangulations.insert(chords);
        count++;

        cout << "Triangulation #" << count << ": ";
        for (const auto &chord : chords)
        {
            cout << "(" << chord.u << "," << chord.v << ") ";
        }
        cout << endl;
    }

    // Main recursive function
    void findAllChildTriangulationsCycle()
    {
        outputTriangulation();

        // Base case: no generating chords
        if (!hasGeneratingChords())
        {
            return;
        }

        // Find leftmost blocking chord
        Chord blockingChord = findLeftmostBlockingChord();
        int b = blockingChord.v;

        // Try flipping all chords (v1, vj) where j >= b
        for (int j = b; j <= n; j++)
        {
            Chord currentChord(1, j);

            // Check if this chord exists in current triangulation
            if (chords.count(currentChord) && canFlipChord(currentChord))
            {
                // Find the new chord after flipping
                Chord newChord = findFlipChord(currentChord);

                if (newChord.u != -1)
                {
                    // Flip the chord
                    chords.erase(currentChord);
                    chords.insert(newChord);

                    // Recurse
                    findAllChildTriangulationsCycle();

                    // Backtrack: undo the flip
                    chords.erase(newChord);
                    chords.insert(currentChord);
                }
            }
        }
    }

public:
    Triangulation(int vertices) : n(vertices), count(0) {}

    // Main algorithm entry point
    void findAllTriangulationsCycle()
    {
        // seenTriangulations.clear();
        count = 0;

        // Create root triangulation
        createRootTriangulation();

        // Output root
        outputTriangulation();

        // Store initial state
        set<Chord> rootChords = chords;

        // Try flipping each chord from v1 in reverse order
        for (int j = n - 1; j >= 3; j--)
        {
            Chord chord(1, j);

            // Start from root each time
            chords = rootChords;

            if (chords.count(chord) && canFlipChord(chord))
            {
                Chord newChord = findFlipChord(chord);

                if (newChord.u != -1)
                {
                    // Flip the chord
                    chords.erase(chord);
                    chords.insert(newChord);

                    // Recurse
                    findAllChildTriangulationsCycle();
                }
            }
        }

        cout << "\nTotal triangulations found: " << count << endl;
    }

    int getCount() const
    {
        return count;
    }
};

int main()
{
    int n;
    cout << "Enter number of vertices in convex polygon: ";
    cin >> n;

    if (n < 3)
    {
        cout << "Need at least 3 vertices!" << endl;
        return 1;
    }

    Triangulation tri(n);
    tri.findAllTriangulationsCycle();

    // Verify with Catalan number
    if (n <= 10)
    {
        int catalan[] = {1, 1, 2, 5, 14, 42, 132, 429, 1430, 4862};
        cout << "Expected (Catalan number C_" << (n - 2) << "): " << catalan[n - 2] << endl;
    }

    return 0;
}