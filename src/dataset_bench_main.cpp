#include <benchmark/benchmark.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include "GraphTriangulation.hpp"
#include "memory_tracker.hpp"

namespace fs = std::filesystem;
namespace {

enum class Algorithm { Biconnected, Oneconnected };
struct Options {
    Algorithm algorithm = Algorithm::Biconnected;
    fs::path input_root = fs::path("test-cases") / "input";
    fs::path output_dir = "benchmark-results";
    std::string case_filter;
    bool list_cases = false;
    bool rerun_existing = false;
    int64_t runs_per_case = 1;
};
struct InputCase { fs::path path; std::string category; long long vertices = 0; };
struct GraphInput { std::vector<std::vector<long long>> faces; long long vertices = 0; };

Options options;
std::vector<InputCase> cases;
std::mutex csv_mutex;

std::string algorithm_name() {
    return options.algorithm == Algorithm::Biconnected ? "biconnected" : "oneconnected";
}
std::string timestamp() {
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    char buffer[32]{};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local);
    return buffer;
}
GraphInput read_input(const fs::path& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot open input file: " + path.string());
    long long face_count = 0;
    if (!(input >> face_count) || face_count < 1) throw std::runtime_error("invalid face count in: " + path.string());
    GraphInput result;
    std::unordered_set<long long> vertices;
    for (long long face_index = 0; face_index < face_count; ++face_index) {
        long long face_size = 0;
        if (!(input >> face_size) || face_size < 3) throw std::runtime_error("invalid face in: " + path.string());
        std::vector<long long> face;
        for (long long vertex_index = 0; vertex_index < face_size; ++vertex_index) {
            long long vertex = 0;
            if (!(input >> vertex)) throw std::runtime_error("truncated face in: " + path.string());
            face.push_back(vertex);
            vertices.insert(vertex);
        }
        result.faces.push_back(std::move(face));
    }
    result.vertices = static_cast<long long>(vertices.size());
    return result;
}
std::string benchmark_name(const InputCase& test_case) {
    std::string name = test_case.path.lexically_relative(options.input_root).generic_string();
    std::replace(name.begin(), name.end(), '.', '_');
    std::replace(name.begin(), name.end(), '/', '_');
    return algorithm_name() + "/" + name;
}
fs::path csv_path(const std::string& category) {
    fs::path path = options.output_dir / algorithm_name();
    for (const auto& component : fs::path(category)) path /= component;
    return path.string() + ".csv";
}
bool csv_contains_case(const InputCase& test_case) {
    const fs::path path = csv_path(test_case.category);
    std::ifstream input(path);
    if (!input) return false;

    std::string line;
    std::getline(input, line); // header
    while (std::getline(input, line)) {
        const auto comma = line.find(',');
        if (comma != std::string::npos && line.substr(0, comma) == test_case.path.filename().string()) return true;
    }
    return false;
}
long long append_csv(const InputCase& test_case, const GraphInput& input, GraphTriangulation& graph,
                     double seconds, long peak_memory_kb, const std::string& start) {
    std::lock_guard<std::mutex> lock(csv_mutex);
    const fs::path path = csv_path(test_case.category);
    fs::create_directories(path.parent_path());
    const bool header = !fs::exists(path) || fs::file_size(path) == 0;
    long long run_index = 1;
    if (!header) {
        std::ifstream existing(path);
        std::string line;
        std::getline(existing, line);
        while (std::getline(existing, line)) {
            const auto comma = line.find(',');
            if (comma != std::string::npos && line.substr(0, comma) == test_case.path.filename().string()) ++run_index;
        }
    }
    std::ofstream output(path, std::ios::app);
    if (!output) throw std::runtime_error("cannot open result CSV: " + path.string());
    if (header) output << "filename,runIndex,vertices,triangulations,timeSeconds,peakMemoryKB,memoryPerVertexKB,startTime,endTime,status,totalChecks,successfulChecks,failedChecks,checkSuccessRate,invalidTraversals,totalTraversalsExtended,traversalSuccessRate\n";
    const long long traversals = graph.invalidTraversals + graph.totalTriangulations;
    output << test_case.path.filename().string() << ',' << run_index << ',' << input.vertices << ',' << graph.totalTriangulations << ','
           << std::fixed << std::setprecision(9) << seconds << ',' << peak_memory_kb << ','
           << std::fixed << std::setprecision(6) << (input.vertices ? static_cast<double>(peak_memory_kb) / input.vertices : 0.0) << ','
           << start << ',' << timestamp() << ",completed," << graph.totalChecks << ',' << graph.successfulChecks << ','
           << graph.totalChecks - graph.successfulChecks << ',' << std::fixed << std::setprecision(2)
           << (graph.totalChecks ? 100.0 * graph.successfulChecks / graph.totalChecks : 0.0) << ','
           << graph.invalidTraversals << ',' << traversals << ','
           << (traversals ? 100.0 * graph.totalTriangulations / traversals : 0.0) << '\n';
    return run_index;
}
std::unique_ptr<GraphTriangulation> make_graph(std::vector<std::vector<long long>>& faces) {
    if (options.algorithm == Algorithm::Biconnected) return std::make_unique<GraphTriangulationBiconnectedPerformance>(faces);
    return std::make_unique<GraphTriangulationOneconnectedPerformance>(faces);
}
void run_case(benchmark::State& state, const InputCase& test_case) {
    for (auto _ : state) {
        state.PauseTiming();
        GraphInput input = read_input(test_case.path);
        membench::ResetAllocationCounters();
        const auto before = membench::TakeSnapshot();
        auto graph = make_graph(input.faces);
        const std::string start = timestamp();
        std::cout << "[RUNNING] " << test_case.path.lexically_relative(options.input_root).generic_string()
                  << " | algorithm=" << algorithm_name() << " | iteration=" << (state.iterations() + 1) << std::endl;
        state.ResumeTiming();
        const auto begin = std::chrono::steady_clock::now();
        graph->getAllTriangulations();
        const auto end = std::chrono::steady_clock::now();
        const double seconds = std::chrono::duration<double>(end - begin).count();
        state.SetIterationTime(seconds);
        state.PauseTiming();
        const auto memory = membench::Diff(before, membench::TakeSnapshot());
        state.counters["triangulations"] = static_cast<double>(graph->totalTriangulations);
        state.counters["totalChecks"] = static_cast<double>(graph->totalChecks);
        state.counters["successfulChecks"] = static_cast<double>(graph->successfulChecks);
        state.counters["invalidTraversals"] = static_cast<double>(graph->invalidTraversals);
        state.counters["peakMemoryKB"] = static_cast<double>(memory.peak_rss_kb_at_end);
        state.counters["bytesAllocated"] = static_cast<double>(memory.bytes_allocated_during);
        state.counters["allocations"] = static_cast<double>(memory.alloc_count_during);
        const long long run_index = append_csv(test_case, input, *graph, seconds, memory.peak_rss_kb_at_end, start);
        std::cout << "[DONE] " << test_case.path.lexically_relative(options.input_root).generic_string()
              << " | run=" << run_index << " | " << std::fixed << std::setprecision(6) << seconds << " s"
                  << " | triangulations=" << graph->totalTriangulations
                  << " | csv=" << csv_path(test_case.category).generic_string() << std::endl;
        state.ResumeTiming();
    }
}
void register_case(const InputCase& test_case) {
    benchmark::RegisterBenchmark(benchmark_name(test_case).c_str(), [test_case](benchmark::State& state) { run_case(state, test_case); })
        ->UseManualTime()->Iterations(options.runs_per_case)->Unit(benchmark::kMillisecond);
}
std::string option_value(int* index, int argc, char** argv, const std::string& option) {
    const std::string argument = argv[*index];
    if (argument.rfind(option + "=", 0) == 0) return argument.substr(option.size() + 1);
    if (argument == option && *index + 1 < argc) return argv[++*index];
    return {};
}
void parse_options(int* argc, char** argv) {
    std::vector<char*> benchmark_args{argv[0]};
    for (int index = 1; index < *argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--list-cases") options.list_cases = true;
        else if (argument == "--algorithm" || argument.rfind("--algorithm=", 0) == 0) {
            const std::string value = option_value(&index, *argc, argv, "--algorithm");
            if (value == "biconnected") options.algorithm = Algorithm::Biconnected;
            else if (value == "oneconnected") options.algorithm = Algorithm::Oneconnected;
            else throw std::runtime_error("--algorithm must be biconnected or oneconnected");
        } else if (argument == "--input-root" || argument.rfind("--input-root=", 0) == 0) options.input_root = option_value(&index, *argc, argv, "--input-root");
        else if (argument == "--output-dir" || argument.rfind("--output-dir=", 0) == 0) options.output_dir = option_value(&index, *argc, argv, "--output-dir");
        else if (argument == "--case-filter" || argument.rfind("--case-filter=", 0) == 0) options.case_filter = option_value(&index, *argc, argv, "--case-filter");
        else if (argument == "--rerun-existing") options.rerun_existing = true;
        else if (argument == "--runs-per-case" || argument.rfind("--runs-per-case=", 0) == 0) {
            options.runs_per_case = std::stoll(option_value(&index, *argc, argv, "--runs-per-case"));
            if (options.runs_per_case < 1) throw std::runtime_error("--runs-per-case must be at least 1");
        }
        else benchmark_args.push_back(argv[index]);
    }
    std::copy(benchmark_args.begin(), benchmark_args.end(), argv);
    *argc = static_cast<int>(benchmark_args.size());
}
void discover_cases() {
    if (!fs::is_directory(options.input_root)) throw std::runtime_error("input root does not exist: " + options.input_root.string());
    for (const auto& entry : fs::recursive_directory_iterator(options.input_root)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".txt" || entry.path().filename() == "case_manifest.txt") continue;
        const fs::path relative = entry.path().lexically_relative(options.input_root);
        if (!options.case_filter.empty() && relative.generic_string().find(options.case_filter) == std::string::npos) continue;
        InputCase test_case{entry.path(), relative.parent_path().generic_string(), read_input(entry.path()).vertices};
        if (test_case.category.empty()) test_case.category = "root";
        if (!options.list_cases && !options.rerun_existing && csv_contains_case(test_case)) {
            std::cout << "[SKIP] " << relative.generic_string() << " | already present in "
                      << csv_path(test_case.category).generic_string() << std::endl;
            continue;
        }
        cases.push_back(std::move(test_case));
    }
    std::sort(cases.begin(), cases.end(), [](const InputCase& left, const InputCase& right) { return left.path < right.path; });
}
} // namespace

int main(int argc, char** argv) {
    try {
        parse_options(&argc, argv);
        discover_cases();
        if (cases.empty()) {
            std::cout << "No cases remain to run. Matching cases are already present in the CSV files.\n";
            return 0;
        }
        if (options.list_cases) {
            for (const auto& test_case : cases) std::cout << benchmark_name(test_case) << '\n';
            return 0;
        }
        for (const auto& test_case : cases) register_case(test_case);
        benchmark::Initialize(&argc, argv);
        if (benchmark::ReportUnrecognizedArguments(argc, argv)) return 2;
        benchmark::RunSpecifiedBenchmarks();
        benchmark::Shutdown();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "dataset benchmark error: " << error.what() << '\n';
        return 1;
    }
}
