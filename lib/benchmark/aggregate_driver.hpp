#pragma once

#include "common.hpp"

namespace benchmark
{

struct ProgressSnapshot
{
    u128 triangulations = 0;
    long long totalChecks = 0;
    long long successfulChecks = 0;
    long long invalidTraversals = 0;
};

inline void writeSimpleProgressFile(const string &path, u128 count)
{
    ofstream out(path, ios::trunc);
    if (out.is_open())
        out << u128_to_string(count) << '\n';
}

inline u128 readSimpleProgressFile(const string &path)
{
    ifstream in(path);
    if (!in.is_open())
        return 0;
    string line;
    getline(in, line);
    return string_to_u128(line);
}

inline void writeProgressFile(const string &path, const ProgressSnapshot &p)
{
    ofstream out(path, ios::trunc);
    if (!out.is_open())
        return;
    out << "triangulations=" << u128_to_string(p.triangulations) << '\n';
    out << "totalChecks=" << p.totalChecks << '\n';
    out << "successfulChecks=" << p.successfulChecks << '\n';
    out << "invalidTraversals=" << p.invalidTraversals << '\n';
}

inline ProgressSnapshot readProgressFile(const string &path)
{
    ProgressSnapshot p;
    ifstream in(path);
    if (!in.is_open())
        return p;

    string line;
    while (getline(in, line))
    {
        if (line.empty())
            continue;
        auto eq = line.find('=');
        if (eq == string::npos)
        {
            p.triangulations = string_to_u128(line);
            continue;
        }
        string key = line.substr(0, eq);
        string value = line.substr(eq + 1);
        if (key == "triangulations")
            p.triangulations = string_to_u128(value);
        else if (key == "totalChecks")
            p.totalChecks = stoll(value);
        else if (key == "successfulChecks")
            p.successfulChecks = stoll(value);
        else if (key == "invalidTraversals")
            p.invalidTraversals = stoll(value);
    }
    return p;
}

struct WorkerResult
{
    string status;
    u128 triangulations = 0;
    double timeSeconds = 0.0;
    size_t peakMemory = 0;
    string startTime;
    string endTime;
    long long totalChecks = 0;
    long long successfulChecks = 0;
    long long invalidTraversals = 0;
};

inline void writeWorkerResultFile(const string &path, const WorkerResult &r, bool extendedMetrics)
{
    ofstream out(path, ios::trunc);
    if (!out.is_open())
        return;
    out << "status=" << r.status << '\n';
    out << "triangulations=" << u128_to_string(r.triangulations) << '\n';
    out << "timeSeconds=" << fixed << setprecision(9) << r.timeSeconds << '\n';
    out << "peakMemoryBytes=" << r.peakMemory << '\n';
    out << "startTime=" << r.startTime << '\n';
    out << "endTime=" << r.endTime << '\n';
    if (extendedMetrics)
    {
        out << "totalChecks=" << r.totalChecks << '\n';
        out << "successfulChecks=" << r.successfulChecks << '\n';
        out << "invalidTraversals=" << r.invalidTraversals << '\n';
    }
}

inline bool readWorkerResultFile(const string &path, WorkerResult &r)
{
    ifstream in(path);
    if (!in.is_open())
        return false;

    string line;
    bool any = false;
    while (getline(in, line))
    {
        if (line.empty())
            continue;
        auto eq = line.find('=');
        if (eq == string::npos)
            continue;
        string key = line.substr(0, eq);
        string value = line.substr(eq + 1);
        any = true;
        if (key == "status")
            r.status = value;
        else if (key == "triangulations")
            r.triangulations = string_to_u128(value);
        else if (key == "timeSeconds")
            r.timeSeconds = stod(value);
        else if (key == "peakMemoryBytes")
            r.peakMemory = (size_t)stoull(value);
        else if (key == "startTime")
            r.startTime = value;
        else if (key == "endTime")
            r.endTime = value;
        else if (key == "totalChecks")
            r.totalChecks = stoll(value);
        else if (key == "successfulChecks")
            r.successfulChecks = stoll(value);
        else if (key == "invalidTraversals")
            r.invalidTraversals = stoll(value);
    }
    return any;
}

struct RunRecord
{
    string filename;
    int runIndex;
    int distinctVertices;
    string triangStr;
    double timeSeconds;
    size_t peakMemory;
    double memoryPerVertex;
    string startTime;
    string endTime;
    string status;

    long long totalChecks = 0;
    long long successfulChecks = 0;
    long long failedChecks = 0;
    double checkSuccessRate = 0.0;
    long long invalidTraversals = 0;
    long long totalTraversalsExtended = 0;
    double traversalSuccessRate = 0.0;
};

inline string csvPathForCategory(const string &category)
{
    return "results_" + category + ".csv";
}

inline int countExistingRuns(const string &csvPath, const string &filename)
{
    ifstream in(csvPath);
    if (!in.is_open())
        return 0;

    string line;
    if (!getline(in, line))
        return 0;

    int count = 0;
    while (getline(in, line))
    {
        if (line.empty())
            continue;
        stringstream ss(line);
        string field;
        getline(ss, field, ',');
        if (field == filename)
            count++;
    }
    return count;
}

inline void appendRunCSV(const string &csvPath, const RunRecord &r, bool extendedMetrics)
{
    bool needHeader = !fs::exists(csvPath);
    ofstream out(csvPath, ios::app);
    if (!out.is_open())
    {
        cerr << "Error: could not open " << csvPath << " for writing\n";
        return;
    }
    if (needHeader)
    {
        out << "filename,runIndex,vertices,triangulations,timeSeconds,peakMemoryBytes,memoryPerVertex,startTime,endTime,status";
        if (extendedMetrics)
        {
            out << ",totalChecks,successfulChecks,failedChecks,checkSuccessRate,"
                << "invalidTraversals,totalTraversalsExtended,traversalSuccessRate";
        }
        out << '\n';
    }
    out << r.filename << ',' << r.runIndex << ',' << r.distinctVertices << ','
        << r.triangStr << ',' << fixed << setprecision(9) << r.timeSeconds << ','
        << r.peakMemory << ',' << fixed << setprecision(6) << r.memoryPerVertex << ','
        << r.startTime << ',' << r.endTime << ',' << r.status;
    if (extendedMetrics)
    {
        out << ',' << r.totalChecks << ',' << r.successfulChecks << ',' << r.failedChecks << ','
            << fixed << setprecision(2) << r.checkSuccessRate << ','
            << r.invalidTraversals << ',' << r.totalTraversalsExtended << ','
            << fixed << setprecision(2) << r.traversalSuccessRate;
    }
    out << '\n';
}

inline void fillExtendedRates(RunRecord &rec, const ProgressSnapshot &snap)
{
    rec.totalChecks = snap.totalChecks;
    rec.successfulChecks = snap.successfulChecks;
    rec.failedChecks = snap.totalChecks - snap.successfulChecks;
    rec.checkSuccessRate =
        (snap.totalChecks > 0) ? (static_cast<double>(snap.successfulChecks) / snap.totalChecks) * 100.0 : 0.0;
    rec.invalidTraversals = snap.invalidTraversals;
    long long successfulTraversals = static_cast<long long>(snap.triangulations);
    rec.totalTraversalsExtended = snap.invalidTraversals + successfulTraversals;
    rec.traversalSuccessRate = (rec.totalTraversalsExtended > 0)
                                   ? (static_cast<double>(successfulTraversals) / rec.totalTraversalsExtended) * 100.0
                                   : 0.0;
}

template <typename Engine, bool ExtendedMetrics>
inline u128 engineTriangulationCount(Engine *bc)
{
    if constexpr (ExtendedMetrics)
        return static_cast<u128>(bc->totalTriangulations);
    else
        return bc->totalTriangulations;
}

template <typename Engine, bool ExtendedMetrics>
inline ProgressSnapshot snapshotFromEngine(Engine *bc)
{
    ProgressSnapshot p;
    p.triangulations = engineTriangulationCount<Engine, ExtendedMetrics>(bc);
    if constexpr (ExtendedMetrics)
    {
        p.totalChecks = bc->totalChecks;
        p.successfulChecks = bc->successfulChecks;
        p.invalidTraversals = bc->invalidTraversals;
    }
    return p;
}

template <typename Engine, bool ExtendedMetrics>
inline int runWorkerMode(const char *inputPath, const char *resultPath, const char *progressPath)
{
    int distinctVertices = 0;
    vector<vector<int>> faces = readInput(inputPath, distinctVertices);
    if (faces.empty())
        return 1;

    string startTs = currentTimeString();
    size_t memBefore = getCurrentMemoryUsage();

    Engine *bc = new Engine(faces);
    bc->storeTriangulationChords = false;
    std::atomic<bool> stopProgress{false};

    std::thread progressThread([&]()
                               {
        while (!stopProgress.load(std::memory_order_relaxed))
        {
            if constexpr (ExtendedMetrics)
                writeProgressFile(progressPath, snapshotFromEngine<Engine, ExtendedMetrics>(bc));
            else
                writeSimpleProgressFile(progressPath, bc->totalTriangulations);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        if constexpr (ExtendedMetrics)
            writeProgressFile(progressPath, snapshotFromEngine<Engine, ExtendedMetrics>(bc));
        else
            writeSimpleProgressFile(progressPath, bc->totalTriangulations); });

    using clock = std::chrono::steady_clock;
    auto runStart = clock::now();
    bc->getAllTriangulations();
    auto runEnd = clock::now();

    stopProgress.store(true, std::memory_order_relaxed);
    progressThread.join();

    double runSec = std::chrono::duration_cast<std::chrono::duration<double>>(runEnd - runStart).count();
    string endTs = currentTimeString();

    size_t memAfter = getCurrentMemoryUsage();
    size_t memUsed = (memAfter > memBefore) ? (memAfter - memBefore) : 0;

    WorkerResult result;
    result.status = "completed";
    result.triangulations = engineTriangulationCount<Engine, ExtendedMetrics>(bc);
    result.timeSeconds = runSec;
    result.peakMemory = memUsed;
    result.startTime = startTs;
    result.endTime = endTs;
    if constexpr (ExtendedMetrics)
    {
        result.totalChecks = bc->totalChecks;
        result.successfulChecks = bc->successfulChecks;
        result.invalidTraversals = bc->invalidTraversals;
    }
    writeWorkerResultFile(resultPath, result, ExtendedMetrics);

    delete bc;
    return 0;
}

template <typename Engine, bool ExtendedMetrics>
inline void printUsage(const vector<string> &categories, const char *progName)
{
    cerr << "Usage: " << progName << " <category-index|all>\n\n";
    cerr << "Available categories (alphabetical order):\n";
    for (size_t i = 0; i < categories.size(); i++)
    {
        cerr << "  " << (i + 1) << " -> " << categories[i] << "\n";
    }
    cerr << "  all -> run every category above\n";
}

template <typename Engine, bool ExtendedMetrics>
inline void runCategory(const string &category, const string &inputRoot)
{
    string categoryFolder = inputRoot + "/" + category;
    string csvPath = csvPathForCategory(category);

    cout << "\n--- Category: " << category << " (CSV: " << csvPath << ") ---\n";

    vector<string> fileList;
    for (const auto &entry : fs::directory_iterator(categoryFolder))
    {
        if (entry.is_regular_file())
            fileList.push_back(entry.path().filename().string());
    }
    sort(fileList.begin(), fileList.end());

    for (const auto &filename : fileList)
    {
        string fullPath = categoryFolder + "/" + filename;

        int alreadyDone = countExistingRuns(csvPath, filename);
        if (alreadyDone >= RUNS_PER_CASE)
        {
            cout << "  " << filename << ": Found " << alreadyDone << " run(s) in CSV. Already complete ("
                 << RUNS_PER_CASE << "/" << RUNS_PER_CASE << "), skipping.\n";
            continue;
        }

        int remaining = RUNS_PER_CASE - alreadyDone;
        cout << "  " << filename << ": Found " << alreadyDone << " run(s) in CSV. Need "
             << remaining << " more run(s).\n";

        int distinctVertices = 0;
        vector<vector<int>> faces = readInput(fullPath, distinctVertices);
        if (faces.empty())
        {
            cerr << "    Warning: skipping empty/invalid file: " << filename << "\n";
            continue;
        }

        for (int localRun = 1; localRun <= remaining; localRun++)
        {
            int globalRunIndex = alreadyDone + localRun;

            string startTs = currentTimeString();
            cout << "    Running " << getOrdinal(globalRunIndex) << " run (start " << startTs
                 << ", time limit " << TIME_LIMIT_SECONDS << "s)..." << flush;

            fs::path tempDir = fs::temp_directory_path() /
                               ("tri_benchmark_" + category + "_" + filename + "_" + to_string(globalRunIndex));
            fs::create_directories(tempDir);
            string progressPath = (tempDir / "progress.txt").string();
            string resultPath = (tempDir / "result.txt").string();

            size_t memBefore = getCurrentMemoryUsage();

            using clock = std::chrono::steady_clock;
            auto runStart = clock::now();
            int subprocessStatus = runInSubprocess(fullPath, resultPath, progressPath, TIME_LIMIT_SECONDS);
            auto runEnd = clock::now();
            double runSec = std::chrono::duration_cast<std::chrono::duration<double>>(runEnd - runStart).count();

            string endTs = currentTimeString();
            size_t memAfter = getCurrentMemoryUsage();
            size_t memUsed = (memAfter > memBefore) ? (memAfter - memBefore) : 0;

            RunRecord rec;
            rec.filename = filename;
            rec.runIndex = globalRunIndex;
            rec.distinctVertices = distinctVertices;
            rec.startTime = startTs;
            rec.endTime = endTs;

            if (subprocessStatus == 0)
            {
                WorkerResult workerResult;
                if (readWorkerResultFile(resultPath, workerResult))
                {
                    rec.triangStr = u128_to_string(workerResult.triangulations);
                    rec.timeSeconds = workerResult.timeSeconds;
                    rec.peakMemory = workerResult.peakMemory;
                    rec.startTime = workerResult.startTime.empty() ? startTs : workerResult.startTime;
                    rec.endTime = workerResult.endTime.empty() ? endTs : workerResult.endTime;
                    rec.status = "completed";
                    if constexpr (ExtendedMetrics)
                    {
                        ProgressSnapshot snap;
                        snap.triangulations = workerResult.triangulations;
                        snap.totalChecks = workerResult.totalChecks;
                        snap.successfulChecks = workerResult.successfulChecks;
                        snap.invalidTraversals = workerResult.invalidTraversals;
                        fillExtendedRates(rec, snap);
                    }

                    cout << " done (end " << rec.endTime << ") -> " << fixed << setprecision(6)
                         << rec.timeSeconds << " s, " << rec.triangStr << " triangulations, "
                         << formatBytes(rec.peakMemory) << "\n";
                }
                else
                {
                    rec.timeSeconds = runSec;
                    rec.peakMemory = memUsed;
                    rec.status = "error";
                    if constexpr (ExtendedMetrics)
                    {
                        ProgressSnapshot snap = readProgressFile(progressPath);
                        rec.triangStr = u128_to_string(snap.triangulations);
                        fillExtendedRates(rec, snap);
                    }
                    else
                    {
                        rec.triangStr = u128_to_string(readSimpleProgressFile(progressPath));
                    }
                    cout << " ERROR: worker finished but result file missing (end " << endTs << ")\n";
                }
            }
            else if (subprocessStatus == 1)
            {
                rec.timeSeconds = runSec;
                rec.peakMemory = memUsed;
                rec.status = "time_limit_exceeded";
                if constexpr (ExtendedMetrics)
                {
                    ProgressSnapshot snap = readProgressFile(progressPath);
                    rec.triangStr = u128_to_string(snap.triangulations);
                    fillExtendedRates(rec, snap);
                }
                else
                {
                    rec.triangStr = u128_to_string(readSimpleProgressFile(progressPath));
                }

                cout << " TIME LIMIT EXCEEDED (end " << endTs << ") -> " << fixed << setprecision(6)
                     << rec.timeSeconds << " s, " << rec.triangStr
                     << " triangulations reached before time limit, "
                     << formatBytes(rec.peakMemory) << "\n";
            }
            else
            {
                rec.timeSeconds = runSec;
                rec.peakMemory = memUsed;
                rec.status = "error";
                if constexpr (ExtendedMetrics)
                {
                    ProgressSnapshot snap = readProgressFile(progressPath);
                    rec.triangStr = u128_to_string(snap.triangulations);
                    fillExtendedRates(rec, snap);
                }
                else
                {
                    rec.triangStr = u128_to_string(readSimpleProgressFile(progressPath));
                }
                cout << " ERROR: failed to spawn or wait on worker process (end " << endTs << ")\n";
            }

            rec.memoryPerVertex = (distinctVertices > 0) ? (double)rec.peakMemory / distinctVertices : 0.0;
            appendRunCSV(csvPath, rec, ExtendedMetrics);

            error_code ec;
            fs::remove_all(tempDir, ec);
        }
    }
}

template <typename Engine, bool ExtendedMetrics>
inline int runAggregateMain(int argc, char *argv[], const string &inputRoot, const char *bannerTitle)
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if (argc == 5 && string(argv[1]) == "--worker")
        return runWorkerMode<Engine, ExtendedMetrics>(argv[2], argv[3], argv[4]);

    if (!fs::exists(inputRoot) || !fs::is_directory(inputRoot))
    {
        cerr << "Input folder '" << inputRoot << "' does not exist.\n";
        return 1;
    }

    vector<string> categories;
    for (const auto &entry : fs::directory_iterator(inputRoot))
    {
        if (entry.is_directory())
            categories.push_back(entry.path().filename().string());
    }
    sort(categories.begin(), categories.end());

    if (categories.empty())
    {
        cerr << "No category subfolders found under '" << inputRoot << "'.\n";
        return 1;
    }

    if (argc < 2)
    {
        printUsage<Engine, ExtendedMetrics>(categories, argv[0]);
        return 1;
    }

    string arg = argv[1];
    vector<string> categoriesToRun;

    if (arg == "all")
    {
        categoriesToRun = categories;
    }
    else
    {
        bool isNumber = !arg.empty() && all_of(arg.begin(), arg.end(), ::isdigit);
        if (!isNumber)
        {
            cerr << "Invalid argument: '" << arg << "'\n\n";
            printUsage<Engine, ExtendedMetrics>(categories, argv[0]);
            return 1;
        }

        int idx = stoi(arg);
        if (idx < 1 || idx > (int)categories.size())
        {
            cerr << "Category index out of range: " << idx << "\n\n";
            printUsage<Engine, ExtendedMetrics>(categories, argv[0]);
            return 1;
        }

        categoriesToRun.push_back(categories[idx - 1]);
    }

    cout << "\n";
    cout << "================================================================\n";
    cout << "   " << bannerTitle << "\n";
    cout << "   Target runs per case: " << RUNS_PER_CASE << "\n";
    cout << "   Time limit per run: " << TIME_LIMIT_SECONDS << " seconds\n";
    cout << "================================================================\n";

    for (const auto &category : categoriesToRun)
        runCategory<Engine, ExtendedMetrics>(category, inputRoot);

    cout << "\nSelected categories processed. Per-category CSVs contain one row per individual run.\n";
    return 0;
}

} // namespace benchmark
