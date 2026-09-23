#pragma once
// =======================================================================
// Checkpoint writer for surviving SIGKILL.
//
// THE PROBLEM: when the benchmark harness times out a case, it sends
// SIGKILL to the child process running the algorithm. SIGKILL cannot be
// caught or handled -- the OS terminates the process immediately, with
// zero opportunity for that process to run any code first. This means
// the *parent* process can never read the child's in-memory stats
// (totalChecks, totalTriangulations, etc.) after a kill, because it
// never had access to that memory -- it's a separate address space.
// There is no way to "grab the stats right before killing" from outside
// the process being killed.
//
// THE FIX: have the CHILD itself periodically write its current stats
// to a small checkpoint file, in ordinary (non-signal-handler) code, at
// a point that already runs naturally during the algorithm -- here,
// inside GraphTriangulation::output(), which fires once per completed
// triangulation. If the process is later SIGKILLed, the checkpoint file
// still exists on disk with the last values written before the kill.
// The PARENT process, on detecting a timeout, reads this checkpoint
// file instead of writing an all-zero row.
//
// This gives you "stats as of the last checkpoint," not "stats at the
// exact instant of the kill" -- there's a small gap bounded by how often
// you checkpoint (see kCheckpointEveryN below). That gap is the
// necessary tradeoff for not having to touch the recursive algorithm's
// control flow or deal with signal-handler-safety at all.
// =======================================================================

#include <atomic>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

namespace checkpoint {

struct Stats {
    long long vertices = 0;
    long long triangulations = 0;
    long long totalChecks = 0;
    long long successfulChecks = 0;
    long long invalidTraversals = 0;
    long peakMemoryKb = 0;
    double elapsedSeconds = 0.0;
};

class Writer {
public:
    // path: where the checkpoint file lives. Typically a per-case temp
    // file the parent knows how to find (see dataset_bench_main.cpp's
    // checkpoint_path() helper) -- NOT the final CSV, just scratch state.
    // everyN: write to disk every N calls to Tick(), not every single
    // one -- checkpointing on every triangulation would add an fopen+
    // fwrite+fclose per triangulation, which is real overhead at
    // millions/sec throughput. Every 1000-10000 is a reasonable default:
    // frequent enough that a killed run only loses a fraction of a
    // second of progress, infrequent enough to not show up in profiling.
    Writer(std::filesystem::path path, uint64_t everyN = 2000)
        : path_(std::move(path)), everyN_(everyN) {}

    // Call this from a point that runs naturally and often during the
    // algorithm (GraphTriangulation::output() is exactly this point --
    // ordinary function-call code, not a signal handler, so plain
    // ofstream I/O here is completely safe).
    void Tick(const Stats& current) {
        if (++counter_ % everyN_ != 0) return;
        WriteNow(current);
    }

    // Always call this once, unconditionally, right before the process
    // would normally exit successfully -- so the checkpoint file reflects
    // "process finished cleanly" and a stale partial checkpoint from an
    // EARLIER attempt at this same case-file doesn't get misread as this
    // run's data if you rerun the same case later.
    void WriteFinal(const Stats& current) {
        WriteNow(current);
    }

    // Call on clean exit (not on kill, obviously -- this is for the
    // NORMAL completion path) to remove the scratch file so it doesn't
    // accumulate / doesn't get misread by a later unrelated run using
    // the same path.
    void Cleanup() {
        std::error_code ec;
        std::filesystem::remove(path_, ec); // ignore failure; best-effort
    }

private:
    void WriteNow(const Stats& s) {
        // Write to a temp file then rename -- atomic on POSIX (same
        // filesystem), so the parent NEVER observes a half-written
        // checkpoint file even if the kill lands mid-write.
        auto tmpPath = path_;
        tmpPath += ".tmp";

        std::ofstream out(tmpPath, std::ios::trunc);
        if (!out.is_open()) return; // best-effort; don't crash the algorithm over this
        out << "vertices=" << s.vertices << "\n"
            << "triangulations=" << s.triangulations << "\n"
            << "totalChecks=" << s.totalChecks << "\n"
            << "successfulChecks=" << s.successfulChecks << "\n"
            << "invalidTraversals=" << s.invalidTraversals << "\n"
            << "peakMemoryKb=" << s.peakMemoryKb << "\n"
            << "elapsedSeconds=" << s.elapsedSeconds << "\n";
        out.close();

        std::error_code ec;
        std::filesystem::rename(tmpPath, path_, ec);
        // if rename fails (e.g. cross-device), fall back to a direct
        // write; rare in practice since tmpPath and path_ share a parent
        if (ec) {
            std::ofstream direct(path_, std::ios::trunc);
            if (direct.is_open()) {
                direct << "vertices=" << s.vertices << "\n"
                       << "triangulations=" << s.triangulations << "\n"
                       << "totalChecks=" << s.totalChecks << "\n"
                       << "successfulChecks=" << s.successfulChecks << "\n"
                       << "invalidTraversals=" << s.invalidTraversals << "\n"
                       << "peakMemoryKb=" << s.peakMemoryKb << "\n"
                       << "elapsedSeconds=" << s.elapsedSeconds << "\n";
            }
        }
    }

    std::filesystem::path path_;
    uint64_t everyN_;
    uint64_t counter_ = 0;
};

// Reads a checkpoint file written by Writer. Returns false (and leaves
// out untouched) if the file doesn't exist or is malformed -- callers
// should treat that as "no partial data available, fall back to zeros."
inline bool Read(const std::filesystem::path& path, Stats& out) {
    std::ifstream in(path);
    if (!in.is_open()) return false;

    std::string line;
    while (std::getline(in, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        try {
            if (key == "vertices") out.vertices = std::stoll(value);
            else if (key == "triangulations") out.triangulations = std::stoll(value);
            else if (key == "totalChecks") out.totalChecks = std::stoll(value);
            else if (key == "successfulChecks") out.successfulChecks = std::stoll(value);
            else if (key == "invalidTraversals") out.invalidTraversals = std::stoll(value);
            else if (key == "peakMemoryKb") out.peakMemoryKb = std::stol(value);
            else if (key == "elapsedSeconds") out.elapsedSeconds = std::stod(value);
        } catch (...) {
            return false; // malformed value -- treat whole file as unusable
        }
    }
    return true;
}

} // namespace checkpoint
