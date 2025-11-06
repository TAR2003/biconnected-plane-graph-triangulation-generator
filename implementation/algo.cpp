#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

/**
 * Exact implementation (Section 4) of:
 * Parvez, Rahman, Nakano (2011) – Generating All Triangulations of Plane Graphs
 *
 * This program generates all labeled triangulations of a cycle C of n vertices
 * with O(1) time per triangulation and O(n) space, by:
 *  - Maintaining only local state (arrays) and performing in-place flips
 *  - Updating at most two opposite pairs per flip (Figures 13–14 in the paper)
 *  - Traversing the genealogical tree T(C) in DFS order
 *  - Outputting only the difference (the flipped chord), which is O(1) per node
 */

struct FlipState
{
    int j;              // flipped chord (1, j)
    int prev_j;         // previous neighbor among present chords
    int next_j;         // next neighbor among present chords
    int old_b;          // previous b (leftmost blocking chord's lower endpoint)
    bool old_has_block; // whether parent had a blocking chord
    int i;              // neighbor > j at time of flip
    int k;              // neighbor < j at time of flip
    int old_o1_i;       // saved opposite of (1,i)
    int old_o2_k;       // saved opposite of (1,k)
};

class CycleEnumerator
{
public:
    explicit CycleEnumerator(int n_) : n(n_)
    {
        reset();
    }

    // Run full enumeration and return count
    long long run(bool verbose)
    {
        count = 0;
        this->verbose = verbose;

        // Output root (no flip needed). This is T_r in the paper.
        output_root();

        // Case 1 in Section 4.4: root has all generating chords (n-3 of them)
        for (int j = lastPresent; j >= firstPresent; j = prevIdx[j])
        {
            flip_push(j);
            output_flip(j);       // Output T(1, j) difference in O(1)
            enumerate_children(); // Recurse (Case 2 in Section 4.4)
            undo_pop();
        }
        return count;
    }

private:
    int n; // vertices v1..vn
    // Chords incident to v1 that are currently present: (1, j) for j in [3..n-1]
    vector<char> present;         // present[j] ∈ {0,1}
    vector<int> nextIdx, prevIdx; // doubly-linked list over present indices
    vector<int> o1, o2;           // opposite pairs for candidate chords (1, j)

    int firstPresent, lastPresent; // list ends among present chords

    // Current leftmost blocking chord (vb, vb'), we store only b (lower endpoint)
    // Root has no blocking chord. Afterwards, has_block = true and b is valid.
    bool has_block = false;
    int b = 3;

    // Output and options
    bool verbose = false;
    long long count = 0;

    // Stack of flips to undo (for in-place backtracking)
    vector<FlipState> stk;

    void reset()
    {
        present.assign(n + 1, 0);
        nextIdx.assign(n + 1, 0);
        prevIdx.assign(n + 1, 0);
        o1.assign(n + 1, 0);
        o2.assign(n + 1, 0);

        // Root triangulation: all chords (1, j) present for j = 3..n-1
        firstPresent = 3;
        lastPresent = n - 1;
        for (int j = 3; j <= n - 1; ++j)
        {
            present[j] = 1;
            nextIdx[j] = (j == n - 1 ? 0 : j + 1);
            prevIdx[j] = (j == 3 ? 0 : j - 1);
            // Opposite pair in the root: (j-1, j+1)
            o1[j] = j - 1; // may be 2 when j=3 (on boundary)
            o2[j] = j + 1; // may be n when j=n-1 (on boundary)
        }
        has_block = false; // root has full vision
        b = 3;
        stk.clear();
    }

    void output_root()
    {
        ++count;
        if (verbose)
        {
            cout << "Root (all chords incident to v1)" << '\n';
        }
    }

    inline void output_flip(int j)
    {
        // Output difference only: flip (1, j) -> (o1[j], o2[j]) at the moment of flip
        ++count;
        if (verbose)
        {
            cout << "flip (1," << j << ") -> (" << o1[j] << "," << o2[j] << ")" << '\n';
        }
    }

    // Perform flip of (1, j) in O(1) time and push state for undo
    inline void flip_push(int j)
    {
        FlipState st{};
        st.j = j;
        st.prev_j = prevIdx[j];
        st.next_j = nextIdx[j];
        st.old_b = b;
        st.old_has_block = has_block;

        int i = nextIdx[j];
        int k = prevIdx[j];
        st.i = i;
        st.k = k;

        // The opposite pair for (1,j) BEFORE the flip is (vo, vo') = (k, i)
        // because in the invariant we maintain o1[j] = k and o2[j] = i
        const int vo = o1[j];
        const int vop = o2[j];

        // Save opposite entries to restore later (only two affected neighbors)
        if (i)
            st.old_o1_i = o1[i];
        if (k)
            st.old_o2_k = o2[k];

        // Remove (1, j) from the star of v1 (unlink from doubly-linked list)
        present[j] = 0;
        if (k)
            nextIdx[k] = i;
        else
            firstPresent = i;
        if (i)
            prevIdx[i] = k;
        else
            lastPresent = k;

        // Update opposite pairs locally (Figures 13–14):
        // For neighbor i (> j): opposite (1,i) becomes (vo, old_o2_i)
        if (i)
            o1[i] = vo;
        // For neighbor k (< j): opposite (1,k) becomes (old_o1_k, vop)
        if (k)
            o2[k] = vop;

        // New leftmost blocking chord becomes (vo, vop) (Lemma 5)
        has_block = true;
        b = vo; // lower endpoint of the leftmost blocking chord

        stk.push_back(st);
    }

    // Undo last flip in O(1)
    inline void undo_pop()
    {
        auto st = stk.back();
        stk.pop_back();

        int j = st.j;
        int i = st.i;
        int k = st.k;

        // Restore opposite pairs of neighbors
        if (i)
            o1[i] = st.old_o1_i;
        if (k)
            o2[k] = st.old_o2_k;

        // Reinsert (1, j) between k and i
        present[j] = 1;
        prevIdx[j] = k;
        nextIdx[j] = i;
        if (k)
            nextIdx[k] = j;
        else
            firstPresent = j;
        if (i)
            prevIdx[i] = j;
        else
            lastPresent = j;

        // Restore leftmost blocking state
        has_block = st.old_has_block;
        b = st.old_b;
    }

    // Enumerate children of current triangulation (we are not at root here)
    void enumerate_children()
    {
        if (!has_block)
            return; // should not happen after first flip

        // All generating chords are (1, j) with j ≥ b among PRESENT chords.
        // Iterate from the largest present j down to b using prev pointers.
        for (int j = lastPresent; j && j >= b; j = prevIdx[j])
        {
            flip_push(j);
            output_flip(j);
            enumerate_children();
            undo_pop();
        }
    }
};

static long long catalan(int k)
{
    // Compute Catalan number C(k) = binom(2k,k)/(k+1)
    long long res = 1;
    for (int i = 1; i <= k; ++i)
    {
        res = res * (k + i) / i; // multiply by (k+i)/i progressively to avoid overflow
    }
    return res / (k + 1);
}

int main()
{
    cout << "Triangulation Generation (Exact O(1) per triangulation, O(n) space)" << '\n';
    cout << "Reference: Parvez Rahman Nakano (JGAA 2011), Section 4" << "\n\n";

    for (int n : {6, 7, 8})
    {
        cout << "n = " << n << '\n';
        CycleEnumerator enumr(n);
        // verbose=false: do not print each flip; set true to see differences
        bool verbose = false; // Set true to see each flip
        long long cnt = enumr.run(verbose);
        cout << "Generated: " << cnt << '\n';
        cout << "Expected Catalan C(" << (n - 2) << ") = " << catalan(n - 2) << '\n';
        cout << (cnt == catalan(n - 2) ? "OK" : "MISMATCH") << "\n\n";
    }
    return 0;
}
