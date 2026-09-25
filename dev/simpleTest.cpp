#include <bits/stdc++.h>
using namespace std;
#include "GraphTriangulation.hpp"
#include "GraphTriangulationTriconnected.hpp"

vector<vector<long long>> readInput(const string &filename)
{
    ifstream infile(filename);
    if (!infile.is_open())
    {
        cerr << "Error opening file: " << filename << endl;
        return {};
    }
    vector<vector<long long>> faces;
    unordered_set<long long> uniqueVertices;
    long long faceno;
    infile >> faceno;
    for (long long i = 0; i < faceno; i++)
    {
        long long vertices;
        infile >> vertices;
        vector<long long> face;
        for (long long j = 0; j < vertices; j++)
        {
            long long vertex;
            infile >> vertex;
            face.push_back(vertex);
            uniqueVertices.insert(vertex);
        }
        faces.push_back(face);
    }
    return faces;
}

int main ()
{
    vector<vector<long long>> faces = readInput("input/Oneconnected/02_star/star_03.txt");
    // for(auto &face : faces)
    // {
    //     for(auto &vertex : face)
    //     {
    //         cout << vertex << " ";
    //     }
    //     cout << endl;
    // }
    GraphTriangulation *gt = new GraphTriangulationOneconnectedCorrectness(faces, 10000000);
    gt->getAllTriangulations();
    gt->printAllTriangulations();
    GraphTriangulationTriconnected *tc = new GraphTriangulationTriconnected(faces);
    tc->getAllTriangulations();
    tc->refineTriangulations();
    tc->removeDuplicated();
    tc->printAllTriangulations();
    cout << "Total triangulations: " << gt->totalTriangulations << endl;
    delete gt;
}