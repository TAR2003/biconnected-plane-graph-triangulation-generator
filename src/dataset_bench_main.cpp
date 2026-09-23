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
#include <thread>
#include <unordered_set>
#include <vector>

#include "GraphTriangulation.hpp"
#include "memory_tracker.hpp"
#include "checkpoint.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>
#endif
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
    int64_t timeout_seconds = 0;
    std::string case_file;
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
// One checkpoint scratch file per case, named after the case's filename
// (not its category, unlike csv_path) so concurrent/sequential cases
// never collide, and placed in a dedicated subfolder so it's obviously
// not a result file if someone browses output_dir/.
fs::path checkpoint_path(const InputCase& test_case) {
    fs::path path = options.output_dir / algorithm_name() / ".checkpoints";
    for (const auto& component : fs::path(test_case.category)) path /= component;
    return path / (test_case.path.filename().string() + ".checkpoint");
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
void append_timeout_csv(const InputCase& test_case, double seconds) {
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

    // Recover partial stats from the checkpoint file the killed child
    // wrote periodically while it was still alive (see checkpoint.hpp).
    // If no checkpoint exists yet (killed before the first tick, or the
    // case finished so fast/small that it never reached one), this
    // falls back to all zeros exactly as before -- that's a correct,
    // honest "we have no partial data" answer, not a bug.
    checkpoint::Stats partial;
    const fs::path ckptPath = checkpoint_path(test_case);
    const bool hasPartial = checkpoint::Read(ckptPath, partial);

    std::ofstream output(path, std::ios::app);
    if (!output) throw std::runtime_error("cannot open result CSV: " + path.string());
    if (header) output << "filename,runIndex,vertices,triangulations,timeSeconds,peakMemoryKB,memoryPerVertexKB,startTime,endTime,status,totalChecks,successfulChecks,failedChecks,checkSuccessRate,invalidTraversals,totalTraversalsExtended,traversalSuccessRate\n";

    const long long vertices = test_case.vertices;
    const long long triangulations = hasPartial ? partial.triangulations : 0;
    const long peakMemoryKb = hasPartial ? partial.peakMemoryKb : 0;
    const double memoryPerVertex = (hasPartial && vertices) ? static_cast<double>(peakMemoryKb) / vertices : 0.0;
    const long long totalChecks = hasPartial ? partial.totalChecks : 0;
    const long long successfulChecks = hasPartial ? partial.successfulChecks : 0;
    const long long failedChecks = totalChecks - successfulChecks;
    const double checkSuccessRate = totalChecks ? 100.0 * successfulChecks / totalChecks : 0.0;
    const long long invalidTraversals = hasPartial ? partial.invalidTraversals : 0;
    const long long totalTraversalsExtended = invalidTraversals + triangulations;
    const double traversalSuccessRate = totalTraversalsExtended ? 100.0 * triangulations / totalTraversalsExtended : 0.0;
    // status distinguishes "we killed it but recovered partial progress"
    // from "we killed it and have nothing" -- both are legitimate
    // outcomes worth telling apart in a paper's results table.
    const std::string status = hasPartial ? "timed_out_partial" : "timed_out_no_data";

    output << test_case.path.filename().string() << ',' << run_index << ',' << vertices << ','
           << triangulations << ',' << std::fixed << std::setprecision(9) << seconds << ','
           << peakMemoryKb << ',' << std::fixed << std::setprecision(6) << memoryPerVertex << ','
           << timestamp() << ',' << timestamp() << ',' << status << ','
           << totalChecks << ',' << successfulChecks << ',' << failedChecks << ','
           << std::fixed << std::setprecision(2) << checkSuccessRate << ','
           << invalidTraversals << ',' << totalTraversalsExtended << ','
           << std::fixed << std::setprecision(2) << traversalSuccessRate << '\n';

    // The checkpoint file has served its purpose (its data is now in the
    // CSV); remove it so a later rerun of this same case doesn't
    // accidentally read stale data from a previous attempt before its
    // own first checkpoint tick.
    std::error_code ec;
    fs::remove(ckptPath, ec);
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

        // Wire up the checkpoint writer: this survives a SIGKILL that
        // would otherwise wipe every in-memory stat on this process.
        // See checkpoint.hpp for why this has to happen from inside
        // this process rather than from the parent that sends the kill.
        const fs::path ckptPath = checkpoint_path(test_case);
        fs::create_directories(ckptPath.parent_path());
        checkpoint::Writer checkpointWriter(ckptPath);
        const auto runStart = std::chrono::steady_clock::now();
        graph->onProgressTick = [&](const GraphTriangulation& g) {
            checkpoint::Stats s;
            s.vertices = input.vertices;
            s.triangulations = g.totalTriangulations;
            s.totalChecks = g.totalChecks;
            s.successfulChecks = g.successfulChecks;
            s.invalidTraversals = g.invalidTraversals;
            s.peakMemoryKb = membench::TakeSnapshot().peak_rss_kb; // cheap: just reads getrusage, no allocation-counter side effects here
            s.elapsedSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - runStart).count();
            checkpointWriter.Tick(s);
        };

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
        checkpointWriter.Cleanup(); // completed normally -- remove the scratch file so it's not mistaken for a stale partial result later
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
        else if (argument == "--case-file" || argument.rfind("--case-file=", 0) == 0) options.case_file = option_value(&index, *argc, argv, "--case-file");
        else if (argument == "--rerun-existing") options.rerun_existing = true;
        else if (argument == "--timeout" || argument.rfind("--timeout=", 0) == 0) {
            options.timeout_seconds = std::stoll(option_value(&index, *argc, argv, "--timeout"));
            if (options.timeout_seconds < 1) throw std::runtime_error("--timeout must be at least 1 second");
        }
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
        if (!options.case_file.empty() && relative.generic_string() != fs::path(options.case_file).generic_string()) continue;
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

struct ChildResult {
    bool timed_out = false;
    int exit_code = 0;
};

ChildResult run_child(const std::string& executable, const std::vector<std::string>& arguments,
                      std::chrono::seconds timeout) {
#ifdef _WIN32
    std::string command = "\"" + executable + "\"";
    for (const auto& argument : arguments) command += " \"" + argument + "\"";
    STARTUPINFOA startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    std::vector<char> command_line(command.begin(), command.end());
    command_line.push_back('\0');
    if (!CreateProcessA(nullptr, command_line.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr,
                        &startup, &process)) {
        throw std::runtime_error("cannot start benchmark child process");
    }
    const DWORD wait_result = WaitForSingleObject(process.hProcess, static_cast<DWORD>(timeout.count() * 1000));
    ChildResult result;
    if (wait_result == WAIT_TIMEOUT) {
        result.timed_out = true;
        TerminateProcess(process.hProcess, 124);
        WaitForSingleObject(process.hProcess, INFINITE);
    }
    DWORD exit_code = 1;
    GetExitCodeProcess(process.hProcess, &exit_code);
    result.exit_code = static_cast<int>(exit_code);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return result;
#else
    const pid_t child = fork();
    if (child < 0) throw std::runtime_error("cannot fork benchmark child process");
    if (child == 0) {
        std::vector<char*> child_argv;
        child_argv.push_back(const_cast<char*>(executable.c_str()));
        std::vector<std::string> storage = arguments;
        for (auto& argument : storage) child_argv.push_back(argument.data());
        child_argv.push_back(nullptr);
        execv(executable.c_str(), child_argv.data());
        std::_Exit(127);
    }
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    int status = 0;
    while (waitpid(child, &status, WNOHANG) == 0) {
        if (std::chrono::steady_clock::now() >= deadline) {
            kill(child, SIGKILL);
            waitpid(child, &status, 0);
            return {true, 137};
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return {false, WIFEXITED(status) ? WEXITSTATUS(status) : 1};
#endif
}

int run_isolated_cases(const std::string& executable, int argc, char** argv) {
    std::vector<std::string> child_arguments{
        "--algorithm=" + algorithm_name(),
        "--input-root=" + options.input_root.string(),
        "--output-dir=" + options.output_dir.string(),
        "--runs-per-case=" + std::to_string(options.runs_per_case),
        "--rerun-existing"};
    for (int index = 1; index < argc; ++index) child_arguments.push_back(argv[index]);

    for (const auto& test_case : cases) {
        std::vector<std::string> arguments = child_arguments;
        arguments.push_back("--case-file=" + test_case.path.lexically_relative(options.input_root).generic_string());
        std::cout << "[RUNNING] " << test_case.path.lexically_relative(options.input_root).generic_string()
                  << " | timeout=" << options.timeout_seconds << " s" << std::endl;
        const auto started = std::chrono::steady_clock::now();
        const ChildResult result = run_child(executable, arguments, std::chrono::seconds(options.timeout_seconds));
        const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
        if (result.timed_out) {
            append_timeout_csv(test_case, elapsed);
            std::cout << "[TIMEOUT] " << test_case.path.lexically_relative(options.input_root).generic_string() << std::endl;
        } else if (result.exit_code != 0) {
            std::cerr << "[ERROR] " << test_case.path.lexically_relative(options.input_root).generic_string()
                      << " | child exit=" << result.exit_code << std::endl;
        }
    }
    return 0;
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
        if (options.timeout_seconds > 0 && options.case_file.empty()) {
            return run_isolated_cases(argv[0], argc, argv);
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
