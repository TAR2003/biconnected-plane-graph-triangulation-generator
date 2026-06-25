#include <bits/stdc++.h>
using namespace std;
#include "Edge.hpp"
#include "pairHash.hpp"
#include "biconnected.hpp"
#include "FaceTriangulation.hpp"
#include <filesystem>
#include <chrono>

using u128 = unsigned __int128;
namespace fs = std::filesystem;

// ============================================================================
// Input Reader
// ============================================================================
vector<vector<int>> readInput(const string &filename, int &distinctVertices)
{
    ifstream infile(filename);
    if (!infile.is_open())
    {
        cerr << "Error opening file: " << filename << endl;
        distinctVertices = 0;
        return {};
    }
    vector<vector<int>> faces;
    unordered_set<int> uniqueVertices;
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
            uniqueVertices.insert(vertex);
        }
        faces.push_back(face);
    }
    distinctVertices = uniqueVertices.size();
    return faces;
}

static string stemName(const string &filename)
{
    return fs::path(filename).stem().string();
}

static string formatWallClock(const chrono::system_clock::time_point &tp)
{
    auto t = chrono::system_clock::to_time_t(tp);
    tm localTm{};
#ifdef _WIN32
    localtime_s(&localTm, &t);
#else
    localtime_r(&t, &localTm);
#endif
    auto sinceEpoch = tp.time_since_epoch();
    auto micros = chrono::duration_cast<chrono::microseconds>(sinceEpoch).count() % 1'000'000;
    ostringstream oss;
    oss << put_time(&localTm, "%H:%M:%S") << '.' << setfill('0') << setw(6) << micros;
    return oss.str();
}

// Records elapsed nanoseconds since run start at each triangulation.
// All timestamps stay in memory during the run; CSV is written only afterward.
class TriangulationTimer
{
public:
    using SteadyClock = chrono::steady_clock;

    void begin()
    {
        runStart_ = SteadyClock::now();
        wallStart_ = chrono::system_clock::now();
        elapsedNs_.clear();
    }

    void record()
    {
        auto now = SteadyClock::now();
        uint64_t ns = static_cast<uint64_t>(
            chrono::duration_cast<chrono::nanoseconds>(now - runStart_).count());
        elapsedNs_.push_back(ns);
    }

    const vector<uint64_t> &elapsedNs() const { return elapsedNs_; }
    chrono::system_clock::time_point wallStart() const { return wallStart_; }

private:
    SteadyClock::time_point runStart_{};
    chrono::system_clock::time_point wallStart_{};
    vector<uint64_t> elapsedNs_;
};

static void writeTimingCsv(const string &csvPath, const TriangulationTimer &timer)
{
    const auto &elapsed = timer.elapsedNs();
    ofstream out(csvPath);
    if (!out.is_open())
    {
        cerr << "Error: could not write " << csvPath << endl;
        return;
    }

    out << "triangulation_n,wall_clock,elapsed_since_start_ns,incremental_ns,cumulative_avg_ns\n";
    uint64_t prev = 0;
    uint64_t cumulativeSum = 0;
    for (size_t i = 0; i < elapsed.size(); ++i)
    {
        uint64_t incremental = elapsed[i] - prev;
        cumulativeSum += incremental;
        uint64_t cumulativeAvg = cumulativeSum / (i + 1);

        auto wallTp = timer.wallStart() + chrono::nanoseconds(elapsed[i]);
        out << (i + 1) << ','
            << formatWallClock(wallTp) << ','
            << elapsed[i] << ','
            << incremental << ','
            << cumulativeAvg << '\n';
        prev = elapsed[i];
    }
}

static string u128ToString(u128 x)
{
    if (x == 0)
        return "0";
    string s;
    while (x > 0)
    {
        s.push_back('0' + static_cast<char>(x % 10));
        x /= 10;
    }
    reverse(s.begin(), s.end());
    return s;
}

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    fs::create_directories("results");
    fs::create_directories("graphs");

    ofstream logFile("results.txt");
    if (!logFile.is_open())
    {
        cerr << "Error: could not create results.txt\n";
        return 1;
    }

    struct tee_buf : streambuf
    {
        streambuf *sb1, *sb2;
        tee_buf(streambuf *s1, streambuf *s2) : sb1(s1), sb2(s2) {}
        int overflow(int c) override
        {
            if (c == EOF)
                return !EOF;
            int r1 = sb1->sputc(c);
            sb2->sputc(c);
            return (r1 == EOF) ? EOF : c;
        }
        int sync() override
        {
            int r1 = sb1->pubsync();
            int r2 = sb2->pubsync();
            return (r1 == 0 && r2 == 0) ? 0 : -1;
        }
    } tbuf(cout.rdbuf(), logFile.rdbuf());
    ostream out(&tbuf);

    struct FolderGroup
    {
        string folder;
        string label;
    };
    vector<FolderGroup> groups = {
        {"input/small", "small"},
        {"input/medium", "medium"},
        {"input/big", "big"},
    };

    out << "\n";
    out << "================================================================\n";
    out << "  Per-Triangulation Timing Recorder (averageTimeGraph)\n";
    out << "================================================================\n";

    for (const auto &group : groups)
    {
        const string &folder = group.folder;
        out << "\nScanning folder: " << folder << "\n\n";

        if (!fs::exists(folder) || !fs::is_directory(folder))
        {
            cerr << "Folder '" << folder << "' does not exist or is not a directory.\n";
            continue;
        }

        vector<string> fileList;
        for (const auto &entry : fs::directory_iterator(folder))
        {
            if (entry.is_regular_file())
                fileList.push_back(entry.path().filename().string());
        }
        sort(fileList.begin(), fileList.end());

        for (const string &filename : fileList)
        {
            const string fullpath = folder + "/" + filename;
            const string stem = stemName(filename);
            const string csvPath = "results/" + stem + ".csv";

            if (fs::exists(csvPath))
            {
                out << "Skipping already-recorded file: " << filename << "\n";
                continue;
            }

            out << "Processing: " << filename << " ... " << flush;

            int distinctVertices = 0;
            vector<vector<int>> faces = readInput(fullpath, distinctVertices);
            if (faces.empty())
            {
                cerr << "\nWarning: skipping empty or invalid file: " << filename << endl;
                continue;
            }

            TriangulationTimer timer;
            timer.begin();

            biconnected bc(faces);
            bc.onTriangulationGenerated = [&timer]() { timer.record(); };
            bc.getAllTriangulations();

            writeTimingCsv(csvPath, timer);

            out << "done (" << u128ToString(bc.totalTriangulations) << " triangulations, "
                << timer.elapsedNs().size() << " records -> " << csvPath << ")\n";
        }
    }

    out << "\nAttempting to create graphs (requires Python with pandas/matplotlib)...\n";
    if (fs::exists("plot_results.py"))
    {
        FILE *pipe = popen("python3 plot_results.py results 2>&1", "r");
        if (!pipe)
        {
            out << "  (failed to launch plotting script)\n";
        }
        else
        {
            char buf[256];
            while (fgets(buf, sizeof(buf), pipe))
                out << "  " << buf;
            int status = pclose(pipe);
            if (status != 0)
                out << "  (plotting script exited with code " << status << ")\n";
        }
    }
    else
    {
        out << "  (plot_results.py not found)\n";
    }

    logFile.close();
    return 0;
}
