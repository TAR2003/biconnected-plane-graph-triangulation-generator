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

pair<ll, ll> makepair(ll a, ll b);
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

set<pair<ll, ll>> copySetOfPairs(set<pair<ll, ll>> &src)
{
    set<pair<ll, ll>> dest;
    for (auto it = src.begin(); it != src.end(); it++)
    {
        dest.insert(makepair(it->first, it->second));
    }
    return dest;
}

bool checkLeftmostBlockingChord(ll root, ll n, set<pair<ll, ll>> &chords, pair<ll, ll> chord)
{
    // Step 1: Normalize all vertices relative to root (root becomes 0)
    auto normalizeVertex = [&](ll vertex) -> ll
    {
        return (vertex - root + n) % n;
    };

    // Step 2: Normalize all chords to the new coordinate system
    set<pair<ll, ll>> normalizedChords;
    for (auto c : chords)
    {
        ll first_norm = normalizeVertex(c.first);
        ll second_norm = normalizeVertex(c.second);
        normalizedChords.insert(makepair(first_norm, second_norm));
    }

    // Step 3: Normalize the input chord
    ll chord_first_norm = normalizeVertex(chord.first);
    ll chord_second_norm = normalizeVertex(chord.second);
    pair<ll, ll> normalizedChord = makepair(chord_first_norm, chord_second_norm);

    // Step 4: Perform the leftmost blocking chord check in normalized space (root = 0)
    ll currentBlockedChord = n - 1;
    for (ll i = n - 2; i >= 0; i--)
    {
        if (normalizedChords.find(makepair(0, i)) == normalizedChords.end())
        {
            break;
        }
        else
        {
            currentBlockedChord = i;
        }
    }

    // Find all chords that connect to the blocking vertex from vertices > 0 (left of normalized root)
    set<pair<ll, ll>> leftBlockingChords;
    for (auto a : normalizedChords)
    {
        if (a.second == currentBlockedChord && a.first > 0)
        {
            leftBlockingChords.insert(a);
        }
    }

    // Check if we found any left blocking chords
    if (leftBlockingChords.empty())
    {
        return false;
    }

    // Find the leftmost chord (smallest first vertex > 0)
    pair<ll, ll> leftmostChord = *leftBlockingChords.begin();

    // Step 5: Check if the normalized input chord is the leftmost blocking chord
    bool result = (leftmostChord.first == normalizedChord.first &&
                   leftmostChord.second == normalizedChord.second);

    // Note: No need to convert back since we only return a boolean result
    return result;
}

pair<ll, ll> flipChord(ll root, ll n, set<pair<ll, ll>> &chords, set<pair<ll, ll>> &allPairs, pair<ll, ll> chord)
{
    // cout << "Want to flip edge " << chord.first << " " << chord.second << endl;
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
    chords.insert(makepair(commons[0], commons[1]));
    allPairs.insert(makepair(commons[0], commons[1]));
    return makepair(commons[0], commons[1]);
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
}

pair<ll, ll> makepair(ll a, ll b)
{
    if (a < b)
    {
        return make_pair(a, b);
    }
    else
    {
        return make_pair(b, a);
    }
    return make_pair(a, b); // This line is unreachable but added to avoid compiler warnings
}

void printAllTriangulations(set<set<pair<ll, ll>>> &answers)
{
    cout << "Total triangulations found: " << answers.size() << endl;
    // cout << "Printing all triangulations" << endl;
    // for(auto a: answers)
    // {
    //     cout << "{ ";
    //     for(auto b: a)
    //     {
    //         cout << "( " << b.first << " , " << b.second << " ) ";
    //     }
    //     cout << "}" << endl;
    // }
}

ll findHighestSafeRoot(ll n, set<pair<ll, ll>> &innerchords)
{
    set<ll> s;
    for (ll i = 0; i < n; i++)
    {
        s.insert(i);
    }
    for (auto a : innerchords)
    {
        if (s.find(a.first) != s.end())
        {
            s.erase(a.first);
        }
        if (s.find(a.second) != s.end())
        {
            s.erase(a.second);
        }
    }
    return *s.rbegin();
}
bool compare(pair<ll, ll> a, pair<ll, ll> b)
{
    if (a.first < b.first)
    {
        return true;
    }
    else if (a.first == b.first && a.second < b.second)
    {
        return true;
    }
    return false;
}

void generateOuterTriangulationsFromAnInnerTriangulation(ll root, ll n, set<pair<ll, ll>> &allPairs, set<pair<ll, ll>> &chords, set<pair<ll, ll>> &innerChords, set<set<pair<ll, ll>>> &answers)
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
    // cout << "all of the current chords inside the function: " << endl;
    // printAllPairs(chords);
    // cout << "all of the current inner chords inside the function: " << endl;
    // printAllPairs(innerChords);

    // cout << "Found a valid Triangulation::" << endl;
    // printAllPairs(chords);
    auto minimumChord = *chords.begin();
    auto minimumInnerChord = *innerChords.begin();
    if (compare(minimumChord, minimumInnerChord))
    {
        // cout << "check failed!!!" << endl;
        return;
    }
    set<pair<ll, ll>> allChords;
    for (auto a : chords)
    {
        allChords.insert(a);
    }
    for (auto a : innerChords)
    {
        allChords.insert(a);
    }
    ll rightlength = (n - 3) << 1;
    if (allChords.size() < rightlength)
    {
        // cout << "check failed!!!" << endl;
        return;
    }
    // cout << "Checking passed" << endl;
    set<pair<ll, ll>> merged;
    for (auto a : chords)
    {
        merged.insert(a);
    }
    for (auto a : innerChords)
    {
        merged.insert(a);
    }
    answers.insert(merged);

    for (auto a : chords)
    {
        // cout << "Attempting to flip edge " << a.first << " " << a.second << endl;
        set<pair<ll, ll>> allPairsCopy = copySetOfPairs(allPairs);
        set<pair<ll, ll>> chordsCopy = copySetOfPairs(chords);
        auto pair = flipChord(root, n, chordsCopy, allPairsCopy, a);
        bool isLeftmostBlockingChord = checkLeftmostBlockingChord(root, n, chordsCopy, pair);
        // cout << "After flipping the edge " << a.first << " " << a.second << endl;
        // printAllPairs(chordsCopy);
        if (isLeftmostBlockingChord)
        {
            // cout << "A new edge can be fipped " << pair.first << " " << pair.second << endl;
            generateOuterTriangulationsFromAnInnerTriangulation(root, n, allPairsCopy, chordsCopy, innerChords, answers);
        }
    }
}

void printSetOfPairs(set<pair<ll, ll>> &s)
{
    for (auto it = s.begin(); it != s.end(); it++)
    {
        cout << "( " << it->first << " , " << it->second << " ) ";
    }
    cout << endl;
}

void generateAllTriangulationsForBiconnectedGraph(ll root, ll n, set<pair<ll, ll>> &allPairs, set<pair<ll, ll>> &chords, set<set<pair<ll, ll>>> &answers)
{
    set<set<pair<ll, ll>>> tempAnswers;
    set<pair<ll, ll>> innerChords;
    set<pair<ll, ll>> tempAllPairs = copySetOfPairs(allPairs);
    for (ll i = 2; i < n - 1; i++)
    {
        innerChords.insert(makepair(0, i));
        tempAllPairs.insert(makepair(0, i));
    }
    generateAllTriangulations(root, n, tempAllPairs, innerChords, tempAnswers);
    // printAllTriangulations(tempAnswers);
    for (auto a : tempAnswers)
    {
        // printSetOfPairs(a);
        set<pair<ll, ll>> innerChordsCopy = copySetOfPairs(a);
        set<pair<ll, ll>> initChords;
        tempAllPairs = copySetOfPairs(allPairs);
        ll highestSafeRoot = findHighestSafeRoot(n, innerChordsCopy);
        // cout << "Highest safe root is: " << highestSafeRoot << endl;
        for (ll i = 2; i < n - 1; i++)
        {
            initChords.insert(makepair(highestSafeRoot, (highestSafeRoot + i) % n));
            tempAllPairs.insert(makepair(highestSafeRoot, (highestSafeRoot + i) % n));
        }
        // cout << "Printing inner Nodes initial before calling the function:" << endl;
        // printSetOfPairs(innerChordsCopy);
        // cout << "Printing all pairs initial before calling the function:" << endl;
        // printSetOfPairs(tempAllPairs);
        set<set<pair<ll, ll>>> temptempAnswers;
        generateOuterTriangulationsFromAnInnerTriangulation(highestSafeRoot, n, tempAllPairs, initChords, innerChordsCopy, temptempAnswers);

        for (auto b : temptempAnswers)
        {
            // printSetOfPairs(b);
            answers.insert(b);
        }
        // cout << "After this step the triangulations will be printed by far: =-===========" << endl;
        // printAllTriangulations(temptempAnswers);
        // cout << "Triangulations print is done : =-===========" << endl;
    }
    // cout << "Process completed successfully" << endl;

    // printAllTriangulations(tempAnswers);
}

void generateViaMergingAlgorithm(ll n, set<set<pair<ll, ll>>> &mergedAnswers)
{
    set<pair<ll, ll>> allPairs;
    set<pair<ll, ll>> chords;
    set<set<pair<ll, ll>>> answers;
    for (ll i = 0; i < n; i++)
    {
        allPairs.insert(makepair(i, (i + 1) % n));
    }
    for (ll i = 2; i < n - 1; i++)
    {
        chords.insert(makepair(0, i));
        allPairs.insert(makepair(0, i));
    }
    generateAllTriangulations(0, n, allPairs, chords, answers);
    // cout << "In the merging step:::" << endl;
    // printAllTriangulations(answers);
    for (auto a : answers)
    {
        for (auto b : answers)
        {
            ll cnt = 0;
            for (auto c : a)
            {

                if (b.find(c) != b.end())
                {
                    cnt++;
                    break;
                }
            }
            if (cnt == 0)
            {
                set<pair<ll,ll>> merged;
                for (auto c : a)
                {
                    merged.insert(c);
                }
                for (auto c : b)
                {
                    merged.insert(c);
                }
                mergedAnswers.insert(merged);
            }
        }
    }
    // printAllTriangulations(mergedAnswers);
}

bool compareSet(set<pair<ll, ll>> &a, set<pair<ll, ll>> &b)
{
    if (a.size() != b.size())
    {
        return false;
    }
    auto it_a = a.begin();
    auto it_b = b.begin();
    while (it_a != a.end() && it_b != b.end())
    {
        if (it_a->first != it_b->first || it_a->second != it_b->second)
        {
            return false;
        }
        
        ++it_a;
        ++it_b;
    }
    return true;
}

bool compareTriangulations(set<set<pair<ll, ll>>> &a, set<set<pair<ll, ll>>> &b)
{
    if(a.size() != b.size())
    {
        return false;
    }
    for (auto it_a = a.begin(), it_b = b.begin(); it_a != a.end() && it_b != b.end(); ++it_a, ++it_b)
    {
        auto tempa = *it_a;
        auto tempb = *it_b;
        if (!compareSet(tempa, tempb))
        {
            return false;
        }
    }
    //cout << "Comparison of All triangulations is successful!!!!!" << endl;
    return true;
}


void calculate(ll n)
{
    set<pair<ll, ll>> allPairs;
    set<pair<ll, ll>> chords;
    set<set<pair<ll, ll>>> answers;
    for (ll i = 0; i < n; i++)
    {
        allPairs.insert(makepair(i, (i + 1) % n));
    }
    // for(ll i = 2; i < n - 1; i++)
    // {
    //     chords.insert(makepair(0, i));
    //     allPairs.insert(makepair(0, i));
    // }
    set<set<pair<ll, ll>>> mergedAnswers;
    generateViaMergingAlgorithm(n, mergedAnswers);
    generateAllTriangulationsForBiconnectedGraph(0, n, allPairs, chords, answers);
    // printAllTriangulations(answers);
    // printAllTriangulations(mergedAnswers);
    bool bl = compareTriangulations(answers, mergedAnswers);
    if(bl) {
        cout << "For n=" << n << ", sizes =" << answers.size() << "==" << mergedAnswers.size() << endl;
    }
}

int main()
{
    cin.tie(0);
    cin.sync_with_stdio(0);
    cout.tie(0);
    cout.sync_with_stdio(0);
    for(ll i = 4; i <= 10 ; i++) {
        calculate(i);
    }
    
    return 0;
}