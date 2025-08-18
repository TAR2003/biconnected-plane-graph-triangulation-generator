#include <bits/stdc++.h>
using namespace std;
typedef long long ll;

struct TriangulationState
{
    vector<pair<ll, ll>> chords;                    // L: List of chords
    vector<pair<ll, ll>> generating_set;            // GS: Generating chords
    map<pair<ll, ll>, pair<ll, ll>> opposite_pairs; // O: Opposite pairs
    pair<ll, ll> leftmost_blocking;                 // Cache leftmost blocking chord
    bool is_root;
};

class CycleTriangulation
{
private:
    ll n;
    ll root_vertex;

public:
    CycleTriangulation(ll vertices, ll root = 0) : n(vertices), root_vertex(root) {}

    // Create root triangulation in O(n) time
    TriangulationState createRoot()
    {
        TriangulationState root_state;
        root_state.is_root = true;
        root_state.leftmost_blocking = {-1, -1}; // No blocking chord

        // Root has all chords incident to root_vertex
        for (ll j = 2; j < n - 1; j++)
        {
            pair<ll, ll> chord = {root_vertex, j};
            root_state.chords.push_back(chord);
            root_state.generating_set.push_back(chord);

            // Opposite pair for root: (j-1, j+1)
            root_state.opposite_pairs[chord] = {j - 1, j + 1};
        }

        return root_state;
    }

    // O(1) chord flipping using pre-computed opposite pairs
    TriangulationState flipChord(const TriangulationState &parent,
                                 pair<ll, ll> chord_to_flip)
    {
        if (parent.opposite_pairs.find(chord_to_flip) == parent.opposite_pairs.end())
        {
            throw invalid_argument("Chord not found in opposite pairs");
        }

        TriangulationState child = parent; // Copy parent state

        // O(1) chord replacement using opposite pairs
        pair<ll, ll> new_chord = parent.opposite_pairs.at(chord_to_flip);

        // Update chord list in O(1)
        for (auto &c : child.chords)
        {
            if (c == chord_to_flip)
            {
                c = new_chord;
                break;
            }
        }

        // Remove flipped chord from generating set
        child.generating_set.erase(
            remove(child.generating_set.begin(), child.generating_set.end(), chord_to_flip),
            child.generating_set.end());

        // Remove from opposite pairs
        child.opposite_pairs.erase(chord_to_flip);

        // Update leftmost blocking chord in O(1)
        child.leftmost_blocking = new_chord;
        child.is_root = false;

        // Update generating set based on new leftmost blocking
        updateGeneratingSet(child);

        // Update opposite pairs for affected chords in O(1)
        updateAffectedOppositePairs(child, chord_to_flip, new_chord);

        return child;
    }

private:
    // O(1) update of generating set
    void updateGeneratingSet(TriangulationState &state)
    {
        if (state.is_root)
            return;

        ll blocking_vertex = state.leftmost_blocking.second;
        state.generating_set.clear();

        // Only chords (root, j) where j >= blocking_vertex are generating
        for (const auto &chord : state.chords)
        {
            if (chord.first == root_vertex && chord.second >= blocking_vertex)
            {
                state.generating_set.push_back(chord);
            }
        }
    }

    // O(1) update of opposite pairs for affected chords
    void updateAffectedOppositePairs(TriangulationState &state,
                                     pair<ll, ll> flipped_chord,
                                     pair<ll, ll> new_chord)
    {
        // Based on paper's Figure 13-14: at most 2 chords are affected
        ll vi = flipped_chord.second;
        ll vk = flipped_chord.first;
        ll vo = new_chord.first;
        ll vo_prime = new_chord.second;

        // Update opposite pairs for the two affected chords
        for (auto &[chord, opposite] : state.opposite_pairs)
        {
            if (chord.first == root_vertex)
            {
                if (chord.second == vi)
                {
                    // Update opposite pair for (root, vi)
                    opposite = {vo, opposite.second};
                }
                else if (chord.second == vk)
                {
                    // Update opposite pair for (root, vk)
                    opposite = {opposite.first, vo_prime};
                }
            }
        }
    }

public:
    // Generate all triangulations using the corrected algorithm
    void generateAllTriangulations(const TriangulationState &current,
                                   vector<TriangulationState> &results)
    {
        results.push_back(current);

        // Generate children by flipping each generating chord
        for (const auto &generating_chord : current.generating_set)
        {
            try
            {
                TriangulationState child = flipChord(current, generating_chord);
                generateAllTriangulations(child, results);
            }
            catch (const exception &e)
            {
                // Skip invalid flips
                continue;
            }
        }
    }
};

// Corrected main function
int main()
{
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    ll n = 6; // Example: hexagon
    CycleTriangulation triangulator(n, 0);

    // Create root triangulation
    TriangulationState root = triangulator.createRoot();

    // Generate all triangulations
    vector<TriangulationState> all_triangulations;
    triangulator.generateAllTriangulations(root, all_triangulations);

    cout << "Total triangulations found: " << all_triangulations.size() << endl;

    // Print triangulations
    for (size_t i = 0; i < all_triangulations.size(); i++)
    {
        cout << "Triangulation " << i + 1 << ": { ";
        for (const auto &chord : all_triangulations[i].chords)
        {
            cout << "(" << chord.first << "," << chord.second << ") ";
        }
        cout << "}" << endl;
    }

    return 0;
}
