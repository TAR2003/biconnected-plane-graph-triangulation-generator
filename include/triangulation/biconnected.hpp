#pragma once
#include <bits/stdc++.h>
using namespace std;
#include "Edge.hpp"
#include "pairHash.hpp"

using u128 = unsigned __int128;

class FaceTriangulation;

/// Core biconnected triangulation enumerator (thesis algorithm).
class biconnected
{
public:
    vector<vector<int>> faces;
    unordered_set<pair<int, int>, PairHash> present;
    vector<vector<pair<int, int>>> allTriangulations;
    vector<FaceTriangulation *> faceTriangulations;
    u128 totalTriangulations = 0;

    /// When false, only counts triangulations (benchmark / timing modes).
    bool storeTriangulationChords = true;

    /// Optional hook invoked after each complete triangulation is counted.
    std::function<void()> onTriangulationGenerated;

    explicit biconnected(vector<vector<int>> &faces)
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
            for (int i = 0; i < (int)face.size() - 1; i++)
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

#include "FaceTriangulation.hpp"

inline void biconnected::getAllTriangulations()
{
    faceTriangulations[0] = new FaceTriangulation(faces[0].size(), faces[0], present, 0, this);
    faceTriangulations[0]->generateAllTriangulations();
}

inline void biconnected::output(int serial)
{
    if (serial == (int)faces.size() - 1)
    {
        if (storeTriangulationChords)
            addTriangulation();
        totalTriangulations++;
        if (onTriangulationGenerated)
            onTriangulationGenerated();
    }
    else
    {
        delete faceTriangulations[serial + 1];
        faceTriangulations[serial + 1] =
            new FaceTriangulation(faces[serial + 1].size(), faces[serial + 1], present, serial + 1, this);
        faceTriangulations[serial + 1]->generateAllTriangulations();
    }
}

inline void biconnected::addTriangulation()
{
    vector<pair<int, int>> currentTriangulations;
    for (auto a : faceTriangulations)
    {
        vector<pair<int, int>> currentTriangulation;
        for (auto &chord : a->chords)
        {
            currentTriangulation.push_back(a->getPair(chord));
        }
        sort(currentTriangulation.begin(), currentTriangulation.end());
        currentTriangulations.insert(currentTriangulations.end(), currentTriangulation.begin(),
                                     currentTriangulation.end());
    }
    allTriangulations.push_back(currentTriangulations);
}
