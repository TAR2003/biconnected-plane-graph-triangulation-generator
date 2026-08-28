#pragma once
#include <bits/stdc++.h>
using namespace std;

#include "Edge.hpp"
#include "pairHash.hpp"
#include "FaceTriangulation.hpp"
#include "ParvezRahmanNakano.hpp"
#include "TaskAborted.hpp"

class triconnected
{
public:
    vector<vector<int>> faces;
    vector<vector<pair<int, int>>> allTriangulations;
    vector<vector<vector<pair<int, int>>>> triangulation_per_face;
    unordered_set<pair<int, int>, PairHash> present;

    size_t totalCount = 0;
    ofstream *outStream = nullptr;

    // Cooperative-cancellation support.
    // Set by the caller (main.cpp) to a shared atomic<bool>* before calling
    // refineTriangulationsToFile(). Checked periodically (every checkInterval
    // writes) inside the DFS leaf so a runaway task can be stopped mid-flight
    // instead of only being detected after it finishes.
    atomic<bool> *abortFlag = nullptr;
    size_t checkInterval = 256; // how many writes between abortFlag checks
    size_t writesSinceCheck = 0;

    triconnected(vector<vector<int>> &faces)
    {
        this->faces = faces;
        present = unordered_set<pair<int, int>, PairHash>();
        initiatePresent();
        triangulation_per_face = vector<vector<vector<pair<int, int>>>>(faces.size());
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

    void getAllTriangulations()
    {
        ParvezRahmanNakano *ft;
        int pos = 0;
        for (auto &face : faces)
        {
            int n = face.size();
            ft = new ParvezRahmanNakano(n);
            ft->generateAllTriangulations();
            for (auto &triangulation : ft->allTriangulations)
            {
                vector<pair<int, int>> mappedTriangulation;
                for (auto &chord : triangulation)
                {
                    mappedTriangulation.push_back({min(face[chord.first], face[chord.second]), max(face[chord.first], face[chord.second])});
                }
                sort(mappedTriangulation.begin(), mappedTriangulation.end());
                triangulation_per_face[pos].push_back(mappedTriangulation);
            }
            delete ft;
            pos++;
        }
    }

    void sortTriangulations()
    {
        for (auto &triangulations : triangulation_per_face)
        {
            sort(triangulations.begin(), triangulations.end());
        }
    }

    // Checks the shared abort flag; throws if tripped. Called right before
    // each disk write so we bail out promptly without writing more data.
    inline void checkAbort()
    {
        if (!abortFlag)
            return;
        if (++writesSinceCheck < checkInterval)
            return;
        writesSinceCheck = 0;
        if (abortFlag->load(memory_order_relaxed))
        {
            throw TaskAbortedException();
        }
    }

    void combineTriangulationsDFS(
        int faceIndex,
        vector<pair<int, int>> &current,
        unordered_set<pair<int, int>, PairHash> &usedChords)
    {
        while (faceIndex < (int)triangulation_per_face.size() &&
               triangulation_per_face[faceIndex].empty())
        {
            faceIndex++;
        }

        if (faceIndex == (int)triangulation_per_face.size())
        {
            checkAbort(); // may throw TaskAbortedException

            totalCount++;
            if (outStream && outStream->is_open())
            {
                uint32_t sz = static_cast<uint32_t>(current.size());
                outStream->write(reinterpret_cast<const char *>(&sz), sizeof(sz));
                outStream->write(reinterpret_cast<const char *>(current.data()), sz * sizeof(pair<int, int>));
            }
            else
            {
                allTriangulations.push_back(current);
            }
            return;
        }

        for (const auto &tri : triangulation_per_face[faceIndex])
        {
            bool valid = true;
            for (const auto &raw : tri)
            {
                int a = min(raw.first, raw.second);
                int b = max(raw.first, raw.second);

                if (present.find({a, b}) != present.end() || usedChords.find({a, b}) != usedChords.end())
                {
                    valid = false;
                    break;
                }
            }
            if (!valid)
                continue;

            {
                set<pair<int, int>> withinFace;
                bool internalDup = false;
                for (const auto &raw : tri)
                {
                    int a = min(raw.first, raw.second);
                    int b = max(raw.first, raw.second);
                    if (!withinFace.insert({a, b}).second)
                    {
                        internalDup = true;
                        break;
                    }
                }
                if (internalDup)
                    continue;
            }

            size_t oldSize = current.size();
            vector<pair<int, int>> addedChords;
            addedChords.reserve(tri.size());

            for (const auto &raw : tri)
            {
                int a = min(raw.first, raw.second);
                int b = max(raw.first, raw.second);
                current.push_back({a, b});
                usedChords.insert({a, b});
                addedChords.push_back({a, b});
            }

            // combineTriangulationsDFS may throw TaskAbortedException; let it
            // propagate up through the recursion. The catch below (or the
            // caller in refineTriangulationsToFile) ensures the ofstream is
            // still closed via RAII / explicit close in the caller.
            combineTriangulationsDFS(faceIndex + 1, current, usedChords);

            current.resize(oldSize);
            for (const auto &c : addedChords)
            {
                usedChords.erase(c);
            }
        }
    }

    // Runs the DFS writing to outputPath. If abortFlag is set and trips
    // mid-run, throws TaskAbortedException; the output stream is always
    // closed (even on throw) and outStream is reset to nullptr.
    void refineTriangulationsToFile(const string &outputPath, atomic<bool> *externalAbortFlag = nullptr)
    {
        sortTriangulations();
        totalCount = 0;
        writesSinceCheck = 0;
        abortFlag = externalAbortFlag;

        ofstream file(outputPath, ios::binary);
        outStream = &file;

        vector<pair<int, int>> current;
        unordered_set<pair<int, int>, PairHash> usedChords;

        try
        {
            combineTriangulationsDFS(0, current, usedChords);
        }
        catch (...)
        {
            // Guarantee stream is closed / state reset even on abort or any
            // other exception, then rethrow so the caller (main.cpp) can
            // classify it and clean up temp files.
            if (file.is_open())
                file.close();
            outStream = nullptr;
            throw;
        }

        file.close();
        outStream = nullptr;
    }

    void refineTriangulations()
    {
        sortTriangulations();
        allTriangulations.clear();
        totalCount = 0;
        outStream = nullptr;
        writesSinceCheck = 0;

        vector<pair<int, int>> current;
        unordered_set<pair<int, int>, PairHash> usedChords;
        combineTriangulationsDFS(0, current, usedChords);
    }

    void printTriangulationsPerFace()
    {
        int face_no = 0;
        for (auto &face_triangulations : triangulation_per_face)
        {
            cout << "Face " << face_no << " has " << face_triangulations.size() << " triangulations:" << endl;
            for (auto &triangulation : face_triangulations)
            {
                for (auto &chord : triangulation)
                {
                    cout << "(" << chord.first << ", " << chord.second << ") , ";
                }
                cout << endl;
            }
            face_no++;
        }
    }

    void printAllTriangulations()
    {
        cout << "Total triangulations in triconnected component: " << (outStream ? totalCount : allTriangulations.size()) << endl;
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