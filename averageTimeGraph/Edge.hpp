#include <bits/stdc++.h>
#pragma once

using namespace std;
class Edge
{
public:
    /// @brief the two endpoints of the edge
    int first, second;
    /// @brief the two endpoints of the opposite edge
    int opposite_first, opposite_second;
    /// @brief iterator to the position of the edge in the list of chords
    list<Edge *>::iterator chordItrGS;
    list<Edge *>::iterator chordItrVGS;
    bool isValid;
    /// @brief  the constructor of the class
    Edge(int a = 0, int b = 0, int c = 0, int d = 0) : first(a), second(b), opposite_first(c), opposite_second(d)
    {
        isValid = true;
    }
    /// @brief flips the edge by swapping its endpoints with the opposite endpoints
    void flip()
    {
        swap(first, opposite_first);
        swap(second, opposite_second);
    }
};