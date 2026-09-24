#pragma once
#include <cstddef>

// Per-run peak heap measurement.
//
// OS-level peak RSS is monotonic for the life of the process, so it would report
// "peak so far" rather than "peak of this run". Instead, MemoryTracker.cpp
// replaces the global operator new/delete and tracks live heap bytes on the
// calling thread, which lets every run open its own fresh window.
namespace memtrack
{
    // Opens a fresh window on the calling thread (discards any previous window).
    void beginWindow() noexcept;

    // Peak live heap bytes, above the level at beginWindow(), reached since then.
    std::size_t peakBytes() noexcept;
}