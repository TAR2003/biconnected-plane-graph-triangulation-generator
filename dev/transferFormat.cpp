#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;
using namespace std;

// ============================================================
// FACE LIST -> ROTATION SYSTEM
// ============================================================

void convertFaceListToRotationSystem(
    int num_faces,
    const vector<vector<int>> &faces,
    int &N,
    vector<vector<int>> &adj)
{
    set<int> unique_vertices;

    for (const auto &face : faces)
    {
        for (int v : face)
            unique_vertices.insert(v);
    }

    if (unique_vertices.empty())
    {
        N = 0;
        adj.clear();
        return;
    }

    map<int, int> orig_to_idx;
    vector<int> idx_to_orig;

    int idx = 0;

    for (int v : unique_vertices)
    {
        orig_to_idx[v] = idx++;
        idx_to_orig.push_back(v);
    }

    N = static_cast<int>(idx_to_orig.size());

    map<pair<int, int>, vector<int>> next_out;
    vector<set<int>> incoming_edges(N);

    for (const auto &face : faces)
    {
        int k = static_cast<int>(face.size());

        if (k < 2)
            continue;

        for (int i = 0; i < k; ++i)
        {
            int w = orig_to_idx[face[i]];
            int u = orig_to_idx[face[(i + 1) % k]];
            int v = orig_to_idx[face[(i + 2) % k]];

            next_out[{u, w}].push_back(v);
            incoming_edges[u].insert(w);
        }
    }

    adj.assign(N, vector<int>());

    for (int u = 0; u < N; ++u)
    {
        if (incoming_edges[u].empty())
            continue;

        vector<int> order;
        set<int> in_order;
        set<pair<int, int>> visited_halfedges;

        for (int start_w : incoming_edges[u])
        {
            if (visited_halfedges.count({u, start_w}))
                continue;

            int curr_w = start_w;

            while (!visited_halfedges.count({u, curr_w}))
            {
                visited_halfedges.insert({u, curr_w});

                auto it = next_out.find({u, curr_w});

                if (it != next_out.end() && !it->second.empty())
                {
                    int next_v = it->second.front();

                    if (in_order.find(next_v) == in_order.end())
                    {
                        order.push_back(next_v);
                        in_order.insert(next_v);
                    }

                    if (it->second.size() > 1)
                        it->second.erase(it->second.begin());

                    curr_w = next_v;
                }
                else
                {
                    break;
                }
            }
        }

        adj[u] = order;
    }

    // Ensure adjacency is symmetric.
    for (int u = 0; u < N; ++u)
    {
        for (int v : adj[u])
        {
            auto it = find(adj[v].begin(), adj[v].end(), u);

            if (it == adj[v].end())
                adj[v].push_back(u);
        }
    }
}

// ============================================================
// ROTATION SYSTEM -> FACE LIST
// ============================================================

vector<vector<long long>> rotationSystemToFaces(
    long long totalNodes,
    const vector<vector<long long>> &rawAdjacency)
{
    struct HalfEdge
    {
        long long u;
        long long v;
        bool visited = false;
    };

    set<long long> uniqueNodes;

    for (long long u = 0; u < totalNodes; ++u)
    {
        uniqueNodes.insert(u);

        if (u < static_cast<long long>(rawAdjacency.size()))
        {
            for (long long v : rawAdjacency[u])
                uniqueNodes.insert(v);
        }
    }

    map<long long, long long> originalToIndex;
    vector<long long> indexToOriginal;

    for (long long node : uniqueNodes)
    {
        originalToIndex[node] =
            static_cast<long long>(indexToOriginal.size());

        indexToOriginal.push_back(node);
    }

    const long long nodeCount =
        static_cast<long long>(indexToOriginal.size());

    vector<vector<long long>> adjacency(
        static_cast<size_t>(nodeCount));

    for (long long u = 0; u < totalNodes; ++u)
    {
        auto source = originalToIndex.find(u);

        if (source == originalToIndex.end())
            continue;

        if (u >= static_cast<long long>(rawAdjacency.size()))
            continue;

        for (long long v : rawAdjacency[u])
        {
            auto target = originalToIndex.find(v);

            if (target != originalToIndex.end())
            {
                adjacency[source->second].push_back(
                    target->second);
            }
        }
    }

    // --------------------------------------------------------
    // Create half-edges
    // --------------------------------------------------------

    map<pair<long long, long long>, long long> edgeId;

    vector<HalfEdge> halfEdges;

    for (long long u = 0; u < nodeCount; ++u)
    {
        for (long long v : adjacency[u])
        {
            if (edgeId.find({u, v}) == edgeId.end())
            {
                edgeId[{u, v}] =
                    static_cast<long long>(halfEdges.size());

                halfEdges.push_back({u, v});
            }
        }
    }

    // --------------------------------------------------------
    // Determine next half-edge around each face
    // --------------------------------------------------------

    vector<long long> nextEdge(
        halfEdges.size(), -1);

    for (long long i = 0;
         i < static_cast<long long>(halfEdges.size());
         ++i)
    {
        const long long u = halfEdges[i].u;
        const long long v = halfEdges[i].v;

        const auto &neighbors = adjacency[v];

        if (neighbors.empty())
            continue;

        auto it = find(
            neighbors.begin(),
            neighbors.end(),
            u);

        if (it == neighbors.end())
            continue;

        const long long position =
            static_cast<long long>(
                it - neighbors.begin());

        const long long previous =
            (position - 1 + neighbors.size()) %
            neighbors.size();

        auto next =
            edgeId.find(
                {v, neighbors[previous]});

        if (next != edgeId.end())
            nextEdge[i] = next->second;
    }

    // --------------------------------------------------------
    // Traverse faces
    // --------------------------------------------------------

    vector<vector<long long>> faces;

    for (long long i = 0;
         i < static_cast<long long>(halfEdges.size());
         ++i)
    {
        if (halfEdges[i].visited)
            continue;

        if (nextEdge[i] == -1)
            continue;

        vector<long long> face;

        long long current = i;

        while (current != -1 &&
               !halfEdges[current].visited)
        {
            halfEdges[current].visited = true;

            face.push_back(
                indexToOriginal[halfEdges[current].u]);

            current = nextEdge[current];
        }

        if (!face.empty())
            faces.push_back(std::move(face));
    }

    return faces;
}

// ============================================================
// NORMALIZE ORIGINAL INPUT
//
// If ANY original vertex is 0:
//     input is considered 0-based.
//
// Otherwise:
//     input is considered 1-based,
//     so subtract 1 from every vertex.
// ============================================================

void normalizeOriginalFaces(
    vector<vector<int>> &faces)
{
    bool isZeroBased = false;

    for (const auto &face : faces)
    {
        for (int v : face)
        {
            if (v == 0)
            {
                isZeroBased = true;
                break;
            }
        }

        if (isZeroBased)
            break;
    }

    if (!isZeroBased)
    {
        for (auto &face : faces)
        {
            for (int &v : face)
                --v;
        }
    }
}

// ============================================================
// GET MULTIPLICITY PATTERN OF A FACE
//
// Example:
//
//  0 0 1 2
//
// frequencies:
//  0 -> 2
//  1 -> 1
//  2 -> 1
//
// pattern = [1, 1, 2]
//
// The actual vertex labels do not matter.
// ============================================================

vector<int> getMultiplicityPattern(
    const vector<int> &face)
{
    map<int, int> frequency;

    for (int v : face)
        ++frequency[v];

    vector<int> pattern;

    for (const auto &[vertex, count] : frequency)
        pattern.push_back(count);

    sort(pattern.begin(), pattern.end());

    return pattern;
}

vector<int> getMultiplicityPattern(
    const vector<long long> &face)
{
    map<long long, int> frequency;

    for (long long v : face)
        ++frequency[v];

    vector<int> pattern;

    for (const auto &[vertex, count] : frequency)
        pattern.push_back(count);

    sort(pattern.begin(), pattern.end());

    return pattern;
}

struct FailedTestCase
{
    fs::path path;
    string reason;
};

template <typename T>
string facesToString(const vector<vector<T>> &faces)
{
    ostringstream out;

    for (size_t i = 0; i < faces.size(); ++i)
    {
        out << "    Face " << i + 1 << ": ";

        for (T v : faces[i])
            out << v << ' ';

        out << '\n';
    }

    return out.str();
}

string buildMismatchDescription(
    const vector<vector<int>> &originalFaces,
    const vector<vector<long long>> &generatedFaces)
{
    vector<pair<int, vector<int>>> originalSignatures;
    vector<pair<int, vector<int>>> generatedSignatures;

    for (const auto &face : originalFaces)
    {
        originalSignatures.push_back(
            {static_cast<int>(face.size()),
             getMultiplicityPattern(face)});
    }

    for (const auto &face : generatedFaces)
    {
        generatedSignatures.push_back(
            {static_cast<int>(face.size()),
             getMultiplicityPattern(face)});
    }

    sort(originalSignatures.begin(), originalSignatures.end());
    sort(generatedSignatures.begin(), generatedSignatures.end());

    vector<pair<int, vector<int>>> missing;
    vector<pair<int, vector<int>>> extra;

    set_difference(
        originalSignatures.begin(), originalSignatures.end(),
        generatedSignatures.begin(), generatedSignatures.end(),
        back_inserter(missing));

    set_difference(
        generatedSignatures.begin(), generatedSignatures.end(),
        originalSignatures.begin(), originalSignatures.end(),
        back_inserter(extra));

    auto signatureToString = [](const pair<int, vector<int>> &signature)
    {
        ostringstream out;

        out << "size=" << signature.first << " multiplicities={";

        for (size_t i = 0; i < signature.second.size(); ++i)
        {
            if (i)
                out << ',';

            out << signature.second[i];
        }

        out << '}';
        return out.str();
    };

    ostringstream out;

    out << "Original face count: " << originalFaces.size()
        << ", Generated face count: " << generatedFaces.size()
        << "\n  Missing / mismatched original faces:\n";

    for (const auto &signature : missing)
        out << "    - " << signatureToString(signature) << '\n';

    out << "  Extra / mismatched generated faces:\n";

    for (const auto &signature : extra)
        out << "    + " << signatureToString(signature) << '\n';

    return out.str();
}

// ============================================================
// COMPARE FACES
//
// Checks:
//
// 1. Same number of faces
// 2. Same multiset of face sizes
// 3. Same multiplicity pattern for every face
//
// Face ordering is ignored.
// Vertex ordering is ignored.
// Actual numerical labels are irrelevant after normalization.
// ============================================================

bool compareFaces(
    const vector<vector<int>> &originalFaces,
    const vector<vector<long long>> &generatedFaces)
{
    // --------------------------------------------------------
    // Same number of faces?
    // --------------------------------------------------------

    if (originalFaces.size() != generatedFaces.size())
        return false;

    // --------------------------------------------------------
    // Build comparable representation
    // --------------------------------------------------------

    vector<pair<int, vector<int>>> originalNormalized;
    vector<pair<int, vector<int>>> generatedNormalized;

    for (const auto &face : originalFaces)
    {
        int size = static_cast<int>(face.size());

        originalNormalized.push_back(
            {size,
             getMultiplicityPattern(face)});
    }

    for (const auto &face : generatedFaces)
    {
        int size = static_cast<int>(face.size());

        generatedNormalized.push_back(
            {size,
             getMultiplicityPattern(face)});
    }

    // --------------------------------------------------------
    // Face order does not matter
    // --------------------------------------------------------

    sort(
        originalNormalized.begin(),
        originalNormalized.end());

    sort(
        generatedNormalized.begin(),
        generatedNormalized.end());

    return originalNormalized ==
           generatedNormalized;
}

// ============================================================
// PRINT FACES
// ============================================================

template <typename T>
void printFaces(
    const vector<vector<T>> &faces)
{
    for (size_t i = 0; i < faces.size(); ++i)
    {
        cout << "  Face " << i + 1 << ": ";

        for (T v : faces[i])
            cout << v << ' ';

        cout << '\n';
    }
}

// ============================================================
// MAIN
// ============================================================

int main()
{
    const fs::path inputDirectory = "previnput";

    if (!fs::exists(inputDirectory))
    {
        cerr << "ERROR: Directory 'previnput' does not exist.\n";
        return 1;
    }

    size_t totalFiles = 0;
    size_t passedFiles = 0;
    size_t failedFiles = 0;

    vector<FailedTestCase> failedTestCases;

    // --------------------------------------------------------
    // Recursively process all regular files
    // --------------------------------------------------------

    for (const auto &entry :
         fs::recursive_directory_iterator(inputDirectory))
    {
        if (!entry.is_regular_file())
            continue;

        ++totalFiles;

        const fs::path filePath = entry.path();

        cout << "\n============================================================\n";
        cout << "TEST CASE: " << filePath << '\n';
        cout << "============================================================\n";

        ifstream fin(filePath);

        if (!fin)
        {
            cerr << "Could not open file.\n";

            ++failedFiles;
            failedTestCases.push_back({filePath, "Could not open file."});

            continue;
        }

        // ----------------------------------------------------
        // Read number of faces
        // ----------------------------------------------------

        int numFaces;

        if (!(fin >> numFaces))
        {
            cerr << "Could not read number of faces.\n";

            ++failedFiles;
            failedTestCases.push_back(
                {filePath, "Could not read number of faces."});

            continue;
        }

        vector<vector<int>> originalFaces;

        bool readError = false;

        // ----------------------------------------------------
        // Read faces
        //
        // Expected format:
        //
        // numFaces
        // size v1 v2 v3 ...
        // size v1 v2 ...
        // ----------------------------------------------------

        for (int i = 0; i < numFaces; ++i)
        {
            int faceSize;

            if (!(fin >> faceSize))
            {
                readError = true;
                break;
            }

            vector<int> face(faceSize);

            for (int j = 0; j < faceSize; ++j)
            {
                if (!(fin >> face[j]))
                {
                    readError = true;
                    break;
                }
            }

            if (readError)
                break;

            originalFaces.push_back(std::move(face));
        }

        if (readError)
        {
            cerr << "Error while reading faces.\n";

            ++failedFiles;
            failedTestCases.push_back(
                {filePath, "Error while reading faces."});

            continue;
        }

        // ----------------------------------------------------
        // Detect 0-based / 1-based and normalize.
        // ----------------------------------------------------

        bool isZeroBased = false;

        for (const auto &face : originalFaces)
        {
            for (int v : face)
            {
                if (v == 0)
                {
                    isZeroBased = true;
                    break;
                }
            }

            if (isZeroBased)
                break;
        }

        if (!isZeroBased)
        {
            // 1-based -> 0-based
            for (auto &face : originalFaces)
            {
                for (int &v : face)
                    --v;
            }

            cout << "Detected representation: 1-based\n";
            cout << "Converted original vertices to 0-based.\n";
        }
        else
        {
            cout << "Detected representation: 0-based\n";
            cout << "No vertex conversion performed.\n";
        }

        // ----------------------------------------------------
        // Convert face list -> rotation system
        // ----------------------------------------------------

        int N = 0;
        vector<vector<int>> adjacency;

        convertFaceListToRotationSystem(
            numFaces,
            originalFaces,
            N,
            adjacency);

        // ----------------------------------------------------
        // Convert adjacency to long long representation
        // ----------------------------------------------------

        vector<vector<long long>> adjacencyLongLong(N);

        for (int u = 0; u < N; ++u)
        {
            for (int v : adjacency[u])
            {
                adjacencyLongLong[u].push_back(
                    static_cast<long long>(v));
            }
        }

        // ----------------------------------------------------
        // Convert rotation system -> faces
        // ----------------------------------------------------

        vector<vector<long long>> generatedFaces =
            rotationSystemToFaces(
                N,
                adjacencyLongLong);

        // ----------------------------------------------------
        // Compare
        // ----------------------------------------------------

        bool passed =
            compareFaces(
                originalFaces,
                generatedFaces);

        if (passed)
        {
            ++passedFiles;

            cout << "\nRESULT: PASS\n";
        }
        else
        {
            ++failedFiles;

            {
                ostringstream reason;

                reason << "Face signature mismatch.\n  "
                       << buildMismatchDescription(originalFaces, generatedFaces)
                       << "\n\n  Original normalized faces:\n"
                       << facesToString(originalFaces)
                       << "\n  Generated faces:\n"
                       << facesToString(generatedFaces);

                failedTestCases.push_back({filePath, reason.str()});
            }

            cout << "\nRESULT: FAIL\n";

            cout << "\nOriginal normalized faces:\n";
            printFaces(originalFaces);

            cout << "\nGenerated faces:\n";
            printFaces(generatedFaces);

            cout << "\nOriginal face count: "
                 << originalFaces.size()
                 << '\n';

            cout << "Generated face count: "
                 << generatedFaces.size()
                 << '\n';

            cout << "\nOriginal face signatures:\n";

            for (const auto &face : originalFaces)
            {
                cout << "  size=" << face.size()
                     << " multiplicities={";

                vector<int> pattern =
                    getMultiplicityPattern(face);

                for (size_t i = 0; i < pattern.size(); ++i)
                {
                    if (i)
                        cout << ',';

                    cout << pattern[i];
                }

                cout << "}\n";
            }

            cout << "\nGenerated face signatures:\n";

            for (const auto &face : generatedFaces)
            {
                cout << "  size=" << face.size()
                     << " multiplicities={";

                vector<int> pattern =
                    getMultiplicityPattern(face);

                for (size_t i = 0; i < pattern.size(); ++i)
                {
                    if (i)
                        cout << ',';

                    cout << pattern[i];
                }

                cout << "}\n";
            }
        }
    }

    // ========================================================
    // FINAL SUMMARY
    // ========================================================

    cout << "\n\n";
    cout << "============================================================\n";
    cout << "FINAL SUMMARY\n";
    cout << "============================================================\n";

    cout << "Total test cases : " << totalFiles << '\n';
    cout << "Passed           : " << passedFiles << '\n';
    cout << "Failed           : " << failedFiles << '\n';

    cout << "\nFAILED TEST CASES:\n";

    if (failedTestCases.empty())
    {
        cout << "  None\n";
    }
    else
    {
        for (size_t i = 0;
             i < failedTestCases.size();
             ++i)
        {
              cout << "  "
                  << i + 1
                  << ". "
                  << failedTestCases[i].path
                  << "\nREASON:\n  "
                  << failedTestCases[i].reason
                  << '\n';
        }
    }

    cout << "============================================================\n";

    return failedFiles == 0 ? 0 : 1;
}