#include <bits/stdc++.h>
using namespace std;
#include "Edge.hpp"
#include "FaceTriangulation.hpp"
#include "pairHash.hpp"
#include "biconnected.hpp"

vector<vector<int>> solve(string filename)
{
    ifstream infile(filename);
    if(!infile.is_open())
    {
        cerr << "Error opening file: " << filename << endl;
        return {};
    }
    vector<vector<int>> faces;
    int faceno;
    infile >> faceno;
    for(int i = 0 ; i < faceno ; i++) {
        int vertices;
        infile >> vertices;
        vector<int> face;
        for(int j = 0 ; j < vertices ; j++) {
            int vertex;
            infile >> vertex;
            face.push_back(vertex);
        }
        faces.push_back(face);
    }
    return faces;
}

int main()
{
    string filename = "input/simple3.txt";
    vector<vector<int>> faces = solve(filename);
    biconnected bc(faces);
    bc.getAllTriangulations();
    unordered_set<pair<int, int>, PairHash> present;
    vector<int> elements = {1,2,3,4};
    FaceTriangulation *f = new FaceTriangulation(4, elements, present);
    f->generateAllTriangulations();
    f->printAllTriangulations();
    return 0;
}
