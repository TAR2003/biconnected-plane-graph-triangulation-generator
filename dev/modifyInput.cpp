#include <bits/stdc++.h>

using namespace std;

struct HalfEdge
{
    int u, v;
    bool visited = false;
};

// Converts a Rotation System into a Face-List representation
void convertRotationSystemToFaceList(int N, const vector<vector<int>> &adj)
{
    // Map directed edges (u -> v) to an internal ID
    map<pair<int, int>, int> edge_id;
    vector<HalfEdge> half_edges;

    for (int u = 0; u < N; ++u)
    {
        for (int v : adj[u])
        {
            edge_id[{u, v}] = half_edges.size();
            half_edges.push_back({u, v, false});
        }
    }

    // Precompute next half-edge for every directed edge (u -> v)
    // Next edge exiting v is the neighbor immediately BEFORE u in v's CCW order
    vector<int> next_edge(half_edges.size());

    for (size_t i = 0; i < half_edges.size(); ++i)
    {
        int u = half_edges[i].u;
        int v = half_edges[i].v;

        const auto &v_neighbors = adj[v];
        int deg_v = v_neighbors.size();

        // Find position of u in v's adjacency list
        auto it = find(v_neighbors.begin(), v_neighbors.end(), u);
        int pos = distance(v_neighbors.begin(), it);

        // Previous element in CCW order (cyclic)
        int prev_pos = (pos - 1 + deg_v) % deg_v;
        int w = v_neighbors[prev_pos];

        // Next half-edge is v -> w
        next_edge[i] = edge_id[{v, w}];
    }

    // Trace all faces
    vector<vector<int>> faces;

    for (size_t i = 0; i < half_edges.size(); ++i)
    {
        if (half_edges[i].visited)
            continue;

        vector<int> face;
        int curr = i;

        while (!half_edges[curr].visited)
        {
            half_edges[curr].visited = true;
            face.push_back(half_edges[curr].u);
            curr = next_edge[curr];
        }

        faces.push_back(face);
    }

    // Output in your specific format
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
    // Example Input (Rotation System):
    // Vertices: 4 (0, 1, 2, 3), Edges: 5 (a planar square with diagonal 0-2)
    int N = 5;
    vector<vector<int>> adj(N);

    // Neighbors listed in Counter-Clockwise (CCW) order
    adj[0] = {1, 3, 4};
    adj[1] = {2, 0};
    adj[2] = {3, 1};
    adj[3] = {0, 2};
    adj[4] = {0};
    convertRotationSystemToFaceList(N, adj);

    return 0;
}