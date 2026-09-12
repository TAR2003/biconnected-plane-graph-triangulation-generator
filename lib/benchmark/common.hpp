#pragma once

#include <bits/stdc++.h>
using namespace std;
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
