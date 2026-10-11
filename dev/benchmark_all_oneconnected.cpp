#include <bits/stdc++.h>
#include <filesystem>
#include <chrono>
#include "GenerateOneconnectedTriangulations.hpp"
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
    long long totalChecks = 0;
    long long unsuccessfulChecks = 0;
    long long totalTraversals = 0;
    long long unsuccessfulTraversals = 0;
};

struct BenchmarkRunner
{
    string name;
    function<AlgorithmResult(InputGraph &, long long limit)> run;
};

// Returns peak resident memory (VmHWM) in megabytes for Linux/WSL.
double getPeakMemoryMB()
{
    ifstream statusFile("/proc/self/status");
    string line;
    while (getline(statusFile, line))
    {
        if (line.compare(0, 6, "VmHWM:") == 0)
        {
            size_t value = 0;
            stringstream ss(line.substr(6));
            ss >> value;
            return value / 1024.0;
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
        for (auto &vertex : neighbors)
        {
            if (!(infile >> vertex))
                return {0, {}};
        }
    }
    return graph;
}

int main()
{
    const string rootDir = "./input/Oneconnected/";
    const string csvFile = "benchmark_results_oneconnected.csv";
    const long long LIMIT = 1000000;

    vector<BenchmarkRunner> algorithms = {
        {"GraphTriangulationOneconnectedPerformance",
         [](InputGraph &graph, long long limit)
         {
             double memBefore = getPeakMemoryMB();
             auto start = high_resolution_clock::now();

             auto *triangulation = new GraphTriangulationOneconnectedPerformance(
                 graph.vertexCount, graph.adjacency, limit);
             triangulation->getAllTriangulations();

             auto end = high_resolution_clock::now();
             long long count = triangulation->totalTriangulations;
             double timeMs = duration<double, milli>(end - start).count();
             double memUsed = max(0.0, getPeakMemoryMB() - memBefore);
             long long totalChecks = triangulation->totalChecks;
             long long unsuccessfulChecks = totalChecks - triangulation->successfulChecks;
             long long unsuccessfulTraversals = triangulation->invalidTraversals;
             long long totalTraversals = count + unsuccessfulTraversals;

             delete triangulation;
             return AlgorithmResult{count, timeMs, memUsed, totalChecks,
                                    unsuccessfulChecks, totalTraversals,
                                    unsuccessfulTraversals};
         }},
        {"GenerateOneconnectedTriangulations",
         [](InputGraph &graph, long long limit)
         {
             double memBefore = getPeakMemoryMB();
             auto start = high_resolution_clock::now();

             auto *generator = new GenerateOneconnectedTriangulations(
                 graph.vertexCount, graph.adjacency, limit);
             generator->generateAllTriangulations();

             auto end = high_resolution_clock::now();
             long long count = generator->totalTriangulations;
             double timeMs = duration<double, milli>(end - start).count();
             double memUsed = max(0.0, getPeakMemoryMB() - memBefore);
             long long totalChecks = generator->totalChecks;
             long long unsuccessfulChecks = generator->unsuccessfulChecks;
             long long totalTraversals = generator->totalTraversals;
             long long unsuccessfulTraversals = generator->unsuccessfulTraversals;

             delete generator;
             return AlgorithmResult{count, timeMs, memUsed, totalChecks,
                                    unsuccessfulChecks, totalTraversals,
                                    unsuccessfulTraversals};
         }}};

    unordered_set<string> processedFiles;
    bool csvExists = fs::exists(csvFile);
    if (csvExists)
    {
        ifstream csvIn(csvFile);
        string line;
        getline(csvIn, line);
        while (getline(csvIn, line))
        {
            stringstream ss(line);
            string filename;
            if (getline(ss, filename, ','))
                processedFiles.insert(filename);
        }
    }

    ofstream csvOut(csvFile, ios::app);
    if (!csvOut)
    {
        cerr << "Unable to open benchmark output file: " << csvFile << endl;
        return 1;
    }
    if (!csvExists)
    {
        csvOut << "TestFile";
        for (const auto &algorithm : algorithms)
        {
            csvOut << "," << algorithm.name << "_Count"
                   << "," << algorithm.name << "_TimeMS"
                   << "," << algorithm.name << "_MemMB"
                   << "," << algorithm.name << "_TotalChecks"
                   << "," << algorithm.name << "_UnsuccessfulChecks"
                   << "," << algorithm.name << "_TotalTraversals"
                   << "," << algorithm.name << "_UnsuccessfulTraversals";
        }
        csvOut << ",Status\n";
    }

    if (!fs::exists(rootDir))
    {
        cerr << "Input directory does not exist: " << rootDir << endl;
        return 1;
    }

    vector<string> testFiles;
    for (const auto &entry : fs::recursive_directory_iterator(rootDir))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".txt")
            testFiles.push_back(entry.path().string());
    }
    sort(testFiles.begin(), testFiles.end());

    cout << left << setw(35) << "Test File";
    for (const auto &algorithm : algorithms)
        cout << setw(98) << algorithm.name;
    cout << '\n'
         << left << setw(35) << "";
    for (size_t i = 0; i < algorithms.size(); ++i)
    {
        cout << setw(14) << "Count"
             << setw(12) << "Time(ms)"
             << setw(12) << "Mem(MB)"
             << setw(14) << "Total checks"
             << setw(14) << "Failed checks"
             << setw(14) << "Traversals"
             << setw(18) << "Failed traversals";
    }
    cout << "Status" << endl;
    cout << string(35 + algorithms.size() * 98 + 10, '-') << endl;

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
        {
            cerr << "Skipping invalid input file: " << filePath << endl;
            continue;
        }

        vector<AlgorithmResult> results;
        bool allMatch = true;
        for (size_t i = 0; i < algorithms.size(); ++i)
        {
            AlgorithmResult result = algorithms[i].run(graph, LIMIT);
            results.push_back(result);
            if (i > 0 && result.count != results[0].count)
                allMatch = false;
        }

        string status = allMatch ? "OK" : "MISMATCH";
        cout << left << setw(35) << (shortName.length() > 33 ? shortName.substr(0, 30) + "..." : shortName);
        for (const auto &result : results)
        {
            cout << setw(14) << result.count
                 << setw(12) << fixed << setprecision(1) << result.timeMs
                 << setw(12) << fixed << setprecision(2) << result.peakMemMB
                 << setw(14) << result.totalChecks
                 << setw(14) << result.unsuccessfulChecks
                 << setw(14) << result.totalTraversals
                 << setw(14) << result.unsuccessfulTraversals;
        }
        cout << "[" << status << "]" << endl;

        csvOut << shortName;
        for (const auto &result : results)
        {
            csvOut << "," << result.count
                   << "," << result.timeMs
                   << "," << result.peakMemMB
                   << "," << result.totalChecks
                   << "," << result.unsuccessfulChecks
                   << "," << result.totalTraversals
                   << "," << result.unsuccessfulTraversals;
        }
        csvOut << "," << status << "\n";
        csvOut.flush();
    }
    return 0;
}
