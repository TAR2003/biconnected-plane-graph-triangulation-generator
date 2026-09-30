#include <bits/stdc++.h>
#include <chrono>
#include <filesystem>
using namespace std;
namespace fs = std::filesystem;

#include "GraphTriangulation.hpp"

static constexpr long long TRIANGULATION_LIMIT = 10000000;

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

static string csvEscape(const string &value)
{
    string escaped = value;
    size_t position = 0;
    while ((position = escaped.find('"', position)) != string::npos)
    {
        escaped.insert(position, 1, '"');
        position += 2;
    }
    return '"' + escaped + '"';
}

static void writeHeader(ofstream &output)
{
    output << "filename,vertices,triangulations,timeSeconds,totalChecks,"
              "successfulChecks,invalidTraversals,status\n";
}

static bool benchmarkFile(const string &filename, ofstream &output)
{
    InputGraph graph = readInput(filename);
    if (graph.vertexCount == 0)
    {
        cerr << "[ERROR] Invalid input: " << filename << '\n';
        return false;
    }

    GraphTriangulationBiconnectedWithoutVGSPerformance algorithm(
        graph.vertexCount, graph.adjacency, TRIANGULATION_LIMIT);
    auto start = chrono::steady_clock::now();
    algorithm.getAllTriangulations();
    auto finish = chrono::steady_clock::now();
    double seconds = chrono::duration<double>(finish - start).count();
    string status = algorithm.totalTriangulations >= TRIANGULATION_LIMIT
                        ? "triangulation_limit_exceeded"
                        : "completed";

    output << csvEscape(fs::path(filename).filename().string()) << ','
           << graph.vertexCount << ',' << algorithm.totalTriangulations << ','
           << fixed << setprecision(9) << seconds << ','
           << algorithm.totalChecks << ',' << algorithm.successfulChecks << ','
           << algorithm.invalidTraversals << ',' << status << '\n';

    cout << filename << " | vertices=" << graph.vertexCount
         << " | triangulations=" << algorithm.totalTriangulations
         << " | time=" << fixed << setprecision(6) << seconds << " s"
         << " | status=" << status << '\n';
    return true;
}

int main(int argc, char **argv)
{
    const string folder = argc > 1 ? argv[1] : "input/Biconnected";
    const string csvFilename = argc > 2
                                   ? argv[2]
                                   : "results_time_complexity_check_biconnected_without_vgs.csv";
    if (!fs::exists(folder) || !fs::is_directory(folder))
    {
        cerr << "Input folder '" << folder << "' does not exist.\n";
        return 1;
    }

    ofstream output(csvFilename);
    if (!output.is_open())
    {
        cerr << "Could not open CSV report '" << csvFilename << "'.\n";
        return 1;
    }
    writeHeader(output);

    vector<fs::path> files;
    for (const auto &entry : fs::recursive_directory_iterator(folder))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".txt")
            files.push_back(entry.path());
    }
    sort(files.begin(), files.end());
    if (files.empty())
    {
        cerr << "No .txt input files found under '" << folder << "'.\n";
        return 1;
    }

    bool allCompleted = true;
    for (const auto &file : files)
        allCompleted = benchmarkFile(file.string(), output) && allCompleted;
    return allCompleted ? 0 : 1;
}
