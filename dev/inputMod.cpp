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
    // 1. Collect all unique vertices present in the input
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

    // 2. Map original vertex labels to contiguous 0..N-1 indices
    map<int, int> orig_to_idx;
    vector<int> idx_to_orig;

    int idx = 0;
    for (int v : unique_vertices)
    {
        orig_to_idx[v] = idx++;
        idx_to_orig.push_back(v);
    }

    N = idx_to_orig.size(); // N is exact count of unique vertices
    vector<vector<int>> raw_succ(N);

    // 3. Extract ordering steps per vertex from face walks
    for (const auto &face : faces)
    {
        int k = face.size();
        if (k < 2)
            continue;

        for (int i = 0; i < k; ++i)
        {
            int u = orig_to_idx[face[i]];
            int v = orig_to_idx[face[(i + 1) % k]];

            raw_succ[u].push_back(v);
        }
    }

    // 4. Construct contiguous adjacency list
    adj.assign(N, vector<int>());
    for (int u = 0; u < N; ++u)
    {
        set<int> seen;
        for (int v : raw_succ[u])
        {
            if (seen.find(v) == seen.end())
            {
                seen.insert(v);
                // Map internal index back to original vertex label
                adj[u].push_back(idx_to_orig[v]);
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
        cerr << "Input directory 'input' does not exist!" << endl;
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