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
    unordered_set<pair<int, int>, PairHash> present;
    vector<vector<pair<int, int>>> allTriangulations;
    triconnected(vector<vector<int>> &faces)
    {
        this->faces = faces;
        present = unordered_set<pair<int, int>, PairHash>();
    }
    void getAllTriangulations()
    {
        ParvezRahmanNakano *ft;
        for (auto &face : faces)
        {
            int n = face.size();
            present = unordered_set<pair<int, int>, PairHash>();
            ft = new ParvezRahmanNakano(n);
            ft->generateAllTriangulations();
            for (auto &triangulation : ft->allTriangulations)
            {
                vector<pair<int, int>> mappedTriangulation;
                for (auto &chord : triangulation)
                {
                    mappedTriangulation.push_back({face[chord.first], face[chord.second]});
                    present.insert({min(face[chord.first], face[chord.second]), max(face[chord.first], face[chord.second])});
                }
                allTriangulations.push_back(mappedTriangulation);
            }
            delete ft;
        }
        printAllTriangulations();
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