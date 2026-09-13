#include <bits/stdc++.h>
using namespace std;
#include "Edge.hpp"
#include "pairHash.hpp"
#include "biconnected.hpp"
#include "FaceTriangulation.hpp"

using u128 = unsigned __int128;

static string u128_to_string(u128 x)
{
    if (x == 0)
        return "0";
    string s;
    while (x > 0)
    {
        long long digit = (long long)(x % 10);
        s.push_back('0' + digit);
        x /= 10;
    }
    reverse(s.begin(), s.end());
    return s;
}

vector<vector<long long>> input(string filename)
{
    ifstream infile(filename);
    if (!infile.is_open())
    {
        cerr << "Error opening file: " << filename << endl;
        return {};
    }
    vector<vector<long long>> faces;
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
        }
        faces.push_back(face);
    }
    return faces;
}

void output(vector<vector<pair<long long, long long>>> &allTriangulations, string filename)
{
    ofstream outfile(filename);
    if (!outfile.is_open())
    {
        cerr << "Error opening output file." << endl;
        return;
    }
    outfile << "Total triangulations in biconnected component: " << allTriangulations.size() << endl;
    for (auto &triangulation : allTriangulations)
    {
        for (auto &chord : triangulation)
        {
            outfile << "(" << chord.first << ", " << chord.second << ") , ";
        }
        outfile << endl;
    }

}

#include <filesystem>
namespace fs = std::filesystem;

long long main()
{
    string filename = "input.txt";

    cout << "Processing: " << filename << endl;

    vector<vector<long long>> faces = input(filename);
    
    biconnected *bc = new biconnected(faces);

    bc->getAllTriangulations();

    // bc->printAllTriangulations();
    u128 totalTriangulations = bc->totalTriangulations;
    cout << "Total triangulations in biconnected component: " << u128_to_string(totalTriangulations) << endl;
    output(bc->allTriangulations, "output.txt");
    delete bc;



    return 0;
}