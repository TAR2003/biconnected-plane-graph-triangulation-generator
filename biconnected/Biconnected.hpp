#include <bits/stdc++.h>
using namespace std;
#pragma once
#include "Edge.hpp"
#include "PairHash.hpp"
#include "TriangulationHasher.hpp"

using u128 = unsigned __int128;

// Forward declaration to avoid circular dependency
class FaceTriangulation;

class Biconnected
{
public:
    vector<vector<int>> faces;
    unordered_set<pair<int, int>, PairHash> present;
    vector<vector<pair<int, int>>> allTriangulations;
    vector<FaceTriangulation *> faceTriangulations;
    TriangulationRunStats *runStats = nullptr;
    enum class BenchmarkType{
        BruteForceCorrectness, 
        HashCorrectness,
        TimeComplexity
    };
    BenchmarkType type;
    u128 totalTriangulations = 0;

    Biconnected(vector<vector<int>> &faces, TriangulationRunStats *stats = nullptr, string typeOfOperation = "timeComplexity")
    {
        this->faces = faces;
        runStats = stats;
        present = unordered_set<pair<int, int>, PairHash>();
        initiatePresent();
        faceTriangulations = vector<FaceTriangulation *>(faces.size(), nullptr);
        if(typeOfOperation == "hashCorrectness") {
            type = BenchmarkType::HashCorrectness;
        }
        if(typeOfOperation == "bruteForceCorrectness")
        {
            type = BenchmarkType::BruteForceCorrectness;
        }
        if(typeOfOperation == "timeComplexity")
        {
            type = BenchmarkType::TimeComplexity;
        }
    }

    ~Biconnected();

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
inline Biconnected::~Biconnected()
{
    for (auto *ft : faceTriangulations)
    {
        delete ft;
    }
}

inline void Biconnected::getAllTriangulations()
{
    faceTriangulations[0] = new FaceTriangulation(faces[0].size(), faces[0], present, 0, this);
    faceTriangulations[0]->generateAllTriangulations();
    // printAllTriangulations();
}

inline void Biconnected::output(int serial)
{
    if (serial == faces.size() - 1)
    {
        if(type == BenchmarkType::BruteForceCorrectness || type == BenchmarkType::HashCorrectness) 
        {
           addTriangulation(); 
        }
        if(type == BenchmarkType::TimeComplexity)
        {
            totalTriangulations++;
        }
        
    }
    else
    {
        delete faceTriangulations[serial + 1];
        faceTriangulations[serial + 1] = new FaceTriangulation(faces[serial + 1].size(), faces[serial + 1], present, serial + 1, this);
        faceTriangulations[serial + 1]->generateAllTriangulations();
    }
}

inline void Biconnected::addTriangulation()
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
    sort(currentTriangulations.begin(), currentTriangulations.end());

    if (runStats != nullptr)
    {
        runStats->recordTriangulation(currentTriangulations, &allTriangulations);
    }
    else
    {
        allTriangulations.push_back(currentTriangulations);
    }
}