#include <bits/stdc++.h>
#include "ParvezRahmanNakano.hpp"
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


int main()
{

    ParvezRahmanNakano *parvezRahmanNakano = new ParvezRahmanNakano(7);
    parvezRahmanNakano->generateAllTriangulations();
    parvezRahmanNakano->printAllTriangulations();

    return 0;
}