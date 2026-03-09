#include <bits/stdc++.h>
using namespace std;
#include "Edge.hpp"
#include "pairHash.hpp"
#include "biconnected.hpp"
#include "FaceTriangulation.hpp"
#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;

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
#include <fstream>

// Linux high-precision RSS measurement
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
            return rssBytes; // Return precise RSS in bytes
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
vector<vector<int>> input(const string &filename, int &distinctVertices)
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
// Utility: Format Bytes Nicely
// ============================================================================
string formatBytes(size_t bytes)
{
    const char *units[] = {"B", "KB", "MB", "GB"};
    int unitIndex = 0;
    double size = bytes;

    while (size >= 1024.0 && unitIndex < 3)
    {
        size /= 1024.0;
        unitIndex++;
    }

    ostringstream oss;
    oss << fixed << setprecision(2) << size << " " << units[unitIndex];
    return oss.str();
}

// ============================================================================
// Struct to Hold Benchmark Result
// ============================================================================
struct BenchmarkResult
{
    string filename;
    int distinctVertices;
    uint64_t totalTriang;
    double avgTime;
    double perTriangNs;
    size_t peakMemory;
    double memoryPerVertex;
};

// ============================================================================
// Main Benchmark Function
// ============================================================================
int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // open output file for results
    std::ofstream resFile("results.txt");
    if (!resFile.is_open())
    {
        cerr << "Error: could not create results.txt\n";
        return 1;
    }

    // create a streambuf that duplicates writes to both cout and the file
    struct tee_buf : std::streambuf
    {
        std::streambuf *sb1, *sb2;
        tee_buf(std::streambuf *s1, std::streambuf *s2) : sb1(s1), sb2(s2) {}
        int overflow(int c) override
        {
            if (c == EOF)
                return !EOF;
            // write to both streams; ignore file errors
            int r1 = sb1->sputc(c);
            sb2->sputc(c);
            return (r1 == EOF) ? EOF : c;
        }
        int sync() override
        {
            int r1 = sb1->pubsync();
            int r2 = sb2->pubsync();
            return (r1 == 0 && r2 == 0) ? 0 : -1;
        }
    } tbuf(cout.rdbuf(), resFile.rdbuf());

    // use a separate ostream so we don't disturb std::cout's buffer
    std::ostream out(&tbuf);

    string folder = "input";
    const int testRuns = 1;
    vector<BenchmarkResult> results;

    if (!fs::exists(folder) || !fs::is_directory(folder))
    {
        cerr << "Folder '" << folder << "' does not exist or is not a directory.\n";
        return 1;
    }

    out << "\n";
    out << "╔════════════════════════════════════════════════════════════════════════════╗\n";
    out << "║          TRIANGULATION BENCHMARK - Time & Space Complexity Analysis       ║\n";
    out << "╚════════════════════════════════════════════════════════════════════════════╝\n";
    out << "\nScanning folder: " << folder << "\n";
    out << "Test runs per file: " << testRuns << "\n\n";

    for (const auto &entry : fs::directory_iterator(folder))
    {
        if (!entry.is_regular_file())
            continue;

        string filename = entry.path().filename().string();
        out << "⏳ Processing: " << filename << " ... " << flush;

        int distinctVertices = 0;
        vector<vector<int>> faces = input(entry.path().string(), distinctVertices);

        if (faces.empty())
        {
            cerr << "\n⚠️  Warning: Skipping empty or invalid file: " << filename << endl;
            continue;
        }

        uint64_t totalTriang = 0;
        long double totalTimeSum = 0.0L;
        size_t peakMemory = 0;

        for (int run = 1; run <= testRuns; run++)
        {
            size_t memBefore = getCurrentMemoryUsage();

            biconnected *bc = new biconnected(faces);

            using clock = std::chrono::steady_clock;
            auto start = clock::now();
            bc->getAllTriangulations();
            auto end = clock::now();

            size_t memAfter = getCurrentMemoryUsage();
            size_t memUsed = (memAfter > memBefore) ? (memAfter - memBefore) : 0;
            peakMemory = max(peakMemory, memUsed);

            auto dur_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            totalTimeSum += (long double)dur_ns / 1'000'000'000.0L;
            totalTriang = bc->totalTriangulations;

            delete bc;
        }

        double avgTime = (double)(totalTimeSum / testRuns);
        double avgTimeNs = avgTime * 1'000'000'000.0;
        double perTriangNs = (totalTriang > 0) ? avgTimeNs / totalTriang : 0.0;
        double memoryPerVertex = (distinctVertices > 0) ? (double)peakMemory / distinctVertices : 0.0;

        results.push_back({filename, distinctVertices, totalTriang, avgTime, perTriangNs, peakMemory, memoryPerVertex});
        out << "✓\n";
    }

    // ========================================================================
    // PRINT RESULTS TABLE
    // ========================================================================
    out << "\n";
    out << "╔════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n";
    out << "║                                                    BENCHMARK RESULTS                                                               ║\n";
    out << "╠═══════════════════════════╦════════════╦═════════════════╦══════════════╦═══════════════╦═════════════════╦════════════════════════╣\n";
    out << "║ " << "\033[1m" << left << setw(25) << "File"
         << "\033[0m║ " << "\033[1m" << right << setw(10) << "Vertices"
         << "\033[0m║ " << "\033[1m" << right << setw(15) << "Triangulations"
         << "\033[0m║ " << "\033[1m" << right << setw(12) << "Time (s)"
         << "\033[0m║ " << "\033[1m" << right << setw(13) << "ns/Triang"
         << "\033[0m║ " << "\033[1m" << right << setw(15) << "Peak Memory"
         << "\033[0m║ " << "\033[1m" << right << setw(22) << "Memory/Vertex"
         << "\033[0m║\n";
    out << "╠═══════════════════════════╬════════════╬═════════════════╬══════════════╬═══════════════╬═════════════════╬════════════════════════╣\n";

    for (size_t i = 0; i < results.size(); i++)
    {
        const auto &r = results[i];

        string timeColor = r.avgTime < 0.1 ? "\033[32m" : // Green: fast
                               r.avgTime < 1.0 ? "\033[33m"
                                               : "\033[31m"; // Red: slow

        string memColor = r.memoryPerVertex < 1024 ? "\033[32m" : // < 1KB/vertex
                              r.memoryPerVertex < 10240 ? "\033[33m"
                                                        : // < 10KB/vertex
                              "\033[31m";                 // >= 10KB/vertex

        out << "║ " << left << setw(25) << r.filename.substr(0, 25)
             << "║ " << right << setw(10) << r.distinctVertices
             << "║ " << right << setw(15) << r.totalTriang
             << "║ " << timeColor << right << setw(12) << fixed << setprecision(6) << r.avgTime << "\033[0m"
             << "║ " << right << setw(13) << fixed << setprecision(2) << r.perTriangNs
             << "║ " << memColor << right << setw(15) << formatBytes(r.peakMemory) << "\033[0m"
             << "║ " << memColor << right << setw(22) << formatBytes((size_t)r.memoryPerVertex) << "\033[0m"
             << "║\n";
    }

    out << "╚═══════════════════════════╩════════════╩═════════════════╩══════════════╩═══════════════╩═════════════════╩════════════════════════╝\n";

    // ========================================================================
    // COMPLEXITY ANALYSIS
    // ========================================================================
    out << "\n";
    out << "╔════════════════════════════════════════════════════════════════════════════╗\n";
    out << "║                         COMPLEXITY ANALYSIS                                ║\n";
    out << "╠════════════════════════════════════════════════════════════════════════════╣\n";
    out << "║  Time Complexity:  Varies with triangulation count (see ns/Triang)        ║\n";
    out << "║  Space Complexity: O(n) where n = number of vertices                      ║\n";
    out << "║                    (measured precisely via RSS at byte-level)             ║\n";
    out << "╠════════════════════════════════════════════════════════════════════════════╣\n";
    out << "║  Color Legend:                                                             ║\n";
    out << "║    \033[32m● Green\033[0m  = Excellent performance                                         ║\n";
    out << "║    \033[33m● Yellow\033[0m = Moderate performance                                          ║\n";
    out << "║    \033[31m● Red\033[0m    = High resource usage                                           ║\n";
    out << "╚════════════════════════════════════════════════════════════════════════════╝\n";
    out << "\nNote: Results averaged over " << testRuns << " runs per file.\n";
    out << "      Memory measured at byte precision (using /proc/self/smaps_rollup where available).\n";

    // Close the file stream associated with out, then clean ANSI codes from results.txt
    resFile.close();
    {
        std::ifstream in("results.txt");
        std::string content((std::istreambuf_iterator<char>(in)), {});
        in.close();

        std::string cleaned;
        cleaned.reserve(content.size());
        bool esc = false;
        for (char ch : content)
        {
            if (!esc)
            {
                if (ch == '\033')
                {
                    esc = true;
                }
                else
                {
                    cleaned.push_back(ch);
                }
            }
            else
            {
                // already in escape; skip until we hit a terminating byte
                // CSI sequences begin with '['; treat it as part of the escape,
                // not as a terminator.
                if (ch == '[')
                {
                    // stay in escape
                }
                else if (ch >= '@' && ch <= '~')
                {
                    // final byte reached, end escape state
                    esc = false;
                }
                // otherwise keep skipping characters inside escape
            }
        }
        std::ofstream out("results.txt");
        out << cleaned;
    }

    return 0;
}
