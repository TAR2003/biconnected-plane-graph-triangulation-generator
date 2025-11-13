#include <bits/stdc++.h>
using namespace std;
#pragma once
#include "Edge.hpp"
#include "pairHash.hpp"
#include "FaceTriangulation.hpp"
#include "ParvezRahmanNakano.hpp"

class triconnected
{
public:
    vector<vector<int>> faces;
    vector<vector<pair<int, int>>> allTriangulations;
    vector<vector<vector<pair<int, int>>>> triangulation_per_face;
    unordered_set<pair<int, int>, PairHash> present;
    triconnected(vector<vector<int>> &faces)
    {
        this->faces = faces;

        present = unordered_set<pair<int, int>, PairHash>();
        initiatePresent();
        triangulation_per_face = vector<vector<vector<pair<int, int>>>>(faces.size());
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

    void combineTriangulations(
        const vector<vector<vector<pair<int, int>>>> &triangulation_per_face,
        int index,
        vector<pair<int, int>> &current,
        vector<vector<pair<int, int>>> &allTriangulations)
    {
        // Skip faces with 0 triangulations
        while (index < (int)triangulation_per_face.size() &&
               triangulation_per_face[index].empty())
        {
            index++;
        }

        // Base case: all faces processed
        if (index == (int)triangulation_per_face.size())
        {
            allTriangulations.push_back(current);
            return;
        }

        // Recursive case: combine current triangulation with next face's triangulations
        for (const auto &tri : triangulation_per_face[index])
        {
            size_t oldSize = current.size();
            current.insert(current.end(), tri.begin(), tri.end());
            combineTriangulations(triangulation_per_face, index + 1, current, allTriangulations);
            current.resize(oldSize); // backtrack efficiently
        }
    }

    void sortTriangulations()
    {
        for (auto &triangulations : triangulation_per_face)
        {
            sort(triangulations.begin(), triangulations.end());
        }
    }

    void removeDuplicated()
    {
        vector<vector<pair<int, int>>> uniqueTriangulations;
        int totalLength = allTriangulations[0].size();
        for (auto &triangulation : allTriangulations)
        {
            set<pair<int,int>> allPairsInsideThisTriangulation;
            bool arm = false;
            for(auto &p:triangulation) {
                int a = min(p.first, p.second);
                int b = max(p.first, p.second);
                allPairsInsideThisTriangulation.insert({a,b});
                if(present.find({a,b}) != present.end()) {
                    arm = true;
                    break;
                }
            }
            if(allPairsInsideThisTriangulation.size() != totalLength) {
                continue; // skip invalid triangulations
            }
            if(arm) {
                continue; // skip triangulations having edges not in present
            }
            else {
                uniqueTriangulations.push_back(triangulation);
            }
        }
        allTriangulations.clear();
        for (auto triangulation : uniqueTriangulations)
        {
            allTriangulations.push_back(triangulation);
        }
    }

    void refineTriangulations()
    {
        sortTriangulations();
        vector<pair<int, int>> current;
        combineTriangulations(triangulation_per_face, 0, current, allTriangulations);

        removeDuplicated();
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
        cout << "Total triangulations in triconnected component: " << allTriangulations.size() << endl;
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