//-----------------Tawkir Aziz Rahman - Optimized with Original Logic---------------------//

#include <bits/stdc++.h>
using namespace std;
typedef long long ll;

// Optimized data structures to replace sets
class FastEdgeSet
{
private:
    vector<vector<bool>> adj;
    vector<pair<ll, ll>> edges;
    ll n;

public:
    FastEdgeSet(ll size) : n(size)
    {
        adj.assign(size, vector<bool>(size, false));
    }

    void insert(pair<ll, ll> edge)
    {
        ll u = min(edge.first, edge.second);
        ll v = max(edge.first, edge.second);
        if (!adj[u][v])
        {
            adj[u][v] = true;
            edges.push_back({u, v});
        }
    }

    void erase(pair<ll, ll> edge)
    {
        ll u = min(edge.first, edge.second);
        ll v = max(edge.first, edge.second);
        if (adj[u][v])
        {
            adj[u][v] = false;
            edges.erase(remove(edges.begin(), edges.end(), make_pair(u, v)), edges.end());
        }
    }

    bool find(pair<ll, ll> edge)
    {
        ll u = min(edge.first, edge.second);
        ll v = max(edge.first, edge.second);
        return u < n && v < n && adj[u][v];
    }

    vector<pair<ll, ll>> &getEdges() { return edges; }

    FastEdgeSet copy()
    {
        FastEdgeSet result(n);
        result.edges = edges;
        result.adj = adj;
        return result;
    }
};

pair<ll, ll> make_pair_ordered(ll first, ll second)
{
    return make_pair(min(first, second), max(first, second));
}

bool checkLeftmostBlockingChord(ll root, ll n, FastEdgeSet &chords, pair<ll, ll> chord)
{
    ll currentBlockedChord = n - 1;
    for (ll i = n - 2; i >= 0; i--)
    {
        if (!chords.find(make_pair_ordered(root, i)))
        {
            break;
        }
        else
        {
            currentBlockedChord = i;
        }
    }

    vector<pair<ll, ll>> leftBlockingChords;
    for (auto &a : chords.getEdges())
    {
        if (a.second == currentBlockedChord && a.first > root)
        {
            leftBlockingChords.push_back(a);
        }
    }

    if (leftBlockingChords.empty())
        return false;

    pair<ll, ll> leftmostChord = *min_element(leftBlockingChords.begin(), leftBlockingChords.end());
    return (leftmostChord.first == chord.first && leftmostChord.second == chord.second);
}

pair<ll, ll> flipChord(ll root, ll n, FastEdgeSet &chords, FastEdgeSet &allPairs, pair<ll, ll> chord)
{
    vector<ll> assocOfFirst, assocOfSecond;

    // Find neighbors of chord.first
    for (auto &a : allPairs.getEdges())
    {
        if (a.first == chord.first)
        {
            assocOfFirst.push_back(a.second);
        }
        if (a.second == chord.first)
        {
            assocOfFirst.push_back(a.first);
        }
    }

    // Find neighbors of chord.second
    for (auto &a : allPairs.getEdges())
    {
        if (a.first == chord.second)
        {
            assocOfSecond.push_back(a.second);
        }
        if (a.second == chord.second)
        {
            assocOfSecond.push_back(a.first);
        }
    }

    vector<ll> commons;
    sort(assocOfFirst.begin(), assocOfFirst.end());
    sort(assocOfSecond.begin(), assocOfSecond.end());

    set_intersection(assocOfFirst.begin(), assocOfFirst.end(),
                     assocOfSecond.begin(), assocOfSecond.end(),
                     back_inserter(commons));

    chords.erase(chord);
    allPairs.erase(chord);

    pair<ll, ll> newChord = make_pair_ordered(commons[0], commons[1]);
    chords.insert(newChord);
    allPairs.insert(newChord);

    return newChord;
}

void generateAllTriangulations(ll root, ll n, FastEdgeSet allPairs, FastEdgeSet chords,
                               vector<vector<pair<ll, ll>>> &answers)
{
    if (n < 3)
        return;
    if (n == 3)
        return;

    // Add current triangulation
    answers.push_back(chords.getEdges());

    // Try flipping each chord
    auto currentChords = chords.getEdges();
    for (auto &chord : currentChords)
    {
        FastEdgeSet allPairsCopy = allPairs.copy();
        FastEdgeSet chordsCopy = chords.copy();

        auto newChord = flipChord(root, n, chordsCopy, allPairsCopy, chord);

        bool isLeftmostBlockingChord = checkLeftmostBlockingChord(root, n, chordsCopy, newChord);
        if (isLeftmostBlockingChord)
        {
            generateAllTriangulations(root, n, allPairsCopy, chordsCopy, answers);
        }
    }
}

vector<vector<pair<ll, ll>>> generateTriangulationsForFace(vector<ll> &face)
{
    ll n = face.size();
    FastEdgeSet allPairs(n);
    FastEdgeSet chords(n);
    vector<vector<pair<ll, ll>>> answers;

    // Add boundary edges
    for (ll i = 0; i < n; i++)
    {
        allPairs.insert(make_pair_ordered(i, (i + 1) % n));
    }

    // Add initial fan triangulation
    for (ll i = 2; i < n - 1; i++)
    {
        chords.insert(make_pair_ordered(0, i));
        allPairs.insert(make_pair_ordered(0, i));
    }

    generateAllTriangulations(0, n, allPairs, chords, answers);

    // Map indices to actual face vertices
    vector<vector<pair<ll, ll>>> result;
    for (const auto &triangulation : answers)
    {
        vector<pair<ll, ll>> mappedTriangulation;
        for (const auto &edge : triangulation)
        {
            mappedTriangulation.push_back(make_pair_ordered(face[edge.first], face[edge.second]));
        }
        result.push_back(mappedTriangulation);
    }

    return result;
}

// Fast combination generator
void generateAllCombinations(vector<vector<vector<pair<ll, ll>>>> &allTriangulations,
                             vector<vector<pair<ll, ll>>> &allCombinations)
{
    vector<ll> indices(allTriangulations.size(), 0);

    while (true)
    {
        vector<pair<ll, ll>> currentCombination;
        for (size_t i = 0; i < allTriangulations.size(); i++)
        {
            for (const auto &chord : allTriangulations[i][indices[i]])
            {
                currentCombination.push_back(chord);
            }
        }
        allCombinations.push_back(currentCombination);

        // Generate next combination
        ll pos = allTriangulations.size() - 1;
        while (pos >= 0)
        {
            indices[pos]++;
            if (indices[pos] < (ll)allTriangulations[pos].size())
            {
                break;
            }
            indices[pos] = 0;
            pos--;
        }

        if (pos < 0)
            break;
    }
}

void printAllTriangulations(const vector<vector<pair<ll, ll>>> &answers)
{
    cout << "Total triangulations found: " << answers.size() << endl;
    cout << "Printing all triangulations" << endl;
    for (size_t i = 0; i < answers.size(); i++)
    {
        cout << "Triangulation " << i + 1 << ": { ";
        for (const auto &edge : answers[i])
        {
            cout << "( " << edge.first << " , " << edge.second << " ) ";
        }
        cout << "}" << endl;
    }
}

vector<vector<pair<ll, ll>>> eradicateMultiEdge(vector<vector<pair<ll, ll>>> &allCombinations)
{
    vector<vector<pair<ll, ll>>> tempCombinations;
    if (allCombinations.empty())
        return tempCombinations;

    ll standard = allCombinations[0].size();
    for (auto &combination : allCombinations)
    {
        set<pair<ll, ll>> uniqueEdges;
        for (auto &p : combination)
        {
            uniqueEdges.insert(make_pair_ordered(p.first, p.second));
        }
        if (uniqueEdges.size() == standard)
        {
            vector<pair<ll, ll>> cleanCombination(uniqueEdges.begin(), uniqueEdges.end());
            tempCombinations.push_back(cleanCombination);
        }
    }
    return tempCombinations;
}

int main()
{
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);

    vector<vector<ll>> faces;
    faces.push_back({1, 2, 3, 4, 5, 6});

    vector<vector<vector<pair<ll, ll>>>> allTriangulations;
    for (auto &face : faces)
    {
        vector<vector<pair<ll, ll>>> triangulations = generateTriangulationsForFace(face);
        cout << "Generated triangulations for face of size " << face.size()
             << ": " << triangulations.size() << " triangulations" << endl;
        allTriangulations.push_back(triangulations);
    }

    cout << "\nTotal triangulation groups: " << allTriangulations.size() << endl;
    for (size_t i = 0; i < allTriangulations.size(); i++)
    {
        cout << "Face " << i + 1 << ": " << allTriangulations[i].size() << " triangulations" << endl;
    }

    vector<vector<pair<ll, ll>>> allCombinations;
    generateAllCombinations(allTriangulations, allCombinations);
    cout << "Total combinations generated: " << allCombinations.size() << endl;

    allCombinations = eradicateMultiEdge(allCombinations);

    // Sort for consistency
    for (auto &combination : allCombinations)
    {
        sort(combination.begin(), combination.end());
    }
    sort(allCombinations.begin(), allCombinations.end());

    // Remove duplicates
    auto last = unique(allCombinations.begin(), allCombinations.end());
    allCombinations.erase(last, allCombinations.end());

    cout << "Final unique combinations: " << allCombinations.size() << endl;

    // Print results if reasonable size
    if (allCombinations.size() <= 50)
    {
        printAllTriangulations(allCombinations);
    }

    return 0;
}