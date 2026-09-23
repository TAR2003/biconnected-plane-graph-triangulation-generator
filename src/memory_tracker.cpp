#include "memory_tracker.hpp"

#include <sys/resource.h>
#include <atomic>
#include <cstdlib>
#include <dlfcn.h>
#include <cstdio>

// -----------------------------------------------------------------------
// Allocation counting via malloc/free interposition.
//
// This overrides the global malloc/free using dlsym(RTLD_NEXT, ...) to
// find the real libc implementation, then wraps it. This works on
// glibc/Linux. It will NOT work as-is on macOS (needs malloc zones) or
// on MSVC (needs a different hooking mechanism, e.g. _CrtSetAllocHook or
// a custom operator new/delete override instead -- see the portable
// fallback note at the bottom of this file).
// -----------------------------------------------------------------------

namespace {
std::atomic<uint64_t> g_bytes_allocated{0};
std::atomic<uint64_t> g_alloc_count{0};
std::atomic<bool> g_tracking_enabled{false};

using MallocFn = void* (*)(size_t);
using FreeFn = void (*)(void*);

MallocFn real_malloc = nullptr;
FreeFn real_free = nullptr;

// We need to know how big each allocation was when free() is called, but
// libc doesn't give us that directly. We prepend a small header storing
// the size. This is the standard technique for malloc interposition.
struct AllocHeader {
    size_t size;
};

constexpr size_t kHeaderSize = sizeof(AllocHeader);

void EnsureRealFnsLoaded() {
    if (!real_malloc) {
        real_malloc = reinterpret_cast<MallocFn>(dlsym(RTLD_NEXT, "malloc"));
    }
    if (!real_free) {
        real_free = reinterpret_cast<FreeFn>(dlsym(RTLD_NEXT, "free"));
    }
}
} // namespace

extern "C" void* malloc(size_t size) {
    EnsureRealFnsLoaded();
    if (!real_malloc) return nullptr; // still bootstrapping dlsym itself

    void* raw = real_malloc(size + kHeaderSize);
    if (!raw) return nullptr;

    auto* header = reinterpret_cast<AllocHeader*>(raw);
    header->size = size;

    if (g_tracking_enabled.load(std::memory_order_relaxed)) {
        g_bytes_allocated.fetch_add(size, std::memory_order_relaxed);
        g_alloc_count.fetch_add(1, std::memory_order_relaxed);
    }

    return reinterpret_cast<char*>(raw) + kHeaderSize;
}

extern "C" void free(void* ptr) {
    if (!ptr) return;
    EnsureRealFnsLoaded();

    void* raw = reinterpret_cast<char*>(ptr) - kHeaderSize;
    real_free(raw);
}

namespace membench {

MemorySnapshot TakeSnapshot() {
    MemorySnapshot snap;

    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    snap.peak_rss_kb = usage.ru_maxrss; // already KB on Linux

    snap.bytes_allocated = g_bytes_allocated.load(std::memory_order_relaxed);
    snap.alloc_count = g_alloc_count.load(std::memory_order_relaxed);

    g_tracking_enabled.store(true, std::memory_order_relaxed);
    return snap;
}

MemoryDelta Diff(const MemorySnapshot& before, const MemorySnapshot& after) {
    MemoryDelta d;
    d.peak_rss_kb_at_end = after.peak_rss_kb;
    d.bytes_allocated_during = after.bytes_allocated - before.bytes_allocated;
    d.alloc_count_during = after.alloc_count - before.alloc_count;
    return d;
}

void ResetAllocationCounters() {
    g_bytes_allocated.store(0, std::memory_order_relaxed);
    g_alloc_count.store(0, std::memory_order_relaxed);
}

} // namespace membench

// -----------------------------------------------------------------------
// PORTABILITY NOTE:
// If malloc interposition via dlsym(RTLD_NEXT,...) doesn't work on your
// platform/toolchain (e.g. statically-linked binary, macOS, MSVC), the
// fallback is to override `operator new`/`operator delete` instead, which
// is portable C++ and doesn't need dlsym:
//
//   void* operator new(size_t size) {
//       void* p = std::malloc(size);
//       g_bytes_allocated += size;  g_alloc_count++;
//       return p;
//   }
//   void operator delete(void* p) noexcept { std::free(p); }
//
// This only catches C++ `new`/`delete` traffic, not raw malloc()/free()
// calls (e.g. inside STL allocators that call malloc directly on some
// platforms), so it under-counts slightly relative to the malloc-hook
// approach above, but it is far more portable. For this codebase (heavy
// `new Edge(...)` / `delete chord` traffic, and std::list/unordered_map
// node allocation which internally goes through operator new on most
// standard library implementations), operator-new overriding is usually
// good enough.
// -----------------------------------------------------------------------
