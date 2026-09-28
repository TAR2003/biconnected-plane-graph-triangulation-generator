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

// Reconstructs rotation system (adj) from faces representation
void convertFaceListToRotationSystem(int num_faces, const vector<vector<int>> &faces, int &N, vector<vector<int>> &adj)
{
    map<pair<int, int>, int> next_in_face;
    set<int> vertex_set;

    // Map each directed edge u -> v to its next vertex w in the face boundary sequence
    for (const auto &face : faces)
    {
        int k = face.size();
        for (int i = 0; i < k; ++i)
        {
            int u = face[i];
            int v = face[(i + 1) % k];
            int w = face[(i + 2) % k];

            next_in_face[{u, v}] = w;
            vertex_set.insert(u);
            vertex_set.insert(v);
        }
    }

    N = vertex_set.empty() ? 0 : (*vertex_set.rbegin() + 1);
    adj.assign(N, vector<int>());

    // For each directed edge (u -> v), the successor edge exiting u in CCW order
    // is (u -> w) where w is the predecessor of u along the face sharing twin edge (v -> u).
    for (int u = 0; u < N; ++u)
    {
        // Find all outgoing neighbors of u
        set<int> neighbors;
        for (auto const &[edge, next_v] : next_in_face)
        {
            if (edge.first == u)
            {
                neighbors.insert(edge.second);
            }
        }

        if (neighbors.empty())
            continue;

        // Reconstruct order around vertex u
        vector<int> ordered_neighbors;
        int start_v = *neighbors.begin();
        int curr_v = start_v;

        set<int> visited_nbrs;
        while (visited_nbrs.find(curr_v) == visited_nbrs.end())
        {
            ordered_neighbors.push_back(curr_v);
            visited_nbrs.insert(curr_v);

            // Move to opposite face along (curr_v -> u)
            if (next_in_face.count({curr_v, u}))
            {
                curr_v = next_in_face[{curr_v, u}];
            }
            else
            {
                break;
            }
        }

        adj[u] = ordered_neighbors;
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
    fs::path input_root = "input";
    fs::path output_root = "new-input";

    if (!fs::exists(input_root))
    {
        cerr << "Input directory 'input' does not exist!" << endl;
        return 1;
    }

    // Traverse recursively through subdirectories
    for (const auto &entry : fs::recursive_directory_iterator(input_root))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".txt")
        {
            // Compute relative path to preserve directory structure
            fs::path relative_path = fs::relative(entry.path(), input_root);
            fs::path target_path = output_root / relative_path;

            cout << "Processing: " << entry.path() << " -> " << target_path << "\n";
            processFile(entry.path(), target_path);
        }
    }

    cout << "Directory conversion complete." << endl;
    return 0;
}