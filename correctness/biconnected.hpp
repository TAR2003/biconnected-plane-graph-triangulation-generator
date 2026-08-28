#pragma once
#include <bits/stdc++.h>
using namespace std;

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

    size_t totalCount = 0;
    ofstream *outStream = nullptr;

    biconnected(vector<vector<int>> &faces)
    {
        this->faces = faces;
        present = unordered_set<pair<int, int>, PairHash>();
        initiatePresent();
        faceTriangulations = vector<FaceTriangulation *>(faces.size(), nullptr);
    }

    ~biconnected()
    {
        for (auto ft : faceTriangulations)
        {
            delete ft;
        }
    }

    void initiatePresent()
    {
        for (const auto &face : faces)
        {
            for (size_t i = 0; i < face.size() - 1; i++)
            {
                present.insert(make_pair(min(face[i], face[i + 1]), max(face[i], face[i + 1])));
            }
            present.insert(make_pair(min(face[0], face[face.size() - 1]), max(face[0], face[face.size() - 1])));
        }
    }

    void getAllTriangulations();
    void getAllTriangulationsToFile(const string &outputPath);
    void output(int serial);
    void addTriangulation();

    void sortTriangulations()
    {
        sort(allTriangulations.begin(), allTriangulations.end());
    }

    void printAllTriangulations()
    {
        cout << "Total triangulations in biconnected component: " << (outStream ? totalCount : allTriangulations.size()) << endl;
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

inline void biconnected::getAllTriangulations()
{
    totalCount = 0;
    outStream = nullptr;
    faceTriangulations[0] = new FaceTriangulation(faces[0].size(), faces[0], present, 0, this);
    faceTriangulations[0]->generateAllTriangulations();
}

inline void biconnected::getAllTriangulationsToFile(const string &outputPath)
{
    totalCount = 0;
    ofstream file(outputPath, ios::binary);
    outStream = &file;

    faceTriangulations[0] = new FaceTriangulation(faces[0].size(), faces[0], present, 0, this);
    faceTriangulations[0]->generateAllTriangulations();

    file.close();
    outStream = nullptr;
}

inline void biconnected::output(int serial)
{
    if (serial == static_cast<int>(faces.size()) - 1)
    {
        addTriangulation();
    }
    else
    {
        delete faceTriangulations[serial + 1];
        faceTriangulations[serial + 1] = new FaceTriangulation(faces[serial + 1].size(), faces[serial + 1], present, serial + 1, this);
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
        currentTriangulations.insert(currentTriangulations.end(), currentTriangulation.begin(), currentTriangulation.end());
    }

    totalCount++;

    if (outStream && outStream->is_open())
    {
        uint32_t sz = static_cast<uint32_t>(currentTriangulations.size());
        outStream->write(reinterpret_cast<const char *>(&sz), sizeof(sz));
        outStream->write(reinterpret_cast<const char *>(currentTriangulations.data()), sz * sizeof(pair<int, int>));
    }
    else
    {
        allTriangulations.push_back(currentTriangulations);
    }
}