#include <bits/stdc++.h>
using namespace std;
#include "Edge.hpp"
#include "pairHash.hpp"
#include "biconnected.hpp"
#include "FaceTriangulation.hpp"
#include "triconnected.hpp"

vector<vector<int>> solve(string filename)
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

bool matchPairs(const pair<int, int> &p1, const pair<int, int> &p2)
{
    return (p1.first == p2.first && p1.second == p2.second);
}

bool matchTriangulations(const vector<pair<int, int>> &t1, const vector<pair<int, int>> &t2)
{
    if (t1.size() != t2.size())
        return false;
    for (size_t i = 0; i < t1.size(); i++)
    {
        if (!matchPairs(t1[i], t2[i]))
            return false;
    }
    return true;
}

void compareAndPrintTriangulations(
    vector<vector<pair<int, int>>> &triangulationsByAlgo,
    vector<vector<pair<int, int>>> &triangulationsByTriconnectedBruteForce)
{
    // ANSI color codes for terminal output
    const string GREEN = "\033[32m";
    const string YELLOW = "\033[33m";
    const string RED = "\033[31m";
    const string RESET = "\033[0m";

    // Ensure the inner edge pairs are strictly sorted to allow exact equality matching
    for (auto &t : triangulationsByAlgo)
    {
        sort(t.begin(), t.end());
    }
    for (auto &t : triangulationsByTriconnectedBruteForce)
    {
        sort(t.begin(), t.end());
    }

    // Sort outer vectors to display them in a clean, organized order
    sort(triangulationsByAlgo.begin(), triangulationsByAlgo.end());

    // Use a multiset to handle exact duplicate counts (multiplicities)
    multiset<vector<pair<int, int>>> bruteForceSet(
        triangulationsByTriconnectedBruteForce.begin(),
        triangulationsByTriconnectedBruteForce.end());

    cout << "Total triangulations in biconnected component: " << triangulationsByAlgo.size() << endl;

    // 1. Print all Algo triangulations (Green for match, Yellow for extra)
    for (const auto &triangulation : triangulationsByAlgo)
    {
        auto it = bruteForceSet.find(triangulation);
        if (it != bruteForceSet.end())
        {
            cout << GREEN;           // Matched in brute force
            bruteForceSet.erase(it); // Erase exactly one instance to handle duplicates properly
        }
        else
        {
            cout << YELLOW; // Extra in Algo, not in brute force
        }

        // for (const auto &chord : triangulation)
        // {
        //     cout << "(" << chord.first << ", " << chord.second << ") , ";
        // }
        cout << RESET; // Reset color at the end of the line
    }

    // 2. Print any remaining Brute Force triangulations NOT found in Algo (Red)
    if (!bruteForceSet.empty())
    {
        cout << "\nMissing triangulations (In Brute Force, but NOT in Algo):" << endl;
        for (const auto &triangulation : bruteForceSet)
        {
            cout << RED;
            for (const auto &chord : triangulation)
            {
                cout << "(" << chord.first << ", " << chord.second << ") , ";
            }
            cout << RESET;
        }
    }
    else
    {
        cout << "\nAll brute force triangulations are successfully contained in the Algorithm!" << endl;
    }
}

bool compareAndOutput(
    vector<vector<pair<int, int>>> &triangulationsByAlgo,
    vector<vector<pair<int, int>>> &triangulationsByTriconnectedBruteForce,
    const string &filename)
{
    ofstream outFile(filename);
    if (!outFile.is_open())
    {
        cerr << "Error: Could not open file " << filename << " for writing." << endl;
        return false;
    }

    // Standardize edge orientations and internal order
    for (auto &t : triangulationsByAlgo)
    {
        for (auto &edge : t)
        {
            if (edge.first > edge.second)
                swap(edge.first, edge.second);
        }
        sort(t.begin(), t.end());
    }

    for (auto &t : triangulationsByTriconnectedBruteForce)
    {
        for (auto &edge : t)
        {
            if (edge.first > edge.second)
                swap(edge.first, edge.second);
        }
        sort(t.begin(), t.end());
    }

    sort(triangulationsByAlgo.begin(), triangulationsByAlgo.end());

    multiset<vector<pair<int, int>>> bruteForceSet(
        triangulationsByTriconnectedBruteForce.begin(),
        triangulationsByTriconnectedBruteForce.end());

    outFile << "Total triangulations in biconnected component: " << triangulationsByAlgo.size() << "\n\n";

    // 1. Process Algorithm Triangulations
    for (const auto &triangulation : triangulationsByAlgo)
    {
        auto it = bruteForceSet.find(triangulation);
        if (it != bruteForceSet.end())
        {
            outFile << "[MATCH] "; // Matched in brute force
            bruteForceSet.erase(it);
        }
        else
        {
            outFile << "[EXTRA] "; // Extra in Algo (not present in brute force)
        }

        for (const auto &chord : triangulation)
        {
            outFile << "(" << chord.first << ", " << chord.second << ") , ";
        }
        outFile << "\n";
    }

    // 2. Process Remaining Brute Force Triangulations
    bool isFullyContained = bruteForceSet.empty();

    if (!isFullyContained)
    {
        outFile << "\n[MISSING] Triangulations in Brute Force but NOT in Algo:\n";
        for (const auto &triangulation : bruteForceSet)
        {
            outFile << "[MISSING] ";
            for (const auto &chord : triangulation)
            {
                outFile << "(" << chord.first << ", " << chord.second << ") , ";
            }
            outFile << "\n";
        }
    }
    else
    {
        outFile << "\n[STATUS] All brute force triangulations are successfully contained in the Algorithm!\n";
    }

    outFile.close();
    return isFullyContained;
}

bool matchTwoAlgorithms(string filename)
{
    vector<vector<int>> faces = solve(filename);

    biconnected *bc = new biconnected(faces);
    bc->getAllTriangulations();
    bc->sortTriangulations();

    triconnected *tc = new triconnected(faces);
    tc->getAllTriangulations();
    tc->refineTriangulations();
    tc->removeDuplicated();

    // Ensure the output directory exists
    filesystem::create_directories("output");

    // Extract bare filename (e.g., "input/actual_name.txt" -> "actual_name.txt")
    string bareFilename = filesystem::path(filename).filename().string();

    // Save directly to output/<actual_name.txt>
    string outFilePath = "output/" + bareFilename;

    // Call compareAndOutput to write results to file
    bool isContained = compareAndOutput(bc->allTriangulations, tc->allTriangulations, outFilePath);

    size_t algoCount = bc->allTriangulations.size();
    size_t bruteForceCount = tc->allTriangulations.size();

    // Calculate ratio safely
    double ratio = (bruteForceCount > 0) ? static_cast<double>(algoCount) / bruteForceCount : 0.0;

    // Traversal statistics
    size_t successfulTraversals = algoCount; // Successful triangulations count
    size_t totalTraversals = bc->invalidTraversals + algoCount;

    // Percentage calculations (with division-by-zero protection)
    double checkSuccessPercentage = (bc->totalChecks > 0)
                                        ? (static_cast<double>(bc->successfulChecks) / bc->totalChecks) * 100.0
                                        : 0.0;

    double traversalSuccessPercentage = (totalTraversals > 0)
                                            ? (static_cast<double>(successfulTraversals) / totalTraversals) * 100.0
                                            : 0.0;

    // Terminal colors
    const string GREEN = "\033[1;32m";
    const string RED = "\033[1;31m";
    const string RESET = "\033[0m";

    // 1. Print Checks & Traversals Metrics
    cout << fixed << setprecision(2);
    cout << "\n================ Search Metrics ================" << endl;
    cout << "Total Checks: " << bc->totalChecks << endl;
    cout << "Successful Checks: " << bc->successfulChecks << endl;
    cout << "Failed Checks: " << (bc->totalChecks - bc->successfulChecks) << endl;
    cout << "Check Success Rate: " << checkSuccessPercentage << "%" << endl;
    cout << "------------------------------------------------" << endl;
    cout << "Total Traversals: " << totalTraversals << endl;
    cout << "Successful Traversals: " << successfulTraversals << endl;
    cout << "Invalid Traversals: " << bc->invalidTraversals << endl;
    cout << "Traversal Success Rate: " << traversalSuccessPercentage << "%" << endl;
    cout << "================================================" << endl;

    // 2. Print Algorithm Match/Containment Results
    if (isContained)
    {
        cout << GREEN;
        cout << "[SUCCESS] File: " << bareFilename << endl;
        cout << "Algo Total: " << algoCount << " | Brute Force Total: " << bruteForceCount << endl;
        cout << "Ratio (Algo / Brute Force): " << ratio << endl;
        cout << "All brute force triangulations are contained in the algorithm output." << RESET << endl;
    }
    else
    {
        cout << RED;
        cout << "[FAILED] File: " << bareFilename << endl;
        cout << "Algo Total: " << algoCount << " | Brute Force Total: " << bruteForceCount << endl;
        cout << "Ratio (Algo / Brute Force): " << ratio << endl;
        cout << "Some brute force triangulations are MISSING from the algorithm output." << RESET << endl;
    }

    // Clean up heap allocations
    delete bc;
    delete tc;

    return isContained;
}

#include <filesystem>
namespace fs = std::filesystem;

int main()
{
    string folder = "input";
    vector<pair<string, bool>> files;
    // Loop through all files in the folder
    for (const auto &entry : fs::directory_iterator(folder))
    {
        if (entry.is_regular_file())
        {
            string filename = entry.path().string();
            cout << "Processing: " << filename << endl;
            bool result = matchTwoAlgorithms(filename);
            files.push_back({filename, result});
        }
    }
    for (int i = 0; i < files.size(); i++)
    {
        if (files[i].second)
        {
            cout << "File: " << " => \033[1;32mMatched: " << files[i].first << " \033[0m" << endl;
        }
        else
        {
            cout << "File: " << " => \033[1;31mMismatched: " << files[i].first << " \033[0m" << endl;
        }
    }

    return 0;
}