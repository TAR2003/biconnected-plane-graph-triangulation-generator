#pragma once
// -----------------------------------------------------------------------
// Google Benchmark has NO built-in per-case timeout. If a triangulation
// count is combinatorially explosive for a given N, a benchmark case can
// simply run "forever" (or for hours), stalling your whole suite.
//
// Two strategies, pick based on what you need:
//
//   A) WATCHDOG THREAD (in-process): a background thread sleeps for the
//      timeout duration, then if the main work hasn't signalled
//      completion, it calls std::abort() (or exits with a distinct code).
//      Pro: simple, no process-management complexity.
//      Con: aborting mid-allocation can corrupt benchmark output for
//           OTHER cases if they share a process; also can't cleanly
//           "skip and continue" within a single benchmark binary run,
//           since abort() kills the whole process.
//
//   B) EXTERNAL PROCESS WRAPPER (recommended for a full sweep): run each
//      (algorithm, N) combination as a SEPARATE process invocation of the
//      benchmark binary (using --benchmark_filter to select just that
//      one case), wrapped in a shell-level timeout. This is the standard
//      approach for combinatorial-generation research code because it
//      lets you skip a hung case and keep collecting data for the rest
///     of the sweep. See run_sweep.sh below for this.
//
// This header implements (A) as an in-process safety net (useful when
// running interactively / during development), and run_sweep.sh
// implements (B) for actual data collection.
// -----------------------------------------------------------------------

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace timeoutguard {

class Watchdog {
public:
    explicit Watchdog(std::chrono::seconds timeout, const char* case_label)
        : timeout_(timeout), label_(case_label), done_(false) {
        thread_ = std::thread([this] { Run(); });
    }

    ~Watchdog() {
        done_.store(true, std::memory_order_relaxed);
        cv_.notify_all();
        if (thread_.joinable()) thread_.join();
    }

    Watchdog(const Watchdog&) = delete;
    Watchdog& operator=(const Watchdog&) = delete;

private:
    void Run() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!cv_.wait_for(lock, timeout_, [this] { return done_.load(); })) {
            // Timed out: the guarded work did not finish in time.
            std::fprintf(stderr,
                "\n[timeout_guard] Case '%s' exceeded %lld s -- aborting process.\n"
                "                Re-run this case alone with a longer --timeout,\n"
                "                or treat it as 'did not complete' in your results.\n",
                label_, static_cast<long long>(timeout_.count()));
            std::fflush(stderr);
            std::_Exit(124); // 124 mirrors the coreutils `timeout` exit code convention
        }
    }

    std::chrono::seconds timeout_;
    const char* label_;
    std::atomic<bool> done_;
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

} // namespace timeoutguard

// Usage inside a benchmark case:
//
//   for (auto _ : state) {
//       timeoutguard::Watchdog guard(std::chrono::seconds(30), "Biconnected/N=5000");
//       graph.getAllTriangulations();
//       // guard destructs here, cancelling the watchdog if we got this far in time
//   }
