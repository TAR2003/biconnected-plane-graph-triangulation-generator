#include <bits/stdc++.h>
using namespace std;
#include "GenerateTriangulations.hpp"

class GenerateBiconnectedTriangulationsWithoutVGS : public GenerateTriangulations
{
public:
    long long totalChecks = 0, unsuccessfulChecks = 0;
    GenerateBiconnectedTriangulationsWithoutVGS(long long totalVertices, const vector<vector<long long>> &adjacentSet, long long triangulationLimit = LONG_LONG_MAX) : GenerateTriangulations(totalVertices, adjacentSet, triangulationLimit)
    {
    }

    ~GenerateBiconnectedTriangulationsWithoutVGS()
    {

    }

    bool isValidChildTriangulation(long long faceIndex, Chord *c)
    {
        totalChecks++;
        if (present.find({positions[faceIndex][c->oppositeFirst], positions[faceIndex][c->oppositeSecond]}))
        {
            unsuccessfulChecks++;
            return false;
        }
        return true;
    }
};

class GenerateBiconnectedTriangulationsWithVGS : public GenerateTriangulations
{
public:
    GenerateBiconnectedTriangulationsWithVGS(long long totalVertices, const vector<vector<long long>> &adjacentSet, long long triangulationLimit = LONG_LONG_MAX) : GenerateTriangulations(totalVertices, adjacentSet, triangulationLimit)
    {
    }

    ~GenerateBiconnectedTriangulationsWithVGS()
    {
    }

    void setupRootTriangulation(long long faceIndex)
    {
        setupChords(faceIndex);
        setupGS(faceIndex);
        setupVGS(faceIndex);
    }

    void setupVGS(long long faceIndex)
    {
        headVGS[faceIndex] = nullptr;
        auto tempGS = headGS[faceIndex];
        auto tempVGS = headVGS[faceIndex];
        while (tempGS != nullptr)
        {
            if (!present.find({positions[faceIndex][tempGS->oppositeFirst], positions[faceIndex][tempGS->oppositeSecond]}))
            {
                tempGS->isValid = true;
                if (headVGS[faceIndex] == nullptr)
                {
                    headVGS[faceIndex] = tempGS;
                    tempVGS = headVGS[faceIndex];
                    tempVGS->prevVGS = nullptr;
                    tempVGS->nextVGS = nullptr;
                }
                else
                {
                    tempVGS->nextVGS = tempGS;
                    tempGS->prevVGS = tempVGS;
                    tempGS->nextVGS = nullptr;
                    tempVGS = tempGS;
                }
            }
            else
            {
                tempGS->isValid = false;
            }
            tempGS = tempGS->nextGS;
        }
        if (headVGS[faceIndex] != nullptr)
        {
            headVGS[faceIndex]->prevVGS = nullptr;
        }
    }

    void visitAllBranches(long long faceIndex, Chord *itr)
    {
        while (itr != nullptr)
        {
            if (limitExceeded())
            {
                return;
            }
            generateChildTriangulations(faceIndex, itr);
            itr = itr->nextVGS;
        }
    }

    void updateAssociatedChord(long long faceIndex, Chord *c, Chord *itr)
    {
        updateOppositePair(faceIndex, c, itr);
        updateVGSForAssociatedChord(faceIndex, c, itr);
    }

    void updateVGSForAssociatedChord(long long faceIndex, Chord *c, Chord *itr)
    {
        if (itr == nullptr)
        {
            return;
        }
        if (itr->isValid && present.find({positions[faceIndex][itr->oppositeFirst], positions[faceIndex][itr->oppositeSecond]}))
        {
            itr->isValid = false;
            removeChordFromVGS(faceIndex, itr);
        }
        if (!itr->isValid && !present.find({positions[faceIndex][itr->oppositeFirst], positions[faceIndex][itr->oppositeSecond]}))
        {
            itr->isValid = true;
            if (itr->prevGS == c)
            {
                itr->prevVGS = c;
                itr->nextVGS = c->nextVGS;
            }
            if (itr->nextGS == c)
            {
                itr->nextVGS = c;
                itr->prevVGS = c->prevVGS;
            }
            addChordToVGS(faceIndex, itr);
        }
    }

    void removeChordFromVGS(long long faceIndex, Chord *c)
    {
        if (c->prevVGS != nullptr)
        {
            c->prevVGS->nextVGS = c->nextVGS;
        }
        else
        {
            headVGS[faceIndex] = c->nextVGS;
        }
        if (c->nextVGS != nullptr)
        {
            c->nextVGS->prevVGS = c->prevVGS;
        }
    }

    void removeChord(long long faceIndex, Chord *c)
    {
        removeChordFromGS(faceIndex, c);
        removeChordFromVGS(faceIndex, c);
    }

    void addChordToVGS(long long faceIndex, Chord *c)
    {
        if (c->prevVGS != nullptr)
        {
            c->prevVGS->nextVGS = c;
        }
        else
        {
            headVGS[faceIndex] = c;
        }
        if (c->nextVGS != nullptr)
        {
            c->nextVGS->prevVGS = c;
        }
    }

     void addChord(long long faceIndex, Chord *c)
    {
        addChordToGS(faceIndex, c);
        addChordToVGS(faceIndex, c);
    }

    Chord *getChordToFlip(long long faceIndex, Chord *c)
    {
        if (c == nullptr)
            return headVGS[faceIndex];
        if (c->prevGS != nullptr && c->prevGS->isValid)
            return c->prevGS;
        return c->nextVGS;
    }
};