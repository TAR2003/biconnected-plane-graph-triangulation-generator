#include <bits/stdc++.h>
#include <filesystem>
#include <chrono>
#include <unistd.h>
#include "GenerateBiconnectedTriangulations.hpp"
#include "GraphTriangulation.hpp"

namespace fs = std::filesystem;
using namespace std;
using namespace std::chrono;

struct InputGraph
{
    long long vertexCount;
    vector<vector<long long>> adjacency;
};

struct AlgorithmResult
{
    long long count = 0;
    double timeMs = 0.0;
    double peakMemMB = 0.0;
};

// Represents a unified interface for any triangulation algorithm
struct BenchmarkRunner
{
    string name;
    function<AlgorithmResult(InputGraph &, long long limit)> run;
};

// Returns Peak RAM (VmHWM) in Megabytes for Linux/WSL
double getPeakMemoryMB()
{
    ifstream statusFile("/proc/self/status");
    string line;
    while (getline(statusFile, line))
    {
        if (line.compare(0, 6, "VmHWM:") == 0)
        {
            size_t val = 0;
            stringstream ss(line.substr(6));
            ss >> val;
            return val / 1024.0; // kB to MB
        }
    }
    return 0.0;
}

InputGraph readInput(const string &filename)
{
    ifstream infile(filename);
    if (!infile.is_open())
        return {0, {}};
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
        for (auto &v : neighbors)
        {
            if (!(infile >> v))
                return {0, {}};
        }
    }
    return graph;
}

int main()
{
    string rootDir = "./input/Biconnected/9_general_biconnected/";
    string csvFile = "benchmark_results.csv";
    const long long LIMIT = 1000000;

    // Define all algorithms to benchmark in order
    // (Note: InputGraph is passed as non-const reference to accommodate non-const constructors)
    vector<BenchmarkRunner> algorithms = {
        {"Old_Biconnected",
         [](InputGraph &g, long long limit)
         {
             double memBefore = getPeakMemoryMB();
             auto start = high_resolution_clock::now();

             auto *gt = new GraphTriangulationBiconnectedPerformance(g.vertexCount, g.adjacency, limit);
             gt->getAllTriangulations();

             auto end = high_resolution_clock::now();
             long long count = gt->totalTriangulations;
             double timeMs = duration<double, milli>(end - start).count();
             double memUsed = max(0.0, getPeakMemoryMB() - memBefore);

             delete gt;
             return AlgorithmResult{count, timeMs, memUsed};
         }},
        {"WithoutVGS",
         [](InputGraph &g, long long limit)
         {
             double memBefore = getPeakMemoryMB();
             auto start = high_resolution_clock::now();

             auto *gt = new GenerateBiconnectedTriangulationsWithoutVGS(g.vertexCount, g.adjacency, limit);
             gt->generateAllTriangulations();

             auto end = high_resolution_clock::now();
             long long count = gt->totalTriangulations;
             double timeMs = duration<double, milli>(end - start).count();
             double memUsed = max(0.0, getPeakMemoryMB() - memBefore);

             delete gt;
             return AlgorithmResult{count, timeMs, memUsed};
         }},
        {"WithVGS",
         [](InputGraph &g, long long limit)
         {
             double memBefore = getPeakMemoryMB();
             auto start = high_resolution_clock::now();

             auto *gt = new GenerateBiconnectedTriangulationsWithVGS(g.vertexCount, g.adjacency, limit);
             gt->generateAllTriangulations();

             auto end = high_resolution_clock::now();
             long long count = gt->totalTriangulations;
             double timeMs = duration<double, milli>(end - start).count();
             double memUsed = max(0.0, getPeakMemoryMB() - memBefore);

             delete gt;
             return AlgorithmResult{count, timeMs, memUsed};
         }}
        // To add future algorithms, simply add a new entry here:
        // { "AlgoName", [](InputGraph& g, long long limit) { ... } }
    };

    // Check existing CSV to skip processed files
    unordered_set<string> processedFiles;
    bool csvExists = fs::exists(csvFile);
    if (csvExists)
    {
        ifstream csvIn(csvFile);
        string line;
        getline(csvIn, line); // Skip header
        while (getline(csvIn, line))
        {
            stringstream ss(line);
            string fname;
            if (getline(ss, fname, ','))
            {
                processedFiles.insert(fname);
            }
        }
    }

    // Open CSV in append mode
    ofstream csvOut(csvFile, ios::app);
    if (!csvExists)
    {
        csvOut << "TestFile";
        for (const auto &algo : algorithms)
        {
            csvOut << "," << algo.name << "_Count"
                   << "," << algo.name << "_TimeMS"
                   << "," << algo.name << "_MemMB";
        }
        csvOut << ",Status\n";
    }

    // Collect and sort test files
    vector<string> testFiles;
    for (const auto &entry : fs::recursive_directory_iterator(rootDir))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".txt")
        {
            testFiles.push_back(entry.path().string());
        }
    }
    sort(testFiles.begin(), testFiles.end());

    // Print Header
    cout << left << setw(35) << "Test File";
    for (const auto &algo : algorithms)
    {
        cout << setw(14) << ("Count(" + algo.name + ")")
             << setw(12) << (algo.name + "(ms)")
             << setw(12) << (algo.name + "(MB)");
    }
    cout << "Status" << endl;
    cout << string(35 + algorithms.size() * 38 + 10, '-') << endl;

    // Run Benchmarks
    for (const auto &filePath : testFiles)
    {
        string shortName = fs::relative(filePath, rootDir).string();

        if (processedFiles.count(shortName))
        {
            cout << left << setw(35) << shortName << "[SKIPPED - Already in CSV]" << endl;
            continue;
        }

        InputGraph graph = readInput(filePath);
        if (graph.vertexCount <= 0)
            continue;

        vector<AlgorithmResult> results;
        bool allMatch = true;

        for (size_t i = 0; i < algorithms.size(); ++i)
        {
            AlgorithmResult res = algorithms[i].run(graph, LIMIT);
            results.push_back(res);

            if (i > 0 && res.count != results[0].count)
            {
                allMatch = false;
            }
        }

        string status = allMatch ? "OK" : "MISMATCH";

        // Terminal Output
        cout << left << setw(35) << (shortName.length() > 33 ? shortName.substr(0, 30) + "..." : shortName);
        for (const auto &res : results)
        {
            cout << setw(14) << res.count
                 << setw(12) << fixed << setprecision(1) << res.timeMs
                 << setw(12) << fixed << setprecision(2) << res.peakMemMB;
        }
        cout << "[" << status << "]" << endl;

        // CSV Output
        csvOut << shortName;
        for (const auto &res : results)
        {
            csvOut << "," << res.count
                   << "," << res.timeMs
                   << "," << res.peakMemMB;
        }
        csvOut << "," << status << "\n";
        csvOut.flush();
    }

    return 0;
}