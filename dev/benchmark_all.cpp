#include <bits/stdc++.h>
#include <filesystem>
#include <chrono>
#include <unistd.h>
#include "PlanarTriangulationGenerator.hpp"
// #include "GenerateTriangulations.hpp"
#include "GraphTriangulation.hpp"

namespace fs = std::filesystem;
using namespace std;
using namespace std::chrono;

struct InputGraph
{
    long long vertexCount;
    vector<vector<long long>> adjacency;
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
    const long long LIMIT = 100000;

    // Read already processed test cases from CSV to skip them
    unordered_set<string> processedFiles;
    bool csvExists = fs::exists(csvFile);
    if (csvExists)
    {
        ifstream csvIn(csvFile);
        string line;
        getline(csvIn, line); // Skip CSV header
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
        csvOut << "TestFile,OldCount,NewCount,OldTimeMS,NewTimeMS,OldPeakMemMB,NewPeakMemMB,Status\n";
    }

    // Collect all input files
    vector<string> testFiles;
    for (const auto &entry : fs::recursive_directory_iterator(rootDir))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".txt")
        {
            testFiles.push_back(entry.path().string());
        }
    }
    sort(testFiles.begin(), testFiles.end());

    cout << left << setw(45) << "Test File"
         << setw(10) << "Old Count"
         << setw(10) << "New Count"
         << setw(10) << "Old(ms)"
         << setw(10) << "New(ms)"
         << setw(12) << "OldMem(MB)"
         << setw(12) << "NewMem(MB)"
         << "Status" << endl;
    cout << string(110, '-') << endl;

    for (const auto &filePath : testFiles)
    {
        string shortName = fs::relative(filePath, rootDir).string();

        // Skip already benchmarked runs
        if (processedFiles.count(shortName))
        {
            cout << left << setw(45) << shortName << "[SKIPPED - Already in CSV]" << endl;
            continue;
        }

        InputGraph graph = readInput(filePath);
        if (graph.vertexCount <= 0)
            continue;

        // --- Run Old Algorithm ---
        double memBeforeOld = getPeakMemoryMB();
        auto start1 = high_resolution_clock::now();
        GraphTriangulation *gtOld = new GraphTriangulationBiconnectedPerformance(graph.vertexCount, graph.adjacency, LIMIT);
        gtOld->getAllTriangulations();
        auto end1 = high_resolution_clock::now();
        long long oldCount = gtOld->totalTriangulations;
        double oldTime = duration<double, milli>(end1 - start1).count();
        double oldMem = getPeakMemoryMB() - memBeforeOld;
        delete gtOld;

        // --- Run New Algorithm ---
        double memBeforeNew = getPeakMemoryMB();
        auto start2 = high_resolution_clock::now();
        GenerateTriangulations *gtNew = new GenerateTriangulations(graph.vertexCount, graph.adjacency, LIMIT);
        gtNew->generateAllTriangulations();
        auto end2 = high_resolution_clock::now();
        long long newCount = gtNew->totalTriangulations;
        double newTime = duration<double, milli>(end2 - start2).count();
        double newMem = getPeakMemoryMB() - memBeforeNew;
        delete gtNew;

        bool match = (oldCount == newCount);
        string status = match ? "OK" : "MISMATCH";

        // Print to Terminal
        cout << left << setw(45) << (shortName.length() > 43 ? shortName.substr(0, 40) + "..." : shortName)
             << setw(10) << oldCount
             << setw(10) << newCount
             << setw(10) << fixed << setprecision(1) << oldTime
             << setw(10) << fixed << setprecision(1) << newTime
             << setw(12) << fixed << setprecision(2) << max(0.0, oldMem)
             << setw(12) << fixed << setprecision(2) << max(0.0, newMem)
             << "[" << status << "]" << endl;

        // Save immediately to CSV
        csvOut << shortName << ","
               << oldCount << ","
               << newCount << ","
               << oldTime << ","
               << newTime << ","
               << max(0.0, oldMem) << ","
               << max(0.0, newMem) << ","
               << status << "\n";
        csvOut.flush();
    }

    return 0;
}