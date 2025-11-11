#include <bits/stdc++.h>
using namespace std;
class Edge
{
public:
    /// @brief the two endpoints of the edge
    int u, v;
    /// @brief the two endpoints of the opposite edge
    int opposite_u, opposite_v;
    /// @brief iterator to the position of the edge in the list of chords
    list<Edge*>::iterator chordItr;
    /// @brief  the constructor of the class
    Edge(int a = 0, int b = 0, int c = 0, int d = 0) : u(a), v(b), opposite_u(c), opposite_v(d) {}
    void flip()
    {
        swap(u, opposite_u);
        swap(v, opposite_v);
    }

};