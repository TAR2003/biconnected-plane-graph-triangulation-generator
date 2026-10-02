// bench_time.cpp - CPU-time benchmark of the three algorithms using Google Benchmark.
//
// * One Google Benchmark per (limit, algorithm, category, file) case.
// * Iterations(1): each case is a single, complete enumeration (it is far too
//   long/heavy to be repeated); Google Benchmark's own timers measure it.
// * CPU time (not wall time) is what ends up in the CSV as cpuTimeSeconds.
//   Wall time is also stored (realTimeSeconds) only for reference.
// * Timed region = construct GraphTriangulation (rotation system -> faces)
//   + getAllTriangulations(). Input parsing and object destruction are NOT timed.
// * Memory is deliberately NOT measured here (see bench_memory.cpp).
// * Every finished case is appended to the CSV immediately; on start-up all
//   cases already present in the CSV are not even registered.
#include <benchmark/benchmark.h>

#include "BenchCommon.hpp"

#include <unordered_map>

static std::unordered_map<std::string, tb::Case> g_cases; // benchmark name -> case

static const char *kHeader =
    "algorithm,category,filename,limit,vertices,cpuTimeSeconds,realTimeSeconds,triangulations,status,"
    "totalChecks,successfulChecks,failedChecks,checkSuccessRate,invalidTraversals,timestamp";

static void runCase(benchmark::State &state, const tb::Case &c)
{
    tb::InputGraph g = tb::readInput(c.path); // untimed: before the loop
    if (g.vertexCount == 0)
    {
        state.SkipWithError("invalid or empty input file");
        return;
    }

    std::unique_ptr<GraphTriangulation> gt;
    for (auto _ : state) // <- the only timed region
    {
        gt = tb::makeGraph(c.algo, g, c.limit);
        gt->getAllTriangulations();
        benchmark::ClobberMemory();
    }
    // Timers are stopped here; counters and destruction are not measured.
    state.counters["vertices"] = (double)g.vertexCount;
    state.counters["triangulations"] = (double)gt->totalTriangulations;
    state.counters["limit_hit"] = tb::limitHit(*gt, c.limit) ? 1.0 : 0.0;
    state.counters["totalChecks"] = (double)gt->totalChecks;
    state.counters["successfulChecks"] = (double)gt->successfulChecks;
    state.counters["invalidTraversals"] = (double)gt->invalidTraversals;
}

// Console output as usual + one CSV row appended per finished benchmark.
class CsvAppendReporter : public benchmark::ConsoleReporter
{
public:
    explicit CsvAppendReporter(std::string csv) : csv_(std::move(csv)) {}

    void ReportRuns(const std::vector<Run> &runs) override
    {
        ConsoleReporter::ReportRuns(runs);
        for (const Run &r : runs)
        {
            if (r.run_type != Run::RT_Iteration || r.skipped)
                continue; // aggregates / failed runs are not recorded (failed ones are retried next time)
            auto it = g_cases.find(r.run_name.function_name);
            if (it == g_cases.end())
                continue;
            const tb::Case &c = it->second;
            auto cnt = [&](const char *n) -> long long
            {
                auto f = r.counters.find(n);
                return f == r.counters.end() ? 0 : (long long)f->second.value;
            };
            double iters = r.iterations > 0 ? (double)r.iterations : 1.0;
            long long total = cnt("totalChecks"), ok = cnt("successfulChecks");
            std::string catCsv = tb::getCategoryCsvPath(csv_, c.algo, c.category);
            std::vector<std::string> fields = {
                tb::algoName(c.algo), c.category, c.file, std::to_string(c.limit),
                std::to_string(cnt("vertices")),
                tb::fmtD(r.cpu_accumulated_time / iters, 9),
                tb::fmtD(r.real_accumulated_time / iters, 9),
                std::to_string(cnt("triangulations")),
                tb::statusString(cnt("limit_hit") != 0),
                std::to_string(total), std::to_string(ok), std::to_string(total - ok),
                tb::fmtD(total > 0 ? 100.0 * ok / total : 0.0, 2),
                std::to_string(cnt("invalidTraversals")), tb::nowString()};
            tb::appendCsvRow(catCsv, kHeader, fields);
            std::cout << "CSV append [" << catCsv << "]: " << tb::csvRow(fields) << '\n';
        }
    }

private:
    std::string csv_;
};

int main(int argc, char **argv)
{
    benchmark::Initialize(&argc, argv); // consumes --benchmark_* flags
    tb::Config cfg;
    if (!tb::parseConfig(argc, argv, cfg, "benchmark-results/results_time.csv")) // consumes --tri_* flags
        return 1;
    if (argc > 1 && benchmark::ReportUnrecognizedArguments(argc, argv))
        return 1;

    tb::pinToCpu(cfg.cpu);

    auto cases = tb::planCases(cfg);
    if (cases.empty())
    {
        std::cerr << "Nothing to do - all requested cases are already completed.\n";
        return 0;
    }
    for (const tb::Case &c : cases)
    {
        std::string name = std::string(tb::algoName(c.algo)) + "/" + c.category + "/" + c.file +
                           "/limit:" + std::to_string(c.limit);
        g_cases.emplace(name, c);
        benchmark::RegisterBenchmark(name, [c](benchmark::State &st)
                                     { runCase(st, c); })
            ->Iterations(1)
            ->Repetitions(1)
            ->MeasureProcessCPUTime() // CPU time of the whole process, never wall time
            ->Unit(benchmark::kMillisecond);
    }

    CsvAppendReporter reporter(cfg.csv);
    benchmark::RunSpecifiedBenchmarks(&reporter);
    benchmark::Shutdown();
    return 0;
}
