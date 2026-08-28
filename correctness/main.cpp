#include <bits/stdc++.h>
using namespace std;
#include "Edge.hpp"
#include "pairHash.hpp"
#include "biconnected.hpp"
#include "FaceTriangulation.hpp"
#include "triconnected.hpp"

#include <filesystem>
namespace fs = std::filesystem;

// ============================================================================
// Input reader (same format as before: face count, then per-face vertex list)
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

bool matchPairs(const pair<int, int> &p1, const pair<int, int> &p2)
{
    return (p1.first == p2.first && p1.second == p2.second);
}

bool matchTriangulations(const vector<pair<int, int>> &t1, const vector<pair<int, int>> &t2)
{
    if (t1.size() != t2.size())
        return false;
    for (size_t i = 0; i < t1.size(); i++)
    {
        if (!matchPairs(t1[i], t2[i]))
            return false;
    }
    return true;
}

// Returns: 1 = matched, 0 = mismatched, -1 = error (couldn't read file)
int matchTwoAlgorithms(const string &filename, size_t &newCount, size_t &oldCount)
{
    vector<vector<int>> faces = solve(filename);
    if (faces.empty())
    {
        newCount = oldCount = 0;
        return -1;
    }

    biconnected *bc = new biconnected(faces);
    bc->getAllTriangulations();
    bc->sortTriangulations();

    triconnected *tc = new triconnected(faces);
    tc->getAllTriangulations();
    tc->refineTriangulations();

    auto newalgo = bc->allTriangulations;
    auto oldalgo = tc->allTriangulations;

    newCount = newalgo.size();
    oldCount = oldalgo.size();

    bool result = true;
    if (newalgo.size() != oldalgo.size())
    {
        result = false;
    }
    else
    {
        for (size_t i = 0; i < newalgo.size(); i++)
        {
            if (!matchTriangulations(newalgo[i], oldalgo[i]))
            {
                result = false;
                break;
            }
        }
    }

    delete bc;
    delete tc;

    return result ? 1 : 0;
}

// ============================================================================
// Per-category CSV helpers
// ============================================================================

// CSV format: filename,result,newCount,oldCount
// result is one of: MATCH, MISMATCH, ERROR

static string csvPathForCategory(const string &category)
{
    return "results_" + category + ".csv";
}

// Load the set of filenames (base name only, not full path) already present
// in a category's CSV, so we can skip them.
static unordered_set<string> loadProcessedFiles(const string &csvPath)
{
    unordered_set<string> processed;
    ifstream in(csvPath);
    if (!in.is_open())
        return processed;

    string line;
    // skip header
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

// ============================================================================
// Category discovery: every immediate subdirectory of the input root folder
// is treated as its own category, e.g.
//   input/cycle_3_to_15
//   input/delaunay_subdivide_1x
//   input/delaunay_subdivide_2x
//   input/delaunay_subdivide_3x
//   input/halin_graph
// New categories can simply be added as new folders under "input" and will
// be picked up automatically; no code changes required.
// ============================================================================

int main()
{
    const string rootFolder = "input";

    if (!fs::exists(rootFolder) || !fs::is_directory(rootFolder))
    {
        cerr << "Input folder '" << rootFolder << "' does not exist.\n";
        return 1;
    }

    // gather categories (subdirectories of input/)
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

    // overall summary across all categories, printed at the end
    struct Summary
    {
        string category;
        int matched = 0;
        int mismatched = 0;
        int errors = 0;
        int skipped = 0;
    };
    vector<Summary> summaries;

    for (const auto &category : categories)
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
            else
            {
                resultStr = "ERROR";
                summ.errors++;
                cout << "\033[1;33mERROR (could not read file)\033[0m\n";
            }

            appendResultCSV(csvPath, filename, resultStr, newCount, oldCount);
        }

        summaries.push_back(summ);
    }

    // ------------------------------------------------------------------
    // Final summary
    // ------------------------------------------------------------------
    cout << "\n=================== SUMMARY ===================\n";
    for (const auto &s : summaries)
    {
        cout << "Category: " << s.category << "\n";
        cout << "  Matched:    " << s.matched << "\n";
        cout << "  Mismatched: " << s.mismatched << "\n";
        cout << "  Errors:     " << s.errors << "\n";
        cout << "  Skipped:    " << s.skipped << " (already tested previously)\n";
    }
    cout << "=================================================\n";

    return 0;
}