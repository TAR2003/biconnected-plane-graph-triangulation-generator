// bench_memory.cpp - PEAK memory measurement. Completely independent of
// Google Benchmark and of the timing run.
//
// Every case runs in its own freshly exec'ed worker process, so no allocator
// state or earlier peak can leak from one case into the next. Two numbers are
// recorded per case:
//   * peakDeltaBytes   : peak RSS reached by the algorithm minus the RSS right
//                        after the input was loaded (kernel high-water mark is
//                        reset first on Linux) -> memory the algorithm needed.
//   * peakRssBytes     : absolute peak RSS of the worker process.
//   * childMaxRssBytes : peak RSS of the worker as seen by the parent via
//                        wait4()/ru_maxrss (kernel-side cross-check, POSIX only).
// Resume: cases already in the CSV are never re-run; rows are appended as soon
// as a case finishes.
#include "BenchCommon.hpp"

#include <cstring>
#include <map>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <spawn.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach/mach.h>
#endif
extern char **environ;
#endif

namespace fs = tb::fs;

// --------------------------------------------------------------- memory probes
namespace mem
{
#if defined(_WIN32)
    static size_t currentRss()
    {
        PROCESS_MEMORY_COUNTERS pmc{};
        GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
        return pmc.WorkingSetSize;
    }
    static size_t peakRss()
    {
        PROCESS_MEMORY_COUNTERS pmc{};
        GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
        return pmc.PeakWorkingSetSize;
    }
    static void resetPeak() {}
#elif defined(__APPLE__)
    static bool info(mach_task_basic_info &i)
    {
        mach_msg_type_number_t n = MACH_TASK_BASIC_INFO_COUNT;
        return task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&i, &n) == KERN_SUCCESS;
    }
    static size_t currentRss()
    {
        mach_task_basic_info i{};
        return info(i) ? i.resident_size : 0;
    }
    static size_t peakRss()
    {
        mach_task_basic_info i{};
        return info(i) ? i.resident_size_max : 0;
    }
    static void resetPeak() {}
#else // Linux
    static size_t statusKb(const char *key)
    {
        std::ifstream in("/proc/self/status");
        std::string line;
        size_t klen = std::strlen(key);
        while (std::getline(in, line))
            if (line.compare(0, klen, key) == 0)
            {
                std::istringstream iss(line.substr(klen));
                size_t kb = 0;
                iss >> kb;
                return kb * 1024;
            }
        return 0;
    }
    static size_t currentRss() { return statusKb("VmRSS:"); }
    static size_t peakRss() { return statusKb("VmHWM:"); }
    static void resetPeak()
    {
        std::ofstream f("/proc/self/clear_refs");
        if (f.is_open())
            f << "5"; // resets VmHWM to the current RSS (Linux >= 4.0)
    }
#endif
} // namespace mem

// ------------------------------------------------------------------ worker mode
// argv: --worker <algo> <inputFile> <limit> <resultFile>
static int workerMain(char **argv)
{
    tb::Algo algo;
    if (!tb::parseAlgo(argv[2], algo))
        return 2;
    std::string path = argv[3];
    long long limit = std::stoll(argv[4]);
    std::string resultPath = argv[5];

    tb::InputGraph g = tb::readInput(path);
    if (g.vertexCount == 0)
        return 3;

    mem::resetPeak();
    size_t baseline = mem::currentRss();

    tb::GeneratorStats stats;
    bool hit;
    size_t peak;
    {
        tb::CoutSilencer silence;
        auto gt = tb::makeGraph(algo, g, limit);
        gt->generateAllTriangulations();
        peak = mem::peakRss(); // read before destruction (peak is monotonic anyway)
        stats = gt->stats();
        hit = tb::limitHit(stats, limit);
    }

    std::ofstream out(resultPath, std::ios::trunc);
    if (!out.is_open())
        return 4;
    out << "vertices=" << g.vertexCount << '\n'
        << "triangulations=" << stats.triangulations << '\n'
        << "status=" << tb::statusString(hit) << '\n'
        << "peakRssBytes=" << peak << '\n'
        << "baselineRssBytes=" << baseline << '\n'
        << "peakDeltaBytes=" << (peak > baseline ? peak - baseline : 0) << '\n'
        << "totalChecks=" << (stats.checksAvailable ? std::to_string(stats.totalChecks) : "") << '\n'
        << "successfulChecks=" << (stats.checksAvailable ? std::to_string(stats.successfulChecks) : "") << '\n'
        << "invalidTraversals=\n";
    return 0;
}

// ------------------------------------------------------------------ parent mode
static std::string selfExe(const char *argv0)
{
#ifdef __linux__
    std::error_code ec;
    fs::path p = fs::read_symlink("/proc/self/exe", ec);
    if (!ec)
        return p.string();
#endif
    return argv0;
}

// 0 = worker exited normally, otherwise failure. childMaxRss filled on POSIX.
static int spawnWorker(const std::string &exe, const tb::Case &c, const std::string &resultPath, size_t &childMaxRss)
{
    childMaxRss = 0;
#ifdef _WIN32
    std::string cmd = "\"\"" + exe + "\" --worker " + tb::algoName(c.algo) + " \"" + c.path + "\" " +
                      std::to_string(c.limit) + " \"" + resultPath + "\"\"";
    return std::system(cmd.c_str()) == 0 ? 0 : 1;
#else
    std::vector<std::string> a = {exe, "--worker", tb::algoName(c.algo), c.path, std::to_string(c.limit), resultPath};
    std::vector<char *> args;
    for (auto &s : a)
        args.push_back(const_cast<char *>(s.c_str()));
    args.push_back(nullptr);

    pid_t pid;
    if (posix_spawn(&pid, exe.c_str(), nullptr, nullptr, args.data(), environ) != 0)
        return -1;
    int status = 0;
    struct rusage ru{};
    if (wait4(pid, &status, 0, &ru) < 0)
        return -1;
#ifdef __APPLE__
    childMaxRss = (size_t)ru.ru_maxrss; // bytes
#else
    childMaxRss = (size_t)ru.ru_maxrss * 1024; // KiB
#endif
    return (WIFEXITED(status) && WEXITSTATUS(status) == 0) ? 0 : 1;
#endif
}

static std::map<std::string, std::string> readKeyValues(const std::string &path)
{
    std::map<std::string, std::string> m;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line))
    {
        auto eq = line.find('=');
        if (eq != std::string::npos)
            m[line.substr(0, eq)] = line.substr(eq + 1);
    }
    return m;
}

static std::string humanBytes(double b)
{
    const char *u[] = {"B", "KiB", "MiB", "GiB", "TiB"};
    int i = 0;
    while (b >= 1024.0 && i < 4)
    {
        b /= 1024.0;
        i++;
    }
    return tb::fmtD(b, 2) + " " + u[i];
}

static const char *kHeader =
    "algorithm,category,filename,limit,triangulationLimit,vertices,triangulations,status,"
    "peakDeltaBytes,peakDeltaMiB,peakRssBytes,baselineRssBytes,childMaxRssBytes,peakDeltaBytesPerVertex,"
    "totalChecks,successfulChecks,invalidTraversals,timestamp";

int main(int argc, char **argv)
{
    if (argc == 6 && std::string(argv[1]) == "--worker")
        return workerMain(argv);

    tb::Config cfg;
    // Code-configured run settings. Edit these values for repeatable runs;
    // matching --tri_* command-line flags still take precedence.
    cfg.inputRoot = "input";
    cfg.algos = {tb::Algo::BiconnectedWithoutVGS, tb::Algo::BiconnectedWithVGS, tb::Algo::Oneconnected};
    cfg.limits = {10, 100, 1000, 10000, 100000, 1000000};
    cfg.csv = "benchmark-results/results_memory.csv";
    if (!tb::parseConfig(argc, argv, cfg, "benchmark-results/results_memory.csv"))
        return 1;
    if (argc > 1)
    {
        std::cerr << "Unrecognized argument: " << argv[1] << " (see --tri_help)\n";
        return 1;
    }
    tb::pinToCpu(cfg.cpu);

    auto cases = tb::planCases(cfg);
    if (cases.empty())
    {
        std::cerr << "Nothing to do - all requested cases are already completed.\n";
        return 0;
    }

    const std::string exe = selfExe(argv[0]);
    size_t n = 0;
    for (const tb::Case &c : cases)
    {
        ++n;
        std::cout << "[" << n << "/" << cases.size() << "] " << tb::algoName(c.algo) << "/" << c.category << "/" << c.file
                  << " limit=" << c.limit << " ... " << std::flush;
        std::error_code ec;
        fs::path result = fs::temp_directory_path() /
                          ("tri_mem_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".txt");
        size_t childMax = 0;
        int rc = spawnWorker(exe, c, result.string(), childMax);
        auto kv = readKeyValues(result.string());
        fs::remove(result, ec);

        if (rc != 0 || kv.find("peakDeltaBytes") == kv.end())
        {
            std::cout << "FAILED (worker crashed / killed / bad input); not recorded, will retry next run\n";
            continue;
        }
        auto num = [&](const char *k) -> unsigned long long
        { return std::stoull(kv[k]); };
        unsigned long long delta = num("peakDeltaBytes"), verts = num("vertices");

        std::string catCsv = tb::getCategoryCsvPath(cfg.csv, c.algo, c.category);
        std::vector<std::string> fields = {
            tb::algoName(c.algo), c.category, c.file, std::to_string(c.limit), std::to_string(c.limit),
            std::to_string(verts), kv["triangulations"], kv["status"],
            std::to_string(delta), tb::fmtD(delta / 1048576.0, 4),
            kv["peakRssBytes"], kv["baselineRssBytes"], std::to_string(childMax),
            tb::fmtD(verts ? (double)delta / verts : 0.0, 3),
            kv["totalChecks"], kv["successfulChecks"], kv["invalidTraversals"], tb::nowString()};
        tb::appendCsvRow(catCsv, kHeader, fields);
        std::cout << "CSV append [" << catCsv << "]: " << tb::csvRow(fields) << '\n';
        std::cout << kv["status"] << ", " << kv["triangulations"] << " triangulations, peak +"
                  << humanBytes((double)delta) << " (RSS peak " << humanBytes((double)num("peakRssBytes")) << ")\n";
    }
    return 0;
}
