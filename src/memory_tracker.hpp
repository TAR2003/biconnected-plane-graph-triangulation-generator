#pragma once
// -----------------------------------------------------------------------
// Memory measurement for benchmark cases.
//
// Google Benchmark has NO built-in memory measurement. You get two
// independent signals here, both cheap enough to not distort timing much:
//
//   1. Peak RSS (resident set size) via getrusage(RUSAGE_SELF) -- this is
//      the OS's own bookkeeping of the process's high-water-mark physical
//      memory. Linux-only nuance: ru_maxrss is already a MONOTONIC peak
//      for the life of the process, so to get a PER-BENCHMARK-CASE delta
//      you snapshot before and after and also snapshot right before the
//      call to catch cases where memory is freed before you measure again.
//
//   2. Live allocation byte-count via malloc hook counters -- this tells
//      you how many bytes your code asked the allocator for during the
//      case, independent of whether the OS decided to actually reclaim
//      pages afterward (RSS can under-report short-lived spikes because
//      glibc doesn't always return memory to the OS immediately, and can
//      over-report because RSS includes stack/other threads/etc).
//
// Use both. RSS tells you "did this actually cost physical memory";
// the allocation counter tells you "how much churn did the algorithm
// generate," which is what you want when Edge objects are being
// new'd/deleted at high frequency (see the pooling discussion below).
// -----------------------------------------------------------------------

#include <cstdint>
#include <cstddef>

namespace membench {

struct MemorySnapshot {
    long peak_rss_kb = 0;        // getrusage ru_maxrss, kilobytes, monotonic high-water-mark
    uint64_t bytes_allocated = 0; // cumulative bytes passed to malloc since program start
    uint64_t alloc_count = 0;     // number of malloc calls since program start
};

// Call at the start of a benchmark case.
MemorySnapshot TakeSnapshot();

// diff = after - before, giving you a per-case delta. peak_rss_kb in the
// diff is NOT a true delta (ru_maxrss can't be reset mid-process on Linux
// without extra syscalls) -- it is "after.peak_rss_kb", i.e. the
// high-water-mark AS OF the end of this case, which in practice is what
// you want: the worst physical-memory footprint your process ever hit up
// to and including this case.
struct MemoryDelta {
    long peak_rss_kb_at_end = 0;
    uint64_t bytes_allocated_during = 0;
    uint64_t alloc_count_during = 0;
};

MemoryDelta Diff(const MemorySnapshot& before, const MemorySnapshot& after);

// Resets the cumulative allocation counters (NOT peak RSS, which the OS
// won't let you reset). Call this once per benchmark iteration if you
// want per-iteration allocation counts rather than cumulative-since-start.
void ResetAllocationCounters();

} // namespace membench
