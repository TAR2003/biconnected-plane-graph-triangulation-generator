#include <bits/stdc++.h>
using namespace std;

/*
  All-triangulations of a labeled cycle, following:
  Parvez, Rahman, Nakano (JGAA 2011) "Generating All Triangulations of Plane Graphs",
  Section 4 (triangulations of a cycle).
  Key ideas used (matching the paper):
    - Root triangulation Tr: all chords incident to v1 (Section 4.3).
    - Generating set GS: chords (v1, vj) where j >= b, with (vb, vb') the leftmost blocking chord (Section 4.2, 4.4).
    - Opposite pair O for a generating chord (v1, vj) is (vo, vo') so that <v1, vo, vj, vo'> is a quadrilateral.
      We realize O implicitly via a doubly-linked list on indices [2..n] so that opposite(j) = (left[j], right[j]).
      This matches the O(1) update rules depicted in Figures 13–14 (Section 4.3).
    - Flipping a generating chord (v1, vj) creates the leftmost blocking chord (vo, vo') in the child (Lemma 5/6).

  Program goal (as requested):
    For each face (list of labels), compute the UNION of all chord pairs that occur across ALL triangulations.
    We do not need to materialize every triangulation's full chord list; it suffices to:
      * insert all root chords (v1, vj) for j=3..n-1, and
      * whenever we flip (v1, vj), also insert the newly created blocking chord (left[j], right[j]).
    Over the recursion, this collects exactly all possible chords for the face.

  I/O:
    Input file:  "input.txt"  (see format in the header of this source)
    Output file: "output.txt"

  Complexity:
    The traversal visits each triangulation once (paper guarantees O(1) delay with the representation).
    Our union collection adds O(1) work per flip. Memory is O(n) per face.
*/

struct Triangulator
{
    // labels in clockwise order: labels[0] = v1, labels[1] = v2, ..., labels[n-1] = vn
    vector<int> labels;
    int n;

    // Doubly linked neighbors over the index sequence 2..n (we allocate [0..n+1] for convenience).
    // We index vertices by their position in 'labels' + 1, so positions are 1..n.
    // left[j] and right[j] give the immediate neighbors in this linear order.
    vector<int> leftNeighbor, rightNeighbor;

    // present[j] tells whether the chord (v1, vj) is currently present (i.e., still a candidate chord)
    // Only meaningful for j in [3..n-1].
    vector<char> present;

    // Stack of blocking chords (by index positions) along the recursion path.
    // The chord currently added when flipping j is (leftBefore, rightBefore).
    vector<pair<int, int>> blockingStack;

    // For generating set: if root, no blocking; otherwise after a flip, leftmost blocking is the last pushed.
    bool hasLeftmostBlocking = false;
    int b_idx = -1, bp_idx = -1; // indices in 1..n (positions), representing (vb, vb')

    // Collected union of chord pairs for this face (using actual labels, ordered (min, max)).
    set<pair<int, int>> chordUnion;

    Triangulator(const vector<int> &faceLabels) : labels(faceLabels)
    {
        n = (int)labels.size();
        leftNeighbor.assign(n + 2, 0);
        rightNeighbor.assign(n + 2, 0);
        present.assign(n + 2, 0);
    }

    // Map index position j in [1..n] to the actual vertex label.
    inline int L(int j) const { return labels[j - 1]; }

    // Insert a chord into the union set using vertex labels, ensuring (a<b).
    inline void addChordByIdx(int a_idx, int b_idx)
    {
        int a = L(a_idx), b = L(b_idx);
        if (a > b)
            std::swap(a, b);
        chordUnion.insert({a, b});
    }

    // Initialize the root triangulation representation.
    void initRoot()
    {
        // Build the doubly-linked list over [2..n] with sentinels 2 and n participating.
        for (int j = 2; j <= n; ++j)
        {
            leftNeighbor[j] = j - 1;
            rightNeighbor[j] = j + 1;
        }
        rightNeighbor[n] = n; // self to denote end; not strictly needed but safe.

        // Root: chords (v1, vj) for j = 3..n-1 are present (candidate + generating).
        for (int j = 3; j <= n - 1; ++j)
        {
            present[j] = 1;
            // Add root chords to the union output right away.
            addChordByIdx(1, j);
        }
        hasLeftmostBlocking = false; // no blocking chord at the root
        b_idx = bp_idx = -1;
        blockingStack.clear();
    }

    // Enumerate all triangulations via the genealogical tree traversal, collecting union of chords.
    void enumerateAll()
    {
        // Determine generating set boundary:
        int startJ = 3;
        if (hasLeftmostBlocking)
        {
            // Generating chords are (v1, vj) with j >= b (paper Section 4.4 Case 2).
            startJ = max(startJ, b_idx);
        }

        // Try each generating chord (v1, vj)
        for (int j = startJ; j <= n - 1; ++j)
        {
            if (!present[j])
                continue; // not a current candidate chord

            // Opposite pair before flip: (vo, vo') = (left[j], right[j])
            int vo = leftNeighbor[j];
            int vop = rightNeighbor[j];

            // Record the blocking chord created by this flip
            addChordByIdx(vo, vop);

            // Save state needed to backtrack:
            int leftBefore = leftNeighbor[j];
            int rightBefore = rightNeighbor[j];
            int rightLeftOfJ = rightNeighbor[leftNeighbor[j]];
            int leftRightOfJ = leftNeighbor[rightNeighbor[j]];

            // Splice out j from the neighbor list (O(1)), as per the O updates in Section 4.3
            rightNeighbor[leftNeighbor[j]] = rightNeighbor[j];
            leftNeighbor[rightNeighbor[j]] = leftNeighbor[j];
            present[j] = 0;

            // Push the newly created blocking chord; it is the leftmost blocking chord in the child (Lemma 5/6)
            blockingStack.push_back({vo, vop});
            bool prevHasBlock = hasLeftmostBlocking;
            int prev_b = b_idx, prev_bp = bp_idx;
            hasLeftmostBlocking = true;
            b_idx = vo;
            bp_idx = vop;

            // Recurse into the child triangulation
            enumerateAll();

            // --- Backtrack: undo everything in O(1) ---
            // Restore leftmost blocking chord status
            hasLeftmostBlocking = prevHasBlock;
            b_idx = prev_b;
            bp_idx = prev_bp;
            blockingStack.pop_back();

            // Reinsert (v1, vj): relink neighbors
            // Carefully restore the exact previous links
            leftNeighbor[j] = leftBefore;
            rightNeighbor[j] = rightBefore;
            rightNeighbor[leftBefore] = j;
            leftNeighbor[rightBefore] = j;
            present[j] = 1;

            // (No need to remove the union chord; union keeps all chords seen.)
            (void)rightLeftOfJ;
            (void)leftRightOfJ; // silence unused warnings (documentation remnants)
        }
    }

    // Public API: compute and return the union of all chord pairs for the face.
    vector<pair<int, int>> computeChordUnion()
    {
        if (n <= 2)
            return {}; // degenerate
        if (n == 3)
            return {}; // triangle has no chords

        initRoot();
        enumerateAll();

        // Move from set to sorted vector
        vector<pair<int, int>> res(chordUnion.begin(), chordUnion.end());

        // Exclude polygon boundary edges, in case they ever got inserted (shouldn't happen with logic above).
        // Still, we ensure we only keep *non-consecutive* pairs.
        auto isBoundary = [&](int a, int b) -> bool
        {
            // find positions of a and b
            unordered_map<int, int> pos;
            pos.reserve(n * 2);
            for (int i = 0; i < n; ++i)
                pos[labels[i]] = i; // 0-based
            int ia = pos[a], ib = pos[b];
            int diff = abs(ia - ib);
            return diff == 1 || diff == n - 1; // consecutive on the cycle
        };

        vector<pair<int, int>> filtered;
        filtered.reserve(res.size());
        // Precompute position map once
        unordered_map<int, int> pos;
        pos.reserve(n * 2);
        for (int i = 0; i < n; ++i)
            pos[labels[i]] = i; // 0-based

        auto isBoundaryFast = [&](int a, int b) -> bool
        {
            int ia = pos[a], ib = pos[b];
            int diff = abs(ia - ib);
            return diff == 1 || diff == n - 1;
        };

        for (auto &p : res)
        {
            if (!isBoundaryFast(p.first, p.second))
                filtered.push_back(p);
        }
        return filtered;
    }
};

// Utility: read faces from input.txt (format explained above)
static bool readFacesFromFile(const string &filename, vector<vector<int>> &faces)
{
    ifstream fin(filename);
    if (!fin)
        return false;
    int F;
    if (!(fin >> F))
        return false;
    faces.clear();
    faces.reserve(F);
    for (int i = 0; i < F; ++i)
    {
        int k;
        fin >> k;
        vector<int> face(k);
        for (int j = 0; j < k; ++j)
            fin >> face[j];
        faces.push_back(face);
    }
    return true;
}

// Utility: write result to output.txt
static bool writeChordsToFile(const string &filename,
                              const vector<vector<pair<int, int>>> &allFacesChords)
{
    ofstream fout(filename);
    if (!fout)
        return false;
    for (size_t i = 0; i < allFacesChords.size(); ++i)
    {
        fout << "Face " << (i + 1) << " chords:";
        for (auto &p : allFacesChords[i])
        {
            fout << " (" << p.first << "," << p.second << ")";
        }
        fout << "\n";
    }
    return true;
}

int main(int argc, char **argv)
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // If you want to supply your own input file names:
    // Usage: ./a.out [input.txt] [output.txt]
    string inFile = "input.txt";
    string outFile = "output.txt";
    if (argc >= 2)
        inFile = argv[1];
    if (argc >= 3)
        outFile = argv[2];

    vector<vector<int>> faces;
    if (!readFacesFromFile(inFile, faces))
    {
        // Fallback: read a single face from stdin as: k v1 v2 ... vk
        // Example:
        // 4 1 2 3 4
        int k;
        if (!(cin >> k))
        {
            cerr << "Failed to read input. Provide input.txt or stdin.\n";
            return 1;
        }
        vector<int> face(k);
        for (int i = 0; i < k; ++i)
            cin >> face[i];
        faces.push_back(face);
        // And try writing to default output.txt
    }

    vector<vector<pair<int, int>>> allFacesChords;
    allFacesChords.reserve(faces.size());

    for (const auto &face : faces)
    {
        Triangulator tri(face);
        auto chords = tri.computeChordUnion();
        // Sort lexicographically just to make output deterministic and tidy
        sort(chords.begin(), chords.end());
        allFacesChords.push_back(chords);
    }

    if (!writeChordsToFile(outFile, allFacesChords))
    {
        // If writing to file fails, print to stdout
        for (size_t i = 0; i < allFacesChords.size(); ++i)
        {
            cout << "Face " << (i + 1) << " chords:";
            for (auto &p : allFacesChords[i])
            {
                cout << " (" << p.first << "," << p.second << ")";
            }
            cout << "\n";
        }
    }

    return 0;
}
