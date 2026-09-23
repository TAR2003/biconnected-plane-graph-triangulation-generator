// =======================================================================
// Google Benchmark harness for the triangulation-generation algorithms.
//
// Build:
//   mkdir build && cd build
//   cmake .. -DCMAKE_BUILD_TYPE=Release
//   make -j
//
// Run everything:
//   ./triangulation_bench
//
// Run a subset (regex filter on benchmark name):
//   ./triangulation_bench --benchmark_filter=Biconnected
//
// Export machine-readable results (recommended for a research write-up):
//   ./triangulation_bench --benchmark_format=json --benchmark_out=results.json
//   ./triangulation_bench --benchmark_format=csv  --benchmark_out=results.csv
//
// Repeat each case N times for statistics (mean/median/stddev):
//   ./triangulation_bench --benchmark_repetitions=10 --benchmark_report_aggregates_only=true
//
// Control minimum measurement time per case (Benchmark auto-repeats a
// case internally until this elapsed; separate from --benchmark_repetitions
// above, which re-runs the whole already-converged case N more times):
//   ./triangulation_bench --benchmark_min_time=2s
// =======================================================================

#include <benchmark/benchmark.h>
#include <vector>
#include <chrono>

#include "memory_tracker.hpp"
#include "timeout_guard.hpp"
#include "graph_generator.hpp"

// Your algorithm headers -- adjust include path / filenames as needed.
#include "GraphTriangulation.hpp"
#include "GraphTriangulationTriconnected.hpp"

namespace {

// Wall-clock timeout applied to EVERY iteration of EVERY case in this
// binary. Override per-case if some cases legitimately need longer
// (e.g. large N is expected to be slow and you still want the data).
constexpr auto kDefaultTimeout = std::chrono::seconds(60);

// ---------------------------------------------------------------------
// Shared fixture: wires up memory + custom counters. All concrete
// benchmarks below derive from this so the reporting logic lives in
// exactly one place.
// ---------------------------------------------------------------------
class TriangulationFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State&) override {
        membench::ResetAllocationCounters();
        mem_before_ = membench::TakeSnapshot();
    }

    void TearDown(benchmark::State& state) override {
        auto mem_after = membench::TakeSnapshot();
        auto delta = membench::Diff(mem_before_, mem_after);

        // These show up as extra columns in the console/JSON/CSV output.
        state.counters["peak_rss_KB"] =
            static_cast<double>(delta.peak_rss_kb_at_end);
        state.counters["bytes_allocated"] = benchmark::Counter(
            static_cast<double>(delta.bytes_allocated_during),
            benchmark::Counter::kAvgIterations); // report per-iteration average
        state.counters["alloc_count"] = benchmark::Counter(
            static_cast<double>(delta.alloc_count_during),
            benchmark::Counter::kAvgIterations);
    }

private:
    membench::MemorySnapshot mem_before_;
};

} // namespace

// =======================================================================
// Biconnected face triangulation, single face, N vertices.
// This is where combinatorial blowup is most direct (Catalan-number
// growth in the triangulation count), so it's the case most likely to
// need the timeout guard as N grows.
// =======================================================================
BENCHMARK_DEFINE_F(TriangulationFixture, BiconnectedSingleFace)(benchmark::State& state) {
    const long long n = state.range(0);

    for (auto _ : state) {
        state.PauseTiming(); // don't count graph construction against algorithm time
        auto polygon = graphgen::SimplePolygon(n);
        std::vector<std::vector<long long>> faces{polygon};
        auto* graph = new GraphTriangulationBiconnectedPerformance(faces);
        state.ResumeTiming();

        {
            timeoutguard::Watchdog guard(kDefaultTimeout, "BiconnectedSingleFace");
            graph->getAllTriangulations();
        }

        state.PauseTiming();
        // Report the triangulation count as a custom counter (not per-
        // iteration averaged -- it's a property of the input, constant
        // across iterations, so just overwrite it each time).
        state.counters["triangulations_found"] =
            static_cast<double>(graph->totalTriangulations);
        delete graph;
        state.ResumeTiming();
    }

    // Lets Benchmark report items/sec = triangulations/sec, which is
    // usually the number you actually want in a paper, not raw time.
    state.SetComplexityN(n);
}
// Sweep N. Use ->Arg(x) for specific sizes, ->Range(lo, hi) for
// power-of-two-ish sweeps, or ->DenseRange(lo, hi, step) for a linear
// sweep -- linear is usually clearer for small N where you care about
// every point, e.g. 4..20.
BENCHMARK_REGISTER_F(TriangulationFixture, BiconnectedSingleFace)
    ->DenseRange(4, 16, 2)   // N = 4,6,8,...,16 -- keep the high end modest, see note below
    ->Unit(benchmark::kMillisecond)
    ->Complexity(); // fits a curve (e.g. O(N), O(N^2)) against your DenseRange points

// NOTE ON UPPER BOUND: the number of triangulations of a convex N-gon is
// the Catalan number C(N-2), which is already ~208012 at N=14 and grows
// ~4x per +2 in N. If GraphTriangulationBiconnectedPerformance still
// visits every triangulation even in "Performance" (discard) mode -- it
// does, storeTriangulation() is a no-op but generateAllTriangulations()
// still recurses into every one -- your wall-clock time scales with the
// Catalan number directly. Don't extend DenseRange much past ~20 without
// expecting multi-minute individual cases; use the external-process
// timeout wrapper (run_sweep.sh) rather than DenseRange for an
// exploratory "how far can this go" sweep.

// =======================================================================
// One-connected face triangulation, single face, N vertices.
// =======================================================================
BENCHMARK_DEFINE_F(TriangulationFixture, OneconnectedSingleFace)(benchmark::State& state) {
    const long long n = state.range(0);

    for (auto _ : state) {
        state.PauseTiming();
        auto polygon = graphgen::SimplePolygon(n);
        std::vector<std::vector<long long>> faces{polygon};
        auto* graph = new GraphTriangulationOneconnectedPerformance(faces);
        state.ResumeTiming();

        {
            timeoutguard::Watchdog guard(kDefaultTimeout, "OneconnectedSingleFace");
            graph->getAllTriangulations();
        }

        state.PauseTiming();
        state.counters["triangulations_found"] =
            static_cast<double>(graph->totalTriangulations);
        state.counters["total_checks"] = static_cast<double>(graph->totalChecks);
        state.counters["successful_checks"] = static_cast<double>(graph->successfulChecks);
        state.counters["invalid_traversals"] = static_cast<double>(graph->invalidTraversals);
        delete graph;
        state.ResumeTiming();
    }
    state.SetComplexityN(n);
}
BENCHMARK_REGISTER_F(TriangulationFixture, OneconnectedSingleFace)
    ->DenseRange(4, 16, 2)
    ->Unit(benchmark::kMillisecond)
    ->Complexity();

// =======================================================================
// Multi-face sweep: fan-of-faces, fixed vertices-per-face, growing face
// count. This stresses the GraphTriangulation-level orchestration
// (present-set sharing, recursive face-to-face dispatch in output())
// rather than a single face's combinatorics -- a different, and
// arguably more "real-world", scaling axis than single-face N.
// =======================================================================
BENCHMARK_DEFINE_F(TriangulationFixture, BiconnectedFanOfFaces)(benchmark::State& state) {
    const long long numFaces = state.range(0);
    constexpr long long kVerticesPerFace = 6; // kept small & fixed: isolates face-COUNT scaling

    for (auto _ : state) {
        state.PauseTiming();
        auto faces = graphgen::FanOfFaces(numFaces, kVerticesPerFace);
        auto* graph = new GraphTriangulationBiconnectedPerformance(faces);
        state.ResumeTiming();

        {
            timeoutguard::Watchdog guard(kDefaultTimeout, "BiconnectedFanOfFaces");
            graph->getAllTriangulations();
        }

        state.PauseTiming();
        state.counters["triangulations_found"] =
            static_cast<double>(graph->totalTriangulations);
        delete graph;
        state.ResumeTiming();
    }
    state.SetComplexityN(numFaces);
}
BENCHMARK_REGISTER_F(TriangulationFixture, BiconnectedFanOfFaces)
    ->RangeMultiplier(2)
    ->Range(1, 32)
    ->Unit(benchmark::kMillisecond)
    ->Complexity();

// =======================================================================
// Triconnected component benchmark -- different code path entirely
// (ParvezRahmanNakano per-face + DFS combination), worth its own sweep
// since the combination step (combineTriangulationsDFS) has its own
// scaling behavior separate from per-face generation.
// =======================================================================
BENCHMARK_DEFINE_F(TriangulationFixture, TriconnectedFanOfFaces)(benchmark::State& state) {
    const long long numFaces = state.range(0);
    constexpr long long kVerticesPerFace = 6;

    for (auto _ : state) {
        state.PauseTiming();
        auto faces = graphgen::FanOfFaces(numFaces, kVerticesPerFace);
        auto* graph = new GraphTriangulationTriconnected(faces);
        state.ResumeTiming();

        {
            timeoutguard::Watchdog guard(kDefaultTimeout, "TriconnectedFanOfFaces:generate");
            graph->getAllTriangulations();
        }
        {
            timeoutguard::Watchdog guard(kDefaultTimeout, "TriconnectedFanOfFaces:refine");
            graph->refineTriangulations();
        }

        state.PauseTiming();
        state.counters["triangulations_found"] =
            static_cast<double>(graph->allTriangulations.size());
        delete graph;
        state.ResumeTiming();
    }
    state.SetComplexityN(numFaces);
}
BENCHMARK_REGISTER_F(TriangulationFixture, TriconnectedFanOfFaces)
    ->RangeMultiplier(2)
    ->Range(1, 32)
    ->Unit(benchmark::kMillisecond)
    ->Complexity();

BENCHMARK_MAIN();
