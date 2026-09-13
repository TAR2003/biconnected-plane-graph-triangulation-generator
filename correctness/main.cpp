#include <bits/stdc++.h>
using namespace std;
#include "Edge.hpp"
#include "pairHash.hpp"
#include "biconnected.hpp"
#include "FaceTriangulation.hpp"
#include "triconnected.hpp"
#include "TriangulationHasher.hpp"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>
namespace fs = std::filesystem;

// ============================================================================
// Input reader (same format as before: face count, then per-face vertex list)
// ============================================================================
vector<vector<int>> solve(const string &filename)
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

// ============================================================================
// Comparison helpers
// ============================================================================

static bool matchPairs(const pair<int, int> &p1, const pair<int, int> &p2)
{
    return (p1.first == p2.first && p1.second == p2.second);
}

static bool matchTriangulations(
    const vector<pair<int, int>> &t1,
    const vector<pair<int, int>> &t2)
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

static string formatTimestamp(const chrono::system_clock::time_point &tp)
{
    const auto tt = chrono::system_clock::to_time_t(tp);
    tm localTm{};
#ifdef _WIN32
    localtime_s(&localTm, &tt);
#else
    localtime_r(&tt, &localTm);
#endif
    ostringstream oss;
    oss << put_time(&localTm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

static string formatDurationSeconds(double seconds)
{
    ostringstream oss;
    oss << fixed << setprecision(3) << seconds;
    return oss.str();
}

enum class MatchVerdict
{
    MATCHED,
    UNMATCHED
};

enum class ExactVerdict
{
    MATCHED,
    UNMATCHED,
    NOT_RUN
};

struct ComparisonResult
{
    bool fileReadError = false;
    MatchVerdict triangulationCountMatch = MatchVerdict::UNMATCHED;
    MatchVerdict hashMatch = MatchVerdict::UNMATCHED;
    ExactVerdict exactMatch = ExactVerdict::UNMATCHED;
    MatchVerdict overallMatch = MatchVerdict::UNMATCHED;

    TriangulationRunStats newStats;
    TriangulationRunStats oldStats;

    string startTime;
    string endTime;
    double totalSeconds = 0.0;
};

static MatchVerdict toMatchVerdict(bool matched)
{
    return matched ? MatchVerdict::MATCHED : MatchVerdict::UNMATCHED;
}

static string verdictToString(MatchVerdict v)
{
    return v == MatchVerdict::MATCHED ? "MATCHED" : "UNMATCHED";
}

static string exactVerdictToString(ExactVerdict v)
{
    switch (v)
    {
    case ExactVerdict::MATCHED:
        return "MATCHED";
    case ExactVerdict::UNMATCHED:
        return "UNMATCHED";
    case ExactVerdict::NOT_RUN:
        return "NOT_RUN";
    }
    return "UNKNOWN";
}

static bool hashAggregatesMatch(const TriangulationRunStats &a, const TriangulationRunStats &b)
{
    return a.aggregatesEqual(b);
}

static ExactVerdict compareExact(
    const vector<vector<pair<int, int>>> &newStored,
    const vector<vector<pair<int, int>>> &oldStored,
    const TriangulationRunStats &newStats,
    const TriangulationRunStats &oldStats)
{
    if (newStats.storageStopped || oldStats.storageStopped)
        return ExactVerdict::NOT_RUN;

    if (newStored.size() != oldStored.size())
        return ExactVerdict::UNMATCHED;

    auto sortedNew = newStored;
    auto sortedOld = oldStored;
    sort(sortedNew.begin(), sortedNew.end());
    sort(sortedOld.begin(), sortedOld.end());

    for (size_t i = 0; i < sortedNew.size(); ++i)
    {
        if (!matchTriangulations(sortedNew[i], sortedOld[i]))
            return ExactVerdict::UNMATCHED;
    }
    return ExactVerdict::MATCHED;
}

static MatchVerdict computeOverallMatch(
    MatchVerdict countMatch,
    MatchVerdict hashMatchResult,
    ExactVerdict exactMatchResult)
{
    if (countMatch != MatchVerdict::MATCHED || hashMatchResult != MatchVerdict::MATCHED)
        return MatchVerdict::UNMATCHED;

    if (exactMatchResult == ExactVerdict::MATCHED ||
        exactMatchResult == ExactVerdict::NOT_RUN)
    {
        return MatchVerdict::MATCHED;
    }
    return MatchVerdict::UNMATCHED;
}

// Returns: 1 = matched, 0 = mismatched, -1 = error (couldn't read file)
static ComparisonResult compareTwoAlgorithms(
    const string &filename,
    size_t memoryLimitBytes)
{
    ComparisonResult result;

    const auto startClock = chrono::steady_clock::now();
    result.startTime = formatTimestamp(chrono::system_clock::now());

    vector<vector<int>> faces = solve(filename);
    if (faces.empty())
    {
        result.fileReadError = true;
        result.endTime = formatTimestamp(chrono::system_clock::now());
        return result;
    }

    result.newStats.memoryLimitBytes = memoryLimitBytes;
    result.oldStats.memoryLimitBytes = memoryLimitBytes;

    biconnected *bc = new biconnected(faces, &result.newStats);
    bc->getAllTriangulations();
    bc->sortTriangulations();

    triconnected *tc = new triconnected(faces, &result.oldStats);
    tc->getAllTriangulations();
    tc->refineTriangulations();

    const auto endClock = chrono::steady_clock::now();
    result.endTime = formatTimestamp(chrono::system_clock::now());
    result.totalSeconds = chrono::duration<double>(endClock - startClock).count();

    result.triangulationCountMatch = toMatchVerdict(
        result.newStats.totalTriangulationCount == result.oldStats.totalTriangulationCount);
    result.hashMatch = toMatchVerdict(hashAggregatesMatch(result.newStats, result.oldStats));
    result.exactMatch = compareExact(
        bc->allTriangulations,
        tc->allTriangulations,
        result.newStats,
        result.oldStats);
    result.overallMatch = computeOverallMatch(
        result.triangulationCountMatch,
        result.hashMatch,
        result.exactMatch);

    delete bc;
    delete tc;

    return result;
}

// ============================================================================
// Per-category CSV helpers
// ============================================================================

static string csvPathForCategory(const string &category)
{
    return "results_" + category + ".csv";
}

static unordered_set<string> loadProcessedFiles(const string &csvPath)
{
    unordered_set<string> processed;
    ifstream in(csvPath);
    if (!in.is_open())
        return processed;

    string line;
    if (!getline(in, line))
        return processed;

    while (getline(in, line))
    {
        if (line.empty())
            continue;
        stringstream ss(line);
        string filename;
        if (!getline(ss, filename, ','))
            continue;
        processed.insert(filename);
    }
    return processed;
}

static void appendResultCSV(const string &csvPath, const string &filename, const ComparisonResult &r)
{
    bool needHeader = !fs::exists(csvPath);
    ofstream out(csvPath, ios::app);
    if (!out.is_open())
    {
        cerr << "Error: could not open " << csvPath << " for writing\n";
        return;
    }

    if (needHeader)
    {
        out << "filename,startTime,endTime,totalTimeSeconds,"
            << "triangulationCountA,triangulationCountB,"
            << "newStoredCount,oldStoredCount,"
            << "newSpaceExceeded,oldSpaceExceeded,"
            << "xxHashXorA,xxHashXorB,xxHashSumA,xxHashSumB,"
            << "sipHashXorA,sipHashXorB,sipHashSumA,sipHashSumB,"
            << "sha256XorA,sha256XorB,sha256SumA,sha256SumB,"
            << "triangulationCountMatch,hashMatch,exactMatch,matched\n";
    }

    out << filename << ','
        << r.startTime << ','
        << r.endTime << ','
        << formatDurationSeconds(r.totalSeconds) << ','
        << r.newStats.totalTriangulationCount << ','
        << r.oldStats.totalTriangulationCount << ','
        << r.newStats.storedTriangulationCount << ','
        << r.oldStats.storedTriangulationCount << ','
        << (r.newStats.storageStopped ? "YES" : "NO") << ','
        << (r.oldStats.storageStopped ? "YES" : "NO") << ','
        << formatHashHex(r.newStats.hashXxXor) << ','
        << formatHashHex(r.oldStats.hashXxXor) << ','
        << formatHashHex(r.newStats.hashXxSum) << ','
        << formatHashHex(r.oldStats.hashXxSum) << ','
        << formatHashHex(r.newStats.hashSipXor) << ','
        << formatHashHex(r.oldStats.hashSipXor) << ','
        << formatHashHex(r.newStats.hashSipSum) << ','
        << formatHashHex(r.oldStats.hashSipSum) << ','
        << formatSha256WordsHex(r.newStats.sha256Xor) << ','
        << formatSha256WordsHex(r.oldStats.sha256Xor) << ','
        << formatSha256WordsHex(r.newStats.sha256Sum) << ','
        << formatSha256WordsHex(r.oldStats.sha256Sum) << ','
        << verdictToString(r.triangulationCountMatch) << ','
        << verdictToString(r.hashMatch) << ','
        << exactVerdictToString(r.exactMatch) << ','
        << verdictToString(r.overallMatch) << '\n';
}

// ============================================================================
// Category discovery
// ============================================================================

static void printUsage(const vector<string> &categories, const char *progName, size_t memoryLimitGb)
{
    cerr << "Usage: " << progName << " <category-index|all> [--memory-limit-gb N]\n\n";
    cerr << "Options:\n";
    cerr << "  --memory-limit-gb N   Stop storing triangulations after ~N GiB (default: "
        << memoryLimitGb << ")\n\n";
    cerr << "Available categories (alphabetical order):\n";
    for (size_t i = 0; i < categories.size(); i++)
    {
        cerr << "  " << (i + 1) << " -> " << categories[i] << "\n";
    }
    cerr << "  all -> run every category above\n";
}

struct Summary
{
    string category;
    int matched = 0;
    int mismatched = 0;
    int errors = 0;
    int skipped = 0;
};

static Summary runCategory(
    const string &rootFolder,
    const string &category,
    size_t memoryLimitBytes)
{
    const string categoryFolder = rootFolder + "/" + category;
    const string csvPath = csvPathForCategory(category);

    cout << "\n=== Category: " << category << " ===\n";
    cout << "Results CSV: " << csvPath << "\n";
    cout << "Memory limit for stored triangulations: "
         << (memoryLimitBytes / (1024ULL * 1024ULL * 1024ULL)) << " GiB\n";

    unordered_set<string> processed = loadProcessedFiles(csvPath);

    Summary summ;
    summ.category = category;

    vector<string> fileList;
    for (const auto &entry : fs::directory_iterator(categoryFolder))
    {
        if (entry.is_regular_file())
            fileList.push_back(entry.path().filename().string());
    }
    sort(fileList.begin(), fileList.end());

    for (const string &filename : fileList)
    {
        if (processed.find(filename) != processed.end())
        {
            cout << "  Skipping already-tested: " << filename << "\n";
            summ.skipped++;
            continue;
        }

        const string fullPath = categoryFolder + "/" + filename;
        cout << "  Processing: " << filename << " ... " << flush;

        ComparisonResult result = compareTwoAlgorithms(fullPath, memoryLimitBytes);

        if (result.fileReadError)
        {
            summ.errors++;
            cout << "\033[1;33mERROR (could not read file)\033[0m\n";
            appendResultCSV(csvPath, filename, result);
            continue;
        }

        const bool isMatched = result.overallMatch == MatchVerdict::MATCHED;
        if (isMatched)
        {
            summ.matched++;
            cout << "\033[1;32mMATCHED\033[0m"
                 << " (count=" << result.newStats.totalTriangulationCount
                 << ", time=" << formatDurationSeconds(result.totalSeconds) << "s";
            if (result.exactMatch == ExactVerdict::NOT_RUN)
                cout << ", exact=NOT_RUN";
            cout << ")\n";
        }
        else
        {
            summ.mismatched++;
            cout << "\033[1;31mUNMATCHED\033[0m"
                 << " (new=" << result.newStats.totalTriangulationCount
                 << ", old=" << result.oldStats.totalTriangulationCount
                 << ", countMatch=" << verdictToString(result.triangulationCountMatch)
                 << ", hashMatch=" << verdictToString(result.hashMatch)
                 << ", exactMatch=" << exactVerdictToString(result.exactMatch)
                 << ")\n";
        }

        cout << "    start=" << result.startTime
             << "  end=" << result.endTime
             << "  duration=" << formatDurationSeconds(result.totalSeconds) << "s\n";
        cout << "    new xx: xor=" << formatHashHex(result.newStats.hashXxXor)
             << " sum=" << formatHashHex(result.newStats.hashXxSum) << "\n";
        cout << "    new sip: xor=" << formatHashHex(result.newStats.hashSipXor)
             << " sum=" << formatHashHex(result.newStats.hashSipSum) << "\n";
        cout << "    new sha256 xor: "
             << formatSha256WordsHex(result.newStats.sha256Xor) << "\n";
        cout << "    new sha256 sum: "
             << formatSha256WordsHex(result.newStats.sha256Sum) << "\n";
        cout << "    old xx: xor=" << formatHashHex(result.oldStats.hashXxXor)
             << " sum=" << formatHashHex(result.oldStats.hashXxSum) << "\n";
        cout << "    old sip: xor=" << formatHashHex(result.oldStats.hashSipXor)
             << " sum=" << formatHashHex(result.oldStats.hashSipSum) << "\n";
        cout << "    old sha256 xor: "
             << formatSha256WordsHex(result.oldStats.sha256Xor) << "\n";
        cout << "    old sha256 sum: "
             << formatSha256WordsHex(result.oldStats.sha256Sum) << "\n";

        appendResultCSV(csvPath, filename, result);
    }

    return summ;
}

static size_t parseMemoryLimitGb(int argc, char *argv[], size_t defaultGb)
{
    for (int i = 2; i < argc; ++i)
    {
        string arg = argv[i];
        if (arg == "--memory-limit-gb" && i + 1 < argc)
        {
            return static_cast<size_t>(stoull(argv[i + 1]));
        }
    }
    return defaultGb;
}

int main(int argc, char *argv[])
{
    const string rootFolder = "input";
    const size_t defaultMemoryLimitGb = 2;
    const size_t memoryLimitGb = parseMemoryLimitGb(argc, argv, defaultMemoryLimitGb);
    const size_t memoryLimitBytes = memoryLimitGb * 1024ULL * 1024ULL * 1024ULL;

    if (!fs::exists(rootFolder) || !fs::is_directory(rootFolder))
    {
        cerr << "Input folder '" << rootFolder << "' does not exist.\n";
        return 1;
    }

    vector<string> categories;
    for (const auto &entry : fs::directory_iterator(rootFolder))
    {
        if (entry.is_directory())
            categories.push_back(entry.path().filename().string());
    }
    sort(categories.begin(), categories.end());

    if (categories.empty())
    {
        cerr << "No category subfolders found under '" << rootFolder << "'.\n";
        return 1;
    }

    if (argc < 2)
    {
        printUsage(categories, argv[0], defaultMemoryLimitGb);
        return 1;
    }

    const string arg = argv[1];
    vector<string> categoriesToRun;

    if (arg == "all")
    {
        categoriesToRun = categories;
    }
    else
    {
        const bool isNumber = !arg.empty() && all_of(arg.begin(), arg.end(), ::isdigit);
        if (!isNumber)
        {
            cerr << "Invalid argument: '" << arg << "'\n\n";
            printUsage(categories, argv[0], defaultMemoryLimitGb);
            return 1;
        }

        const int idx = stoi(arg);
        if (idx < 1 || idx > static_cast<int>(categories.size()))
        {
            cerr << "Category index out of range: " << idx << "\n\n";
            printUsage(categories, argv[0], defaultMemoryLimitGb);
            return 1;
        }

        categoriesToRun.push_back(categories[idx - 1]);
    }

    cout << "Correctness verification started at "
         << formatTimestamp(chrono::system_clock::now()) << "\n";
    cout << "Memory limit per algorithm: " << memoryLimitGb << " GiB\n";

    vector<Summary> summaries;
    for (const string &category : categoriesToRun)
        summaries.push_back(runCategory(rootFolder, category, memoryLimitBytes));

    cout << "\n=================== SUMMARY ===================\n";
    for (const auto &s : summaries)
    {
        cout << "Category: " << s.category << "\n";
        cout << "  Matched:    " << s.matched << "\n";
        cout << "  Unmatched:  " << s.mismatched << "\n";
        cout << "  Errors:     " << s.errors << "\n";
        cout << "  Skipped:    " << s.skipped << " (already tested previously)\n";
    }
    cout << "Finished at " << formatTimestamp(chrono::system_clock::now()) << "\n";
    cout << "=================================================\n";

    return 0;
}
