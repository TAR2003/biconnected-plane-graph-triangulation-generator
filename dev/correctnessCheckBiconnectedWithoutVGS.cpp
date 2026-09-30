#include <bits/stdc++.h>
#include <filesystem>
using namespace std;
namespace fs = std::filesystem;

#include "GraphTriangulation.hpp"
#include "GraphTriangulationTriconnected.hpp"

struct InputGraph
{
    long long vertexCount;
    vector<vector<long long>> adjacency;
};

static InputGraph readInput(const string &filename)
{
    ifstream input(filename);
    InputGraph graph{0, {}};
    if (!(input >> graph.vertexCount) || graph.vertexCount <= 0)
        return graph;

    graph.adjacency.resize(graph.vertexCount);
    for (auto &neighbors : graph.adjacency)
    {
        long long degree;
        if (!(input >> degree) || degree < 0)
            return {0, {}};
        neighbors.resize(static_cast<size_t>(degree));
        for (auto &neighbor : neighbors)
        {
            if (!(input >> neighbor))
                return {0, {}};
        }
    }
    return graph;
}

using Triangulation = vector<pair<long long, long long>>;

static void normalize(vector<Triangulation> &triangulations)
{
    for (auto &triangulation : triangulations)
    {
        for (auto &edge : triangulation)
        {
            if (edge.first > edge.second)
                swap(edge.first, edge.second);
        }
        sort(triangulation.begin(), triangulation.end());
    }
    sort(triangulations.begin(), triangulations.end());
}

static bool contained(const vector<Triangulation> &algorithm,
                      const vector<Triangulation> &reference)
{
    multiset<Triangulation> remaining(reference.begin(), reference.end());
    for (const auto &triangulation : algorithm)
    {
        auto found = remaining.find(triangulation);
        if (found == remaining.end())
            return false;
        remaining.erase(found);
    }
    return remaining.empty();
}

static bool checkFile(const string &filename)
{
    InputGraph graph = readInput(filename);
    if (graph.vertexCount == 0)
    {
        cerr << "[ERROR] Invalid input: " << filename << '\n';
        return false;
    }

    GraphTriangulationBiconnectedWithoutVGSCorrectness algorithm(
        graph.vertexCount, graph.adjacency);
    algorithm.getAllTriangulations();
    algorithm.sortTriangulations();

    GraphTriangulationTriconnected reference(graph.vertexCount, graph.adjacency);
    reference.getAllTriangulations();
    reference.refineTriangulations();
    reference.removeDuplicated();
    reference.sortTriangulations();

    normalize(algorithm.allTriangulations);
    normalize(reference.allTriangulations);
    bool matches = contained(algorithm.allTriangulations, reference.allTriangulations);

    cout << (matches ? "[MATCHED] " : "[MISMATCHED] ")
         << fs::path(filename).filename().string()
         << " | WithoutVGS=" << algorithm.allTriangulations.size()
         << " | reference=" << reference.allTriangulations.size()
         << " | checks=" << algorithm.totalChecks
         << " | successfulChecks=" << algorithm.successfulChecks
         << " | invalidTraversals=" << algorithm.invalidTraversals << '\n';
    return matches;
}

int main(int argc, char **argv)
{
    const string folder = argc > 1 ? argv[1] : "input/Biconnected";
    if (!fs::exists(folder) || !fs::is_directory(folder))
    {
        cerr << "Input folder '" << folder << "' does not exist.\n";
        return 1;
    }

    bool allMatched = true;
    size_t files = 0;
    for (const auto &entry : fs::recursive_directory_iterator(folder))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".txt")
        {
            ++files;
            allMatched = checkFile(entry.path().string()) && allMatched;
        }
    }

    if (files == 0)
    {
        cerr << "No .txt input files found under '" << folder << "'.\n";
        return 1;
    }
    cout << "Checked " << files << " file(s): "
         << (allMatched ? "all matched" : "mismatches found") << '\n';
    return allMatched ? 0 : 2;
}
