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

using u128 = unsigned __int128;
namespace fs = std::filesystem;

// ============================================================================
// CONFIG: how many timing runs every single test case should have.
// ============================================================================
static const int RUNS_PER_CASE = 5;

// ============================================================================
// CONFIG: time limit (in seconds) for a single run. Change this value to adjust.
// ============================================================================
static const double TIME_LIMIT_SECONDS = 360.0;

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

static u128 string_to_u128(const string &s)
{
    u128 x = 0;
    for (char c : s)
    {
        if (c >= '0' && c <= '9')
            x = x * 10 + (u128)(c - '0');
    }
    return x;
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
#include <mach-o/dyld.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
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
#include <sys/wait.h>
#include <signal.h>
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

static string getExecutablePath()
{
#ifdef _WIN32
    char path[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (len == 0 || len == MAX_PATH)
        return string();
    return string(path);
#elif __APPLE__
    char path[4096];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) != 0)
        return string();
    return string(path);
#else
    char path[4096];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len <= 0)
        return string("./benchmark");
    path[len] = '\0';
    return string(path);
#endif
}

static void writeProgressFile(const string &path, u128 count)
{
    ofstream out(path, ios::trunc);
    if (out.is_open())
        out << u128_to_string(count) << '\n';
}

static u128 readProgressFile(const string &path)
{
    ifstream in(path);
    if (!in.is_open())
        return 0;
    string line;
    getline(in, line);
    return string_to_u128(line);
}

struct WorkerResult
{
    string status;
    u128 triangulations = 0;
    double timeSeconds = 0.0;
    size_t peakMemory = 0;
    string startTime;
    string endTime;
    // Additional search-quality metrics (from check-stats.cpp)
    long long totalChecks = 0;
    long long successfulChecks = 0;
    long long invalidTraversals = 0;
};

static void writeWorkerResultFile(const string &path, const WorkerResult &r)
{
    ofstream out(path, ios::trunc);
    if (!out.is_open())
        return;
    out << "status=" << r.status << '\n';
    out << "triangulations=" << u128_to_string(r.triangulations) << '\n';
    out << "timeSeconds=" << fixed << setprecision(9) << r.timeSeconds << '\n';
    out << "peakMemoryBytes=" << r.peakMemory << '\n';
    out << "startTime=" << r.startTime << '\n';
    out << "endTime=" << r.endTime << '\n';
    out << "totalChecks=" << r.totalChecks << '\n';
    out << "successfulChecks=" << r.successfulChecks << '\n';
    out << "invalidTraversals=" << r.invalidTraversals << '\n';
}

static bool readWorkerResultFile(const string &path, WorkerResult &r)
{
    ifstream in(path);
    if (!in.is_open())
        return false;

    string line;
    bool any = false;
    while (getline(in, line))
    {
        if (line.empty())
            continue;
        auto eq = line.find('=');
        if (eq == string::npos)
            continue;
        string key = line.substr(0, eq);
        string value = line.substr(eq + 1);
        any = true;
        if (key == "status")
            r.status = value;
        else if (key == "triangulations")
            r.triangulations = string_to_u128(value);
        else if (key == "timeSeconds")
            r.timeSeconds = stod(value);
        else if (key == "peakMemoryBytes")
            r.peakMemory = (size_t)stoull(value);
        else if (key == "startTime")
            r.startTime = value;
        else if (key == "endTime")
            r.endTime = value;
        else if (key == "totalChecks")
            r.totalChecks = stoll(value);
        else if (key == "successfulChecks")
            r.successfulChecks = stoll(value);
        else if (key == "invalidTraversals")
            r.invalidTraversals = stoll(value);
    }
    return any;
}

// ============================================================================
// Per-run CSV record
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
    string startTime;
    string endTime;
    string status; // "completed", "time_limit_exceeded", or "error"

    // Additional search-quality metrics (from check-stats.cpp)
    long long totalChecks = 0;
    long long successfulChecks = 0;
    long long failedChecks = 0;
    double checkSuccessRate = 0.0;
    long long invalidTraversals = 0;
    long long totalTraversalsExtended = 0; // invalidTraversals + successful triangulations
    double traversalSuccessRate = 0.0;
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
    if (!getline(in, line))
        return 0;

    int count = 0;
    while (getline(in, line))
    {
        if (line.empty())
            continue;
        stringstream ss(line);
        string field;
        getline(ss, field, ',');
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
        out << "filename,runIndex,vertices,triangulations,timeSeconds,peakMemoryBytes,memoryPerVertex,startTime,endTime,status,"
            << "totalChecks,successfulChecks,failedChecks,checkSuccessRate,"
            << "invalidTraversals,totalTraversalsExtended,traversalSuccessRate\n";
    }
    out << r.filename << ',' << r.runIndex << ',' << r.distinctVertices << ','
        << r.triangStr << ',' << fixed << setprecision(9) << r.timeSeconds << ','
        << r.peakMemory << ',' << fixed << setprecision(6) << r.memoryPerVertex << ','
        << r.startTime << ',' << r.endTime << ',' << r.status << ','
        << r.totalChecks << ',' << r.successfulChecks << ',' << r.failedChecks << ','
        << fixed << setprecision(2) << r.checkSuccessRate << ','
        << r.invalidTraversals << ',' << r.totalTraversalsExtended << ','
        << fixed << setprecision(2) << r.traversalSuccessRate << '\n';
}

// ============================================================================
// Subprocess execution
// Returns: 0 = child finished in time, 1 = time limit exceeded (child killed),
//          -1 = spawn/wait error
// ============================================================================
static int runInSubprocess(const string &inputPath,
                           const string &resultPath,
                           const string &progressPath,
                           double timeLimitSeconds)
{
    string exePath = getExecutablePath();
    if (exePath.empty())
        return -1;

#ifdef _WIN32
    ostringstream cmd;
    cmd << '"' << exePath << "\" --worker \"" << inputPath << "\" \""
        << resultPath << "\" \"" << progressPath << '"';
    string cmdLine = cmd.str();
    vector<char> cmdBuf(cmdLine.begin(), cmdLine.end());
    cmdBuf.push_back('\0');

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    if (!CreateProcessA(NULL, cmdBuf.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        return -1;

    DWORD timeoutMs = (DWORD)(timeLimitSeconds * 1000.0);
    DWORD waitResult = WaitForSingleObject(pi.hProcess, timeoutMs);

    if (waitResult == WAIT_TIMEOUT)
    {
        TerminateProcess(pi.hProcess, 1);
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return 1;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return 0;

#else
    pid_t pid = fork();
    if (pid == 0)
    {
        execl(exePath.c_str(), exePath.c_str(), "--worker",
              inputPath.c_str(), resultPath.c_str(), progressPath.c_str(), (char *)NULL);
        _exit(127);
    }
    if (pid < 0)
        return -1;

    using clock = std::chrono::steady_clock;
    auto deadline = clock::now() + std::chrono::duration<double>(timeLimitSeconds);

    while (true)
    {
        int status = 0;
        pid_t waited = waitpid(pid, &status, WNOHANG);
        if (waited == pid)
            return 0;
        if (waited < 0)
            return -1;
        if (clock::now() >= deadline)
        {
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            return 1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
#endif
}

// ============================================================================
// Worker mode: run one timed computation in an isolated child process.
// ============================================================================
static int runWorkerMode(const char *inputPath, const char *resultPath, const char *progressPath)
{
    int distinctVertices = 0;
    vector<vector<int>> faces = readInput(inputPath, distinctVertices);
    if (faces.empty())
        return 1;

    string startTs = currentTimeString();
    size_t memBefore = getCurrentMemoryUsage();

    biconnected *bc = new biconnected(faces);
    std::atomic<bool> stopProgress{false};

    std::thread progressThread([&]()
                               {
        while (!stopProgress.load(std::memory_order_relaxed))
        {
            writeProgressFile(progressPath, bc->totalTriangulations);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        writeProgressFile(progressPath, bc->totalTriangulations); });

    using clock = std::chrono::steady_clock;
    auto runStart = clock::now();
    bc->getAllTriangulations();
    auto runEnd = clock::now();

    stopProgress.store(true, std::memory_order_relaxed);
    progressThread.join();

    double runSec = std::chrono::duration_cast<std::chrono::duration<double>>(runEnd - runStart).count();
    string endTs = currentTimeString();

    size_t memAfter = getCurrentMemoryUsage();
    size_t memUsed = (memAfter > memBefore) ? (memAfter - memBefore) : 0;

    WorkerResult result;
    result.status = "completed";
    result.triangulations = bc->totalTriangulations;
    result.timeSeconds = runSec;
    result.peakMemory = memUsed;
    result.startTime = startTs;
    result.endTime = endTs;
    result.totalChecks = bc->totalChecks;
    result.successfulChecks = bc->successfulChecks;
    result.invalidTraversals = bc->invalidTraversals;
    writeWorkerResultFile(resultPath, result);

    delete bc;
    return 0;
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

            string startTs = currentTimeString();
            cout << "    Running " << getOrdinal(globalRunIndex) << " run (start " << startTs
                 << ", time limit " << TIME_LIMIT_SECONDS << "s)..." << flush;

            fs::path tempDir = fs::temp_directory_path() /
                               ("tri_benchmark_" + category + "_" + filename + "_" + to_string(globalRunIndex));
            fs::create_directories(tempDir);
            string progressPath = (tempDir / "progress.txt").string();
            string resultPath = (tempDir / "result.txt").string();

            size_t memBefore = getCurrentMemoryUsage();

            using clock = std::chrono::steady_clock;
            auto runStart = clock::now();
            int subprocessStatus = runInSubprocess(fullPath, resultPath, progressPath, TIME_LIMIT_SECONDS);
            auto runEnd = clock::now();
            double runSec = std::chrono::duration_cast<std::chrono::duration<double>>(runEnd - runStart).count();

            string endTs = currentTimeString();
            size_t memAfter = getCurrentMemoryUsage();
            size_t memUsed = (memAfter > memBefore) ? (memAfter - memBefore) : 0;

            RunRecord rec;
            rec.filename = filename;
            rec.runIndex = globalRunIndex;
            rec.distinctVertices = distinctVertices;
            rec.startTime = startTs;
            rec.endTime = endTs;

            if (subprocessStatus == 0)
            {
                WorkerResult workerResult;
                if (readWorkerResultFile(resultPath, workerResult))
                {
                    rec.triangStr = u128_to_string(workerResult.triangulations);
                    rec.timeSeconds = workerResult.timeSeconds;
                    rec.peakMemory = workerResult.peakMemory;
                    rec.startTime = workerResult.startTime.empty() ? startTs : workerResult.startTime;
                    rec.endTime = workerResult.endTime.empty() ? endTs : workerResult.endTime;
                    rec.status = "completed";

                    rec.totalChecks = workerResult.totalChecks;
                    rec.successfulChecks = workerResult.successfulChecks;
                    rec.failedChecks = workerResult.totalChecks - workerResult.successfulChecks;
                    rec.checkSuccessRate = (workerResult.totalChecks > 0)
                                               ? (static_cast<double>(workerResult.successfulChecks) / workerResult.totalChecks) * 100.0
                                               : 0.0;
                    rec.invalidTraversals = workerResult.invalidTraversals;
                    {
                        long long successfulTraversals = static_cast<long long>(workerResult.triangulations);
                        rec.totalTraversalsExtended = workerResult.invalidTraversals + successfulTraversals;
                        rec.traversalSuccessRate = (rec.totalTraversalsExtended > 0)
                                                       ? (static_cast<double>(successfulTraversals) / rec.totalTraversalsExtended) * 100.0
                                                       : 0.0;
                    }

                    cout << " done (end " << rec.endTime << ") -> " << fixed << setprecision(6)
                         << rec.timeSeconds << " s, " << rec.triangStr << " triangulations, "
                         << formatBytes(rec.peakMemory) << "\n";
                }
                else
                {
                    rec.triangStr = u128_to_string(readProgressFile(progressPath));
                    rec.timeSeconds = runSec;
                    rec.peakMemory = memUsed;
                    rec.status = "error";
                    cout << " ERROR: worker finished but result file missing (end " << endTs << ")\n";
                }
            }
            else if (subprocessStatus == 1)
            {
                u128 triangCount = readProgressFile(progressPath);
                rec.triangStr = u128_to_string(triangCount);
                rec.timeSeconds = runSec;
                rec.peakMemory = memUsed;
                rec.status = "time_limit_exceeded";

                cout << " TIME LIMIT EXCEEDED (end " << endTs << ") -> " << fixed << setprecision(6)
                     << rec.timeSeconds << " s, " << rec.triangStr
                     << " triangulations reached before time limit, "
                     << formatBytes(rec.peakMemory) << "\n";
            }
            else
            {
                rec.triangStr = u128_to_string(readProgressFile(progressPath));
                rec.timeSeconds = runSec;
                rec.peakMemory = memUsed;
                rec.status = "error";
                cout << " ERROR: failed to spawn or wait on worker process (end " << endTs << ")\n";
            }

            rec.memoryPerVertex = (distinctVertices > 0) ? (double)rec.peakMemory / distinctVertices : 0.0;
            appendRunCSV(csvPath, rec);

            error_code ec;
            fs::remove_all(tempDir, ec);
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

    if (argc == 5 && string(argv[1]) == "--worker")
        return runWorkerMode(argv[2], argv[3], argv[4]);

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
    cout << "   Time limit per run: " << TIME_LIMIT_SECONDS << " seconds\n";
    cout << "================================================================\n";

    for (const auto &category : categoriesToRun)
    {
        runCategory(category);
    }

    cout << "\nSelected categories processed. Per-category CSVs contain one row per individual run.\n";
    return 0;
}