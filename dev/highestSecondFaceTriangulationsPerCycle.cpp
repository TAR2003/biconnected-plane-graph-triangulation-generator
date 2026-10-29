// Highest number of second-face triangulations on cycle graphs.
//
// For each cycle C_n, n = 3..MAX_N, run GenerateBiconnectedTriangulationsWithoutVGS
// unchanged. The cycle has two facial cycles. The search triangulates the first
// face, and for each of those triangulations it triangulates the second face.
// This program records, over every first-face triangulation, the minimum,
// maximum, and average number of second-face triangulations that complete the
// graph. Check counters are the algorithm's own total / successful / unsuccessful
// counts for that whole enumeration.
//
// Results are appended to a CSV. Orders already present in that CSV are skipped,
// so a later run continues with the remaining cycles.
//
//   highestSecondFaceTriangulationsPerCycle [MAX_N] [csv_path]
//
// MAX_N defaults to 12. The default CSV is
// dev/highest_second_face_triangulations_per_cycle.csv when run from the repo root.

#include <bits/stdc++.h>
#include <filesystem>
#include <chrono>
#include "../src/GenerateBiconnectedTriangulations.hpp"

using namespace std;
namespace fs = std::filesystem;

static const long long MAX_N = 20;
static const char *CSV_HEADER =
    "n,first_face_triangulations,min_second_face_triangulations,max_second_face_triangulations,"
    "avg_second_face_triangulations,total_triangulations,total_checks,successful_checks,unsuccessful_checks,"
    "percentage_of_successful_checks,time_seconds";

struct CycleResult
{
    long long n = 0;
    long long firstFaceTriangulations = 0;
    long long minSecondFace = 0;
    long long maxSecondFace = 0;
    double avgSecondFace = 0;
    long long totalTriangulations = 0;
    long long totalChecks = 0;
    long long successfulChecks = 0;
    long long unsuccessfulChecks = 0;
    double timeSeconds = 0;
};

class CycleSecondFaceExperiment : public GenerateBiconnectedTriangulationsWithoutVGS
{
public:
    long long firstFaceTriangulations = 0;
    long long minSecondFace = LLONG_MAX;
    long long maxSecondFace = 0;
    long long sumSecondFace = 0;

    CycleSecondFaceExperiment(long long totalVertices, const vector<vector<long long>> &adjacentSet)
        : GenerateBiconnectedTriangulationsWithoutVGS(totalVertices, adjacentSet)
    {
    }

    void output(long long faceIndex) override
    {
        if (faceIndex == 0 && faces.size() >= 2)
        {
            long long before = totalTriangulations;
            generateFaceTriangulations(faceIndex + 1);
            long long produced = totalTriangulations - before;
            ++firstFaceTriangulations;
            sumSecondFace += produced;
            if (produced < minSecondFace)
                minSecondFace = produced;
            if (produced > maxSecondFace)
                maxSecondFace = produced;
            if (firstFaceTriangulations % 1000 == 0)
            {
                cout << "  C_" << totalVertices << ": " << firstFaceTriangulations
                     << " first-face triangulations done\n"
                     << flush;
            }
            return;
        }
        GenerateTriangulations::output(faceIndex);
    }
};

static vector<vector<long long>> cycleAdjacency(long long n)
{
    vector<vector<long long>> adjacency(n);
    for (long long i = 0; i < n; ++i)
    {
        long long previous = (i + n - 1) % n;
        long long next = (i + 1) % n;
        adjacency[i] = {previous, next};
    }
    return adjacency;
}

// C_k, the number of triangulations of a convex (k+2)-gon.
static long long catalan(long long k)
{
    long long value = 1;
    for (long long i = 0; i < k; ++i)
    {
        long long numerator = value * (4 * i + 2);
        long long denominator = i + 2;
        if (numerator % denominator != 0)
            throw runtime_error("Catalan number is not integral");
        value = numerator / denominator;
    }
    return value;
}

static string defaultCsvPath()
{
    if (fs::exists("dev"))
        return "dev/highest_second_face_triangulations_per_cycle.csv";
    return "highest_second_face_triangulations_per_cycle.csv";
}

static string trim(string text)
{
    while (!text.empty() && (text.back() == '\r' || text.back() == ' ' || text.back() == '\t'))
        text.pop_back();
    size_t start = 0;
    while (start < text.size() && (text[start] == ' ' || text[start] == '\t'))
        ++start;
    return text.substr(start);
}

static int columnCount(const string &line)
{
    if (trim(line).empty())
        return 0;
    return (int)count(line.begin(), line.end(), ',') + 1;
}

static set<long long> loadCompletedOrders(const string &csvPath)
{
    set<long long> done;
    if (!fs::exists(csvPath))
        return done;
    ifstream in(csvPath);
    if (!in)
        throw runtime_error("could not read " + csvPath);
    string line;
    while (getline(in, line))
    {
        line = trim(line);
        if (line.empty())
            continue;
        if (columnCount(line) < 10)
            continue;
        string first = line.substr(0, line.find(','));
        try
        {
            size_t used = 0;
            long long n = stoll(first, &used);
            if (used == first.size() && n >= 3)
                done.insert(n);
        }
        catch (const exception &)
        {
        }
    }
    return done;
}

static void writeHeaderIfNeeded(ofstream &out, const string &csvPath)
{
    bool needHeader = !fs::exists(csvPath) || fs::file_size(csvPath) == 0;
    if (needHeader)
        out << CSV_HEADER << "\n";
}

static double successfulCheckPercentage(const CycleResult &row)
{
    return row.totalChecks == 0 ? 0.0 : 100.0 * (double)row.successfulChecks / (double)row.totalChecks;
}

static void appendResult(ofstream &out, const CycleResult &row)
{
    ostringstream average;
    average.setf(ios::fixed);
    average << setprecision(10) << row.avgSecondFace;
    ostringstream percentage;
    percentage.setf(ios::fixed);
    percentage << setprecision(2) << successfulCheckPercentage(row) << "%";
    ostringstream seconds;
    seconds.setf(ios::fixed);
    seconds << setprecision(6) << row.timeSeconds;
    out << row.n << ","
        << row.firstFaceTriangulations << ","
        << row.minSecondFace << ","
        << row.maxSecondFace << ","
        << average.str() << ","
        << row.totalTriangulations << ","
        << row.totalChecks << ","
        << row.successfulChecks << ","
        << row.unsuccessfulChecks << ","
        << percentage.str() << ","
        << seconds.str() << "\n";
    out.flush();
}

static void printResult(const CycleResult &row)
{
    cout << "C_" << row.n
         << ": first=" << row.firstFaceTriangulations
         << "  second-face min=" << row.minSecondFace
         << " max=" << row.maxSecondFace
         << " avg=" << fixed << setprecision(6) << row.avgSecondFace
         << "  total=" << row.totalTriangulations
         << "  checks total=" << row.totalChecks
         << " successful=" << row.successfulChecks
         << " unsuccessful=" << row.unsuccessfulChecks
         << " successful percentage=" << fixed << setprecision(2)
         << successfulCheckPercentage(row) << "%"
         << "  time=" << setprecision(3) << row.timeSeconds << "s\n"
         << flush;
}

static CycleResult runCycle(long long n)
{
    cout << "Running C_" << n << " ...\n" << flush;
    auto adjacency = cycleAdjacency(n);
    auto started = chrono::steady_clock::now();
    CycleSecondFaceExperiment generator(n, adjacency);
    if (n == 3)
    {
        if (!generator.faces.empty())
            throw runtime_error("C_3 should already be triangulated");
    }
    else if ((long long)generator.faces.size() != 2)
    {
        throw runtime_error("C_" + to_string(n) + " should have two faces, found " + to_string(generator.faces.size()));
    }
    generator.generateAllTriangulations();
    auto finished = chrono::steady_clock::now();

    if (generator.successfulChecks + generator.unsuccessfulChecks != generator.totalChecks)
        throw runtime_error("check counters do not add up");

    CycleResult row;
    row.n = n;
    row.totalChecks = generator.totalChecks;
    row.successfulChecks = generator.successfulChecks;
    row.unsuccessfulChecks = generator.unsuccessfulChecks;
    row.timeSeconds = chrono::duration<double>(finished - started).count();

    if (generator.faces.empty())
    {
        row.firstFaceTriangulations = 1;
        row.minSecondFace = 1;
        row.maxSecondFace = 1;
        row.avgSecondFace = 1;
        row.totalTriangulations = generator.totalTriangulations;
        if (row.totalTriangulations != 1)
            throw runtime_error("an already triangulated cycle should have one triangulation");
    }
    else
    {
        if (generator.firstFaceTriangulations == 0)
            throw runtime_error("no first-face triangulations were generated");
        if (generator.sumSecondFace != generator.totalTriangulations)
            throw runtime_error("second-face counts do not sum to the total triangulation count");
        long long expectedFirst = catalan(n - 2);
        if (generator.firstFaceTriangulations != expectedFirst)
        {
            throw runtime_error("first-face count " + to_string(generator.firstFaceTriangulations) +
                                " does not match Catalan C_" + to_string(n - 2) + " = " + to_string(expectedFirst));
        }
        row.firstFaceTriangulations = generator.firstFaceTriangulations;
        row.minSecondFace = generator.minSecondFace;
        row.maxSecondFace = generator.maxSecondFace;
        row.avgSecondFace = (double)generator.sumSecondFace / (double)generator.firstFaceTriangulations;
        row.totalTriangulations = generator.totalTriangulations;
    }
    return row;
}

int main(int argc, char **argv)
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    long long maxN = MAX_N;
    string csvPath = defaultCsvPath();
    try
    {
        if (argc >= 2)
            maxN = stoll(argv[1]);
        if (argc >= 3)
            csvPath = argv[2];
    }
    catch (const exception &)
    {
        cerr << "Usage: highestSecondFaceTriangulationsPerCycle [MAX_N] [csv_path]\n";
        return 1;
    }
    if (maxN < 3)
    {
        cerr << "MAX_N must be at least 3\n";
        return 1;
    }

    try
    {
        set<long long> done = loadCompletedOrders(csvPath);
        vector<long long> pending;
        for (long long n = 3; n <= maxN; ++n)
        {
            if (!done.count(n))
                pending.push_back(n);
        }

        cout << "CSV: " << csvPath << "\n";
        cout << "Cycle orders already recorded: " << done.size() << "\n";
        if (pending.empty())
        {
            cout << "Every cycle from C_3 through C_" << maxN << " is already in the CSV. Nothing to run.\n";
            return 0;
        }
        cout << "Running remaining cycles:";
        for (long long n : pending)
            cout << " C_" << n;
        cout << "\n" << flush;

        ofstream out(csvPath, ios::app);
        if (!out)
        {
            cerr << "Could not open " << csvPath << " for writing\n";
            return 1;
        }
        writeHeaderIfNeeded(out, csvPath);

        for (long long n : pending)
        {
            CycleResult row = runCycle(n);
            appendResult(out, row);
            printResult(row);
        }
    }
    catch (const exception &error)
    {
        cerr << "Error: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
