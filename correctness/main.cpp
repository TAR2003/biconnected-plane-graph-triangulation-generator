#include <bits/stdc++.h>
using namespace std;
#include "Edge.hpp"
#include "pairHash.hpp"
#include "biconnected.hpp"
#include "FaceTriangulation.hpp"
#include "triconnected.hpp"

vector<vector<int>> input(string filename)
{
    ifstream infile(filename);
    if (!infile.is_open())
    {
        cerr << "Error opening file: " << filename << endl;
        return {};
    }
    vector<vector<int>> faces;
    int faceno;
    infile >> faceno;
    for (int i = 0; i < faceno; i++)
    {
        int vertices;
        infile >> vertices;
        vector<int> face;
        for (int j = 0; j < vertices; j++)
        {
            int vertex;
            infile >> vertex;
            face.push_back(vertex);
        }
        faces.push_back(face);
    }
    return faces;
}

bool matchPairs(const pair<int, int> &p1, const pair<int, int> &p2)
{
    return (p1.first == p2.first && p1.second == p2.second);
}

bool matchTriangulations(const vector<pair<int, int>> &t1, const vector<pair<int, int>> &t2)
{
    if (t1.size() != t2.size())
        return false;
    for (size_t i = 0; i < t1.size(); i++)
    {
        if (!matchPairs(t1[i], t2[i]))
            return false;
    }
    return true;
}

bool matchTwoAlgorithms(string filename)
{
    vector<vector<int>> faces = input(filename);
    biconnected *bc = new biconnected(faces);
    bc->getAllTriangulations();
    bc->sortTriangulations();
    triconnected *tc = new triconnected(faces);
    tc->getAllTriangulations();
    tc->refineTriangulations();
    auto newalgo = bc->allTriangulations;
    auto oldalgo = tc->allTriangulations;
    cout << "Comparing results from both algorithms..." << endl;
    if (newalgo.size() != oldalgo.size())
    {
        cout << "Mismatch in number of triangulations: New Algo = " << newalgo.size() << ", Old Algo = " << oldalgo.size() << endl;
        return false;
    }
    else
    {
        bool allMatch = true;
        for (size_t i = 0; i < newalgo.size(); i++)
        {
            if (!matchTriangulations(newalgo[i], oldalgo[i]))
            {
                cout << "\033[1;31m Mismatch found in triangulation " << i + 1 << "\033[0m" << endl;
                allMatch = false;
                break;
            }
        }
        if (allMatch)
        {
            cout << "\033[1;32m All triangulations match between both algorithms.\033[0m" << endl;
        }
        return allMatch;
    }
}

#include <filesystem>
namespace fs = std::filesystem;

int main()
{
    string folder = "input";
    vector<pair<string, bool>> files;
    // Loop through all files in the folder
    for (const auto &entry : fs::directory_iterator(folder))
    {
        if (entry.is_regular_file())
        {
            string filename = entry.path().string();
            cout << "Processing: " << filename << endl;
            bool result = matchTwoAlgorithms(filename);
            files.push_back({filename, result});
        }
    }
    for (int i = 0; i < files.size(); i++)
    {
        if (files[i].second)
        {
            cout << "File: " << " => \033[1;32mMatched: " << files[i].first << " \033[0m" << endl;
        }
        else
        {
            cout << "File: " << " => \033[1;31mMismatched: " << files[i].first << " \033[0m" << endl;
        }
    }

    return 0;
}