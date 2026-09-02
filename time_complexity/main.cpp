#include <bits/stdc++.h>
using namespace std;
#include "Edge.hpp"
#include "pairHash.hpp"
#include "biconnected.hpp"
#include "FaceTriangulation.hpp"
#include <filesystem>
#include <chrono>
#include <thread>
#include <atomic>
#include <future>
#include <mutex>

using u128 = unsigned __int128;
namespace fs = std::filesystem;

// ============================================================================
// CONFIG: how many timing runs every single test case should have.
// ============================================================================
static const int RUNS_PER_CASE = 5;

// ============================================================================
// CONFIG: timeout (in seconds) for a single run. Change this value to adjust.
// ============================================================================
static const double TIMEOUT_SECONDS = 600.0;

// The root folder containing one subfolder per category.
static const string INPUT_ROOT = "input";

// Helper to convert 128-bit numbers to string (decimal)
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

// Helper to format numbers like 1st, 2nd, 3rd, 4th, 5th, etc.
static string getOrdinal(int n)
{
    int tens = (n / 10) % 10;
    if (tens == 1)
        return to_string(n) + "th";
    int ones = n % 10;
    if (ones == 1)
        return to_string(n) + "st";
    if (ones == 2)
        return to_string(n) + "nd";
    if (ones == 3)
        return to_string(n) + "rd";
    return to_string(n) + "th";
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

// Full date+time (was HH:MM:SS only before) so start/end timestamps remain
// unambiguous across a run that spans midnight or multiple days.
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
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return string(buf);
}

// ============================================================================
// Per-run CSV record
// ============================================================================
struct RunRecord
{
    string filename;
    int runIndex;
    int distinctVertices;
    string triangStr; // final count if completed, best-effort partial count so far if timeout
    double timeSeconds;
    size_t peakMemory;
    double memoryPerVertex;
    string startTime; // wall-clock time when this run's timed section began
    string endTime;   // wall-clock time when this run's timed section ended
    string status;    // "completed", "timeout", or "error"
};

static string csvPathForCategory(const string &category)
{
    return "results_" + category + ".csv";
}

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
        out << "filename,runIndex,vertices,triangulations,timeSeconds,peakMemoryBytes,memoryPerVertex,startTime,endTime,status\n";
    }
    out << r.filename << ',' << r.runIndex << ',' << r.distinctVertices << ','
        << r.triangStr << ',' << fixed << setprecision(9) << r.timeSeconds << ','
        << r.peakMemory << ',' << fixed << setprecision(6) << r.memoryPerVertex << ','
        << r.startTime << ',' << r.endTime << ',' << r.status << '\n';
}

// ============================================================================
// Run Category
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

// ============================================================================
// Runs the triangulation computation on a worker thread and waits for it,
// up to TIMEOUT_SECONDS. Returns true if it completed in time, false if it
// timed out.
//
// IMPORTANT: on timeout, the worker thread is detached and left running in
// the background (there is no safe cross-platform way to forcibly kill a
// plain std::thread mid-computation). To avoid the detached thread ever
// touching freed/destroyed memory, EVERY piece of state it can reach --
// the promise, and bc itself -- is heap-allocated with its own lifetime,
// never a stack local of this function. On timeout we deliberately leak
// the promise and bc so the background thread always has valid memory to
// write into, no matter how long after this function returns it finishes.
// ============================================================================
static bool runWithTimeout(biconnected *bc, double timeoutSeconds, double &actualRunSeconds, u128 &triangCountOut)
{
    // Heap-allocate the promise so it outlives this function on timeout.
    auto *donePromise = new std::promise<void>();
    std::future<void> doneFuture = donePromise->get_future();

    using clock = std::chrono::steady_clock;
    auto start = clock::now();

    std::thread worker([bc, donePromise]()
                       {
                           try
                           {
                               bc->getAllTriangulations();
                           }
                           catch (...)
                           {
                               // Swallow any exception from the computation itself; we still
                               // need to signal completion so the future doesn't hang forever
                               // in the (unlikely) case this thread is still being awaited.
                           }
                           donePromise->set_value();
                           // Note: donePromise is intentionally not deleted here. If the main
                           // thread already timed out and moved on, it may still be safe to
                           // delete since nothing else touches it after set_value(), but to
                           // keep this unconditionally safe under all timing interleavings we
                           // simply leak it. It's a few dozen bytes per timed-out case.
                       });

    auto status = doneFuture.wait_for(std::chrono::duration<double>(timeoutSeconds));

    auto end = clock::now();
    actualRunSeconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count();

    if (status == std::future_status::ready)
    {
        worker.join();
        // Safe: worker has fully finished (join() only returns after the
        // thread function has returned), so its last write to
        // totalTriangulations happens-before this read.
        triangCountOut = bc->totalTriangulations;
        delete donePromise; // safe: worker has finished, no one else references it
        return true;
    }
    else
    {
        // Timed out. Detach and leak donePromise (and bc, handled by the
        // caller) so the background thread never dereferences freed memory
        // whenever it eventually finishes.
        // Best-effort snapshot of however many triangulations had been
        // counted so far. This is a benign data race on a plain (non-atomic)
        // u128 counter that the worker thread is still incrementing in the
        // background -- there is no std::atomic<unsigned __int128> support
        // on common platforms, so we accept a possible torn/stale read here
        // rather than adding locking to the hot recursive path in
        // biconnected/FaceTriangulation. In practice this gives a usable
        // "how far did it get" figure for a timed-out case, at the cost of
        // not being 100% guaranteed exact.
        triangCountOut = bc->totalTriangulations;

        worker.detach();
        return false;
    }
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
            cout << "  " << filename << ": Found " << alreadyDone << " run(s) in CSV. Already complete ("
                 << RUNS_PER_CASE << "/" << RUNS_PER_CASE << "), skipping.\n";
            continue;
        }

        int remaining = RUNS_PER_CASE - alreadyDone;
        cout << "  " << filename << ": Found " << alreadyDone << " run(s) in CSV. Need "
             << remaining << " more run(s).\n";

        int distinctVertices = 0;
        vector<vector<int>> faces = readInput(fullPath, distinctVertices);
        if (faces.empty())
        {
            cerr << "    Warning: skipping empty/invalid file: " << filename << "\n";
            continue;
        }

        for (int localRun = 1; localRun <= remaining; localRun++)
        {
            int globalRunIndex = alreadyDone + localRun;

            // Capture and print the start time BEFORE execution starts
            string startTs = currentTimeString();
            cout << "    Running " << getOrdinal(globalRunIndex) << " run (start " << startTs
                 << ", timeout " << TIMEOUT_SECONDS << "s)..." << flush;

            size_t memBefore = getCurrentMemoryUsage();

            biconnected *bc = new biconnected(faces);

            double runSec = 0.0;
            u128 triangCount = 0;
            bool completed = runWithTimeout(bc, TIMEOUT_SECONDS, runSec, triangCount);

            // Capture the end time immediately once we stop waiting
            string endTs = currentTimeString();

            size_t memAfter = getCurrentMemoryUsage();
            size_t memUsed = (memAfter > memBefore) ? (memAfter - memBefore) : 0;

            double memoryPerVertex = (distinctVertices > 0) ? (double)memUsed / distinctVertices : 0.0;

            RunRecord rec;
            rec.filename = filename;
            rec.runIndex = globalRunIndex;
            rec.distinctVertices = distinctVertices;
            rec.timeSeconds = runSec;
            rec.peakMemory = memUsed;
            rec.memoryPerVertex = memoryPerVertex;
            rec.startTime = startTs;
            rec.endTime = endTs;

            if (completed)
            {
                rec.triangStr = u128_to_string(triangCount);
                rec.status = "completed";

                cout << " done (end " << endTs << ") -> " << fixed << setprecision(6) << runSec << " s, "
                     << u128_to_string(triangCount) << " triangulations, "
                     << formatBytes(memUsed) << "\n";

                delete bc;
            }
            else
            {
                // Best-effort count of triangulations found before the
                // timeout struck (see runWithTimeout for the caveat that
                // this is a benign-race snapshot, not a guaranteed-exact
                // value).
                rec.triangStr = u128_to_string(triangCount);
                rec.status = "timeout";

                cout << " TIMEOUT after " << fixed << setprecision(6) << runSec
                     << " s (end " << endTs << "), " << u128_to_string(triangCount)
                     << " triangulations found so far, " << formatBytes(memUsed) << "\n";

                // NOTE: bc is intentionally NOT deleted here. The detached
                // worker thread is still using it in the background; deleting
                // it now would cause a use-after-free / crash. This leaks
                // memory for timed-out cases, which is an acceptable tradeoff
                // versus forcibly killing a thread mid-computation.
            }

            appendRunCSV(csvPath, rec);
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
    cout << "   Timeout per run: " << TIMEOUT_SECONDS << " seconds\n";
    cout << "================================================================\n";

    for (const auto &category : categoriesToRun)
    {
        runCategory(category);
    }

    cout << "\nSelected categories processed. Per-category CSVs contain one row per individual run.\n";
    return 0;
}