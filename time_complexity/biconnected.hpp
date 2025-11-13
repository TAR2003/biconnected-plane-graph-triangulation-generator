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
    vector<FaceTriangulation *> faceTriangulations;
    int totalTriangulations = 0;
    biconnected(vector<vector<int>> &faces)
    {
        this->faces = faces;
        present = unordered_set<pair<int, int>, PairHash>();
        initiatePresent();
        faceTriangulations = vector<FaceTriangulation *>(faces.size());
    }
    void initiatePresent()
    {
        for (auto face : faces)
        {
            for (int i = 0; i < face.size() - 1; i++)
            {
                present.insert(make_pair(min(face[i], face[i + 1]), max(face[i], face[i + 1])));
            }
            present.insert(make_pair(min(face[0], face[face.size() - 1]), max(face[0], face[face.size() - 1])));
        }
    }
    void getAllTriangulations();
    void output(int serial);
    void addTriangulation();
    void sortTriangulations()
    {
        sort(allTriangulations.begin(), allTriangulations.end());
    }
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
        // addTriangulation();
        totalTriangulations++;
    }
    else
    {
        delete faceTriangulations[serial + 1];
        faceTriangulations[serial + 1] = new FaceTriangulation(faces[serial + 1].size(), faces[serial + 1], present, serial + 1, this);
        faceTriangulations[serial + 1]->generateAllTriangulations();
    }
}

// inline void biconnected::addTriangulation()
// {
//     vector<pair<int, int>> currentTriangulations;
//     for (auto a : faceTriangulations)
//     {
//         vector<pair<int, int>> currentTriangulation;
//         for (auto &chord : a->chords)
//         {
//             currentTriangulation.push_back(a->getPair(chord));
//         }
//         sort(currentTriangulation.begin(), currentTriangulation.end());
//         currentTriangulations.insert(currentTriangulations.end(), currentTriangulation.begin(), currentTriangulation.end());
//     }
//     allTriangulations.push_back(currentTriangulations);
// }
