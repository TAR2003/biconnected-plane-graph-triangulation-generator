#include <bits/stdc++.h>
using namespace std;
#include "GraphTriangulation.hpp"
#include "GraphTriangulationTriconnected.hpp"

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
    GraphTriangulation *gt = new GraphTriangulationOneconnectedCorrectness(graph.vertexCount, graph.adjacency, 10000000);
    gt->getAllTriangulations();
    gt->printAllTriangulations();
    // GraphTriangulationTriconnected *tc = new GraphTriangulationTriconnected(graph.vertexCount, graph.adjacency);
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