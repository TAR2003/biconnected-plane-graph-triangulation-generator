// ============================================================================
// SET 2: Zero-disk, hash-based correctness comparator.
//
// Instead of writing every triangulation from both algorithms to files,
// sorting them, and diffing byte-for-byte, this variant:
//   1. Runs each algorithm fully in RAM (using their existing in-memory
//      `allTriangulations` path — no file output at all, since we never
//      call getAllTriangulationsToFile / refineTriangulationsToFile).
//   2. Canonicalizes each triangulation (already sorted by chord, as the
//      original code did) and hashes it with a strong 64-bit hash (splitmix64
//      finalizer applied per chord, combined — order-independent within the
//      chord list because we sort first).
//   3. Combines all triangulation hashes for one algorithm into TWO
//      different order-independent aggregates:
//        - XOR aggregate  (catches missing/extra/altered elements; XOR is
//          its own inverse, so a differing element cannot silently cancel
//          out via addition wraparound tricks)
//        - Sum aggregate (mod 2^64, wrapping) - different collision profile
//          than XOR, so an adversarial/unlucky case that fools XOR alone is
//          extremely unlikely to also fool the sum.
//   4. Compares: total counts must match, AND both aggregates must match.
//      Any single one of these three checks failing => MISMATCH.
//
// This costs zero disk I/O and only as much RAM as one algorithm's full
// triangulation set at a time (they are generated and hashed sequentially,
// so peak RAM is ~1 set, not both at once - see Phase 1/Phase 2 below,
// each set is discarded before starting the next one keeps that low but
// still in-RAM since we need to fully generate it before hashing it -- that
// is a property of how biconnected/triconnected currently expose results,
// see the NOTE below if you want true incremental streaming later).
//
// NOTE on going further (optional): right now biconnected/triconnected only
// hand back a finished vector<vector<pair<int,int>>> (`allTriangulations`)
// once generation is fully done - there's no per-item callback. If you
// want genuinely zero-materialization streaming (hash each triangulation
// the instant it's produced, discard immediately, hold nothing but the
// two running aggregates), the only change needed is a one-line hook in
// biconnected::addTriangulation() and triconnected::combineTriangulationsDFS()
// leaf: instead of (or in addition to) allTriangulations.push_back(...),
// call an optional std::function<void(const vector<pair<int,int>>&)>
// callback if one is set. That's the only edit those two headers would
// ever need for this approach; everything else stays in main.cpp. Ask if
// you want that wired up - it's a small, additive change.
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
#include <chrono>

namespace fs = std::filesystem;

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
// Hashing
// ============================================================================

// splitmix64 finalizer - excellent avalanche, standard choice for combining
// integer hashes cheaply and well. (Public-domain construction, widely used
// e.g. inside various STL/Abseil-adjacent hash combiners.)
static inline uint64_t splitmix64(uint64_t x)
{
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    x = x ^ (x >> 31);
    return x;
}

static inline uint64_t hashCombine(uint64_t seed, uint64_t v)
{
    // boost::hash_combine-style mixing, but seeded through splitmix64 first.
    seed ^= splitmix64(v) + 0x9E3779B97F4A7C15ULL + (seed << 6) + (seed >> 2);
    return seed;
}

// Hashes ONE canonicalized (already-sorted) triangulation into a single
// 64-bit value. Order of chords WITHIN a triangulation matters for this to
// be deterministic across algorithms, which is exactly why both algorithms
// already sort each triangulation's chord list before this point (matches
// your existing `sort(currentTriangulation.begin(), ...)` convention).
static uint64_t hashOneTriangulation(const vector<pair<int, int>> &chords)
{
    uint64_t h = 1469598103934665603ULL; // FNV offset basis, arbitrary fixed seed
    for (const auto &c : chords)
    {
        uint64_t packed = (static_cast<uint64_t>(static_cast<uint32_t>(c.first)) << 32) |
                          static_cast<uint32_t>(c.second);
        h = hashCombine(h, packed);
    }
    return h;
}

// Aggregate holds both an XOR-fold and a wrapping-sum-fold of all
// per-triangulation hashes, plus the count. Two triangulation SETS are
// considered equal only if count matches AND both aggregates match.
struct SetAggregate
{
    uint64_t xorAgg = 0;
    uint64_t sumAgg = 0;
    size_t count = 0;

    void addTriangulation(const vector<pair<int, int>> &sortedChords)
    {
        uint64_t h = hashOneTriangulation(sortedChords);
        xorAgg ^= h;
        sumAgg += h; // wraps naturally in uint64_t, that's fine/expected
        count++;
    }

    bool operator==(const SetAggregate &other) const
    {
        return count == other.count && xorAgg == other.xorAgg && sumAgg == other.sumAgg;
    }
};

// ============================================================================
// Result bundle
// ============================================================================
struct MatchResult
{
    int code = -1; // 1 = matched, 0 = mismatched, -1 = error
    size_t newCount = 0;
    size_t oldCount = 0;
    bool countMatch = false;
    bool xorMatch = false;
    bool sumMatch = false;
    string startTs;
    string endTs;
};

// Runs both algorithms fully in RAM (no disk I/O anywhere), aggregates each
// into a SetAggregate, and compares. Peak memory is bounded by whichever of
// the two algorithms' full triangulation sets is larger (they run
// sequentially - algorithm A's vector is fully consumed into hashes and
// then goes out of scope before algorithm B's run starts), NOT both at
// once.
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

    try
    {
        SetAggregate aggBC;
        {
            // Phase 1: biconnected, in-memory path (outStream stays nullptr,
            // so biconnected::output()/addTriangulation() pushes into
            // bc.allTriangulations instead of writing to disk).
            biconnected bc(faces);
            bc.getAllTriangulations();

            for (auto &tri : bc.allTriangulations)
            {
                // bc.allTriangulations entries are already sorted per your
                // existing addTriangulation() logic (sort(...) is called on
                // each face's chords before insertion, and the overall
                // concatenation order follows face order deterministically
                // for a given face list, so this is already canonical
                // per-triangulation).
                aggBC.addTriangulation(tri);
            }
            res.newCount = bc.totalCount;
            // bc (and its allTriangulations) goes out of scope here, freeing
            // that memory before Phase 2 starts.
        }

        SetAggregate aggTC;
        {
            // Phase 2: triconnected, in-memory path (refineTriangulations(),
            // not refineTriangulationsToFile(), so outStream stays nullptr
            // and results land in tc.allTriangulations instead of a file).
            triconnected tc(faces);
            tc.getAllTriangulations();
            tc.refineTriangulations();

            for (auto &tri : tc.allTriangulations)
            {
                aggTC.addTriangulation(tri);
            }
            res.oldCount = tc.totalCount;
        }

        res.countMatch = (res.newCount == res.oldCount);
        res.xorMatch = (aggBC.xorAgg == aggTC.xorAgg);
        res.sumMatch = (aggBC.sumAgg == aggTC.sumAgg);

        bool matched = res.countMatch && res.xorMatch && res.sumMatch;

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

// ============================================================================
// CSV helpers & Driver
// ============================================================================
static string csvPathForCategory(const string &category)
{
    return "results_hash_" + category + ".csv";
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
               "countMatch,xorHashMatch,sumHashMatch,"
               "startTime,endTime,elapsedSeconds\n";
    }
    out << filename << ',' << resultStr << ',' << res.newCount << ',' << res.oldCount << ','
        << (res.countMatch ? "yes" : "no") << ',' << (res.xorMatch ? "yes" : "no") << ','
        << (res.sumMatch ? "yes" : "no") << ','
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
                 << elapsed << "s, end " << res.endTs << ")\n";
        }
        else if (res.code == 0)
        {
            resultStr = "MISMATCH";
            summ.mismatched++;
            cout << "\033[1;31mMISMATCH\033[0m (new=" << res.newCount << ", old=" << res.oldCount
                 << ", countMatch=" << (res.countMatch ? "yes" : "no")
                 << ", xorMatch=" << (res.xorMatch ? "yes" : "no")
                 << ", sumMatch=" << (res.sumMatch ? "yes" : "no")
                 << ", " << elapsed << "s, end " << res.endTs << ")\n";
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
        cout << "  Errors:         " << s.errors << "\n";
        cout << "  Skipped:        " << s.skipped << " (already tested previously)\n";
    }
    cout << "=================================================\n";

    return 0;
}