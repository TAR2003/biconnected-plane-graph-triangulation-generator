#include <bits/stdc++.h>
#include <filesystem>
using namespace std;
namespace fs = std::filesystem;

#include "Edge.hpp"
#include "pairHash.hpp"
#include "biconnected.hpp"
#include "FaceTriangulation.hpp"
#include "triconnected.hpp"

// Structure to hold metrics for CSV reporting
struct FileMetrics
{
    string filename;
    size_t algoTotal;
    size_t bruteForceTotal;
    double ratio;
    long long totalChecks;
    long long successfulChecks;
    long long failedChecks;
    double checkSuccessRate;
    long long totalTraversals;
    long long successfulTraversals;
    long long invalidTraversals;
    double traversalSuccessRate;
    bool isContained;
};

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

    // Ensure inner edge pairs are sorted for accurate matching
    for (auto &t : triangulationsByAlgo)
    {
        sort(t.begin(), t.end());
    }
    for (auto &t : triangulationsByTriconnectedBruteForce)
    {
        sort(t.begin(), t.end());
    }

    sort(triangulationsByAlgo.begin(), triangulationsByAlgo.end());

    multiset<vector<pair<int, int>>> bruteForceSet(
        triangulationsByTriconnectedBruteForce.begin(),
        triangulationsByTriconnectedBruteForce.end());

    cout << "Total triangulations in biconnected component: " << triangulationsByAlgo.size() << endl;

    // 1. Process Algo triangulations
    for (const auto &triangulation : triangulationsByAlgo)
    {
        auto it = bruteForceSet.find(triangulation);
        if (it != bruteForceSet.end())
        {
            cout << GREEN;
            bruteForceSet.erase(it);
        }
        else
        {
            cout << YELLOW;
        }
        cout << RESET;
    }

    // 2. Process Remaining Brute Force triangulations
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
    const string &filename,
    bool enableFileOutput)
{
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

    // Evaluate containment status
    multiset<vector<pair<int, int>>> checkSet = bruteForceSet;
    for (const auto &triangulation : triangulationsByAlgo)
    {
        auto it = checkSet.find(triangulation);
        if (it != checkSet.end())
        {
            checkSet.erase(it);
        }
    }
    bool isFullyContained = checkSet.empty();

    // If file writing is toggled OFF, return early
    if (!enableFileOutput)
    {
        return isFullyContained;
    }

    ofstream outFile(filename);
    if (!outFile.is_open())
    {
        cerr << "Error: Could not open file " << filename << " for writing." << endl;
        return isFullyContained;
    }

    outFile << "Total triangulations in biconnected component: " << triangulationsByAlgo.size() << "\n\n";

    // 1. Process Algorithm Triangulations
    for (const auto &triangulation : triangulationsByAlgo)
    {
        auto it = bruteForceSet.find(triangulation);
        if (it != bruteForceSet.end())
        {
            outFile << "[MATCH] ";
            bruteForceSet.erase(it);
        }
        else
        {
            outFile << "[EXTRA] ";
        }

        for (const auto &chord : triangulation)
        {
            outFile << "(" << chord.first << ", " << chord.second << ") , ";
        }
        outFile << "\n";
    }

    // 2. Process Remaining Brute Force Triangulations
    if (!bruteForceSet.empty())
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

FileMetrics matchTwoAlgorithms(string filename, bool enableFileOutput)
{
    vector<vector<int>> faces = solve(filename);

    biconnected *bc = new biconnected(faces);
    bc->getAllTriangulations();
    bc->sortTriangulations();

    triconnected *tc = new triconnected(faces);
    tc->getAllTriangulations();
    tc->refineTriangulations();
    tc->removeDuplicated();

    string bareFilename = fs::path(filename).filename().string();
    string outFilePath = "output/" + bareFilename;

    if (enableFileOutput)
    {
        fs::create_directories("output");
    }

    bool isContained = compareAndOutput(bc->allTriangulations, tc->allTriangulations, outFilePath, enableFileOutput);

    size_t algoCount = bc->allTriangulations.size();
    size_t bruteForceCount = tc->allTriangulations.size();
    double ratio = (bruteForceCount > 0) ? static_cast<double>(algoCount) / bruteForceCount : 0.0;

    size_t successfulTraversals = algoCount;
    size_t totalTraversals = bc->invalidTraversals + algoCount;

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

    FileMetrics metrics{
        bareFilename,
        algoCount,
        bruteForceCount,
        ratio,
        bc->totalChecks,
        bc->successfulChecks,
        bc->totalChecks - bc->successfulChecks,
        checkSuccessPercentage,
        static_cast<long long>(totalTraversals),
        static_cast<long long>(successfulTraversals),
        static_cast<long long>(bc->invalidTraversals),
        traversalSuccessPercentage,
        isContained};

    delete bc;
    delete tc;

    return metrics;
}

void writeCSVReport(const string &reportPath, const vector<FileMetrics> &allMetrics)
{
    ofstream csvFile(reportPath);
    if (!csvFile.is_open())
    {
        cerr << "Error: Could not create CSV report file " << reportPath << endl;
        return;
    }

    // CSV Header
    csvFile << "Filename,Algo Total,Brute Force Total,Ratio (Algo/BF),"
            << "Total Checks,Successful Checks,Failed Checks,Check Success Rate (%),"
            << "Total Traversals,Successful Traversals,Invalid Traversals,Traversal Success Rate (%),"
            << "Status\n";

    for (const auto &m : allMetrics)
    {
        csvFile << m.filename << ","
                << m.algoTotal << ","
                << m.bruteForceTotal << ","
                << fixed << setprecision(4) << m.ratio << ","
                << m.totalChecks << ","
                << m.successfulChecks << ","
                << m.failedChecks << ","
                << fixed << setprecision(2) << m.checkSuccessRate << ","
                << m.totalTraversals << ","
                << m.successfulTraversals << ","
                << m.invalidTraversals << ","
                << fixed << setprecision(2) << m.traversalSuccessRate << ","
                << (m.isContained ? "MATCHED" : "MISMATCHED") << "\n";
    }

    csvFile.close();
    cout << "\n\033[1;34m[REPORT] Summary CSV report successfully written to: " << reportPath << "\033[0m" << endl;
}

int main()
{
    // ================= CONFIGURATION FLAGS =================
    bool ENABLE_FILE_OUTPUT = false; // Set to false to disable per-file detailed triangulation outputs
    string CSV_REPORT_FILENAME = "triangulation_search_report.csv";
    // =======================================================

    string folder = "input";
    vector<FileMetrics> allMetrics;

    if (!fs::exists(folder))
    {
        cerr << "Error: Folder '" << folder << "' does not exist." << endl;
        return 1;
    }

    // Use recursive_directory_iterator to traverse subdirectories
    for (const auto &entry : fs::recursive_directory_iterator(folder))
    {
        // Filter for regular files with .txt extension
        if (entry.is_regular_file() && entry.path().extension() == ".txt")
        {
            string filename = entry.path().string();
            cout << "\nProcessing: " << filename << endl;
            FileMetrics metrics = matchTwoAlgorithms(filename, ENABLE_FILE_OUTPUT);
            allMetrics.push_back(metrics);
        }
    }

    // Print summary results to console
    cout << "\n================ Final Summary ================" << endl;
    for (const auto &m : allMetrics)
    {
        if (m.isContained)
        {
            cout << "File: " << m.filename << " => \033[1;32mMatched\033[0m" << endl;
        }
        else
        {
            cout << "File: " << m.filename << " => \033[1;31mMismatched\033[0m" << endl;
        }
    }

    // Generate CSV Report
    writeCSVReport(CSV_REPORT_FILENAME, allMetrics);

    return 0;
}