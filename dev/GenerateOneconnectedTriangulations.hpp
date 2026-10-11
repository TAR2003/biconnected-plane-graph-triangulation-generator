#include <bits/stdc++.h>
using namespace std;
#include "GenerateTriangulations.hpp"

class GenerateOneconnectedTriangulations : public GenerateTriangulations
{
public:
    long long problems = 0;
    long long totalChecks = 0, unsuccessfulChecks = 0;
    long long totalTraversals = 0, unsuccessfulTraversals = 0;

    GenerateOneconnectedTriangulations(long long totalVertices, const vector<vector<long long>> &adjacentSet, long long triangulationLimit = LONG_LONG_MAX) : GenerateTriangulations(totalVertices, adjacentSet, triangulationLimit)
    {
    }

    ~GenerateOneconnectedTriangulations()
    {
        // Destructor
    }

    bool isValidChildTriangulation(long long faceIndex, Chord *c)
    {
        if (present.find({positions[faceIndex][c->oppositeFirst], positions[faceIndex][c->oppositeSecond]}))
        {
            return false;
        }
        return true;
    }

    void setupRootTriangulation(long long faceIndex)
    {
        setupChords(faceIndex);
        setupGS(faceIndex);
        auto itr = headGS[faceIndex];
        while (itr != nullptr)
        {
            if (present.find({positions[faceIndex][itr->oppositeFirst], positions[faceIndex][itr->oppositeSecond]}))
            {
                itr->isValid = false;
            }
            else
            {
                itr->isValid = true;
            }
            itr = itr->nextGS;
        }
    }

    void output(long long faceIndex) 
    {
        if (problems == 0)
        {
            if (faceIndex == faces.size() - 1)
            {
                totalTriangulations++;
                return;
            }
            else
            {
                generateFaceTriangulations(faceIndex + 1);
            }
        }
    }
};