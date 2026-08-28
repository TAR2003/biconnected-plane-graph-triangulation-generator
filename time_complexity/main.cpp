#include <bits/stdc++.h>
using namespace std;
#include "Edge.hpp"
#include "pairHash.hpp"
#include "biconnected.hpp"
#include "FaceTriangulation.hpp"
#include <filesystem>
#include <chrono>

using u128 = unsigned __int128;
namespace fs = std::filesystem;

// ============================================================================
// CONFIG: how many timing runs every single test case should have.
// If a case already has some runs recorded in its category CSV, only the
// remaining runs (RUNS_PER_CASE - alreadyDone) are performed.
// ============================================================================
static const int RUNS_PER_CASE = 5;

// The root folder containing one subfolder per category. Add more category
// folders under this root and they'll be picked up automatically.
static const string INPUT_ROOT = "input";

// helper to convert 128-bit numbers to string (decimal)
static string u128_to_string(u128 x)
{
    if (x == 0)
        return "0";
    string s;
    while (x > 0)
    {
        int digit = (int)(x % 10);
        s.push_back('0' + digit);
        x /= 10;
    }
    reverse(s.begin(), s.end());
    return s;
}

// ============================================================================
// Cross-Platform High-Precision Memory Measurement
// ============================================================================
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
size_t getCurrentMemoryUsage()
{
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc));
    return pmc.WorkingSetSize; // Bytes
}

#elif __APPLE__
#include <mach/mach.h>
size_t getCurrentMemoryUsage()
{
    struct mach_task_basic_info info;
    mach_msg_type_number_t size = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &size) == KERN_SUCCESS)
        return info.resident_size;
    return 0;
}

#else
#include <unistd.h>
size_t getCurrentMemoryUsage()
{
    // Try the more accurate /proc/self/smaps_rollup (available since Linux 4.14)
    std::ifstream smaps("/proc/self/smaps_rollup");
    if (smaps.is_open())
    {
        std::string line;
        size_t rssBytes = 0;
        while (std::getline(smaps, line))
        {
            if (line.rfind("Rss:", 0) == 0)
            {
                std::istringstream iss(line);
                std::string key, unit;
                size_t value;
                iss >> key >> value >> unit;
                if (unit == "kB")
                    rssBytes += value * 1024;
            }
        }
        smaps.close();
        if (rssBytes > 0)
            return rssBytes;
    }

    // Fallback: statm (page-based, lower precision)
    long rss = 0L;
    FILE *fp = fopen("/proc/self/statm", "r");
    if (fp)
    {
        if (fscanf(fp, "%*s%ld", &rss) == 1)
        {
            fclose(fp);
            return rss * sysconf(_SC_PAGESIZE);
        }
        fclose(fp);
    }
    return 0;
}
#endif

// ============================================================================
// Input Reader
// ============================================================================
vector<vector<int>> readInput(const string &filename, int &distinctVertices)
{
    ifstream infile(filename);
    if (!infile.is_open())
    {
        cerr << "Error opening file: " << filename << endl;
        distinctVertices = 0;
        return {};
    }
    vector<vector<int>> faces;
    unordered_set<int> uniqueVertices;
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
            uniqueVertices.insert(vertex);
        }
        faces.push_back(face);
    }
    distinctVertices = uniqueVertices.size();
    return faces;
}

// ============================================================================
// Utility
// ============================================================================
string formatBytes(size_t bytes)
{
    const char *units[] = {"B", "KB", "MB", "GB"};
    int unitIndex = 0;
    double size = (double)bytes;
    while (size >= 1024.0 && unitIndex < 3)
    {
        size /= 1024.0;
        unitIndex++;
    }
    ostringstream oss;
    oss << fixed << setprecision(2) << size << " " << units[unitIndex];
    return oss.str();
}

static string currentTimeString()
{
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[16];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
    return string(buf);
}

// ============================================================================
// Per-run CSV record
// One row per individual run (NOT an average), so we can resume correctly.
// CSV columns:
//   filename,runIndex,vertices,triangulations,timeSeconds,peakMemoryBytes,memoryPerVertex,timestamp
// ============================================================================
struct RunRecord
{
    string filename;
    int runIndex;
    int distinctVertices;
    string triangStr;
    double timeSeconds;
    size_t peakMemory;
    double memoryPerVertex;
};

static string csvPathForCategory(const string &category)
{
    return "results_" + category + ".csv";
}

// Count how many runs are already recorded for a given filename in this
// category's CSV. Returns 0 if the file/category CSV doesn't exist yet.
static int countExistingRuns(const string &csvPath, const string &filename)
{
    ifstream in(csvPath);
    if (!in.is_open())
        return 0;

    string line;
    if (!getline(in, line)) // header
        return 0;

    int count = 0;
    while (getline(in, line))
    {
        if (line.empty())
            continue;
        stringstream ss(line);
        string field;
        getline(ss, field, ','); // filename
        if (field == filename)
            count++;
    }
    return count;
}

static void appendRunCSV(const string &csvPath, const RunRecord &r)
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
        out << "filename,runIndex,vertices,triangulations,timeSeconds,peakMemoryBytes,memoryPerVertex,timestamp\n";
    }
    out << r.filename << ',' << r.runIndex << ',' << r.distinctVertices << ','
        << r.triangStr << ',' << fixed << setprecision(9) << r.timeSeconds << ','
        << r.peakMemory << ',' << fixed << setprecision(6) << r.memoryPerVertex << ','
        << currentTimeString() << '\n';
}

// ============================================================================
// Category indices (for the command-line argument) are assigned in
// alphabetical order, 1-based: 1st category alphabetically = "1", etc.
// ============================================================================

static void printUsage(const vector<string> &categories, const char *progName)
{
    cerr << "Usage: " << progName << " <category-index|all>\n\n";
    cerr << "Available categories (alphabetical order):\n";
    for (size_t i = 0; i < categories.size(); i++)
    {
        cerr << "  " << (i + 1) << " -> " << categories[i] << "\n";
    }
    cerr << "  all -> run every category above\n";
}

static void runCategory(const string &category)
{
    string categoryFolder = INPUT_ROOT + "/" + category;
    string csvPath = csvPathForCategory(category);

    cout << "\n--- Category: " << category << " (CSV: " << csvPath << ") ---\n";

    vector<string> fileList;
    for (const auto &entry : fs::directory_iterator(categoryFolder))
    {
        if (entry.is_regular_file())
            fileList.push_back(entry.path().filename().string());
    }
    sort(fileList.begin(), fileList.end());

    for (const auto &filename : fileList)
    {
        string fullPath = categoryFolder + "/" + filename;

        int alreadyDone = countExistingRuns(csvPath, filename);
        if (alreadyDone >= RUNS_PER_CASE)
        {
            cout << "  Skipping " << filename << " (already has " << alreadyDone
                 << "/" << RUNS_PER_CASE << " runs)\n";
            continue;
        }

        int remaining = RUNS_PER_CASE - alreadyDone;
        cout << "  " << filename << ": " << alreadyDone << "/" << RUNS_PER_CASE
             << " runs done, performing " << remaining << " more...\n";

        int distinctVertices = 0;
        vector<vector<int>> faces = readInput(fullPath, distinctVertices);
        if (faces.empty())
        {
            cerr << "  Warning: skipping empty/invalid file: " << filename << "\n";
            continue;
        }

        // warm-up run once per file if we're about to do more than one run
        // overall (helps avoid first-run cache effects skewing results)
        if (RUNS_PER_CASE > 1 && alreadyDone == 0)
        {
            biconnected *warm = new biconnected(faces);
            warm->getAllTriangulations();
            delete warm;
        }

        for (int localRun = 1; localRun <= remaining; localRun++)
        {
            int globalRunIndex = alreadyDone + localRun;

            size_t memBefore = getCurrentMemoryUsage();

            biconnected *bc = new biconnected(faces);

            using clock = std::chrono::steady_clock;
            auto start = clock::now();
            bc->getAllTriangulations();
            auto end = clock::now();

            size_t memAfter = getCurrentMemoryUsage();
            size_t memUsed = (memAfter > memBefore) ? (memAfter - memBefore) : 0;

            auto dur_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            double runSec = (double)dur_ns / 1'000'000'000.0;

            u128 totalTriang = bc->totalTriangulations;
            double memoryPerVertex = (distinctVertices > 0) ? (double)memUsed / distinctVertices : 0.0;

            cout << "    run " << globalRunIndex << "/" << RUNS_PER_CASE
                 << " -> " << fixed << setprecision(6) << runSec << " s, "
                 << formatBytes(memUsed) << "\n";

            RunRecord rec;
            rec.filename = filename;
            rec.runIndex = globalRunIndex;
            rec.distinctVertices = distinctVertices;
            rec.triangStr = u128_to_string(totalTriang);
            rec.timeSeconds = runSec;
            rec.peakMemory = memUsed;
            rec.memoryPerVertex = memoryPerVertex;

            appendRunCSV(csvPath, rec);

            delete bc;
        }
    }
}

// ============================================================================
// Main
// ============================================================================
int main(int argc, char *argv[])
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if (!fs::exists(INPUT_ROOT) || !fs::is_directory(INPUT_ROOT))
    {
        cerr << "Input folder '" << INPUT_ROOT << "' does not exist.\n";
        return 1;
    }

    // discover categories = subfolders of INPUT_ROOT, alphabetically sorted
    vector<string> categories;
    for (const auto &entry : fs::directory_iterator(INPUT_ROOT))
    {
        if (entry.is_directory())
            categories.push_back(entry.path().filename().string());
    }
    sort(categories.begin(), categories.end());

    if (categories.empty())
    {
        cerr << "No category subfolders found under '" << INPUT_ROOT << "'.\n";
        return 1;
    }

    // ------------------------------------------------------------------
    // Parse command-line argument
    // ------------------------------------------------------------------
    if (argc < 2)
    {
        printUsage(categories, argv[0]);
        return 1;
    }

    string arg = argv[1];
    vector<string> categoriesToRun;

    if (arg == "all")
    {
        categoriesToRun = categories;
    }
    else
    {
        bool isNumber = !arg.empty() && all_of(arg.begin(), arg.end(), ::isdigit);
        if (!isNumber)
        {
            cerr << "Invalid argument: '" << arg << "'\n\n";
            printUsage(categories, argv[0]);
            return 1;
        }

        int idx = stoi(arg);
        if (idx < 1 || idx > (int)categories.size())
        {
            cerr << "Category index out of range: " << idx << "\n\n";
            printUsage(categories, argv[0]);
            return 1;
        }

        categoriesToRun.push_back(categories[idx - 1]);
    }

    cout << "\n";
    cout << "================================================================\n";
    cout << "   TRIANGULATION BENCHMARK - per-category, resumable per-run   \n";
    cout << "   Target runs per case: " << RUNS_PER_CASE << "\n";
    cout << "================================================================\n";

    for (const auto &category : categoriesToRun)
    {
        runCategory(category);
    }

    cout << "\nSelected categories processed. Per-category CSVs contain one row per individual run.\n";
    return 0;
}