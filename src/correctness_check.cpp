// =======================================================================
// Correctness checker: runs your fast algorithm (Oneconnected or
// Biconnected) against the Triconnected brute-force ground truth on a
// folder of input files, and reports whether every brute-force
// triangulation is contained in the fast algorithm's output.
//
// This is a direct generalization of your correctnessCheckOneconnected.cpp
// -- same comparison logic, verified to produce identical results (see
// the build notes at the bottom of this file) -- with two changes:
//   1. --algorithm lets you pick Oneconnected OR Biconnected at the CLI,
//      instead of hardcoding Oneconnected.
//   2. All the CONFIGURATION FLAGS at the top of main() become CLI args.
//
// USAGE EXAMPLES: see the "COMMAND LINE ARGUMENTS" block below main().
// =======================================================================
#include <bits/stdc++.h>
#include <filesystem>
using namespace std;
namespace fs = std::filesystem;

#include "Edge.hpp"
#include "PairHash.hpp"
#include "GraphTriangulation.hpp"
#include "GraphTriangulationTriconnected.hpp"

namespace {

enum class Algorithm { Oneconnected, Biconnected };

struct Options {
    Algorithm algorithm = Algorithm::Oneconnected;
    fs::path input_folder = "inputs";
    fs::path csv_report_path = "triangulation_search_report.csv";
    fs::path detail_output_dir = "output";
    bool enable_detail_output = false;
    string case_filter;
    bool quiet = false; // suppress the big per-file metrics block, keep summary only
};

Options options;

struct FileMetrics {
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

vector<vector<long long>> readFaces(const string &filename) {
    ifstream infile(filename);
    if (!infile.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        return {};
    }
    vector<vector<long long>> faces;
    long long faceno;
    infile >> faceno;
    for (long long i = 0; i < faceno; i++) {
        long long vertices;
        infile >> vertices;
        vector<long long> face;
        for (long long j = 0; j < vertices; j++) {
            long long vertex;
            infile >> vertex;
            face.push_back(vertex);
        }
        faces.push_back(face);
    }
    return faces;
}

// Builds the fast-algorithm GraphTriangulation instance for whichever
// algorithm was selected on the command line. Always the "*Correctness*"
// variant, since correctness checking requires actually storing and
// comparing every triangulation (the "*Performance*" variants discard
// results as they go and would have nothing to compare here).
unique_ptr<GraphTriangulation> makeFastAlgorithm(vector<vector<long long>> &faces) {
    if (options.algorithm == Algorithm::Oneconnected) {
        return make_unique<GraphTriangulationOneconnectedCorrectness>(faces);
    }
    return make_unique<GraphTriangulationBiconnectedCorrectness>(faces);
}

string algorithmName() {
    return options.algorithm == Algorithm::Oneconnected ? "oneconnected" : "biconnected";
}

bool compareAndOutput(
    vector<vector<pair<long long, long long>>> &triangulationsByAlgo,
    vector<vector<pair<long long, long long>>> &triangulationsByTriconnectedBruteForce,
    const string &filename,
    bool enableFileOutput) {
    for (auto &t : triangulationsByAlgo) {
        for (auto &edge : t) {
            if (edge.first > edge.second) swap(edge.first, edge.second);
        }
        sort(t.begin(), t.end());
    }
    for (auto &t : triangulationsByTriconnectedBruteForce) {
        for (auto &edge : t) {
            if (edge.first > edge.second) swap(edge.first, edge.second);
        }
        sort(t.begin(), t.end());
    }

    sort(triangulationsByAlgo.begin(), triangulationsByAlgo.end());

    multiset<vector<pair<long long, long long>>> bruteForceSet(
        triangulationsByTriconnectedBruteForce.begin(),
        triangulationsByTriconnectedBruteForce.end());

    multiset<vector<pair<long long, long long>>> checkSet = bruteForceSet;
    for (const auto &triangulation : triangulationsByAlgo) {
        auto it = checkSet.find(triangulation);
        if (it != checkSet.end()) checkSet.erase(it);
    }
    bool isFullyContained = checkSet.empty();

    if (!enableFileOutput) return isFullyContained;

    ofstream outFile(filename);
    if (!outFile.is_open()) {
        cerr << "Error: Could not open file " << filename << " for writing." << endl;
        return isFullyContained;
    }

    outFile << "Total triangulations found: " << triangulationsByAlgo.size() << "\n\n";

    for (const auto &triangulation : triangulationsByAlgo) {
        auto it = bruteForceSet.find(triangulation);
        if (it != bruteForceSet.end()) {
            outFile << "[MATCH] ";
            bruteForceSet.erase(it);
        } else {
            outFile << "[EXTRA] ";
        }
        for (const auto &chord : triangulation) {
            outFile << "(" << chord.first << ", " << chord.second << ") , ";
        }
        outFile << "\n";
    }

    if (!bruteForceSet.empty()) {
        outFile << "\n[MISSING] Triangulations in Brute Force but NOT in Algo:\n";
        for (const auto &triangulation : bruteForceSet) {
            outFile << "[MISSING] ";
            for (const auto &chord : triangulation) {
                outFile << "(" << chord.first << ", " << chord.second << ") , ";
            }
            outFile << "\n";
        }
    } else {
        outFile << "\n[STATUS] All brute force triangulations are successfully contained in the Algorithm!\n";
    }

    outFile.close();
    return isFullyContained;
}

FileMetrics checkOneFile(const string &filename) {
    vector<vector<long long>> faces = readFaces(filename);

    auto gt = makeFastAlgorithm(faces);
    if (!options.quiet) {
        cout << "Starting triangulation search (" << algorithmName() << ")..." << endl;
    }
    gt->getAllTriangulations();
    gt->sortTriangulations();
    if (!options.quiet) {
        cout << "Total triangulations found: " << gt->allTriangulations.size() << endl;
    }

    auto tc = make_unique<GraphTriangulationTriconnected>(faces);
    tc->getAllTriangulations();
    tc->refineTriangulations();
    tc->removeDuplicated();
    if (!options.quiet) {
        cout << "Total triangulations in brute-force ground truth: " << tc->allTriangulations.size() << endl;
    }

    string bareFilename = fs::path(filename).filename().string();
    string outFilePath = (options.detail_output_dir / bareFilename).string();

    if (options.enable_detail_output) {
        fs::create_directories(options.detail_output_dir);
    }

    bool isContained = compareAndOutput(gt->allTriangulations, tc->allTriangulations,
                                         outFilePath, options.enable_detail_output);

    size_t algoCount = gt->allTriangulations.size();
    size_t bruteForceCount = tc->allTriangulations.size();
    double ratio = (bruteForceCount > 0) ? static_cast<double>(algoCount) / bruteForceCount : 0.0;

    size_t successfulTraversals = algoCount;
    size_t totalTraversals = gt->invalidTraversals + algoCount;

    double checkSuccessPercentage = (gt->totalChecks > 0)
        ? (static_cast<double>(gt->successfulChecks) / gt->totalChecks) * 100.0 : 0.0;
    double traversalSuccessPercentage = (totalTraversals > 0)
        ? (static_cast<double>(successfulTraversals) / totalTraversals) * 100.0 : 0.0;

    if (!options.quiet) {
        const string GREEN = "\033[1;32m", RED = "\033[1;31m", RESET = "\033[0m";
        cout << fixed << setprecision(2);
        cout << "\n================ Search Metrics ================" << endl;
        cout << "Total Checks: " << gt->totalChecks << endl;
        cout << "Successful Checks: " << gt->successfulChecks << endl;
        cout << "Failed Checks: " << (gt->totalChecks - gt->successfulChecks) << endl;
        cout << "Check Success Rate: " << checkSuccessPercentage << "%" << endl;
        cout << "------------------------------------------------" << endl;
        cout << "Total Traversals: " << totalTraversals << endl;
        cout << "Successful Traversals: " << successfulTraversals << endl;
        cout << "Invalid Traversals: " << gt->invalidTraversals << endl;
        cout << "Traversal Success Rate: " << traversalSuccessPercentage << "%" << endl;
        cout << "================================================" << endl;
        cout << (isContained ? GREEN : RED);
        cout << (isContained ? "[SUCCESS] " : "[FAILED] ") << "File: " << bareFilename << endl;
        cout << "Algo Total: " << algoCount << " | Brute Force Total: " << bruteForceCount << endl;
        cout << "Ratio (Algo / Brute Force): " << ratio << RESET << endl;
    }

    return FileMetrics{
        bareFilename, algoCount, bruteForceCount, ratio,
        gt->totalChecks, gt->successfulChecks, gt->totalChecks - gt->successfulChecks,
        checkSuccessPercentage, static_cast<long long>(totalTraversals),
        static_cast<long long>(successfulTraversals),
        static_cast<long long>(gt->invalidTraversals), traversalSuccessPercentage, isContained};
}

void writeCSVReport(const vector<FileMetrics> &allMetrics) {
    ofstream csvFile(options.csv_report_path);
    if (!csvFile.is_open()) {
        cerr << "Error: Could not create CSV report file " << options.csv_report_path << endl;
        return;
    }
    csvFile << "Filename,Algorithm,Algo Total,Brute Force Total,Ratio (Algo/BF),"
            << "Total Checks,Successful Checks,Failed Checks,Check Success Rate (%),"
            << "Total Traversals,Successful Traversals,Invalid Traversals,Traversal Success Rate (%),"
            << "Status\n";
    for (const auto &m : allMetrics) {
        csvFile << m.filename << "," << algorithmName() << ","
                << m.algoTotal << "," << m.bruteForceTotal << ","
                << fixed << setprecision(4) << m.ratio << ","
                << m.totalChecks << "," << m.successfulChecks << "," << m.failedChecks << ","
                << fixed << setprecision(2) << m.checkSuccessRate << ","
                << m.totalTraversals << "," << m.successfulTraversals << "," << m.invalidTraversals << ","
                << fixed << setprecision(2) << m.traversalSuccessRate << ","
                << (m.isContained ? "MATCHED" : "MISMATCHED") << "\n";
    }
    csvFile.close();
    cout << "\n\033[1;34m[REPORT] Summary CSV report written to: " << options.csv_report_path.string() << "\033[0m" << endl;
}

string optionValue(int *index, int argc, char **argv, const string &opt) {
    string arg = argv[*index];
    if (arg.rfind(opt + "=", 0) == 0) return arg.substr(opt.size() + 1);
    if (arg == opt && *index + 1 < argc) return argv[++*index];
    return {};
}

void parseArgs(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            cout <<
R"(correctness_check -- validate a fast algorithm against brute-force ground truth

  --algorithm=oneconnected|biconnected   which fast algorithm to check (default: oneconnected)
  --input-folder=PATH                    folder of .txt input files, searched recursively (default: inputs)
  --case-filter=SUBSTR                   only process files whose path contains SUBSTR
  --csv-report=PATH                      where to write the summary CSV (default: triangulation_search_report.csv)
  --detail-output                        also write full per-file triangulation listings (see --detail-output-dir)
  --detail-output-dir=PATH               where per-file detail .txt files go if --detail-output is set (default: output)
  --quiet                                suppress the per-file metrics block, print only PASS/FAIL summary
  -h, --help                             show this message
)";
            exit(0);
        } else if (arg == "--algorithm" || arg.rfind("--algorithm=", 0) == 0) {
            string v = optionValue(&i, argc, argv, "--algorithm");
            if (v == "oneconnected") options.algorithm = Algorithm::Oneconnected;
            else if (v == "biconnected") options.algorithm = Algorithm::Biconnected;
            else { cerr << "--algorithm must be oneconnected or biconnected\n"; exit(1); }
        } else if (arg == "--input-folder" || arg.rfind("--input-folder=", 0) == 0) {
            options.input_folder = optionValue(&i, argc, argv, "--input-folder");
        } else if (arg == "--case-filter" || arg.rfind("--case-filter=", 0) == 0) {
            options.case_filter = optionValue(&i, argc, argv, "--case-filter");
        } else if (arg == "--csv-report" || arg.rfind("--csv-report=", 0) == 0) {
            options.csv_report_path = optionValue(&i, argc, argv, "--csv-report");
        } else if (arg == "--detail-output") {
            options.enable_detail_output = true;
        } else if (arg == "--detail-output-dir" || arg.rfind("--detail-output-dir=", 0) == 0) {
            options.detail_output_dir = optionValue(&i, argc, argv, "--detail-output-dir");
        } else if (arg == "--quiet") {
            options.quiet = true;
        } else {
            cerr << "Unrecognized argument: " << arg << " (use --help)\n";
            exit(1);
        }
    }
}

} // namespace

int main(int argc, char **argv) {
    parseArgs(argc, argv);

    vector<FileMetrics> allMetrics;

    if (!fs::exists(options.input_folder)) {
        cerr << "Error: Folder '" << options.input_folder.string() << "' does not exist." << endl;
        return 1;
    }

    for (const auto &entry : fs::recursive_directory_iterator(options.input_folder)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".txt") continue;
        string relative = fs::relative(entry.path(), options.input_folder).generic_string();
        if (!options.case_filter.empty() && relative.find(options.case_filter) == string::npos) continue;

        string filename = entry.path().string();
        cout << "\nProcessing: " << filename << endl;
        allMetrics.push_back(checkOneFile(filename));
    }

    if (allMetrics.empty()) {
        cout << "No matching .txt files found under " << options.input_folder.string() << endl;
        return 0;
    }

    cout << "\n================ Final Summary (" << algorithmName() << ") ================" << endl;
    int mismatchCount = 0;
    for (const auto &m : allMetrics) {
        if (m.isContained) {
            cout << "File: " << m.filename << " => \033[1;32mMatched\033[0m" << endl;
        } else {
            cout << "File: " << m.filename << " => \033[1;31mMismatched\033[0m" << endl;
            mismatchCount++;
        }
    }

    writeCSVReport(allMetrics);

    if (mismatchCount > 0) {
        cerr << "\n" << mismatchCount << " file(s) FAILED correctness check.\n";
        return 2; // distinct nonzero exit code, useful for CI / scripted runs
    }
    return 0;
}

// =======================================================================
// COMMAND LINE ARGUMENTS
//
//   ./correctness_check
//       Default run: Oneconnected algorithm, inputs/ folder, writes
//       triangulation_search_report.csv, no per-file detail files.
//
//   ./correctness_check --algorithm=biconnected
//       Same, but checks the Biconnected algorithm instead.
//
//   ./correctness_check --input-folder=my-cases --csv-report=results/report.csv
//       Custom input folder and CSV output path.
//
//   ./correctness_check --case-filter=hexagon
//       Only process files whose relative path contains "hexagon".
//
//   ./correctness_check --detail-output --detail-output-dir=output/oneconnected
//       Also write a full [MATCH]/[EXTRA]/[MISSING]-annotated triangulation
//       listing per input file (this is what the original file's
//       ENABLE_FILE_OUTPUT=true did, now toggled at the CLI).
//
//   ./correctness_check --quiet
//       Suppress the big per-file metrics block; only prints
//       Matched/Mismatched per file plus the final CSV location. Good
//       for scripted/CI runs where you just want the exit code and CSV.
//
//   Exit codes: 0 = all files matched. 1 = bad arguments / missing
//   input folder. 2 = ran successfully but at least one file MISMATCHED
//   -- check this in CI to fail a build on a correctness regression.
// =======================================================================
