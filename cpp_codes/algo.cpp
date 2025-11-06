// prn_cycle_exact_pass_block.cpp
// C++17
#include <bits/stdc++.h>
using namespace std;

/*
  Exact Section 4 (cycle) implementation of:
  Parvez, Rahman, Nakano (2011) — "Generating All Triangulations of Plane Graphs"

  - Vertices labeled 1..n
  - Root is vertex 1
  - In-place chord objects; flip/unflip in O(1) by local updates + flip-record stack
  - Incremental leftmost-blocking tracking (no global scan)
  - Passing newly generated blocking chord index into recursion
*/

struct Chord
{
    int v1, vj;         // endpoints
    int opp_o, opp_o_p; // opposite pair (vo, vo')
    Chord(int a = 0, int b = 0, int c = 0, int d = 0) : v1(a), vj(b), opp_o(c), opp_o_p(d) {}
};

struct FlipRecord
{
    int chord_idx;
    int original_vj;

    // saved neighbor data for (1,vo)
    int vo_idx;
    int vo_opp_o;
    int vo_opp_o_p;

    // saved neighbor data for (1,vop)
    int vop_idx;
    int vop_opp_o;
    int vop_opp_o_p;

    // saved leftmost-blocking state
    int prev_leftmost_idx;
    int prev_leftmost_b;
};

class PRN_Cycle
{
public:
    int n;
    vector<Chord> chords;      // in-place chord objects
    vector<int> chord_index;   // chord_index[j] = index into chords for chord (1,j) or -1
    int leftmost_blocking_idx; // index into chords of leftmost blocking chord (or -1)
    int leftmost_blocking_b;   // b value (left endpoint) or -1
    vector<FlipRecord> flipStack;

    vector<vector<pair<int, int>>> allTriangulations;
    long long generated = 0;

    PRN_Cycle(int n_ = 0) { init(n_); }

    void init(int n_)
    {
        n = n_;
        chord_index.assign(n + 1, -1);
        chords.clear();
        leftmost_blocking_idx = -1;
        leftmost_blocking_b = -1;
        flipStack.clear();
        allTriangulations.clear();
        generated = 0;
    }

    inline int nxt(int v) const { return (v == n ? 1 : v + 1); }
    inline int prv(int v) const { return (v == 1 ? n : v - 1); }

    void generateRoot()
    {
        chords.clear();
        fill(chord_index.begin(), chord_index.end(), -1);
        leftmost_blocking_idx = -1;
        leftmost_blocking_b = -1;
        flipStack.clear();
        allTriangulations.clear();
        generated = 0;

        // Create chords (1, j) for j = n-1 downto 3
        for (int j = n - 1; j >= 3; --j)
        {
            int vo = j - 1;
            int vop = (j == n - 1) ? 2 : (j + 1); // proper wrap: for (1,n-1) use (n-2,2)
            chords.emplace_back(1, j, vo, vop);
            chord_index[j] = (int)chords.size() - 1;
        }
    }

    // Flip chord at index idx (must currently be (1, original_vj))
    // Mutates chord into blocking chord (vo, vop). Saves minimal state for unflip.
    void flip_inplace(int idx)
    {
        Chord &ch = chords[idx];
        int original_vj = ch.vj;
        int vo = ch.opp_o;
        int vop = ch.opp_o_p;

        FlipRecord rec;
        rec.chord_idx = idx;
        rec.original_vj = original_vj;

        // save neighbor (1,vo)
        rec.vo_idx = (vo >= 1 && vo <= n) ? chord_index[vo] : -1;
        if (rec.vo_idx != -1)
        {
            rec.vo_opp_o = chords[rec.vo_idx].opp_o;
            rec.vo_opp_o_p = chords[rec.vo_idx].opp_o_p;
        }
        else
            rec.vo_opp_o = rec.vo_opp_o_p = -1;

        // save neighbor (1,vop)
        rec.vop_idx = (vop >= 1 && vop <= n) ? chord_index[vop] : -1;
        if (rec.vop_idx != -1)
        {
            rec.vop_opp_o = chords[rec.vop_idx].opp_o;
            rec.vop_opp_o_p = chords[rec.vop_idx].opp_o_p;
        }
        else
            rec.vop_opp_o = rec.vop_opp_o_p = -1;

        // save leftmost state
        rec.prev_leftmost_idx = leftmost_blocking_idx;
        rec.prev_leftmost_b = leftmost_blocking_b;

        flipStack.push_back(rec);

        // remove (1, original_vj)
        chord_index[original_vj] = -1;

        // mutate into blocking chord (vo, vop)
        ch.v1 = vo;
        ch.vj = vop;

        // new opposite is old diagonal
        ch.opp_o = 1;
        ch.opp_o_p = original_vj;

        // update (1,vo) replace original_vj -> vop
        if (rec.vo_idx != -1)
        {
            Chord &cvo = chords[rec.vo_idx];
            if (cvo.opp_o == original_vj)
                cvo.opp_o = vop;
            else if (cvo.opp_o_p == original_vj)
                cvo.opp_o_p = vop;
        }
        // update (1,vop) replace original_vj -> vo
        if (rec.vop_idx != -1)
        {
            Chord &cvop = chords[rec.vop_idx];
            if (cvop.opp_o == original_vj)
                cvop.opp_o = vo;
            else if (cvop.opp_o_p == original_vj)
                cvop.opp_o_p = vo;
        }

        // update leftmost blocking incrementally with tie-break (choose leftmost vo if vj equal)
        if (leftmost_blocking_idx == -1)
        {
            leftmost_blocking_idx = idx;
            leftmost_blocking_b = vo;
        }
        else
        {
            Chord &curL = chords[leftmost_blocking_idx];
            if ((ch.vj > curL.vj) || (ch.vj == curL.vj && vo < curL.v1))
            {
                leftmost_blocking_idx = idx;
                leftmost_blocking_b = vo;
            }
        }
    }

    // Unflip last flip (restore the in-place chord and neighbors, and leftmost state)
    void unflip_inplace()
    {
        if (flipStack.empty())
            return;
        FlipRecord rec = flipStack.back();
        flipStack.pop_back();

        Chord &ch = chords[rec.chord_idx];
        int original_vj = rec.original_vj;

        // current ch is (vo, vop) with opp (1, original_vj)
        int vo = ch.v1;
        int vop = ch.vj;

        // restore chord to (1, original_vj)
        ch.v1 = 1;
        ch.vj = original_vj;

        // restore its opposite pair to (vo, vop)
        ch.opp_o = vo;
        ch.opp_o_p = vop;

        // restore chord_index
        chord_index[original_vj] = rec.chord_idx;

        // restore (1,vo) opps if existed
        if (rec.vo_idx != -1)
        {
            chords[rec.vo_idx].opp_o = rec.vo_opp_o;
            chords[rec.vo_idx].opp_o_p = rec.vo_opp_o_p;
        }
        // restore (1,vop) opps if existed
        if (rec.vop_idx != -1)
        {
            chords[rec.vop_idx].opp_o = rec.vop_opp_o;
            chords[rec.vop_idx].opp_o_p = rec.vop_opp_o_p;
        }

        // restore leftmost-blocking
        leftmost_blocking_idx = rec.prev_leftmost_idx;
        leftmost_blocking_b = rec.prev_leftmost_b;
    }

    // Record current triangulation (store chords as pairs)
    void record_current()
    {
        vector<pair<int, int>> cur;
        cur.reserve(chords.size());
        for (auto &c : chords)
            cur.emplace_back(c.v1, c.vj);
        allTriangulations.push_back(move(cur));
        ++generated;
    }

    // Recursive generator: pass the index of the blocking chord created by the caller (or -1 for root)
    void findAllChildren_passedBlocking(int blocking_idx)
    {
        // record current triangulation
        record_current();

        // threshold b: if blocking_idx == -1 => b = 2 else b = chords[blocking_idx].v1
        int b = (blocking_idx == -1 ? 2 : chords[blocking_idx].v1);

        // iterate j from b..n-1 (paper's generating chords are (1,j) with j >= b)
        for (int j = b; j <= n - 1; ++j)
        {
            int idx = chord_index[j];
            if (idx == -1)
                continue;
            // flip chord at idx -> creates blocking chord at same idx
            flip_inplace(idx);
            // pass the newly created blocking chord idx down
            findAllChildren_passedBlocking(idx);
            // unflip
            unflip_inplace();
        }
    }

    // top-level: build root and start recursion
    void generateAll()
    {
        generateRoot();
        // start recursion passing -1 (no blocking chord at root)
        findAllChildren_passedBlocking(-1);
    }

    // void generateRoot()
    // {
    //     chords.clear();
    //     fill(chord_index.begin(), chord_index.end(), -1);
    //     leftmost_blocking_idx = -1;
    //     leftmost_blocking_b = -1;
    //     flipStack.clear();
    //     allTriangulations.clear();
    //     generated = 0;

    //     for (int j = n - 1; j >= 3; --j)
    //     {
    //         int vo = j - 1;
    //         int vop = (j == n - 1) ? 2 : (j + 1);
    //         chords.emplace_back(1, j, vo, vop);
    //         chord_index[j] = (int)chords.size() - 1;
    //     }
    // }

    const vector<vector<pair<int, int>>> &getAll() const { return allTriangulations; }
    long long count() const { return generated; }
};

long long catalan(int m)
{
    if (m <= 1)
        return 1;
    vector<long long> C(m + 1, 0);
    C[0] = 1;
    for (int i = 1; i <= m; i++)
    {
        long long s = 0;
        for (int k = 0; k < i; k++)
            s += C[k] * C[i - 1 - k];
        C[i] = s;
    }
    return C[m];
}

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    cout << "PRN cycle exact generator (Section 4) — O(1) flips\n";
    int n;
    cout << "Enter n (>=3): ";
    if (!(cin >> n))
        return 0;
    if (n < 3)
    {
        cout << "need n>=3\n";
        return 0;
    }

    PRN_Cycle gen(n);
    auto t1 = chrono::high_resolution_clock::now();
    gen.generateAll();
    auto t2 = chrono::high_resolution_clock::now();

    long long actual = gen.count();
    long long expected = catalan(n - 2);

    cout << "Generated: " << actual << " triangulations\n";
    cout << "Expected (Catalan C(" << (n - 2) << ")) = " << expected << "\n";
    cout << (actual == expected ? "MATCH\n" : "MISMATCH\n");

    auto dur = chrono::duration_cast<chrono::microseconds>(t2 - t1).count();
    cout << "Time: " << dur << " microseconds\n";
    if (actual > 0)
        cout << "avg per triangulation: " << (double)dur / actual << " us\n";

    if (n <= 7)
    {
        auto &all = gen.getAll();
        for (size_t i = 0; i < all.size(); ++i)
        {
            cout << "T" << setw(2) << i + 1 << ": ";
            for (auto &p : all[i])
                cout << "(" << p.first << "," << p.second << ") ";
            cout << "\n";
        }
    }
    return 0;
}
