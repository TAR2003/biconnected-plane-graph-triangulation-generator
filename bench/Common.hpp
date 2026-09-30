#pragma once
#include <benchmark/benchmark.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#ifndef BENCH_PROJECT_ROOT
#define BENCH_PROJECT_ROOT "."
#endif

namespace bench
{
    namespace fs = std::filesystem;
    using Clock = std::chrono::steady_clock;
    using Faces = std::vector<std::vector<long long>>;
    using Adjacency = std::vector<std::vector<long long>>;

    // ------------------------------------------------------------------------
    // Configuration. Defaults live in each main(); every field can be
    // overridden on the command line:  --runs=N --limit=N --input=DIR --output=DIR
    // ------------------------------------------------------------------------
    enum class Family
    {
        Biconnected,
        BiconnectedWithoutVGS,
        Oneconnected
    };

    inline const char *familyName(Family f)
    {
        switch (f)
        {
        case Family::Biconnected:
            return "Biconnected";
        case Family::BiconnectedWithoutVGS:
            return "BiconnectedWithoutVGS";
        case Family::Oneconnected:
            return "Oneconnected";
        }
        return "Unknown";
    }

    struct Config
    {
        fs::path input = fs::path(BENCH_PROJECT_ROOT) / "test-cases" / "input";
        fs::path output = fs::path(BENCH_PROJECT_ROOT) / "benchmark-results";
        int runs;                          // target number of recorded runs per case
        long long limit;                   // triangulation limit passed to the generator
        Family algo = Family::Biconnected; // algorithm under test (required on the command line)
        bool algoGiven = false;
        bool runBiInputs = true; // which input families to run the algorithm on
        bool runOneInputs = true;
        Config(int runs, long long limit) : runs(runs), limit(limit) {}

        // Results of different algorithms never share a folder.
        fs::path resultsDir() const { return output / familyName(algo); }
    };

    inline bool parseFamily(const std::string &v, Family &out)
    {
        if (v == "biconnected")
            out = Family::Biconnected;
        else if (v == "oneconnected")
            out = Family::Oneconnected;
        else if (v == "biconnected-without-vgs" || v == "biconnected_without_vgs" || v == "biconnectedWithoutVGS")
            out = Family::BiconnectedWithoutVGS;
        else
            return false;
        return true;
    }

    // Call after benchmark::Initialize(), which strips the --benchmark_* flags.
    inline bool parseArgs(int argc, char **argv, Config &cfg)
    {
        try
        {
            for (int i = 1; i < argc; ++i)
            {
                const std::string arg = argv[i];
                const auto eq = arg.find('=');
                const std::string key = arg.substr(0, eq);
                const std::string val = eq == std::string::npos ? "" : arg.substr(eq + 1);
                if (key == "--runs")
                    cfg.runs = std::stoi(val);
                else if (key == "--limit")
                    cfg.limit = std::stoll(val);
                else if (key == "--input")
                    cfg.input = val;
                else if (key == "--output")
                    cfg.output = val;
                else if (key == "--algo")
                {
                    if (!parseFamily(val, cfg.algo))
                        throw std::invalid_argument("--algo must be biconnected, biconnected-without-vgs, or oneconnected");
                    cfg.algoGiven = true;
                }
                else if (key == "--cases")
                {
                    cfg.runBiInputs = (val == "biconnected" || val == "all");
                    cfg.runOneInputs = (val == "oneconnected" || val == "all");
                    if (!cfg.runBiInputs && !cfg.runOneInputs)
                        throw std::invalid_argument("--cases must be biconnected, oneconnected or all");
                }
                else
                    throw std::invalid_argument("unknown argument '" + arg + "'");
            }
            if (!cfg.algoGiven)
                throw std::invalid_argument("--algo=biconnected|biconnected-without-vgs|oneconnected is required");
            return true;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: " << e.what()
                      << "\nUsage: --algo=biconnected|biconnected-without-vgs|oneconnected [--cases=biconnected|oneconnected|all]\n"
                         "       [--runs=N] [--limit=N] [--input=DIR] [--output=DIR] [--benchmark_* flags]\n";
            return false;
        }
    }

    // ------------------------------------------------------------------------
    // Test-case discovery
    // ------------------------------------------------------------------------
    struct Case
    {
        Family family;
        fs::path file;        // absolute path of the .txt input
        std::string folderId; // path from input root to its folder, e.g. "Biconnected__grid__small"
        std::string display;  // e.g. "Biconnected/grid/small/case_01.txt"

        std::string name() const { return file.filename().string(); }
        std::string stem() const { return file.stem().string(); }
    };

    // Finds every *.txt under <root>/Biconnected and <root>/Oneconnected (any depth),
    // restricted to the input families selected with --cases.
    inline std::vector<Case> discoverCases(const fs::path &root, bool bi = true, bool one = true)
    {
        std::vector<Case> cases;
        const std::pair<Family, const char *> families[] = {{Family::Biconnected, "Biconnected"},
                                                            {Family::Oneconnected, "Oneconnected"}};
        for (const auto &[family, dirName] : families)
        {
            if ((family == Family::Biconnected && !bi) || (family == Family::Oneconnected && !one))
                continue;
            const fs::path familyDir = root / dirName;
            if (!fs::is_directory(familyDir))
                continue;
            for (const auto &entry : fs::recursive_directory_iterator(familyDir))
            {
                if (!entry.is_regular_file() || entry.path().extension() != ".txt")
                    continue;
                std::string id;
                for (const auto &part : fs::relative(entry.path().parent_path(), root))
                    id += (id.empty() ? "" : "__") + part.string();
                cases.push_back({family, entry.path(), id,
                                 fs::relative(entry.path(), root).generic_string()});
            }
        }
        std::sort(cases.begin(), cases.end(),
                  [](const Case &a, const Case &b)
                  { return a.display < b.display; });
        return cases;
    }

    // Input format: N, then for each vertex: degree followed by neighbors.
    inline bool readAdjacency(const fs::path &file, Adjacency &adjacency, long long &vertices)
    {
        std::ifstream in(file);
        if (!(in >> vertices) || vertices <= 0)
            return false;
        adjacency.assign((size_t)vertices, {});
        for (auto &neighbors : adjacency)
        {
            long long degree = 0;
            if (!(in >> degree) || degree < 0)
                return false;
            neighbors.resize((size_t)degree);
            for (auto &v : neighbors)
            {
                if (!(in >> v))
                    return false;
            }
        }
        return true;
    }

    inline std::string timestamp()
    {
        const std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
        return buf;
    }

    inline bool needsHeader(const fs::path &csv)
    {
        fs::create_directories(csv.parent_path());
        return !fs::exists(csv) || fs::file_size(csv) == 0;
    }

    // ------------------------------------------------------------------------
    // Registers one Google Benchmark per case that still has runs missing.
    //   csvPath(case)          -> CSV the case's results live in
    //   completed(csv, case)   -> number of runs already recorded there
    //   run(state, case, csv)  -> performs ONE run and appends it to the CSV
    // Each benchmark executes exactly one iteration per repetition, so a
    // repetition == one recorded run. Returns the number of registered cases.
    // ------------------------------------------------------------------------
    template <class CsvPath, class Completed, class Run>
    size_t registerCases(const Config &cfg, const std::string &prefix,
                         CsvPath csvPath, Completed completed, Run run)
    {
        size_t registered = 0, skipped = 0;
        for (const Case &c : discoverCases(cfg.input, cfg.runBiInputs, cfg.runOneInputs))
        {
            const fs::path csv = csvPath(c);
            const int remaining = cfg.runs - completed(csv, c);
            if (remaining <= 0)
            {
                ++skipped;
                continue;
            }
            benchmark::RegisterBenchmark(std::string(familyName(cfg.algo)) + "/" + prefix + c.display,
                                         [=](benchmark::State &s)
                                         { run(s, c, csv); })
                ->Iterations(1)
                ->Repetitions(remaining)
                ->UseManualTime()
                ->Unit(benchmark::kMillisecond);
            ++registered;
        }
        std::cerr << "[bench] algorithm: " << familyName(cfg.algo) << "\n[bench] input: " << cfg.input.string()
                  << "\n[bench] output: " << cfg.resultsDir().string()
                  << "\n[bench] " << registered << " case(s) to run, " << skipped
                  << " already complete (target " << cfg.runs << " run(s)/case, limit " << cfg.limit << ")\n";
        return registered;
    }
}