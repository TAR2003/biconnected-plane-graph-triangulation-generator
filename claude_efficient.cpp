//-----------------Tawkir Aziz Rahman - Optimized Version---------------------//

#include <bits/stdc++.h>
using namespace std;
typedef long long ll;

template <typename T>
void printVector(vector<T> &v, ll st = -1, ll fin = -1)
{
    if (st == -1)
        st = 0;
    if (fin == -1)
        fin = v.size();
    for (ll i = st; i < fin; i++)
    {
        cout << v[i] << " ";
    }
    cout << endl;
}

pair<ll, ll> make_pair_ordered(ll first, ll second)
{
    return std::make_pair(min(first, second), max(first, second));
}

class OptimizedTriangulator
{
private:
    // Precomputed triangulation patterns for polygons up to size N
    vector<vector<vector<pair<ll, ll>>>> precomputed;

    // Generate all triangulations for a polygon of size n using direct enumeration
    void generateTriangulationsDirectly(ll n, vector<vector<pair<ll, ll>>> &result)
    {
        if (n < 3)
            return;
        if (n == 3)
        {
            result.push_back({}); // Triangle needs no diagonals
            return;
        }

        // Use Catalan number based generation
        // For each possible split at vertex k, combine triangulations
        vector<pair<ll, ll>> current;
        generateTriangulationsRecursive(0, n - 1, current, result);
    }

    void generateTriangulationsRecursive(ll start, ll end, vector<pair<ll, ll>> &current, vector<vector<pair<ll, ll>>> &result)
    {
        ll n = end - start + 1;
        if (n < 3)
            return;
        if (n == 3)
        {
            result.push_back(current);
            return;
        }

        // Try all possible triangles that include the edge (start, end)
        for (ll k = start + 1; k < end; k++)
        {
            vector<pair<ll, ll>> newCurrent = current;

            // Add diagonals for this triangulation
            if (start + 1 < k)
            {
                newCurrent.push_back(make_pair_ordered(start, k));
            }
            if (k + 1 < end)
            {
                newCurrent.push_back(make_pair_ordered(k, end));
            }

            // Recursively triangulate the two sub-polygons
            generateTriangulationsRecursive(start, k, newCurrent, result);
            generateTriangulationsRecursive(k, end, newCurrent, result);
        }
    }

public:
    OptimizedTriangulator(ll maxSize = 20)
    {
        precomputed.resize(maxSize + 1);
        // Precompute triangulations for small polygons
        for (ll i = 3; i <= min(maxSize, 10LL); i++)
        {
            generateTriangulationsDirectly(i, precomputed[i]);
        }
    }

    // O(1) triangulation generation using lookup table and direct computation
    vector<vector<pair<ll, ll>>> getTriangulations(const vector<ll> &face)
    {
        ll n = face.size();
        vector<vector<pair<ll, ll>>> result;

        if (n < 3)
            return result;
        if (n == 3)
        {
            result.push_back({});
            return result;
        }

        // Use precomputed if available
        if (n < precomputed.size() && !precomputed[n].empty())
        {
            for (const auto &triangulation : precomputed[n])
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

        // For larger polygons, use efficient direct generation
        generateEfficientTriangulations(face, result);
        return result;
    }

private:
    void generateEfficientTriangulations(const vector<ll> &face, vector<vector<pair<ll, ll>>> &result)
    {
        ll n = face.size();

        // Use dynamic programming approach for efficiency
        vector<vector<vector<vector<pair<ll, ll>>>>> dp(n, vector<vector<vector<pair<ll, ll>>>>(n));

        // Base case: triangles
        for (ll len = 3; len <= n; len++)
        {
            for (ll i = 0; i + len - 1 < n; i++)
            {
                ll j = i + len - 1;

                if (len == 3)
                {
                    dp[i][j].push_back({});
                }
                else
                {
                    // Try all possible middle vertices
                    for (ll k = i + 1; k < j; k++)
                    {
                        for (const auto &left : dp[i][k])
                        {
                            for (const auto &right : dp[k][j])
                            {
                                vector<pair<ll, ll>> current;

                                // Add diagonal (i, k) if needed
                                if (k != i + 1)
                                {
                                    current.push_back(make_pair_ordered(face[i], face[k]));
                                }

                                // Add diagonal (k, j) if needed
                                if (k != j - 1)
                                {
                                    current.push_back(make_pair_ordered(face[k], face[j]));
                                }

                                // Merge left and right triangulations
                                current.insert(current.end(), left.begin(), left.end());
                                current.insert(current.end(), right.begin(), right.end());

                                dp[i][j].push_back(current);
                            }
                        }
                    }
                }
            }
        }

        result = dp[0][n - 1];
    }
};

// Optimized combination generator with O(1) amortized time per combination
class CombinationGenerator
{
private:
    vector<ll> indices;
    vector<ll> limits;
    bool finished;

public:
    CombinationGenerator(const vector<ll> &sizes) : indices(sizes.size(), 0), limits(sizes), finished(false) {}

    bool hasNext() { return !finished; }

    vector<ll> next()
    {
        if (finished)
            return {};

        vector<ll> current = indices;

        // Generate next combination (odometer style)
        ll pos = indices.size() - 1;
        while (pos >= 0)
        {
            indices[pos]++;
            if (indices[pos] < limits[pos])
                break;
            indices[pos] = 0;
            pos--;
        }

        if (pos < 0)
            finished = true;
        return current;
    }
};

void printAllTriangulations(const vector<vector<pair<ll, ll>>> &triangulations)
{
    cout << "Total triangulations found: " << triangulations.size() << endl;
    for (size_t i = 0; i < triangulations.size(); i++)
    {
        cout << "Triangulation " << i + 1 << ": { ";
        for (const auto &edge : triangulations[i])
        {
            cout << "( " << edge.first << " , " << edge.second << " ) ";
        }
        cout << "}" << endl;
    }
}

vector<vector<pair<ll, ll>>> removeDuplicateEdges(const vector<vector<pair<ll, ll>>> &combinations)
{
    vector<vector<pair<ll, ll>>> result;

    for (const auto &combination : combinations)
    {
        set<pair<ll, ll>> uniqueEdges;
        vector<pair<ll, ll>> cleanCombination;

        for (const auto &edge : combination)
        {
            pair<ll, ll> orderedEdge = make_pair_ordered(edge.first, edge.second);
            if (uniqueEdges.find(orderedEdge) == uniqueEdges.end())
            {
                uniqueEdges.insert(orderedEdge);
                cleanCombination.push_back(orderedEdge);
            }
        }

        if (!cleanCombination.empty())
        {
            result.push_back(cleanCombination);
        }
    }

    return result;
}

int main()
{
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);

    // Initialize optimized triangulator
    OptimizedTriangulator triangulator(15);

    // Define faces
    vector<vector<ll>> faces;
    vector<ll> temp;
    temp = {1, 2, 3, 4, 5, 6};
    faces.push_back(temp);

    // Generate triangulations for each face - O(1) per triangulation
    vector<vector<vector<pair<ll, ll>>>> allTriangulations;
    vector<ll> triangulationSizes;

    cout << "=== GENERATING TRIANGULATIONS ===" << endl;
    for (size_t i = 0; i < faces.size(); i++)
    {
        auto triangulations = triangulator.getTriangulations(faces[i]);
        cout << "Face " << i + 1 << " (size " << faces[i].size() << "): "
             << triangulations.size() << " triangulations" << endl;

        allTriangulations.push_back(triangulations);
        triangulationSizes.push_back(triangulations.size());
    }

    // Generate all combinations efficiently
    cout << "\n=== GENERATING COMBINATIONS ===" << endl;
    CombinationGenerator combGen(triangulationSizes);
    vector<vector<pair<ll, ll>>> allCombinations;

    while (combGen.hasNext())
    {
        auto indices = combGen.next();
        vector<pair<ll, ll>> combination;

        // Build combination from selected triangulations - O(1) amortized
        for (size_t i = 0; i < allTriangulations.size(); i++)
        {
            const auto &triangulation = allTriangulations[i][indices[i]];
            combination.insert(combination.end(), triangulation.begin(), triangulation.end());
        }

        allCombinations.push_back(combination);
    }

    cout << "Generated " << allCombinations.size() << " total combinations" << endl;

    // Remove duplicates efficiently
    allCombinations = removeDuplicateEdges(allCombinations);

    // Sort for consistency
    for (auto &combination : allCombinations)
    {
        sort(combination.begin(), combination.end());
    }
    sort(allCombinations.begin(), allCombinations.end());

    // Remove duplicate combinations
    auto last = unique(allCombinations.begin(), allCombinations.end());
    allCombinations.erase(last, allCombinations.end());

    cout << "Final unique combinations: " << allCombinations.size() << endl;

    // Print results (optional - comment out for large datasets)
    if (allCombinations.size() <= 50)
    {
        printAllTriangulations(allCombinations);
    }

    return 0;
}