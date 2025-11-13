#include <bits/stdc++.h>
using namespace std;

#include "Edge.hpp"
#include "pairHash.hpp"
#include "biconnected.hpp"
#include "FaceTriangulation.hpp"

#include <filesystem>
#include <chrono>
namespace fs = std::filesystem;

vector<vector<int>> input(const string &filename)
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

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string folder = "input";
    const int testRuns = 1; // repeat each test for averaging

    vector<tuple<string, uint64_t, double, double>> results;

    if (!fs::exists(folder) || !fs::is_directory(folder))
    {
        cerr << "Folder '" << folder << "' does not exist or is not a directory.\n";
        return 1;
    }

    cout << "Scanning folder: " << folder << "\n\n";
    cout << "Each test will run " << testRuns << " times, and results will be averaged.\n\n";

    for (const auto &entry : fs::directory_iterator(folder))
    {
        if (!entry.is_regular_file())
            continue;

        string filename = entry.path().string();
        cout << "Processing: " << filename << endl;

        vector<vector<int>> faces = input(filename);
        if (faces.empty())
        {
            cerr << "Warning: Skipping empty or invalid file: " << filename << endl;
            continue;
        }

        uint64_t totalTriang = 0;
        long double totalTimeSum = 0.0L;

        for (int run = 1; run <= testRuns; run++)
        {
            biconnected *bc = new biconnected(faces);
            using clock = std::chrono::steady_clock;
            auto start = clock::now();
            bc->getAllTriangulations();
            auto end = clock::now();

            auto dur_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            totalTimeSum += (long double)dur_ns / 1'000'000'000.0L;
            totalTriang = bc->totalTriangulations;

            delete bc;
        }

        double avgTime = (double)(totalTimeSum / testRuns);
        double avgTimeNs = avgTime * 1'000'000'000.0;
        double perTriangNs = (totalTriang > 0) ? avgTimeNs / totalTriang : 0.0;

        results.push_back({filename, totalTriang, avgTime, perTriangNs});
    }

    // ============= PRINT TABLE =============
    cout << "\n============================================================\n";
    cout << "\033[1m" // Bold header
         << left << setw(35) << "File"
         << right << setw(18) << "Total Triang"
         << setw(18) << "Avg Time (s)"
         << setw(22) << "Time/Trig (ns)"
         << "\033[0m\n";
    cout << "------------------------------------------------------------\n";

    int row = 0;
    for (const auto &[fname, total, avgSec, perNs] : results)
    {
        // Alternate row background color
        string bg = (row % 2 == 0) ? "\033[48;5;236m" : "\033[48;5;233m";
        string reset = "\033[0m";

        cout << bg
             << left << setw(35) << fname
             << right << setw(18) << total
             << setw(18) << fixed << setprecision(6) << avgSec
             << setw(22) << fixed << setprecision(3) << perNs
             << reset << "\n";

        row++;
    }

    cout << "============================================================\n";
    cout << "Note: Results averaged over " << testRuns << " runs per file.\n";

    return 0;
}
