#include <bits/stdc++.h>
using namespace std;
#include "GraphTriangulation.hpp"


int main ()
{
    vector<vector<long long>> faces = {
        {0, 7, 8, 9, 1, 10, 11, 12, 2, 6, 5, 4},
        {0, 4, 5, 6, 2, 16, 17, 18, 3, 15, 14, 13, 1, 9, 8, 7},
        {1, 13, 14, 15, 3, 18, 17, 16, 2, 12, 11, 10}

    };
    GraphTriangulation *gt = new GraphTriangulationBiconnectedPerformance(faces, 10000000);
    gt->getAllTriangulations();
}