// ============================================================================
// SET 1: RAM-backed temp files (via /dev/shm), 2 GB per-file cap.
//
// Identical pipeline to the disk version (biconnected -> sort -> triconnected
// -> compare), except every temp file lives under /dev/shm instead of the
// current working directory. /dev/shm is a tmpfs mount backed by RAM, so:
//   - Zero physical disk writes, zero SSD/HDD wear.
//   - Much faster (no real I/O latency).
//   - It IS still RAM: if the cap is too close to your free RAM, you can
//     OOM instead of cleanly hitting the size guard. Keep MAX_DISK_BYTES
//     comfortably below free RAM (this file caps at 2 GB per temp file;
//     bump down further if your machine has less than ~8GB free).
//   - If /dev/shm's tmpfs size limit (usually half of total RAM by default,
//     check with `df -h /dev/shm`) is smaller than 2GB, mount a bigger
//     tmpfs or lower MAX_DISK_BYTES.
// ============================================================================
#include <bits/stdc++.h>
using namespace std;
#include "Edge.hpp"
#include "pairHash.hpp"
#include "FaceTriangulation.hpp"
#include "biconnected.hpp"
#include "triconnected.hpp"
#include "TaskAborted.hpp"

#include <filesystem>
#include <thread>
#include <atomic>
#include <chrono>

namespace fs = std::filesystem;

// ============================================================================
// CONFIGURATION
// 2 GB per-file cap, and all temp files live in RAM via /dev/shm.
// ============================================================================
static const size_t MAX_DISK_BYTES = 2ULL * 1024ULL * 1024ULL * 1024ULL; // 2 GB
static const string SHM_DIR = "/dev/shm/triangulation_test/";

static string currentTimeString()
{
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return string(buf);
}

static size_t fileSizeOrZero(const string &path)
{
    error_code ec;
    if (!fs::exists(path, ec))
        return 0;
    size_t sz = fs::file_size(path, ec);
    return ec ? 0 : sz;
}

struct ScopedTempFiles
{
    vector<string> files;
    void add(const string &f) { files.push_back(f); }
    void removeAll()
    {
        for (auto &f : files)
        {
            error_code ec;
            fs::remove(f, ec);
        }
        files.clear();
    }
    void removeNow(const string &f)
    {
        error_code ec;
        fs::remove(f, ec);
    }
    ~ScopedTempFiles() { removeAll(); }
};

class DiskMonitor
{
    atomic<bool> stopFlag{false};
    atomic<bool> limitExceeded{false};
    string exceededFile;
    thread worker;

public:
    void start(const vector<string> &filesToWatch, size_t maxBytes, atomic<bool> *sharedAbortFlag)
    {
        stopFlag = false;
        limitExceeded = false;
        exceededFile.clear();
        worker = thread([this, filesToWatch, maxBytes, sharedAbortFlag]()
                        {
            while (!stopFlag)
            {
                for (const auto &filePath : filesToWatch)
                {
                    if (fs::exists(filePath))
                    {
                        error_code ec;
                        size_t sz = fs::file_size(filePath, ec);
                        if (!ec && sz > maxBytes)
                        {
                            limitExceeded = true;
                            exceededFile = filePath;
                            if (sharedAbortFlag)
                                sharedAbortFlag->store(true, memory_order_relaxed);
                            return;
                        }
                    }
                }
                this_thread::sleep_for(chrono::milliseconds(50));
            } });
    }
    bool stop()
    {
        stopFlag = true;
        if (worker.joinable())
            worker.join();
        return limitExceeded;
    }
    bool hasExceeded() const { return limitExceeded; }
    const string &whichFileExceeded() const { return exceededFile; }
};

vector<vector<int>> solve(const string &filename)
{
    ifstream infile(filename);
    if (!infile.is_open())
    {
        cerr << "Error opening file: " << filename << endl;
        return {};
    }
    vector<vector<int>> faces;
    int faceno;
    infile >> faceno;
    for (int i = 0; i < faceno; i++)
    {
        int vertices;
        infile >> vertices;
        vector<int> face;
        for (int j = 0; j < vertices; j++)
        {
            int vertex;
            infile >> vertex;
            face.push_back(vertex);
        }
        faces.push_back(face);
    }
    return faces;
}

static bool externalSortBinaryFile(const string &inputFile, const string &outputFile,
                                   ScopedTempFiles &tempGuard, vector<string> &chunkFilesOut,
                                   size_t maxRAMEntries = 500000)
{
    chunkFilesOut.clear();

    ifstream in(inputFile, ios::binary);
    if (!in.is_open())
        return false;

    struct TriangulationEntry
    {
        vector<pair<int, int>> chords;
        bool operator<(const TriangulationEntry &other) const { return chords < other.chords; }
        bool operator>(const TriangulationEntry &other) const { return chords > other.chords; }
    };

    vector<string> chunkFiles;
    vector<TriangulationEntry> buffer;
    buffer.reserve(maxRAMEntries);

    int chunkIdx = 0;
    size_t cumulativeBytesWritten = 0;

    auto writeChunk = [&](const vector<TriangulationEntry> &vec, const string &cFile) -> bool
    {
        ofstream out(cFile, ios::binary);
        for (const auto &item : vec)
        {
            uint32_t sz = static_cast<uint32_t>(item.chords.size());
            size_t entryBytes = sizeof(sz) + (sz * sizeof(pair<int, int>));
            if (cumulativeBytesWritten + entryBytes > MAX_DISK_BYTES)
            {
                out.close();
                return false;
            }
            out.write(reinterpret_cast<const char *>(&sz), sizeof(sz));
            out.write(reinterpret_cast<const char *>(item.chords.data()), sz * sizeof(pair<int, int>));
            cumulativeBytesWritten += entryBytes;
        }
        return true;
    };

    bool diskExceeded = false;

    while (in.peek() != EOF)
    {
        uint32_t sz = 0;
        if (!in.read(reinterpret_cast<char *>(&sz), sizeof(sz)))
            break;

        TriangulationEntry entry;
        entry.chords.resize(sz);
        in.read(reinterpret_cast<char *>(entry.chords.data()), sz * sizeof(pair<int, int>));
        buffer.push_back(move(entry));

        if (buffer.size() >= maxRAMEntries)
        {
            sort(buffer.begin(), buffer.end());
            string cFileName = SHM_DIR + "temp_chunk_" + to_string(chunkIdx++) + ".bin";
            tempGuard.add(cFileName);
            if (!writeChunk(buffer, cFileName))
            {
                diskExceeded = true;
                break;
            }
            chunkFiles.push_back(cFileName);
            buffer.clear();
        }
    }

    if (!diskExceeded && !buffer.empty())
    {
        sort(buffer.begin(), buffer.end());
        if (chunkFiles.empty())
        {
            if (!writeChunk(buffer, outputFile))
                diskExceeded = true;
            in.close();
            chunkFilesOut = chunkFiles;
            return !diskExceeded;
        }
        else
        {
            string cFileName = SHM_DIR + "temp_chunk_" + to_string(chunkIdx++) + ".bin";
            tempGuard.add(cFileName);
            if (!writeChunk(buffer, cFileName))
                diskExceeded = true;
            else
                chunkFiles.push_back(cFileName);
            buffer.clear();
        }
    }
    in.close();
    chunkFilesOut = chunkFiles;

    if (diskExceeded)
        return false;

    if (chunkFiles.empty())
    {
        ofstream out(outputFile, ios::binary);
        return true;
    }

    struct MergeNode
    {
        TriangulationEntry entry;
        int chunkIndex;
        bool operator>(const MergeNode &other) const { return entry > other.entry; }
    };

    vector<ifstream> streams(chunkFiles.size());
    priority_queue<MergeNode, vector<MergeNode>, greater<MergeNode>> pq;

    for (size_t i = 0; i < chunkFiles.size(); i++)
    {
        streams[i].open(chunkFiles[i], ios::binary);
        uint32_t sz = 0;
        if (streams[i].read(reinterpret_cast<char *>(&sz), sizeof(sz)))
        {
            TriangulationEntry entry;
            entry.chords.resize(sz);
            streams[i].read(reinterpret_cast<char *>(entry.chords.data()), sz * sizeof(pair<int, int>));
            pq.push({move(entry), static_cast<int>(i)});
        }
    }

    ofstream out(outputFile, ios::binary);

    while (!pq.empty())
    {
        MergeNode top = move(const_cast<MergeNode &>(pq.top()));
        pq.pop();
        int idx = top.chunkIndex;

        uint32_t sz = static_cast<uint32_t>(top.entry.chords.size());
        size_t entryBytes = sizeof(sz) + (sz * sizeof(pair<int, int>));
        if (cumulativeBytesWritten + entryBytes > MAX_DISK_BYTES)
        {
            diskExceeded = true;
            break;
        }
        out.write(reinterpret_cast<const char *>(&sz), sizeof(sz));
        out.write(reinterpret_cast<const char *>(top.entry.chords.data()), sz * sizeof(pair<int, int>));
        cumulativeBytesWritten += entryBytes;

        uint32_t nextSz = 0;
        if (streams[idx].read(reinterpret_cast<char *>(&nextSz), sizeof(nextSz)))
        {
            TriangulationEntry nextEntry;
            nextEntry.chords.resize(nextSz);
            streams[idx].read(reinterpret_cast<char *>(nextEntry.chords.data()), nextSz * sizeof(pair<int, int>));
            pq.push({move(nextEntry), idx});
        }
    }

    out.close();
    for (size_t i = 0; i < chunkFiles.size(); i++)
        streams[i].close();

    return !diskExceeded;
}

bool compareTriangulationFiles(const string &file1, const string &file2, size_t count1, size_t count2)
{
    if (count1 != count2)
        return false;

    ifstream in1(file1, ios::binary);
    ifstream in2(file2, ios::binary);
    if (!in1.is_open() || !in2.is_open())
        return false;

    vector<pair<int, int>> t1, t2;
    for (size_t i = 0; i < count1; i++)
    {
        uint32_t sz1 = 0, sz2 = 0;
        if (!in1.read(reinterpret_cast<char *>(&sz1), sizeof(sz1)))
            return false;
        if (!in2.read(reinterpret_cast<char *>(&sz2), sizeof(sz2)))
            return false;
        if (sz1 != sz2)
            return false;

        t1.resize(sz1);
        t2.resize(sz2);
        in1.read(reinterpret_cast<char *>(t1.data()), sz1 * sizeof(pair<int, int>));
        in2.read(reinterpret_cast<char *>(t2.data()), sz2 * sizeof(pair<int, int>));
        if (t1 != t2)
            return false;
    }
    return true;
}

struct MatchResult
{
    int code = -1;
    size_t newCount = 0;
    size_t oldCount = 0;
    size_t bcSortedBytes = 0;
    size_t tcSortedBytes = 0;
    string startTs;
    string endTs;
};

MatchResult matchTwoAlgorithms(const string &filename)
{
    MatchResult res;
    res.startTs = currentTimeString();

    vector<vector<int>> faces = solve(filename);
    if (faces.empty())
    {
        res.code = -1;
        res.endTs = currentTimeString();
        return res;
    }

    ScopedTempFiles tempGuard;

    const string fileBC_raw = SHM_DIR + "temp_bc_raw.bin";
    const string fileBC_sorted = SHM_DIR + "temp_bc_sorted.bin";
    const string fileTC_sorted = SHM_DIR + "temp_tc_sorted.bin";

    tempGuard.add(fileBC_raw);
    tempGuard.add(fileBC_sorted);
    tempGuard.add(fileTC_sorted);

    atomic<bool> abortFlag{false};
    DiskMonitor monitor;

    try
    {
        // --- Phase 1: Biconnected Run ---
        {
            biconnected bc(faces);
            monitor.start({fileBC_raw}, MAX_DISK_BYTES, &abortFlag);
            bool taskAborted = false;
            try
            {
                bc.getAllTriangulationsToFile(fileBC_raw, &abortFlag);
            }
            catch (const TaskAbortedException &)
            {
                taskAborted = true;
            }
            monitor.stop();
            res.newCount = bc.totalCount;

            if (taskAborted || monitor.hasExceeded())
            {
                res.code = -2;
                res.endTs = currentTimeString();
                return res;
            }
        }

        // --- Phase 2: External Sorting (still in /dev/shm) ---
        vector<string> chunkFiles;
        if (!externalSortBinaryFile(fileBC_raw, fileBC_sorted, tempGuard, chunkFiles))
        {
            res.code = -2;
            res.endTs = currentTimeString();
            return res;
        }

        for (const auto &cf : chunkFiles)
            tempGuard.removeNow(cf);

        tempGuard.removeNow(fileBC_raw);
        res.bcSortedBytes = fileSizeOrZero(fileBC_sorted);

        // --- Phase 3: Triconnected Run ---
        {
            triconnected tc(faces);
            tc.getAllTriangulations();

            monitor.start({fileTC_sorted}, MAX_DISK_BYTES, &abortFlag);
            bool taskAborted = false;
            try
            {
                tc.refineTriangulationsToFile(fileTC_sorted, &abortFlag);
            }
            catch (const TaskAbortedException &)
            {
                taskAborted = true;
            }
            monitor.stop();
            res.oldCount = tc.totalCount;

            if (taskAborted || monitor.hasExceeded())
            {
                res.code = -2;
                res.endTs = currentTimeString();
                return res;
            }
        }

        res.tcSortedBytes = fileSizeOrZero(fileTC_sorted);

        // --- Phase 4: Stream Comparison ---
        bool matched = compareTriangulationFiles(fileBC_sorted, fileTC_sorted, res.newCount, res.oldCount);

        tempGuard.removeAll();
        res.code = matched ? 1 : 0;
        res.endTs = currentTimeString();
        return res;
    }
    catch (const std::exception &e)
    {
        cerr << "    [exception] " << e.what() << "\n";
        res.code = -1;
        res.endTs = currentTimeString();
        return res;
    }
    catch (...)
    {
        cerr << "    [unknown exception]\n";
        res.code = -1;
        res.endTs = currentTimeString();
        return res;
    }
}

static string csvPathForCategory(const string &category)
{
    return "results_ram_" + category + ".csv";
}

static unordered_set<string> loadProcessedFiles(const string &csvPath)
{
    unordered_set<string> processed;
    ifstream in(csvPath);
    if (!in.is_open())
        return processed;
    string line;
    if (!getline(in, line))
        return processed;
    while (getline(in, line))
    {
        if (line.empty())
            continue;
        stringstream ss(line);
        string filename;
        if (!getline(ss, filename, ','))
            continue;
        processed.insert(filename);
    }
    return processed;
}

static void appendResultCSV(const string &csvPath, const string &filename,
                            const string &resultStr, const MatchResult &res,
                            double elapsedSeconds)
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
        out << "filename,result,newAlgoCount,oldAlgoCount,"
               "biconnectedFileBytes,triconnectedFileBytes,"
               "startTime,endTime,elapsedSeconds\n";
    }
    out << filename << ',' << resultStr << ',' << res.newCount << ',' << res.oldCount << ','
        << res.bcSortedBytes << ',' << res.tcSortedBytes << ','
        << res.startTs << ',' << res.endTs << ',' << elapsedSeconds << '\n';
}

static void printUsage(const vector<string> &categories, const char *progName)
{
    cerr << "Usage: " << progName << " <category-index|all>\n\n";
    cerr << "Available categories (alphabetical order):\n";
    for (size_t i = 0; i < categories.size(); i++)
        cerr << "  " << (i + 1) << " -> " << categories[i] << "\n";
    cerr << "  all -> run every category above\n";
}

struct Summary
{
    string category;
    int matched = 0;
    int mismatched = 0;
    int limitExceeded = 0;
    int errors = 0;
    int skipped = 0;
};

static double secondsBetween(const string &startTs, const string &endTs)
{
    std::tm tmStart{}, tmEnd{};
    istringstream ssStart(startTs), ssEnd(endTs);
    ssStart >> std::get_time(&tmStart, "%Y-%m-%d %H:%M:%S");
    ssEnd >> std::get_time(&tmEnd, "%Y-%m-%d %H:%M:%S");
    if (ssStart.fail() || ssEnd.fail())
        return 0.0;
    std::time_t tStart = std::mktime(&tmStart);
    std::time_t tEnd = std::mktime(&tmEnd);
    return std::difftime(tEnd, tStart);
}

static Summary runCategory(const string &rootFolder, const string &category)
{
    string categoryFolder = rootFolder + "/" + category;
    string csvPath = csvPathForCategory(category);

    cout << "\n=== Category: " << category << " ===\n";
    cout << "Results CSV: " << csvPath << "\n";

    unordered_set<string> processed = loadProcessedFiles(csvPath);

    Summary summ;
    summ.category = category;

    vector<string> fileList;
    for (const auto &entry : fs::directory_iterator(categoryFolder))
        if (entry.is_regular_file())
            fileList.push_back(entry.path().filename().string());
    sort(fileList.begin(), fileList.end());

    for (const auto &filename : fileList)
    {
        if (processed.find(filename) != processed.end())
        {
            cout << "  Skipping already-tested: " << filename << "\n";
            summ.skipped++;
            continue;
        }

        string fullPath = categoryFolder + "/" + filename;
        cout << "  Processing: " << filename << " (start " << currentTimeString() << ") ... " << flush;

        MatchResult res;
        try
        {
            res = matchTwoAlgorithms(fullPath);
        }
        catch (const std::exception &e)
        {
            cerr << "\n    [unexpected top-level exception] " << e.what() << "\n";
            res.code = -1;
        }
        catch (...)
        {
            cerr << "\n    [unexpected top-level unknown exception]\n";
            res.code = -1;
        }

        if (res.startTs.empty())
            res.startTs = currentTimeString();
        if (res.endTs.empty())
            res.endTs = currentTimeString();

        double elapsed = secondsBetween(res.startTs, res.endTs);

        string resultStr;
        if (res.code == 1)
        {
            resultStr = "MATCH";
            summ.matched++;
            cout << "\033[1;32mMATCH\033[0m (" << res.newCount << " triangulations, "
                 << "bc=" << res.bcSortedBytes << "B, tc=" << res.tcSortedBytes << "B, "
                 << elapsed << "s, end " << res.endTs << ")\n";
        }
        else if (res.code == 0)
        {
            resultStr = "MISMATCH";
            summ.mismatched++;
            cout << "\033[1;31mMISMATCH\033[0m (new=" << res.newCount << ", old=" << res.oldCount
                 << ", bc=" << res.bcSortedBytes << "B, tc=" << res.tcSortedBytes << "B, "
                 << elapsed << "s, end " << res.endTs << ")\n";
        }
        else if (res.code == -2)
        {
            resultStr = "LIMIT_EXCEEDED";
            summ.limitExceeded++;
            cout << "\033[1;35mLIMIT_EXCEEDED\033[0m (> " << (MAX_DISK_BYTES / (1024 * 1024))
                 << " MB limit, task stopped and temp files removed, "
                 << elapsed << "s, end " << res.endTs << ")\n";
        }
        else
        {
            resultStr = "ERROR";
            summ.errors++;
            cout << "\033[1;33mERROR (could not read file)\033[0m (" << elapsed << "s, end " << res.endTs << ")\n";
        }

        appendResultCSV(csvPath, filename, resultStr, res, elapsed);
    }

    return summ;
}

int main(int argc, char *argv[])
{
    // Ensure the /dev/shm scratch subdirectory exists before we start.
    {
        error_code ec;
        fs::create_directories(SHM_DIR, ec);
        if (ec)
        {
            cerr << "Could not create " << SHM_DIR << ": " << ec.message() << "\n";
            cerr << "Falling back requires editing SHM_DIR to a writable tmpfs path.\n";
            return 1;
        }
    }

    const string rootFolder = "input";

    if (!fs::exists(rootFolder) || !fs::is_directory(rootFolder))
    {
        cerr << "Input folder '" << rootFolder << "' does not exist.\n";
        return 1;
    }

    vector<string> categories;
    for (const auto &entry : fs::directory_iterator(rootFolder))
        if (entry.is_directory())
            categories.push_back(entry.path().filename().string());
    sort(categories.begin(), categories.end());

    if (categories.empty())
    {
        cerr << "No category subfolders found under '" << rootFolder << "'.\n";
        return 1;
    }

    if (argc < 2)
    {
        printUsage(categories, argv[0]);
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
            printUsage(categories, argv[0]);
            return 1;
        }
        int idx = stoi(arg);
        if (idx < 1 || idx > (int)categories.size())
        {
            cerr << "Category index out of range: " << idx << "\n\n";
            printUsage(categories, argv[0]);
            return 1;
        }
        categoriesToRun.push_back(categories[idx - 1]);
    }

    vector<Summary> summaries;
    for (const auto &category : categoriesToRun)
        summaries.push_back(runCategory(rootFolder, category));

    cout << "\n=================== SUMMARY ===================\n";
    for (const auto &s : summaries)
    {
        cout << "Category: " << s.category << "\n";
        cout << "  Matched:        " << s.matched << "\n";
        cout << "  Mismatched:     " << s.mismatched << "\n";
        cout << "  Limit Exceeded: " << s.limitExceeded << "\n";
        cout << "  Errors:         " << s.errors << "\n";
        cout << "  Skipped:        " << s.skipped << " (already tested previously)\n";
    }
    cout << "=================================================\n";

    // Clean up the shm scratch directory itself.
    {
        error_code ec;
        fs::remove_all(SHM_DIR, ec);
    }

    return 0;
}