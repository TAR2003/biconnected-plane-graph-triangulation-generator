#include <bits/stdc++.h>
using namespace std;
#include "GenerateTriangulations.hpp"

struct InputGraph
{
    long long vertexCount;
    vector<vector<long long>> adjacency;
};

InputGraph readInput(const string &filename)
{
    ifstream infile(filename);
    if (!infile.is_open())
    {
        cerr << "Error opening file: " << filename << endl;
        return {0, {}};
    }
    InputGraph graph;
    if (!(infile >> graph.vertexCount) || graph.vertexCount <= 0)
        return {0, {}};
    graph.adjacency.resize(graph.vertexCount);
    for (auto &neighbors : graph.adjacency)
    {
        long long degree;
        if (!(infile >> degree) || degree < 0)
            return {0, {}};
        neighbors.resize(degree);
        for (auto &vertex : neighbors)
        {
            if (!(infile >> vertex))
                return {0, {}};
        }
    }
    return graph;
}

void runCase(string filename)
{
    InputGraph graph = readInput(filename);
    // for(auto &face : faces)
    // {
    //     for(auto &vertex : face)
    //     {
    //         cout << vertex << " ";
    //     }
    //     cout << endl;
    // }
    GenerateTriangulations *gt = new GenerateTriangulations(graph.vertexCount, graph.adjacency, 100000000);
    cout << "Faces count: " << gt->faces.size() << endl;
    gt->generateAllTriangulations();
    // gt->printAllTriangulations();
    // GraphTriangulationTriconnected *tc = new GraphTriangulationTriconnected(graph.vertexCount, graph.adjacency);
    // tc->getAllTriangulations();
    // tc->refineTriangulations();
    // tc->removeDuplicated();
    // tc->printAllTriangulations();
    // delete tc;
    cout << "Total triangulations for : " << filename << " : " << gt->totalTriangulations << endl;
    // cout << "invalid traversals for : " << filename << " : " << gt->invalidTraversals << endl;
    // cout << "success percentage for traversal : " << filename << " : " << (double)(gt->totalTriangulations) / (double)(gt->totalTriangulations + gt->invalidTraversals) * 100.0 << endl;
    // cout << "Total face builds for : " << filename << " : " << gt->faceBuild << endl;
    cout << "Total face count for : " << filename << " : " << gt->faces.size() << endl;
    delete gt;
}

int main()
{
    // std::vector<int> arr = {10, 20, 30, 40, 50, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000};

    // for (const auto &s : arr) // 'const auto &' is better here since 's' is not modified
    // {
    //     std::string path = "../test-cases/input/Biconnected/Biconnected_delaunay_lattice/delaunay_lattice_n" + std::to_string(s) + "_0000.txt";

    //     runCase(path);
    // }
    runCase("../test-cases/input/Biconnected/Biconnected_cycle/cycle_n10_0000.txt");
    // runCase("../test-cases/input/Biconnected/Biconnected_delaunay_lattice/delaunay_lattice_n10_0000.txt");
    // runCase("input.txt");
    // runCase("input5.txt");
    // runCase("oneFace.txt");
    return 0;
}