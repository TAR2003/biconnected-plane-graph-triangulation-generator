// bench_time.cpp - CPU-time benchmark of the three algorithms using Google Benchmark.
//
// * One Google Benchmark per (limit, algorithm, category, file) case.
// * Iterations(1): each case is a single, complete enumeration (it is far too
//   long/heavy to be repeated).
// * The only time written to the CSV is cpuTimeSeconds: Google Benchmark's
//   process CPU time (user + kernel, all threads), in seconds. MeasureProcessCPUTime
//   selects that clock; the value is Run::cpu_accumulated_time.
// * The timed region is two phases and nothing else: constructing the generator
//   (rotation system to faces and the rest of the class), then
//   generateAllTriangulations(). Those phases are not stored as their own columns.
//   Input parsing and object destruction stay outside the timer.
// * One input file at a time: parse it, run every algorithm and limit for that
//   file, then drop it. The full suite is never registered or loaded up front.
// * Memory is deliberately NOT measured here (see bench_memory.cpp).
// * Every finished case is appended to the CSV immediately; on start-up all
//   cases already present in the CSV are not even registered.
#include <benchmark/benchmark.h>

#include "BenchCommon.hpp"

#include <unordered_map>

static std::unordered_map<std::string, tb::Case> g_cases; // benchmark name -> case

// Graph for the file currently being benchmarked. The next file replaces it.
static tb::InputGraph g_loaded;

static const char *kHeader =
    "algorithm,category,filename,limit,triangulationLimit,vertices,cpuTimeSeconds,triangulations,status,"
    "totalChecks,successfulChecks,unsuccessfulChecks,checkSuccessRate,"
    "totalTraversals,successfulTraversals,unsuccessfulTraversals,traversalSuccessRate,timestamp";

static void runCase(benchmark::State &state, const tb::Case &c)
{
    // Parsed already, just before this file's benchmarks were started.
    const tb::InputGraph &g = g_loaded;
    std::cerr << ">> " << tb::algoName(c.algo) << "/" << c.category << "/" << c.file
              << " limit=" << c.limit << std::endl;
    if (g.vertexCount == 0)
    {
        state.SkipWithError("invalid or empty input file");
        return;
    }

    {
        tb::CoutSilencer silence;
        std::unique_ptr<tb::GeneratorRun> gt;
        for (auto _ : state) // build, then generation; Google Benchmark times both
        {
            gt = tb::makeGraph(c.algo, g, c.limit);
            gt->generateAllTriangulations();
            benchmark::ClobberMemory();
        }
        const tb::GeneratorStats stats = gt->stats();
        state.counters["vertices"] = (double)g.vertexCount;
        state.counters["triangulations"] = (double)stats.triangulations;
        state.counters["limit_hit"] = tb::limitHit(stats, c.limit) ? 1.0 : 0.0;
        if (stats.checksAvailable)
        {
            state.counters["totalChecks"] = (double)stats.totalChecks;
            state.counters["successfulChecks"] = (double)stats.successfulChecks;
        }
        if (stats.traversalsAvailable)
        {
            state.counters["totalTraversals"] = (double)stats.totalTraversals;
            state.counters["successfulTraversals"] = (double)stats.successfulTraversals;
        }
    }
    // Timers are stopped here; counters and destruction are not measured.
}

// Console output as usual + one CSV row appended per finished benchmark.
class CsvAppendReporter : public benchmark::ConsoleReporter
{
public:
    explicit CsvAppendReporter(std::string csv) : csv_(std::move(csv)) {}

    // RunSpecifiedBenchmarks is called once per file. Print the host banner once.
    bool ReportContext(const Context &context) override
    {
        if (reportedContext_)
            return true;
        reportedContext_ = true;
        return ConsoleReporter::ReportContext(context);
    }

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
            // cpu_accumulated_time is seconds. MeasureProcessCPUTime makes it
            // process CPU time of the iteration (build + generation).
            const double cpuSeconds = r.cpu_accumulated_time / iters;
            const bool checksAvailable = r.counters.find("totalChecks") != r.counters.end();
            const bool traversalsAvailable = r.counters.find("totalTraversals") != r.counters.end();
            long long total = cnt("totalChecks"), ok = cnt("successfulChecks");
            long long totalTraversals = cnt("totalTraversals"), successfulTraversals = cnt("successfulTraversals");
            std::string catCsv = tb::getCategoryCsvPath(csv_, c.algo, c.category);
            std::vector<std::string> fields = {
                tb::algoName(c.algo), c.category, c.file, std::to_string(c.limit), std::to_string(c.limit),
                std::to_string(cnt("vertices")),
                tb::fmtD(cpuSeconds, 9),
                std::to_string(cnt("triangulations")),
                tb::statusString(cnt("limit_hit") != 0),
                checksAvailable ? std::to_string(total) : "",
                checksAvailable ? std::to_string(ok) : "",
                checksAvailable ? std::to_string(total - ok) : "",
                checksAvailable ? tb::fmtD(total > 0 ? 100.0 * ok / total : 0.0, 2) : "",
                traversalsAvailable ? std::to_string(totalTraversals) : "",
                traversalsAvailable ? std::to_string(successfulTraversals) : "",
                traversalsAvailable ? std::to_string(totalTraversals - successfulTraversals) : "",
                traversalsAvailable ? tb::fmtD(totalTraversals > 0 ? 100.0 * successfulTraversals / totalTraversals : 0.0, 2) : "",
                tb::nowString()};
            tb::appendCsvRow(catCsv, kHeader, fields);
            std::cout << "CSV append [" << catCsv << "]: " << tb::csvRow(fields) << '\n';
        }
    }

private:
    std::string csv_;
    bool reportedContext_ = false;
};

int main(int argc, char **argv)
{
    benchmark::Initialize(&argc, argv); // consumes --benchmark_* flags
    tb::Config cfg;
    // Code-configured run settings. Edit these values for repeatable runs;
    // matching --tri_* command-line flags still take precedence.
    cfg.inputRoot = "input";
    cfg.algos = {tb::Algo::BiconnectedWithoutVGS, tb::Algo::BiconnectedWithVGS, tb::Algo::Oneconnected};
    cfg.limits = {10, 100, 1000, 10000, 100000, 1000000};
    cfg.csv = "benchmark-results/results_time.csv";
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
    // Group by input path so each file stays resident for all of its cases.
    // planCases walks limit-major; running that order would reload every file
    // once per limit. Within a file the original order (ascending limits) stays.
    std::vector<std::string> inputOrder;
    std::unordered_map<std::string, std::vector<size_t>> casesByInput;
    for (size_t i = 0; i < cases.size(); ++i)
    {
        const std::string &path = cases[i].path;
        auto it = casesByInput.find(path);
        if (it == casesByInput.end())
        {
            inputOrder.push_back(path);
            casesByInput[path].push_back(i);
        }
        else
            it->second.push_back(i);
    }
    std::cerr << "Distinct inputs: " << inputOrder.size()
              << ". Each file is parsed when its turn starts, then its cases run.\n";

    CsvAppendReporter reporter(cfg.csv);
    for (size_t fileIndex = 0; fileIndex < inputOrder.size(); ++fileIndex)
    {
        const std::string &path = inputOrder[fileIndex];
        const std::vector<size_t> &indexes = casesByInput.find(path)->second;
        std::cerr << "[" << (fileIndex + 1) << "/" << inputOrder.size() << "] parsing " << path
                  << " (" << indexes.size() << " cases)" << std::endl;
        g_loaded = tb::readInput(path);
        if (g_loaded.vertexCount == 0)
        {
            std::cerr << "  invalid or empty input, skipping" << std::endl;
            continue;
        }

        g_cases.clear();
        for (size_t i : indexes)
        {
            const tb::Case &c = cases[i];
            std::string name = std::string(tb::algoName(c.algo)) + "/" + c.category + "/" + c.file +
                               "/limit:" + std::to_string(c.limit);
            g_cases.emplace(name, c);
            benchmark::RegisterBenchmark(name, [c](benchmark::State &st)
                                         { runCase(st, c); })
                ->Iterations(1)
                ->Repetitions(1)
                ->MeasureProcessCPUTime() // process CPU time, reported below in seconds
                ->Unit(benchmark::kSecond);
        }
        benchmark::RunSpecifiedBenchmarks(&reporter);
        benchmark::ClearRegisteredBenchmarks();
    }
    benchmark::Shutdown();
    return 0;
}
