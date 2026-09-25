#include <bits/stdc++.h>
using namespace std;
#pragma once
#include "Edge.hpp"
#include "PairHash.hpp"
#include <functional>

// Forward declaration to avoid circular dependency
class FaceTriangulation;


class GraphTriangulation
{
public:
    vector<vector<long long>> faces;
    unordered_multiset<pair<long long, long long>, PairHash> present;
    vector<vector<pair<long long, long long>>> allTriangulations;
    vector<FaceTriangulation *> faceTriangulations;
    long long totalChecks = 0;
    long long successfulChecks = 0;
    long long invalidTraversals = 0;
    long long triangulationLimit;

    long long totalTriangulations = 0;

    GraphTriangulation(vector<vector<long long>> &faces, long long triangulationLimit = LONG_LONG_MAX)
    {
        this->faces = faces;
        present = unordered_multiset<pair<long long, long long>, PairHash>();
        initiatePresent();
        this->triangulationLimit = triangulationLimit;
        faceTriangulations = vector<FaceTriangulation *>(faces.size(), nullptr);
    }

    virtual ~GraphTriangulation();

    void initiatePresent()
    {
        for (auto face : faces)
        {
            for (long long i = 0; i < face.size() - 1; i++)
            {
                present.insert(make_pair(min(face[i], face[i + 1]), max(face[i], face[i + 1])));
            }
            present.insert(make_pair(min(face[0], face[face.size() - 1]), max(face[0], face[face.size() - 1])));
        }
    }
    void getNextFaceTriangulation(long long serial);
    virtual void storeTriangulation() = 0;
    virtual FaceTriangulation * getFaceTriangulation(long long serial) = 0;
    void getAllTriangulations();
    void output(long long serial);
    void addTriangulation();
    void sortTriangulations()
    {
        sort(allTriangulations.begin(), allTriangulations.end());
    }
    void printAllTriangulations()
    {
        cout << "Total triangulations in graph: " << allTriangulations.size() << endl;
        for (auto &triangulation : allTriangulations)
        {
            for (auto &chord : triangulation)
            {
                cout << "(" << chord.first << ", " << chord.second << ") , ";
            }
            cout << endl;
        }
    }

    bool crossedLimits()
    {
        return totalTriangulations >= triangulationLimit;
    }

    void printPresent()
    {
        cout << "Printing the present set" << endl;
        for (auto &edge : present)
        {
            cout << "(" << edge.first << ", " << edge.second << ") , ";
        }
        cout << endl;
    }
};

// Include FaceTriangulation.hpp after class declaration to resolve circular dependency
#include "FaceTriangulation.hpp"

// Define methods that use FaceTriangulation after including the header
inline GraphTriangulation::~GraphTriangulation()
{
    for (auto *ft : faceTriangulations)
    {
        delete ft;
    }
}

inline void GraphTriangulation::storeTriangulation()
{
    addTriangulation();
}

inline void GraphTriangulation::getNextFaceTriangulation(long long serial)
{
    if (crossedLimits())
    {
        return;
    }
    faceTriangulations[serial] = getFaceTriangulation(serial);
    faceTriangulations[serial]->generateAllTriangulations();
}

inline void GraphTriangulation::getAllTriangulations()
{
    cout << "starting all triangulatioons" << endl;
    getNextFaceTriangulation(0);
    cout << "ended all triangulations" << endl;
}

inline void GraphTriangulation::output(long long serial)
{
    // cout << "Total triangulation number: " << totalTriangulations << endl;
    if (crossedLimits())
    {
        cout << "Triangulation Limit already reached, still coming to this method shows bug in the code" << endl;
    }
    if (serial == faces.size() - 1)
    {
        totalTriangulations++;
        storeTriangulation();
    }
    else
    {
        delete faceTriangulations[serial + 1];
        getNextFaceTriangulation(serial + 1);
    }
}

inline void GraphTriangulation::addTriangulation()
{
    vector<pair<long long, long long>> currentTriangulations;
    for (auto a : faceTriangulations)
    {
        vector<pair<long long, long long>> currentTriangulation;
        for (auto &chord : a->chords)
        {
            currentTriangulation.push_back(a->getPair(chord));
        }
        sort(currentTriangulation.begin(), currentTriangulation.end());
        currentTriangulations.insert(currentTriangulations.end(), currentTriangulation.begin(), currentTriangulation.end());
    }
    allTriangulations.push_back(currentTriangulations);
}

#include "FaceTriangulationOneconnected.hpp"
#include "FaceTriangulationBiconnected.hpp"

class GraphTriangulationOneconnected : public GraphTriangulation
{
public:
    GraphTriangulationOneconnected(vector<vector<long long>> &faces, long long triangulationLimit = LONG_LONG_MAX) : GraphTriangulation(faces, triangulationLimit) {};
    FaceTriangulation *getFaceTriangulation(long long serial) override;
};

inline FaceTriangulation *GraphTriangulationOneconnected::getFaceTriangulation(long long serial)
{
    return new FaceTriangulationOneconnected(faces[serial].size(), faces[serial], present, serial, this);
}

class GraphTriangulationBiconnected : public GraphTriangulation
{
public:
    GraphTriangulationBiconnected(vector<vector<long long>> &faces, long long triangulationLimit = LONG_LONG_MAX) : GraphTriangulation(faces, triangulationLimit) {};
    FaceTriangulation *getFaceTriangulation(long long serial) override;
};

inline FaceTriangulation *GraphTriangulationBiconnected::getFaceTriangulation(long long serial)
{
    return new FaceTriangulationBiconnected(faces[serial].size(), faces[serial], present, serial, this);
}

class GraphTriangulationBiconnectedPerformance : public GraphTriangulationBiconnected
{
public:
    GraphTriangulationBiconnectedPerformance(vector<vector<long long>> &faces, long long triangulationLimit = LONG_LONG_MAX) : GraphTriangulationBiconnected(faces, triangulationLimit) {};
    void storeTriangulation() override;
};

inline void GraphTriangulationBiconnectedPerformance::storeTriangulation()
{
}

class GraphTriangulationOneconnectedPerformance : public GraphTriangulationOneconnected
{
public:
    GraphTriangulationOneconnectedPerformance(vector<vector<long long>> &faces, long long triangulationLimit = LONG_LONG_MAX) : GraphTriangulationOneconnected(faces, triangulationLimit) {};
    void storeTriangulation() override;
};

inline void GraphTriangulationOneconnectedPerformance::storeTriangulation()
{
}

class GraphTriangulationBiconnectedCorrectness : public GraphTriangulationBiconnected
{
public:
    GraphTriangulationBiconnectedCorrectness(vector<vector<long long>> &faces, long long triangulationLimit = LONG_LONG_MAX) : GraphTriangulationBiconnected(faces, triangulationLimit) {};
    void storeTriangulation() override;
};

inline void GraphTriangulationBiconnectedCorrectness::storeTriangulation()
{
    addTriangulation();
}

class GraphTriangulationOneconnectedCorrectness : public GraphTriangulationOneconnected
{
public:
    GraphTriangulationOneconnectedCorrectness(vector<vector<long long>> &faces, long long triangulationLimit = LONG_LONG_MAX) : GraphTriangulationOneconnected(faces, triangulationLimit){};
    void storeTriangulation() override;
};

inline void GraphTriangulationOneconnectedCorrectness::storeTriangulation()
{
    addTriangulation();
}

// ============================================================================
// Per-triangulation timing (used by the "individual" benchmark)
// ============================================================================

// timesNs[k] = nanoseconds elapsed since start() when the (k+1)-th triangulation
// was generated. The buffer is reserved up-front so that vector reallocation
// never distorts a timestamp in the middle of a run.
class GenerationTimeline
{
public:
    using Clock = std::chrono::steady_clock;

    vector<long long> timesNs;

    void start(long long expected)
    {
        timesNs.clear();
        timesNs.reserve((size_t)min<long long>(expected, 1LL << 20));
        origin = Clock::now();
    }

    void record()
    {
        timesNs.push_back(chrono::duration_cast<chrono::nanoseconds>(Clock::now() - origin).count());
    }

private:
    Clock::time_point origin;
};

class GraphTriangulationBiconnectedIndividualPerformance : public GraphTriangulationBiconnectedPerformance
{
public:
    GenerationTimeline timeline;
    GraphTriangulationBiconnectedIndividualPerformance(vector<vector<long long>> &faces, long long triangulationLimit = LONG_LONG_MAX) : GraphTriangulationBiconnectedPerformance(faces, triangulationLimit) {};
    void storeTriangulation() override;
};

inline void GraphTriangulationBiconnectedIndividualPerformance::storeTriangulation()
{
    timeline.record();
}

class GraphTriangulationOneconnectedIndividualPerformance : public GraphTriangulationOneconnectedPerformance
{
public:
    GenerationTimeline timeline;
    GraphTriangulationOneconnectedIndividualPerformance(vector<vector<long long>> &faces, long long triangulationLimit = LONG_LONG_MAX) : GraphTriangulationOneconnectedPerformance(faces, triangulationLimit) {};
    void storeTriangulation() override;
};

inline void GraphTriangulationOneconnectedIndividualPerformance::storeTriangulation()
{
    timeline.record();
}