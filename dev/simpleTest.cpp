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

void runCase(string filename)
{
    vector<vector<long long>> faces = readInput(filename);
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
    // GraphTriangulationTriconnected *tc = new GraphTriangulationTriconnected(faces);
    // tc->getAllTriangulations();
    // tc->refineTriangulations();
    // tc->removeDuplicated();
    // tc->printAllTriangulations(); 
    // delete tc;
    cout << "Total triangulations for : " << filename << " : " << gt->totalTriangulations << endl;
    cout << "invalid traversals for : " << filename << " : " << gt->invalidTraversals << endl;
    cout << "success percentage for traversal : " << filename << " : " << (double)(gt->totalTriangulations) / (double)(gt->totalTriangulations + gt->invalidTraversals) * 100.0 << endl;
   
    delete gt;
}

int main ()
{
    runCase("input/Oneconnected/02_star/star_12.txt");
    runCase("input/Oneconnected/02_star/star_13.txt");
    return 0;

}