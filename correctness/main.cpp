#include <bits/stdc++.h>
using namespace std;
#include "Edge.hpp"
#include "pairHash.hpp"
#include "FaceTriangulation.hpp"
#include "biconnected.hpp"
#include "triconnected.hpp"

#include <filesystem>
#include <thread>
#include <atomic>
#include <chrono>

namespace fs = std::filesystem;

// ============================================================================
// CONFIGURATION
// Max disk space allowed for binary temporary files per test case (e.g., 1 GB)
// ============================================================================
static const size_t MAX_DISK_BYTES = 1000ULL * 1024ULL * 1024ULL; // 1 GB limit

// ============================================================================
// Disk Monitor (Asynchronous Watchdog Thread)
// ============================================================================
class DiskMonitor
{
    atomic<bool> stopFlag{false};
    atomic<bool> limitExceeded{false};
    thread worker;

public:
    void start(const vector<string> &filesToWatch, size_t maxBytes)
    {
        stopFlag = false;
        limitExceeded = false;
        worker = thread([this, filesToWatch, maxBytes]()
                        {
            while (!stopFlag)
            {
                size_t totalBytes = 0;
                for (const auto &filePath : filesToWatch)
                {
                    if (fs::exists(filePath))
                    {
                        error_code ec;
                        totalBytes += fs::file_size(filePath, ec);
                    }
                }
                if (totalBytes > maxBytes)
                {
                    limitExceeded = true;
                    break;
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

    bool hasExceeded() const
    {
        return limitExceeded;
    }
};

// ============================================================================
// Input reader
// ============================================================================
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

// ============================================================================
// External Sort for binary triangulation files (With Disk Limit Enforcer)
// ============================================================================
struct TriangulationEntry
{
    vector<pair<int, int>> chords;

    bool operator<(const TriangulationEntry &other) const
    {
        return chords < other.chords;
    }
    bool operator>(const TriangulationEntry &other) const
    {
        return chords > other.chords;
    }
};

static bool externalSortBinaryFile(const string &inputFile, const string &outputFile, size_t maxRAMEntries = 500000)
{
    ifstream in(inputFile, ios::binary);
    if (!in.is_open())
        return false;

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
            string cFileName = "temp_chunk_" + to_string(chunkIdx++) + ".bin";
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
            return !diskExceeded;
        }
        else
        {
            string cFileName = "temp_chunk_" + to_string(chunkIdx++) + ".bin";
            if (!writeChunk(buffer, cFileName))
                diskExceeded = true;
            else
                chunkFiles.push_back(cFileName);
            buffer.clear();
        }
    }
    in.close();

    if (diskExceeded)
    {
        for (const auto &file : chunkFiles)
            fs::remove(file);
        return false;
    }

    if (chunkFiles.empty())
    {
        ofstream out(outputFile, ios::binary);
        return true;
    }

    // Priority Queue Min-Heap for K-Way Merge
    struct MergeNode
    {
        TriangulationEntry entry;
        int chunkIndex;

        bool operator>(const MergeNode &other) const
        {
            return entry > other.entry;
        }
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
    {
        streams[i].close();
        fs::remove(chunkFiles[i]);
    }

    return !diskExceeded;
}

// Stream comparison of two binary triangulation files item-by-item
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

// Returns: 1 = matched, 0 = mismatched, -1 = error, -2 = limit exceeded
int matchTwoAlgorithms(const string &filename, size_t &newCount, size_t &oldCount)
{
    vector<vector<int>> faces = solve(filename);
    if (faces.empty())
    {
        newCount = oldCount = 0;
        return -1;
    }

    string fileBC_raw = "temp_bc_raw.bin";
    string fileBC_sorted = "temp_bc_sorted.bin";
    string fileTC_sorted = "temp_tc_sorted.bin";

    DiskMonitor monitor;

    // --- Phase 1: Biconnected Run ---
    biconnected *bc = new biconnected(faces);
    monitor.start({fileBC_raw}, MAX_DISK_BYTES);
    bc->getAllTriangulationsToFile(fileBC_raw);
    bool limitHit = monitor.stop();
    newCount = bc->totalCount;
    delete bc;

    if (limitHit)
    {
        fs::remove(fileBC_raw);
        return -2;
    }

    // --- Phase 2: External Sorting ---
    if (!externalSortBinaryFile(fileBC_raw, fileBC_sorted))
    {
        fs::remove(fileBC_raw);
        fs::remove(fileBC_sorted);
        return -2;
    }
    fs::remove(fileBC_raw);

    // --- Phase 3: Triconnected Run ---
    triconnected *tc = new triconnected(faces);
    tc->getAllTriangulations();

    monitor.start({fileBC_sorted, fileTC_sorted}, MAX_DISK_BYTES);
    tc->refineTriangulationsToFile(fileTC_sorted);
    limitHit = monitor.stop();
    oldCount = tc->totalCount;
    delete tc;

    if (limitHit)
    {
        fs::remove(fileBC_sorted);
        fs::remove(fileTC_sorted);
        return -2;
    }

    // --- Phase 4: Stream Comparison ---
    bool result = compareTriangulationFiles(fileBC_sorted, fileTC_sorted, newCount, oldCount);

    fs::remove(fileBC_sorted);
    fs::remove(fileTC_sorted);

    return result ? 1 : 0;
}

// ============================================================================
// CSV helpers & Driver
// ============================================================================
static string csvPathForCategory(const string &category)
{
    return "results_" + category + ".csv";
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
                            const string &resultStr, size_t newCount, size_t oldCount)
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
        out << "filename,result,newAlgoCount,oldAlgoCount\n";
    }
    out << filename << ',' << resultStr << ',' << newCount << ',' << oldCount << '\n';
}

static void printUsage(const vector<string> &categories, const char *progName)
{
    cerr << "Usage: " << progName << " <category-index|all>\n\n";
    cerr << "Available categories (alphabetical order):\n";
    for (size_t i = 0; i < categories.size(); i++)
    {
        cerr << "  " << (i + 1) << " -> " << categories[i] << "\n";
    }
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
    {
        if (entry.is_regular_file())
        {
            fileList.push_back(entry.path().filename().string());
        }
    }
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
        cout << "  Processing: " << filename << " ... " << flush;

        size_t newCount = 0, oldCount = 0;
        int result = matchTwoAlgorithms(fullPath, newCount, oldCount);

        string resultStr;
        if (result == 1)
        {
            resultStr = "MATCH";
            summ.matched++;
            cout << "\033[1;32mMATCH\033[0m (" << newCount << " triangulations)\n";
        }
        else if (result == 0)
        {
            resultStr = "MISMATCH";
            summ.mismatched++;
            cout << "\033[1;31mMISMATCH\033[0m (new=" << newCount << ", old=" << oldCount << ")\n";
        }
        else if (result == -2)
        {
            resultStr = "LIMIT_EXCEEDED";
            summ.limitExceeded++;
            cout << "\033[1;35mLIMIT_EXCEEDED\033[0m (> " << (MAX_DISK_BYTES / (1024 * 1024)) << " MB limit)\n";
        }
        else
        {
            resultStr = "ERROR";
            summ.errors++;
            cout << "\033[1;33mERROR (could not read file)\033[0m\n";
        }

        appendResultCSV(csvPath, filename, resultStr, newCount, oldCount);
    }

    return summ;
}

int main(int argc, char *argv[])
{
    const string rootFolder = "input";

    if (!fs::exists(rootFolder) || !fs::is_directory(rootFolder))
    {
        cerr << "Input folder '" << rootFolder << "' does not exist.\n";
        return 1;
    }

    vector<string> categories;
    for (const auto &entry : fs::directory_iterator(rootFolder))
    {
        if (entry.is_directory())
        {
            categories.push_back(entry.path().filename().string());
        }
    }
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
    {
        summaries.push_back(runCategory(rootFolder, category));
    }

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

    return 0;
}