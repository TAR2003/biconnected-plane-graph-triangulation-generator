// Individual benchmark: timestamp of every generated triangulation.
// One CSV per input file:  <output>/<Algo>/individual/<InputFamily>__<sub>__<sub>/<case>.csv
#include "Common.hpp"

#include "GraphTriangulation.hpp"

#include <memory>

using namespace bench;

namespace
{
    constexpr int kDefaultRuns = 1;
    constexpr long long kDefaultLimit = 10'000;

    struct TimedGraph
    {
        std::unique_ptr<GraphTriangulation> graph;
        GenerationTimeline *timeline;
    };

    TimedGraph makeGraph(Family algo, Faces &faces, long long limit)
    {
        if (algo == Family::Biconnected)
        {
            auto g = std::make_unique<GraphTriangulationBiconnectedIndividualPerformance>(faces, limit);
            auto *t = &g->timeline;
            return {std::move(g), t};
        }
        auto g = std::make_unique<GraphTriangulationOneconnectedIndividualPerformance>(faces, limit);
        auto *t = &g->timeline;
        return {std::move(g), t};
    }

    // Runs are appended in order, so the last row carries the highest run index.
    int completedRuns(const fs::path &csv, const Case &)
    {
        std::ifstream in(csv, std::ios::binary | std::ios::ate);
        if (!in)
            return 0;
        const std::streamoff size = in.tellg(), chunk = std::min<std::streamoff>(size, 4096);
        std::string tail((size_t)chunk, '\0');
        in.seekg(size - chunk);
        in.read(tail.data(), chunk);
        while (!tail.empty() && (tail.back() == '\n' || tail.back() == '\r'))
            tail.pop_back();
        const auto nl = tail.find_last_of('\n');
        const std::string last = nl == std::string::npos ? tail : tail.substr(nl + 1);
        return (!last.empty() && std::isdigit((unsigned char)last[0])) ? std::atoi(last.c_str()) : 0;
    }

    void runIndividual(benchmark::State &state, const Case &c, const fs::path &csv, const Config &cfg)
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
            auto [graph, timeline] = makeGraph(cfg.algo, faces, cfg.limit);

            timeline->start(cfg.limit);
            const auto t0 = Clock::now();
            graph->getAllTriangulations();
            const auto t1 = Clock::now();
            state.SetIterationTime(std::chrono::duration<double>(t1 - t0).count());

            const bool header = needsHeader(csv);
            std::ofstream out(csv, std::ios::app);
            if (header)
                out << "run,triangulation,cumulativeNs,deltaNs\n";
            long long prev = 0;
            for (size_t i = 0; i < timeline->timesNs.size(); ++i)
            {
                const long long t = timeline->timesNs[i];
                out << runIndex << ',' << i + 1 << ',' << t << ',' << t - prev << '\n';
                prev = t;
            }
            state.counters["triangulations"] = (double)timeline->timesNs.size();
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
        cfg, "individual/",
        [&](const Case &c)
        { return cfg.resultsDir() / "individual" / c.folderId / (c.stem() + ".csv"); },
        completedRuns,
        [&](benchmark::State &s, const Case &c, const fs::path &csv)
        { runIndividual(s, c, csv, cfg); });

    if (registered)
        benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}