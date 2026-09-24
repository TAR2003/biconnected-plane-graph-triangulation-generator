// Total-run benchmark: wall time of the full enumeration + peak heap of that run.
// One CSV per input folder:  <output>/<Algo>/total/<InputFamily>__<sub>__<sub>.csv
#include "Common.hpp"
#include "MemoryTracker.hpp"

#include "GraphTriangulation.hpp"

#include <iomanip>
#include <memory>

using namespace bench;

namespace
{
    constexpr int kDefaultRuns = 3;
    constexpr long long kDefaultLimit = 10'000'000;

    constexpr const char *kHeader =
        "filename,runIndex,vertices,triangulations,timeSeconds,peakMemoryBytes,memoryPerVertex,"
        "startTime,endTime,status,totalChecks,successfulChecks,failedChecks,checkSuccessRate,"
        "invalidTraversals,totalTraversalsExtended,traversalSuccessRate\n";

    std::unique_ptr<GraphTriangulation> makeGraph(Family algo, Faces &faces, long long limit)
    {
        if (algo == Family::Biconnected)
            return std::make_unique<GraphTriangulationBiconnectedPerformance>(faces, limit);
        return std::make_unique<GraphTriangulationOneconnectedPerformance>(faces, limit);
    }

    int completedRuns(const fs::path &csv, const Case &c)
    {
        std::ifstream in(csv);
        std::string line;
        std::getline(in, line); // header
        const std::string prefix = c.name() + ",";
        int n = 0;
        while (std::getline(in, line))
            if (line.rfind(prefix, 0) == 0)
                ++n;
        return n;
    }

    void runTotal(benchmark::State &state, const Case &c, const fs::path &csv, const Config &cfg)
    {
        Faces faces;
        long long vertices = 0;
        if (!readFaces(c.file, faces, vertices))
        {
            state.SkipWithError(("cannot read " + c.display).c_str());
            return;
        }

        for (auto _ : state)
        {
            const int runIndex = completedRuns(csv, c) + 1;
            const std::string startTs = timestamp();

            memtrack::beginWindow(); // window covers graph construction + enumeration
            auto graph = makeGraph(cfg.algo, faces, cfg.limit);

            const auto t0 = Clock::now();
            graph->getAllTriangulations();
            const auto t1 = Clock::now();

            const std::size_t peak = memtrack::peakBytes();
            const std::string endTs = timestamp();
            benchmark::DoNotOptimize(graph->totalTriangulations);

            const double seconds = std::chrono::duration<double>(t1 - t0).count();
            state.SetIterationTime(seconds);

            const long long tri = graph->totalTriangulations;
            const long long checks = graph->totalChecks, okChecks = graph->successfulChecks;
            const long long invalid = graph->invalidTraversals, traversals = invalid + tri;

            const bool header = needsHeader(csv);
            std::ofstream out(csv, std::ios::app);
            if (header)
                out << kHeader;
            out << c.name() << ',' << runIndex << ',' << vertices << ',' << tri << ','
                << std::fixed << std::setprecision(9) << seconds << ','
                << peak << ',' << std::setprecision(6) << (vertices ? (double)peak / vertices : 0.0) << ','
                << startTs << ',' << endTs << ",completed,"
                << checks << ',' << okChecks << ',' << checks - okChecks << ','
                << std::setprecision(2) << (checks ? 100.0 * okChecks / checks : 0.0) << ','
                << invalid << ',' << traversals << ','
                << (traversals ? 100.0 * tri / traversals : 0.0) << '\n';

            state.counters["triangulations"] = (double)tri;
            state.counters["peak_bytes"] = (double)peak;
        }
    }
}

int main(int argc, char **argv)
{
    benchmark::Initialize(&argc, argv);
    Config cfg(kDefaultRuns, kDefaultLimit);
    if (!parseArgs(argc, argv, cfg))
        return 1;

    const size_t registered = registerCases(
        cfg, "total/",
        [&](const Case &c)
        { return cfg.resultsDir() / "total" / (c.folderId + ".csv"); },
        completedRuns,
        [&](benchmark::State &s, const Case &c, const fs::path &csv)
        { runTotal(s, c, csv, cfg); });

    if (registered)
        benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}