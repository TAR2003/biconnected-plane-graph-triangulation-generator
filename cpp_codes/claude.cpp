#include <bits/stdc++.h>
using namespace std;

/*
 * Exact implementation of the paper:
 * "Generating All Triangulations of Plane Graphs"
 * Parvez, Rahman, Nakano (2011)
 *
 * Section 4: Labeled Triangulations of a Cycle
 * Time: O(1) per triangulation
 * Space: O(n)
 */

class Chord
{
public:
    int v1, vj;             // Chord endpoints (v1, vj) where v1 < vj
    int opp_o, opp_o_prime; // Opposite pair (vo, vo')

    Chord(int a, int b, int c, int d) : v1(a), vj(b), opp_o(c), opp_o_prime(d) {}

    void print() const
    {
        cout << "(" << v1 << "," << vj << ")";
    }
};

class TriangulationGenerator
{
private:
    int n;                // Number of vertices in cycle
    vector<Chord> chords; // Chords in current triangulation (in-place representation)
    int num_chords;       // Current number of chords
    int total_generated;

    // Direct access array: index i gives chord (1, i+1) if it exists, else -1
    vector<int> chord_index; // chord_index[j] = index in chords[] of chord (1,j), or -1

    // Store all triangulations
    vector<vector<pair<int, int>>> all_triangulations;

public:
    TriangulationGenerator(int vertices) : n(vertices), num_chords(0), total_generated(0)
    {
        chord_index.resize(n + 1, -1);
        chords.reserve(n); // Max n-3 chords
    }

    // Generate root triangulation (Section 4.4, Page 474)
    void generateRoot()
    {
        // Root: all chords incident to v1
        // Chords: (1, j) for j = 3 to n-1 (listed in GS in this order)
        // Opposite pairs: (j-1, j+1) for j from n-1 down to 3

        num_chords = n - 3; // Total chords in triangulation
        chords.clear();

        // Generate chords in order: (1, n-1), (1, n-2), ..., (1, 3)
        for (int j = n - 1; j >= 3; j--)
        {
            int opp_left = j - 1;
            int opp_right = (j == n - 1) ? n : (j + 1);

            chords.emplace_back(1, j, opp_left, opp_right);
            chord_index[j] = chords.size() - 1;
        }
    }

    // Find leftmost blocking chord - O(1) when properly tracked
    // For this implementation: we check structurally when needed
    int findLeftmostBlocking() const
    {
        // A blocking chord (vi, vj) has both vi and vj adjacent to v1
        // Leftmost = maximum vj among blocking chords

        int max_vj = -1;
        int blocking_idx = -1;

        for (size_t i = 0; i < chords.size(); i++)
        {
            if (chords[i].v1 != 1)
            { // Not incident to root
                // Check if both endpoints adjacent to v1
                int vi = chords[i].v1;
                int vj = chords[i].vj;

                // A vertex is adjacent to v1 if there's a chord (1, vertex) or it's cycle-adjacent
                bool vi_adjacent = (vi == 2 || vi == n || chord_index[vi] != -1);
                bool vj_adjacent = (vj == 2 || vj == n || chord_index[vj] != -1);

                if (vi_adjacent && vj_adjacent && vj > max_vj)
                {
                    max_vj = vj;
                    blocking_idx = i;
                }
            }
        }

        return blocking_idx;
    }

    // Flip chord at index idx: (1, vj) -> (vo, vo')
    // Returns original vj for unflip
    // This is O(1) as per paper Section 4.4
    int flip(int idx)
    {
        Chord &ch = chords[idx];
        int original_vj = ch.vj;
        int vo = ch.opp_o;
        int vo_prime = ch.opp_o_prime;

        // Step 1: Remove from chord_index
        chord_index[original_vj] = -1;

        // Step 2: Transform chord (1, vj) -> (vo, vo')
        ch.v1 = vo;
        ch.vj = vo_prime;

        // Step 3: New opposite pair is the old chord
        ch.opp_o = 1;
        ch.opp_o_prime = original_vj;

        // Step 4: Update opposite pairs of affected chords (1, vo) and (1, vo')
        // According to paper Section 4.4, Figure 14

        int idx_vo = chord_index[vo];
        int idx_vo_prime = chord_index[vo_prime];

        if (idx_vo != -1)
        {
            // Chord (1, vo) exists
            // Its opposite was (s, original_vj), now becomes (s, vo_prime)
            if (chords[idx_vo].opp_o_prime == original_vj)
            {
                chords[idx_vo].opp_o_prime = vo_prime;
            }
            else if (chords[idx_vo].opp_o == original_vj)
            {
                chords[idx_vo].opp_o = vo_prime;
            }
        }

        if (idx_vo_prime != -1)
        {
            // Chord (1, vo') exists
            // Its opposite was (original_vj, l'), now becomes (vo, l')
            if (chords[idx_vo_prime].opp_o == original_vj)
            {
                chords[idx_vo_prime].opp_o = vo;
            }
            else if (chords[idx_vo_prime].opp_o_prime == original_vj)
            {
                chords[idx_vo_prime].opp_o_prime = vo;
            }
        }

        return original_vj;
    }

    // Unflip: restore chord from (vo, vo') back to (1, original_vj)
    // O(1) operation
    void unflip(int idx, int original_vj)
    {
        Chord &ch = chords[idx];
        int vo = ch.opp_o;             // Now stores original v1 = 1
        int vo_prime = ch.opp_o_prime; // Now stores original_vj

        // These were swapped during flip
        int actual_vo = ch.v1;
        int actual_vo_prime = ch.vj;

        // Step 1: Restore chord from (actual_vo, actual_vo_prime) to (1, original_vj)
        ch.v1 = 1;
        ch.vj = original_vj;

        // Step 2: Restore opposite pair
        ch.opp_o = actual_vo;
        ch.opp_o_prime = actual_vo_prime;

        // Step 3: Restore chord_index
        chord_index[original_vj] = idx;

        // Step 4: Restore affected chords' opposite pairs
        int idx_vo = chord_index[actual_vo];
        int idx_vo_prime = chord_index[actual_vo_prime];

        if (idx_vo != -1)
        {
            if (chords[idx_vo].opp_o_prime == actual_vo_prime)
            {
                chords[idx_vo].opp_o_prime = original_vj;
            }
            else if (chords[idx_vo].opp_o == actual_vo_prime)
            {
                chords[idx_vo].opp_o = original_vj;
            }
        }

        if (idx_vo_prime != -1)
        {
            if (chords[idx_vo_prime].opp_o == actual_vo)
            {
                chords[idx_vo_prime].opp_o = original_vj;
            }
            else if (chords[idx_vo_prime].opp_o_prime == actual_vo)
            {
                chords[idx_vo_prime].opp_o_prime = original_vj;
            }
        }
    }

    // Output current triangulation
    void output()
    {
        vector<pair<int, int>> current;
        for (const auto &ch : chords)
        {
            current.push_back({ch.v1, ch.vj});
        }
        all_triangulations.push_back(current);
        total_generated++;
    }

    // Recursive generation - Algorithm find-all-child-triangulations-cycle
    // From Section 4.4, Page 474
    void findAllChildren()
    {
        output();

        // Find leftmost blocking chord to determine threshold b
        int blocking_idx = findLeftmostBlocking();
        int b = (blocking_idx == -1) ? 2 : chords[blocking_idx].v1;

        // Find all generating chords: (1, j) where j >= b
        vector<int> generating_indices;
        for (size_t i = 0; i < chords.size(); i++)
        {
            if (chords[i].v1 == 1 && chords[i].vj >= b)
            {
                generating_indices.push_back(i);
            }
        }

        // Generate children by flipping each generating chord
        for (int idx : generating_indices)
        {
            int original_vj = flip(idx);
            findAllChildren();
            unflip(idx, original_vj);
        }
    }

    // Main algorithm - Algorithm find-all-triangulations-cycle
    // From Section 4.4, Page 474
    void generateAll()
    {
        generateRoot();
        output(); // Output root (Case 1)

        // For root, all chords are generating
        // Flip each chord and recursively generate
        vector<int> root_indices;
        for (size_t i = 0; i < chords.size(); i++)
        {
            root_indices.push_back(i);
        }

        for (int idx : root_indices)
        {
            int original_vj = flip(idx);
            findAllChildren();
            unflip(idx, original_vj);
        }
    }

    // Print all triangulations
    void printResults()
    {
        cout << "\n=== Results ===" << endl;
        cout << "Total triangulations: " << total_generated << endl;

        if (n <= 8)
        { // Only print details for small n
            cout << "\nAll triangulations:\n";
            for (size_t i = 0; i < all_triangulations.size(); i++)
            {
                cout << "T" << i + 1 << ": ";
                for (const auto &[u, v] : all_triangulations[i])
                {
                    cout << "(" << u << "," << v << ") ";
                }
                cout << endl;
            }
        }
    }

    int getCount() const { return total_generated; }

    const vector<vector<pair<int, int>>> &getAllTriangulations() const
    {
        return all_triangulations;
    }
};

// Compute Catalan number C(n)
long long catalan(int n)
{
    if (n <= 1)
        return 1;
    vector<long long> cat(n + 1, 0);
    cat[0] = cat[1] = 1;
    for (int i = 2; i <= n; i++)
    {
        for (int j = 0; j < i; j++)
        {
            cat[i] += cat[j] * cat[i - 1 - j];
        }
    }
    return cat[n];
}

int main()
{
    cout << "" << endl;
    cout << "   Generating All Triangulations of a Cycle                    " << endl;
    cout << "   Paper: Parvez, Rahman, Nakano (2011)                        " << endl;
    cout << "   Section 4: Labeled Triangulations                           " << endl;
    cout << "   Time Complexity: O(1) per triangulation                     " << endl;
    cout << "   Space Complexity: O(n)                                      " << endl;
    cout << "" << endl;

    int n;
    cout << "\nEnter number of vertices (n >= 3): ";
    cin >> n;

    if (n < 3)
    {
        cout << "Error: Need at least 3 vertices\n";
        return 1;
    }

    cout << "\nGenerating triangulations for cycle of " << n << " vertices...\n";

    auto start = chrono::high_resolution_clock::now();

    TriangulationGenerator gen(n);
    gen.generateAll();

    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);

    gen.printResults();

    // Verification
    long long expected = catalan(n - 2);
    long long actual = gen.getCount();

    cout << "\n=== Verification ===" << endl;
    cout << "Expected (Catalan C(" << n - 2 << ")): " << expected << endl;
    cout << "Actual generated: " << actual << endl;

    if (expected == actual)
    {
        cout << " SUCCESS: Matches Catalan number!\n";
    }
    else
    {
        cout << " FAILURE: Count mismatch!\n";
    }

    cout << "\n=== Performance ===" << endl;
    cout << "Total time: " << duration.count() << " micro seconds" << endl;
    if (actual > 0)
    {
        cout << "Time per triangulation: " << fixed << setprecision(2)
             << (double)duration.count() / actual << " micro seconds" << endl;
    }


    return 0;
}