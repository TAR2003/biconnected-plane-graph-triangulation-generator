#include <bits/stdc++.h>
using namespace std;
#include "Edge.hpp"
#include "FaceTriangulation.hpp"
#include "pairHash.hpp"

int main () 
{
    unordered_set<pair<int, int>, PairHash> present;
    int n;
    vector<int> elements = {1,2,3,4,5};
    n = elements.size();
    FaceTriangulation* ft = new FaceTriangulation(n, elements, present);
    ft->generateAllTriangulations();
    ft->printAllTriangulations();
    delete ft;
    return 0;
}
