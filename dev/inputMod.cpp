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

// Reconstructs rotation system (adj) from Face-List representation
void convertFaceListToRotationSystem(int num_faces, const vector<vector<int>> &faces, int &N, vector<vector<int>> &adj)
{
    // 1. Collect all unique original vertex labels
    set<int> unique_vertices;
    for (const auto &face : faces)
    {
        for (int v : face)
        {
            unique_vertices.insert(v);
        }
    }

    if (unique_vertices.empty())
    {
        N = 0;
        adj.clear();
        return;
    }

    // Determine max vertex ID to establish contiguous indexing [0 .. max_v]
    int max_v = *unique_vertices.rbegin();
    int min_v = *unique_vertices.begin();

    // Check if 1-based indexing is used (min_v == 1 and 0 is missing)
    bool is_one_based = (min_v == 1 && unique_vertices.find(0) == unique_vertices.end());

    N = is_one_based ? max_v + 1 : max_v + 1;
    adj.assign(N, vector<int>());

    // Map directed edges (u -> v) to their predecessor in face sequence
    // If face is ... -> w -> u -> v -> ..., then entering u from w means exiting to v.
    // In CCW rotation around u, the edge following (u -> w) is (u -> v).
    map<pair<int, int>, int> next_out;

    for (const auto &face : faces)
    {
        int k = face.size();
        if (k < 2)
            continue;

        for (int i = 0; i < k; ++i)
        {
            int w = face[i];
            int u = face[(i + 1) % k];
            int v = face[(i + 2) % k];

            // Normalize 1-based indices to 0-based if necessary
            if (is_one_based)
            {
                w--;
                u--;
                v--;
            }

            // In face walk w -> u -> v:
            // Traversing edge (w -> u) into u means the next outgoing edge from u in CCW order is (u -> v)
            next_out[{u, w}] = v;
        }
    }

    // 2. Reconstruct rotation system for each vertex u
    for (int u = 0; u < N; ++u)
    {
        // Find all incoming half-edges into u
        set<int> incoming_neighbors;
        for (const auto &[edge, v] : next_out)
        {
            if (edge.first == u)
            {
                incoming_neighbors.insert(edge.second);
            }
        }

        if (incoming_neighbors.empty())
            continue;

        vector<int> ccw_order;
        set<int> visited;

        int start_w = *incoming_neighbors.begin();
        int curr_w = start_w;

        while (visited.find(curr_w) == visited.end())
        {
            visited.insert(curr_w);

            // Get next outgoing neighbor v around u after w
            if (next_out.count({u, curr_w}))
            {
                int next_v = next_out[{u, curr_w}];

                // Convert back to original 1-based label if needed
                int display_v = is_one_based ? (next_v + 1) : next_v;
                ccw_order.push_back(display_v);

                curr_w = next_v;
            }
            else
            {
                break;
            }
        }

        adj[u] = ccw_order;
    }

    // If 1-based input was used, shift the adjacency array down by 1 so vertex 1 maps to row 0
    if (is_one_based)
    {
        N = max_v;
        for (int i = 0; i < N; ++i)
        {
            adj[i] = adj[i + 1];
        }
        adj.resize(N);
    }
}

void processFile(const fs::path &input_filepath, const fs::path &output_filepath)
{
    ifstream fin(input_filepath);
    if (!fin.is_open())
        return;

    int num_faces;
    if (!(fin >> num_faces))
        return;

    vector<vector<int>> faces(num_faces);
    for (int i = 0; i < num_faces; ++i)
    {
        int sz;
        fin >> sz;
        faces[i].resize(sz);
        for (int j = 0; j < sz; ++j)
        {
            fin >> faces[i][j];
        }
    }
    fin.close();

    int N = 0;
    vector<vector<int>> adj;
    convertFaceListToRotationSystem(num_faces, faces, N, adj);

    // Create parent directory for target output path if it doesn't exist
    fs::create_directories(output_filepath.parent_path());

    ofstream fout(output_filepath);
    if (!fout.is_open())
        return;

    // Write output format: N lines / adjacency structure
    fout << N << "\n";
    for (int i = 0; i < N; ++i)
    {
        fout << adj[i].size();
        for (int v : adj[i])
        {
            fout << " " << v;
        }
        fout << "\n";
    }
    fout.close();
}

int main()
{
    fs::path input_root = "previnput";
    fs::path output_root = "input";

    if (!fs::exists(input_root))
    {
        cerr << "Input directory '" << input_root << "' does not exist!" << endl;
        return 1;
    }

    // Traverse recursively through subdirectories
    for (const auto &entry : fs::recursive_directory_iterator(input_root))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".txt")
        {
            fs::path relative_path = fs::relative(entry.path(), input_root);
            fs::path target_path = output_root / relative_path;

            cout << "Processing: " << entry.path() << " -> " << target_path << "\n";
            processFile(entry.path(), target_path);
        }
    }

    cout << "Directory conversion complete." << endl;
    return 0;
}