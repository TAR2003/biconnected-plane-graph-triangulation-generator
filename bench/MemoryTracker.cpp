#include "MemoryTracker.hpp"

#include <cstdlib>
#include <new>

#if defined(_WIN32)
#include <malloc.h>
static inline std::size_t usableSize(void *p) { return _msize(p); }
#elif defined(__APPLE__)
#include <malloc/malloc.h>
static inline std::size_t usableSize(void *p) { return malloc_size(p); }
#else
#include <malloc.h>
static inline std::size_t usableSize(void *p) { return malloc_usable_size(p); }
#endif

namespace
{
    // Thread-local: only the benchmark thread is measured, and no atomics are
    // needed on the allocation hot path.
    thread_local long long g_live = 0; // live heap bytes
    thread_local long long g_peak = 0; // max of g_live within the window
    thread_local long long g_base = 0; // g_live when the window opened

    inline void onAlloc(std::size_t n) noexcept
    {
        g_live += (long long)n;
        if (g_live > g_peak)
            g_peak = g_live;
    }
    inline void onFree(std::size_t n) noexcept { g_live -= (long long)n; }
}

namespace memtrack
{
    void beginWindow() noexcept
    {
        g_base = g_live;
        g_peak = g_live;
    }
    std::size_t peakBytes() noexcept { return (std::size_t)(g_peak - g_base); }
}

// Sizes come from the allocator itself (no per-block header), so the figure
// includes allocator rounding and adds no memory or cache overhead.
void *operator new(std::size_t n)
{
    void *p = std::malloc(n ? n : 1);
    if (!p)
        throw std::bad_alloc();
    onAlloc(usableSize(p));
    return p;
}

void operator delete(void *p) noexcept
{
    if (!p)
        return;
    onFree(usableSize(p));
    std::free(p);
}

void operator delete(void *p, std::size_t) noexcept { operator delete(p); }