#include <bits/stdc++.h>
using namespace std;
#include "GenerateTriangulations.hpp"



class GenerateOneconnectedTriangulations : public GenerateTriangulations {
public:

    GenerateOneconnectedTriangulations(long long totalVertices, const vector<vector<long long>> &adjacentSet, long long triangulationLimit = LONG_LONG_MAX) : GenerateTriangulations(totalVertices, adjacentSet, triangulationLimit) {
        
    }

    ~GenerateOneconnectedTriangulations() {
        // Destructor
    }

    bool isValidChildTriangulation(long long faceIndex, Chord *c) {
        if (present.find({positions[faceIndex][c->oppositeFirst], positions[faceIndex][c->oppositeSecond]})) {
            return false;
        }
        return true;
    }
};