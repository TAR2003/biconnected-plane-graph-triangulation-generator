#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <fstream>

using namespace std;

struct HalfEdge
{
    int u, v;
    bool visited = false;
};

// Converts a Rotation System into a Face-List representation
void convertRotationSystemToFaceList(int N, const vector<vector<int>> &adj, const vector<int> &idx_to_orig)
{
    map<pair<int, int>, int> edge_id;
    vector<HalfEdge> half_edges;

    // Create directed half-edges for all entries in adjacency list
    for (int u = 0; u < N; ++u)
    {
        for (int v : adj[u])
        {
            if (edge_id.find({u, v}) == edge_id.end())
            {
                edge_id[{u, v}] = half_edges.size();
                half_edges.push_back({u, v, false});
            }
        }
    }

    // Precompute next half-edge for every directed edge (u -> v)
    vector<int> next_edge(half_edges.size(), -1);

    for (size_t i = 0; i < half_edges.size(); ++i)
    {
        int u = half_edges[i].u;
        int v = half_edges[i].v;

        const auto &v_neighbors = adj[v];
        int deg_v = v_neighbors.size();

        if (deg_v == 0)
            continue;

        // Find position of u in v's adjacency list
        auto it = find(v_neighbors.begin(), v_neighbors.end(), u);
        if (it == v_neighbors.end())
            continue; // Safety check for mismatched edges

        int pos = distance(v_neighbors.begin(), it);

        // Previous element in CCW order (cyclic)
        int prev_pos = (pos - 1 + deg_v) % deg_v;
        int w = v_neighbors[prev_pos];

        if (edge_id.count({v, w}))
        {
            next_edge[i] = edge_id[{v, w}];
        }
    }

    // Trace all faces
    vector<vector<int>> faces;

    for (size_t i = 0; i < half_edges.size(); ++i)
    {
        if (half_edges[i].visited || next_edge[i] == -1)
            continue;

        vector<int> face;
        int curr = i;

        while (curr != -1 && !half_edges[curr].visited)
        {
            half_edges[curr].visited = true;
            // Store original vertex label
            face.push_back(idx_to_orig[half_edges[curr].u]);
            curr = next_edge[curr];
        }

        if (!face.empty())
        {
            faces.push_back(face);
        }
    }

    // Output result
    cout << faces.size() << "\n";
    for (const auto &f : faces)
    {
        cout << f.size();
        for (int v : f)
        {
            cout << " " << v;
        }
        cout << "\n";
    }
}

int main()
{
    ifstream fin("input.txt");
    if (!fin.is_open())
    {
        cerr << "Error: Could not open input.txt" << endl;
        return 1;
    }

    int total_nodes;
    if (!(fin >> total_nodes))
        return 0;

    vector<vector<int>> raw_adj(total_nodes);
    set<int> unique_nodes;

    for (int i = 0; i < total_nodes; ++i)
    {
        int deg;
        if (!(fin >> deg))
            break;
        raw_adj[i].resize(deg);
        unique_nodes.insert(i);
        for (int j = 0; j < deg; ++j)
        {
            fin >> raw_adj[i][j];
            unique_nodes.insert(raw_adj[i][j]);
        }
    }
    fin.close();

    // Map vertex labels to contiguous 0..N-1 indices
    map<int, int> orig_to_idx;
    vector<int> idx_to_orig;
    int N = 0;
    for (int node : unique_nodes)
    {
        orig_to_idx[node] = N++;
        idx_to_orig.push_back(node);
    }

    vector<vector<int>> adj(N);
    for (int i = 0; i < total_nodes; ++i)
    {
        if (orig_to_idx.count(i))
        {
            int u = orig_to_idx[i];
            for (int v_orig : raw_adj[i])
            {
                if (orig_to_idx.count(v_orig))
                {
                    adj[u].push_back(orig_to_idx[v_orig]);
                }
            }
        }
    }

    convertRotationSystemToFaceList(N, adj, idx_to_orig);

    return 0;
}