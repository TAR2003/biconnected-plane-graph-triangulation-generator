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

// Reconstructs rotation system (adj) from Face-List representation reliably
// Works for Biconnected Graphs, 1-Connected Graphs, and Trees.
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

    // 2. Map original labels to contiguous internal indices [0 .. N-1]
    map<int, int> orig_to_idx;
    vector<int> idx_to_orig;
    int idx = 0;
    for (int v : unique_vertices)
    {
        orig_to_idx[v] = idx++;
        idx_to_orig.push_back(v);
    }

    N = idx_to_orig.size();

    // Map half-edge transition: entering u from w means exiting u towards v
    // key: (u, w) -> list of outgoing neighbors v
    map<pair<int, int>, vector<int>> next_out;
    vector<set<int>> incoming_edges(N);

    // 3. Populate half-edge transition map from faces
    for (const auto &face : faces)
    {
        int k = face.size();
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

    // 4. Reconstruct cyclic CCW order per vertex
    adj.assign(N, vector<int>());

    for (int u = 0; u < N; ++u)
    {
        if (incoming_edges[u].empty())
            continue;

        vector<int> order;
        set<int> in_order;
        set<pair<int, int>> visited_halfedges; // tracks (u, w)

        for (int start_w : incoming_edges[u])
        {
            if (visited_halfedges.count({u, start_w}))
                continue;

            int curr_w = start_w;
            while (!visited_halfedges.count({u, curr_w}))
            {
                visited_halfedges.insert({u, curr_w});

                if (next_out.count({u, curr_w}) && !next_out[{u, curr_w}].empty())
                {
                    // Correct successor: edge leaving u towards next_v
                    int next_v = next_out[{u, curr_w}].front();

                    if (in_order.find(next_v) == in_order.end())
                    {
                        order.push_back(next_v);
                        in_order.insert(next_v);
                    }

                    if (next_out[{u, curr_w}].size() > 1)
                    {
                        next_out[{u, curr_w}].erase(next_out[{u, curr_w}].begin());
                    }
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

    // 5. Enforce bidirectional symmetry for graph topology
    for (int u = 0; u < N; ++u)
    {
        for (int v : adj[u])
        {
            auto it = find(adj[v].begin(), adj[v].end(), u);
            if (it == adj[v].end())
            {
                adj[v].push_back(u);
            }
        }
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

    fs::create_directories(output_filepath.parent_path());

    ofstream fout(output_filepath);
    if (!fout.is_open())
        return;

    // Output format: N followed by N lines of adjacency lists
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
        cerr << "Input directory '" << input_root.string() << "' does not exist!" << endl;
        return 1;
    }

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