#include <bits/stdc++.h>
using namespace std;
#pragma once
#include "Edge.hpp"
#include "pairHash.hpp"

// Forward declaration to avoid circular dependency
class FaceTriangulation;

class biconnected
{
public:
    vector<vector<int>> faces;
    unordered_set<pair<int, int>, PairHash> present;
    vector<vector<pair<int, int>>> allTriangulations;
    vector<FaceTriangulation*> faceTriangulations;
    biconnected(vector<vector<int>> &faces)
    {
        this->faces = faces;
        present = unordered_set<pair<int, int>, PairHash>();
        faceTriangulations = vector<FaceTriangulation*>(faces.size());
    }
    void getAllTriangulations();
    void output(int serial);
    void addTriangulation();

    void printAllTriangulations()
    {
        cout << "Total triangulations in biconnected component: " << allTriangulations.size() << endl;
        for (auto &triangulation : allTriangulations)
        {
            for (auto &chord : triangulation)
            {
                cout << "(" << chord.first << ", " << chord.second << ") , ";
            }
            cout << endl;
        }
    }
};

// Include FaceTriangulation.hpp after class declaration to resolve circular dependency
#include "FaceTriangulation.hpp"

// Define methods that use FaceTriangulation after including the header
inline void biconnected::getAllTriangulations()
{
    faceTriangulations[0] = new FaceTriangulation(faces[0].size(), faces[0], present, 0, this);
    faceTriangulations[0]->generateAllTriangulations();
    // printAllTriangulations();
}

inline void biconnected::output(int serial)
{
    if (serial == faces.size() - 1)
    {
        addTriangulation();
    }
    else {
        faceTriangulations[serial + 1] = new FaceTriangulation(faces[serial + 1].size(), faces[serial + 1], present, serial + 1, this);
        faceTriangulations[serial + 1]->generateAllTriangulations();
    }
}

inline void biconnected::addTriangulation()
{
    vector<pair<int, int>> currentTriangulation;
    for (auto a : faceTriangulations)
    {
        for (auto &chord : a->chords)
        {
            currentTriangulation.push_back(a->getPair(chord));
        }
    }
    for(auto c:currentTriangulation) {
        cout << "(" << c.first << ", " << c.second << ") , ";
    }
    cout << endl;
    allTriangulations.push_back(currentTriangulation);
}
