#include <bits/stdc++.h>
using namespace std;
#include "Edge.hpp"
#include "pairHash.hpp"
#include "biconnected.hpp"
#include "FaceTriangulation.hpp"

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

#include <filesystem>
namespace fs = std::filesystem;

int main()
{
    string filename = "input.txt";

    cout << "Processing: " << filename << endl;

    vector<vector<int>> faces = input(filename);
    
    biconnected *bc = new biconnected(faces);

    bc->getAllTriangulations();

    // bc->printAllTriangulations();
    int totalTriangulations = bc->totalTriangulations;
    cout << "Total triangulations in biconnected component: " << totalTriangulations << endl;

    return 0;
}