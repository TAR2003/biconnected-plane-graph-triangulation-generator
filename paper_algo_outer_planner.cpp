//-----------------Tawkir Aziz Rahman---------------------//

#include <bits/stdc++.h>
#define input(arr, n)          \
    for (ll i = 0; i < n; i++) \
        cin >> arr[i];
#define output(arr, n)             \
    {                              \
        for (ll i = 0; i < n; i++) \
        {                          \
            cout << arr[i] << " "; \
        }                          \
    }
using namespace std;
typedef long long ll;
// policy based data structure
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/tree_policy.hpp>
using namespace __gnu_pbds;
template <class T>
using indexed_set = tree<T, null_type, less<T>, rb_tree_tag, tree_order_statistics_node_update>;
template <class T>
using indexed_multiset = tree<T, null_type, less_equal<T>, rb_tree_tag, tree_order_statistics_node_update>;

// for printing normal 1D vector
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
    return;
}

template <typename T1, typename T2>
void printVectorPair(vector<pair<T1, T2>> &v, ll st = -1, ll fin = -1)
{
    if (st == -1)
        st = 0;
    if (fin == -1)
        fin = v.size();
    for (ll i = st; i < fin; i++)
    {
        cout << "( " << v[i].first << " , " << v[i].second << " ) ";
    }
    cout << endl;
    return;
}

// for printing 2D vectors (matrix)
template <typename T>
void printMatrix(vector<T> &v, ll st = -1, ll fin = -1)
{
    if (st == -1)
        st = 0;
    if (fin == -1)
        fin = v.size();
    for (ll i = st; i < fin; i++)
    {
        printVector(v[i]);
    }
    // cout << endl;
    return;
}

// for printing other STL structures
template <typename T>
void printStructure(T &t)
{
    for (auto itr = t.begin(); itr != t.end(); itr++)
    {
        cout << *itr << " ";
    }
    cout << endl;
}

void printAllPairs(set<pair<ll, ll>> &s)
{
    for (auto it = s.begin(); it != s.end(); it++)
    {
        cout << "( " << it->first << " , " << it->second << " ) ";
    }
    cout << endl;
}

pair<ll, ll> make_pair(ll a, ll b)
{
    return std::make_pair(min(a, b), max(a, b));
}

set<pair<ll, ll>> copySetOfPairs(set<pair<ll, ll>> &src)
{
    set<pair<ll, ll>> dest;
    for (auto it = src.begin(); it != src.end(); it++)
    {
        dest.insert(make_pair(it->first, it->second));
    }
    return dest;
}

bool checkLeftmostBlockingChord(ll root, ll n, set<pair<ll, ll>> &chords, pair<ll, ll> chord)
{
    ll currentBlockedChord = n - 1;
    for (ll i = n - 2; i >= 0; i--)
    {
        if (chords.find(make_pair(root, i)) == chords.end())
        {
            break;
        }
        else
        {
            currentBlockedChord = i;
        }
    }
    // cout << "Current Blocking vertex is : " << currentBlockedChord << endl;
    set<pair<ll, ll>> leftBlockingChords;
    for (auto a : chords)
    {
        if (a.second == currentBlockedChord && a.first > root)
        {
            leftBlockingChords.insert(a);
        }
    }
    pair<ll, ll> leftmostChord = *leftBlockingChords.begin();
    if (leftmostChord.first == chord.first && leftmostChord.second == chord.second)
    {
        return true;
    }
    else
    {
        return false;
    }
}

pair<ll, ll> flipChord(ll root, ll n, set<pair<ll, ll>> &chords, set<pair<ll, ll>> &allPairs, pair<ll, ll> chord)
{
    set<ll> assocOfFirst, assocOfSecond;
    for (auto a : allPairs)
    {
        if (a.first == chord.first)
        {
            assocOfFirst.insert(a.second);
        }
        if (a.second == chord.first)
        {
            assocOfFirst.insert(a.first);
        }
    }
    for (auto a : allPairs)
    {
        if (a.first == chord.second)
        {
            assocOfSecond.insert(a.second);
        }
        if (a.second == chord.second)
        {
            assocOfSecond.insert(a.first);
        }
    }
    // cout << "Printing assocOfFirst" << endl;
    // for(auto a:assocOfFirst)
    // {
    //     cout << a << " ";
    // }
    // cout << endl;

    // // cout << "Printing assocOfSecond" << endl;
    // for(auto a:assocOfSecond)
    // {
    //     cout << a << " ";
    // }
    // cout << endl;

    vector<ll> commons;
    for (auto a : assocOfFirst)
    {
        if (assocOfSecond.find(a) != assocOfSecond.end())
        {
            commons.push_back(a);
        }
    }
    chords.erase(chords.find(chord));
    allPairs.erase(allPairs.find(chord));
    chords.insert(make_pair(commons[0], commons[1]));
    allPairs.insert(make_pair(commons[0], commons[1]));
    return make_pair(commons[0], commons[1]);
}

void generateAllTriangulations(ll root, ll n, set<pair<ll, ll>> &allPairs, set<pair<ll, ll>> &chords, set<set<pair<ll, ll>>> &answers)
{
    if (n < 3)
    {
        cout << "Invalid input for triangulation" << endl;
        return;
    }

    if (n == 3)
    {
        cout << "Base case: Triangulation of a triangle" << endl;
        return;
    }

    // cout << "Found a valid Triangulation::" << endl;
    // printAllPairs(chords);
    answers.insert(chords);

    for (auto a : chords)
    {
        set<pair<ll, ll>> allPairsCopy = copySetOfPairs(allPairs);
        set<pair<ll, ll>> chordsCopy = copySetOfPairs(chords);
        auto pair = flipChord(root, n, chordsCopy, allPairsCopy, a);
        bool isLeftmostBlockingChord = checkLeftmostBlockingChord(root, n, chordsCopy, pair);
        if (isLeftmostBlockingChord)
        {

            generateAllTriangulations(root, n, allPairsCopy, chordsCopy, answers);
        }
    }

    // cout << "here we are" << endl;
    // flipChord(root, n , chords, allPairs, make_pair(0,2));
    // cout << "Printing all pairs " << endl;
    // printAllPairs(allPairs);
    // cout << "Printing all chords " << endl;
    // printAllPairs(chords);
    // if(checkLeftmostBlockingChord(root, n, chords, make_pair(1, 3)))
    // {
    //     cout << "It is aLeftmost blocking chord" << endl;
    // }
    // else
    // {
    //     cout << "It is not a leftmost blocking chord" << endl;
    // }
    // flipChord(root, n, chords, allPairs, make_pair(0, 3));
    // cout << "Printing all pairs " << endl;
    // printAllPairs(allPairs);
    // cout << "Printing all chords " << endl;
    // printAllPairs(chords);
    // if(checkLeftmostBlockingChord(root, n, chords, make_pair(1, 3)))
    // {
    //     cout << "It is a leftmost blocking chord" << endl;
    // }
    // else
    // {
    //     cout << "It is not a leftmost blocking chord" << endl;
    // }
}

void printAllTriangulations(set<set<pair<ll, ll>>> &answers)
{
    cout << "Total triangulations found: " << answers.size() << endl;
    cout << "Printing all triangulations" << endl;
    for (auto a : answers)
    {
        cout << "{ ";
        for (auto b : a)
        {
            cout << "( " << b.first << " , " << b.second << " ) ";
        }
        cout << "}" << endl;
    }
}

void printAllTriangulations(vector<vector<pair<ll, ll>>> &answers)
{
    cout << "Total triangulations found: " << answers.size() << endl;
    cout << "Printing all triangulations" << endl;
    for (auto a : answers)
    {
        cout << "{ ";
        for (auto b : a)
        {
            cout << "( " << b.first << " , " << b.second << " ) ";
        }
        cout << "}" << endl;
    }
}

void printAllTriangulationsOfFaces(vector<vector<vector<pair<ll, ll>>>> &allTriangulations)
{
    cout << "Total faces found: " << allTriangulations.size() << endl;
    for (size_t i = 0; i < allTriangulations.size(); i++)
    {
        cout << "Face " << i + 1 << ": " << endl;
        printAllTriangulations(allTriangulations[i]);
    }
}

vector<vector<pair<ll, ll>>> generateTriangulationsForFace(vector<ll> &face)
{
    vector<pair<ll, ll>> triangulations;
    ll n = face.size();
    set<pair<ll, ll>> allPairs;
    set<pair<ll, ll>> chords;
    set<set<pair<ll, ll>>> answers;
    for (ll i = 0; i < n; i++)
    {
        allPairs.insert(make_pair(i, (i + 1) % n));
    }
    for (ll i = 2; i < n - 1; i++)
    {
        chords.insert(make_pair(0, i));
        allPairs.insert(make_pair(0, i));
    }
    generateAllTriangulations(0, n, allPairs, chords, answers);
    vector<vector<pair<ll, ll>>> allTriangulations;
    for (const auto &a : answers)
    {
        vector<pair<ll, ll>> triangulation;
        for (const auto &b : a)
        {
            triangulation.push_back(make_pair(face[b.first], face[b.second]));
        }
        allTriangulations.push_back(triangulation);
    }
    // printAllTriangulations(answers);
    return allTriangulations;
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
            uniqueEdges.insert(make_pair(p.first, p.second));
        }
        if (uniqueEdges.size() == standard)
        {
            vector<pair<ll, ll>> cleanCombination(uniqueEdges.begin(), uniqueEdges.end());
            tempCombinations.push_back(cleanCombination);
        }
    }
    return tempCombinations;
}

vector<vector<pair<ll, ll>>> removePairsContainingAPair(vector<vector<pair<ll, ll>>> &combinations, const pair<ll, ll> &p)
{
    vector<vector<pair<ll, ll>>> result;
    for (auto &combination : combinations)
    {
        bool b = false;
        for (auto &edge : combination)
        {
            if (edge.first == p.first && edge.second == p.second)
            {
                b = true;
                break;
            }
        }
        if (!b)
            result.push_back(combination);
    }
    return result;
}

void printCombinations(const vector<vector<pair<ll, ll>>> &combinations)
{
    for (size_t i = 0; i < combinations.size(); i++)
    {
        cout << "Combination " << i + 1 << ": { ";
        for (const auto &edge : combinations[i])
        {
            cout << "( " << edge.first << " , " << edge.second << " ) ";
        }
        cout << "}" << endl;
    }
}

int main()
{
    cin.tie(0);
    cin.sync_with_stdio(0);
    cout.tie(0);
    cout.sync_with_stdio(0);
    ll n = 5;
    vector<vector<ll>> faces;
    vector<ll> temp;
    // temp = {1, 4, 5, 6, 7, 8};
    // faces.push_back(temp);
    // temp = {2, 3, 4, 5, 6};
    // faces.push_back(temp);
    // temp = {1, 2, 6, 7, 8};
    // faces.push_back(temp);
    // temp = {1, 2, 3, 4};
    // faces.push_back(temp);

    // temp = {1, 2, 3, 4, 5, 6, 7};
    // faces.push_back(temp);
    // temp = {1, 2, 3, 4, 5};
    // faces.push_back(temp);
    // temp = {1, 5, 6, 7};
    // faces.push_back(temp);

    temp = {1, 2, 3, 4};
    faces.push_back(temp);
    temp = {1, 2, 3, 4, 5, 6};
    faces.push_back(temp);
    temp = {1, 4, 5, 6};
    faces.push_back(temp);

    // temp = {1, 2, 3, 4, 5, 6, 7, 8};
    // faces.push_back(temp);
    // temp = {2, 3, 4, 5, 6, 9};
    // faces.push_back(temp);
    // temp = {1, 2, 9, 6, 7, 8};
    // faces.push_back(temp);
    ll pos = 1;
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
    allCombinations = removePairsContainingAPair(allCombinations, {1, 4});
    cout << "Total combinations generated before removing multiedge: " << allCombinations.size() << endl;
    allCombinations = eradicateMultiEdge(allCombinations);

    cout << "Total combinations generated after removing multiedge : " << allCombinations.size() << endl;
    // printCombinations(allCombinations);
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
    printCombinations(allCombinations);

    return 0;
}