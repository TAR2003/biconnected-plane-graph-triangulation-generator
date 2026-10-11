#include <bits/stdc++.h>
using namespace std;
#include "GenerateTriangulations.hpp"

class GenerateOneconnectedTriangulations : public GenerateTriangulations
{
public:
    long long problems = 0;
    long long totalChecks = 0, unsuccessfulChecks = 0, successfulChecks = 0;
    long long totalTraversals = 0, unsuccessfulTraversals = 0, successfulTraversals = 0;

    GenerateOneconnectedTriangulations(long long totalVertices, const vector<vector<long long>> &adjacentSet, long long triangulationLimit = LONG_LONG_MAX) : GenerateTriangulations(totalVertices, adjacentSet, triangulationLimit)
    {
    }

    ~GenerateOneconnectedTriangulations()
    {
        // Destructor
    }

    void flip(long long faceIndex, Chord *c)
    {

        present.erase({positions[faceIndex][c->first], positions[faceIndex][c->second]}, faceIndex);
        c->flip();

        pair<long long, long long> oldPair = {positions[faceIndex][c->oppositeFirst], positions[faceIndex][c->oppositeSecond]};
        pair<long long, long long> newPair = {positions[faceIndex][c->first], positions[faceIndex][c->second]};
        if ( oldPair.first == oldPair.second || present.find(oldPair))
        {
            problems--;
        }
        if (newPair.first == newPair.second || present.find(newPair))
        {
            problems++;
        }



        present.insert({positions[faceIndex][c->first], positions[faceIndex][c->second]}, faceIndex);
    }
    bool isValidChildTriangulation(long long faceIndex, Chord *c)
    {
        totalChecks++;

        pair<long long, long long> oppositePair = {positions[faceIndex][c->oppositeFirst], positions[faceIndex][c->oppositeSecond]};
        if (oppositePair.first == oppositePair.second)
        {
            unsuccessfulChecks++;
            return false;
        }
        // cout << "On the matter of flipping the chord: " << oppositePair.first << " " << oppositePair.second << endl;
        if (present.find(oppositePair))
        {
            if (oppositePair.first != positions[faceIndex][0] && oppositePair.second != positions[faceIndex][0])
            {
                unsuccessfulChecks++;
                return false; // that means it does not conflict with any of the root triangulation generating set
            }
            if (present.find(oppositePair, faceIndex))
            {
                unsuccessfulChecks++;
                return false; // that means it might have a conflict with the border chord or other edges from other faces
            }
        }
        successfulChecks++;
        return true;
    }

    void setupChords(long long faceIndex)
    {
        auto temp = headChord[faceIndex];
        for (int i = 0; i < faces[faceIndex].size() - 3; i++)
        {
            temp->first = 0;
            temp->second = i + 2;
            if (present.find({positions[faceIndex][temp->first], positions[faceIndex][temp->second]}) || positions[faceIndex][temp->first] == positions[faceIndex][temp->second])
            {
                problems++;
            }
            present.insert({positions[faceIndex][temp->first], positions[faceIndex][temp->second]}, faceIndex);
            temp->oppositeFirst = (i + 1) % faces[faceIndex].size();
            temp->oppositeSecond = (i + 3) % faces[faceIndex].size();
            temp->faceIndex = faceIndex;
            
            temp = temp->nextChord;
        }
    }

    void setupRootTriangulation(long long faceIndex)
    {
        setupChords(faceIndex);
        setupGS(faceIndex);
    }

    void output(long long faceIndex) 
    {
        totalTraversals++;
        if (problems == 0)
        {
            successfulTraversals++;
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
        else 
        {
            unsuccessfulTraversals++;
        }
    }
};