#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <unordered_set>
#include <cassert>

using namespace std;

struct Chord
{
    int i, j; // i <= j
    Chord(int a = -1, int b = -1)
    {
        if (a <= b)
        {
            i = a;
            j = b;
        }
        else
        {
            i = b;
            j = a;
        }
    }
    bool operator==(const Chord &other) const { return i == other.i && j == other.j; }
    bool isValid() const { return i >= 1 && j >= 1; }
};

// for printing convenience
ostream &operator<<(ostream &os, const Chord &c)
{
    os << "(" << c.i << "," << c.j << ")";
    return os;
}

struct GeneratingChord
{
    Chord c;
    pair<int, int> opp;
    GeneratingChord(const Chord &chord = Chord(), pair<int, int> p = {-1, -1}) : c(chord), opp(p) {}
};

// Triangulation class
class Triangulation
{
public:
    int n;
    vector<Chord> L; // set of chords (no ordering guarantee)
    vector<GeneratingChord> GS;

    Triangulation(int vertices = 0) : n(vertices) {}

    // neighbors of v: cycle neighbors + chords
    // returns a set for clarity. For speed replace with unordered_set or dynamic adjacency.
    set<int> getNeighbors(int v) const
    {
        set<int> neighbors;
        // cycle edges
        if (v == 1)
        {
            neighbors.insert(2);
            neighbors.insert(n);
        }
        else if (v == n)
        {
            neighbors.insert(1);
            neighbors.insert(n - 1);
        }
        else
        {
            neighbors.insert(v - 1);
            neighbors.insert(v + 1);
        }
        // chord edges
        for (const auto &c : L)
        {
            if (c.i == v)
                neighbors.insert(c.j);
            if (c.j == v)
                neighbors.insert(c.i);
        }
        return neighbors;
    }

    // find two distinct common neighbors of 1 and j (excluding 1 and j)
    // returns (-1,-1) if not found / invalid
    pair<int, int> findOppositePair(int j) const
    {
        if (j <= 1 || j > n)
            return {-1, -1};
        set<int> N1 = getNeighbors(1);
        set<int> Nj = getNeighbors(j);
        vector<int> common;
        for (int x : N1)
        {
            if (x == 1 || x == j)
                continue;
            if (Nj.count(x))
                common.push_back(x);
        }
        // For a chord (1,j) in a triangulation there should be exactly 2 common neighbors
        // (except degenerate cases). If not 2, mark invalid.
        if (common.size() < 2)
            return {-1, -1};
        // sort the two vertices and return (vo, vo')
        sort(common.begin(), common.end());
        return {common[0], common[1]};
    }

    // leftmost blocking chord: among chords not incident to 1, both endpoints adjacent to 1,
    // pick one with largest right endpoint j (as in paper)
    Chord getLeftmostBlockingChord() const
    {
        int maxJ = -1;
        Chord leftmost(-1, -1);
        set<int> N1 = getNeighbors(1);
        for (const auto &c : L)
        {
            if (c.i == 1 || c.j == 1)
                continue;
            bool ia = N1.count(c.i) > 0;
            bool ja = N1.count(c.j) > 0;
            if (ia && ja)
            {
                if (c.j > maxJ)
                {
                    maxJ = c.j;
                    leftmost = c;
                }
            }
        }
        return leftmost;
    }

    // rebuild GS as defined: GS = {(1,j) in L | j >= b} where (b,b') is leftmost blocking chord
    void rebuildGeneratingSet()
    {
        GS.clear();
        Chord blocking = getLeftmostBlockingChord();
        if (!blocking.isValid())
        {
            // root: all chords incident to 1 are generating
            for (const auto &c : L)
            {
                if (c.i == 1)
                {
                    auto opp = findOppositePair(c.j);
                    if (opp.first != -1)
                        GS.emplace_back(c, opp);
                }
            }
        }
        else
        {
            int b = blocking.i;
            for (const auto &c : L)
            {
                if (c.i == 1 && c.j >= b)
                {
                    auto opp = findOppositePair(c.j);
                    if (opp.first != -1)
                        GS.emplace_back(c, opp);
                }
            }
        }
    }

    void print() const
    {
        cout << "T = {";
        for (size_t i = 0; i < L.size(); ++i)
        {
            cout << L[i];
            if (i + 1 < L.size())
                cout << ", ";
        }
        cout << "}" << endl;
    }

    // helper: remove chord if exists, return true if removed
    bool removeChord(const Chord &c)
    {
        for (auto it = L.begin(); it != L.end(); ++it)
        {
            if (*it == c)
            {
                L.erase(it);
                return true;
            }
        }
        return false;
    }
    // helper: insert chord if not present
    void insertChordUnique(const Chord &c)
    {
        for (const auto &x : L)
            if (x == c)
                return;
        L.push_back(c);
    }
};

// flip (1,j) -> (vo,vo') in-place; assumes gen. chord is present and opp valid
bool flipChordInPlace(Triangulation &T, const GeneratingChord &gen)
{
    const Chord toFlip = gen.c;         // (1, j)
    const pair<int, int> opp = gen.opp; // (vo, vo')

    if (!toFlip.isValid() || opp.first == -1 || opp.second == -1)
        return false;

    // remove (1,j)
    bool removed = T.removeChord(toFlip);
    if (!removed)
        return false; // chord wasn't present

    // add (vo, vo')
    T.insertChordUnique(Chord(opp.first, opp.second));

    // rebuild GS (functional but O(n)). Works correctly.
    T.rebuildGeneratingSet();
    return true;
}

bool unflipChordInPlace(Triangulation &T, const GeneratingChord &gen)
{
    const Chord toFlip = gen.c;         // original (1,j)
    const pair<int, int> opp = gen.opp; // (vo, vo')

    if (!toFlip.isValid() || opp.first == -1 || opp.second == -1)
        return false;

    // remove (vo, vo')
    bool removed = T.removeChord(Chord(opp.first, opp.second));
    if (!removed)
    {
        // it's possible it's already absent — but we expect it to be present
        // continue to try to insert original anyway
    }

    // restore (1,j)
    T.insertChordUnique(toFlip);

    T.rebuildGeneratingSet();
    return true;
}

// small safe Catalan using DP: triangulations count = C_{n-2}
long long catalanCount(int m)
{
    // return C_m = number of triangulations of polygon with m+2 vertices
    vector<long long> C(m + 1, 0);
    C[0] = 1;
    for (int i = 1; i <= m; ++i)
    {
        long long sum = 0;
        for (int k = 0; k < i; ++k)
            sum += C[k] * C[i - 1 - k];
        C[i] = sum;
    }
    return C[m];
}

// Global counter
long long triangulationCount = 0;

void findAllChildTriangulationsCycle(Triangulation &T)
{
    // Output current triangulation
    ++triangulationCount;
    cout << triangulationCount << ". ";
    T.print();

    // if no generating chords, return
    if (T.GS.empty())
        return;

    // copy GS for stable iteration (we mutate T inside)
    vector<GeneratingChord> curGS = T.GS;

    for (const auto &g : curGS)
    {
        // Double-check the chord still exists and its opposite is valid in current T
        // recompute current opposite for safety (it may have changed)
        auto maybeOpp = T.findOppositePair(g.c.j);
        if (maybeOpp.first == -1)
            continue; // can't flip this now

        GeneratingChord currentGen(g.c, maybeOpp);
        bool flipped = flipChordInPlace(T, currentGen);
        if (!flipped)
            continue;

        findAllChildTriangulationsCycle(T);

        // unflip using the same chord/pair we used to flip (we recomputed opp earlier)
        unflipChordInPlace(T, currentGen);
    }
}

void findAllTriangulationsCycle(int n)
{
    long long expected = catalanCount(n - 2);

    cout << "Generating all triangulations of cycle with " << n << " vertices\n";
    cout << "Expected number (Catalan C(" << n - 2 << ")): " << expected << "\n";
    cout << "=============================================================\n";

    Triangulation root(n);
    // root L = {(1, n-1), (1, n-2), ..., (1,3)}
    for (int j = n - 1; j >= 3; --j)
        root.L.push_back(Chord(1, j));
    root.rebuildGeneratingSet();

    triangulationCount = 0;
    // list root
    ++triangulationCount;
    cout << triangulationCount << ". Root: ";
    root.print();

    vector<GeneratingChord> rootGS = root.GS;
    for (const auto &g : rootGS)
    {
        auto opp = root.findOppositePair(g.c.j);
        if (opp.first == -1)
            continue;
        GeneratingChord gc(g.c, opp);
        if (!flipChordInPlace(root, gc))
            continue;
        findAllChildTriangulationsCycle(root);
        unflipChordInPlace(root, gc);
    }

    cout << "=============================================================\n";
    cout << "Total triangulations: " << triangulationCount << "\n";
    cout << "Expected: " << expected << "\n";
    cout << (triangulationCount == expected ? "✓ SUCCESS\n" : "✗ MISMATCH\n");
}

int main()
{
    cout << "Triangulation generator test\n\n";
    for (int n = 4; n <= 10; ++n)
    {
        findAllTriangulationsCycle(n);
        cout << "\n";
    }
    return 0;
}
