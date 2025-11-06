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

// template <typename T1, typename T2>
// void printStructurePair(unordered_set<pair<T1, T2>> &s)
// {
//     for (auto itr = s.begin(); itr != s.end(); itr++)
//     {
//         cout << "( " << itr->first << " , " << itr->second << " ) ";
//     }
//     cout << endl;
// }

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

static inline uint64_t splitmix64(uint64_t x)
{
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

struct pair_hash
{
    size_t operator()(const pair<ll, ll> &p) const
    {
        uint64_t a = splitmix64(p.first);
        uint64_t b = splitmix64(p.second);
        return a ^ (b + 0x9e3779b97f4a7c15ULL + (a << 6) + (a >> 2));
    }
};

class chord
{
public:
    ll p1, p2;
    ll opp1, opp2;
    vector<chord *> edges;
    bool active;
    chord(ll a, ll b, ll c, ll d)
    {
        p1 = a;
        p2 = b;
        opp1 = c;
        opp2 = d;
        active = true;
        edges = vector<chord *>();
    }
    void setRectangle()
    {
    }
    void calculateOppositeChords()
    {
    }
};

class PRN
{
public:
    list<chord *> chords;
    // list<chord *> blockingChords;
    ll n;
    vector<vector<pair<ll, ll>>> allTriangulations;
    PRN(ll n)
    {
        this->n = n;
        findAllTriangulationsCycle(n);
    }
    void findAllChildTriangulationsCycle(chord *blockingChord)
    {
        answer();
        chord *leftmostBlockingChord = blockingChord;
        ll b = leftmostBlockingChord->p1;
        for (auto itr = chords.begin(); itr != chords.end(); itr++)
        {
            if ((*itr)->p2 < b)
            {
                continue;
            }
            flip(*itr);
            findAllChildTriangulationsCycle(*itr);
            unflip(*itr);
        }
    }

    void findAllTriangulationsCycle(ll n)
    {
        generateRoot(n);
        answer();
        for (auto itr = chords.begin(); itr != chords.end(); itr++)
        {
            flip(*itr);
            findAllChildTriangulationsCycle(*itr);
            unflip(*itr);
        }
    }

    void flip(chord *ch)
    {
        ch->active = false;
    }
    void unflip(chord *ch)
    {
        ch->active = true;
    }
    void generateRoot(ll n)
    {
        for (ll i = 2; i < n - 1; i++)
        {
            chord *ch = new chord(1, i, i + 1, n);
            chords.push_back(ch);
        }
        for (auto itr = chords.begin(); itr != chords.end(); itr++)
        {
            if (itr != chords.begin())
            {
                (*itr)->edges.push_back(*prev(itr));
            }
            if (next(itr) != chords.end())
            {
                (*itr)->edges.push_back(*next(itr));
            }
        }
    }
    void answer()
    {
        vector<pair<ll,ll>> currentChords;
        for (auto a : chords)
        {
            currentChords.push_back(make_pair(a->p1, a->p2));
        };
        allTriangulations.push_back(currentChords);
        // printChords(chords);
    }
    void printAnswer()
    {
        cout << "total triangulations:" << allTriangulations.size() << endl;
        for (auto triangulation : allTriangulations)
        {
            printVectorPair(triangulation);
            cout << "-----" << endl;
        }
    }

    void printChords(list<chord> &s)
    {
        for (auto itr = s.begin(); itr != s.end(); itr++)
        {
            cout << "( " << itr->p1 << " , " << itr->p2 << " ) ";
        }
        cout << endl;
    }
};

int main()
{
    ll n;
    // cin >> n;
    n = 6;
    PRN prn(n);
    prn.printAnswer();

    return 0;
}