#pragma once

#include <algorithm>
#include <map>
#include <set>
#include <vector>

inline std::vector<std::vector<long long>> rotationSystemToFaces(
    long long totalNodes, const std::vector<std::vector<long long>> &rawAdjacency)
{
    struct HalfEdge
    {
        long long u;
        long long v;
        bool visited = false;
    };

    std::set<long long> uniqueNodes;
    for (long long u = 0; u < totalNodes; ++u)
    {
        uniqueNodes.insert(u);
        if (u < static_cast<long long>(rawAdjacency.size()))
        {
            for (long long v : rawAdjacency[u])
                uniqueNodes.insert(v);
        }
    }

    std::map<long long, long long> originalToIndex;
    std::vector<long long> indexToOriginal;
    for (long long node : uniqueNodes)
    {
        originalToIndex[node] = static_cast<long long>(indexToOriginal.size());
        indexToOriginal.push_back(node);
    }

    const long long nodeCount = static_cast<long long>(indexToOriginal.size());
    std::vector<std::vector<long long>> adjacency(static_cast<size_t>(nodeCount));
    for (long long u = 0; u < totalNodes; ++u)
    {
        const auto source = originalToIndex.find(u);
        if (source == originalToIndex.end() || u >= static_cast<long long>(rawAdjacency.size()))
            continue;

        for (long long v : rawAdjacency[u])
        {
            const auto target = originalToIndex.find(v);
            if (target != originalToIndex.end())
                adjacency[source->second].push_back(target->second);
        }
    }

    std::map<std::pair<long long, long long>, long long> edgeId;
    std::vector<HalfEdge> halfEdges;
    for (long long u = 0; u < nodeCount; ++u)
    {
        for (long long v : adjacency[u])
        {
            if (edgeId.find({u, v}) == edgeId.end())
            {
                edgeId[{u, v}] = static_cast<long long>(halfEdges.size());
                halfEdges.push_back({u, v});
            }
        }
    }

    std::vector<long long> nextEdge(halfEdges.size(), -1);
    for (long long i = 0; i < static_cast<long long>(halfEdges.size()); ++i)
    {
        const long long u = halfEdges[i].u;
        const long long v = halfEdges[i].v;
        const auto &neighbors = adjacency[v];
        if (neighbors.empty())
            continue;

        const auto it = std::find(neighbors.begin(), neighbors.end(), u);
        if (it == neighbors.end())
            continue;

        const long long position = static_cast<long long>(it - neighbors.begin());
        const long long previous = (position - 1 + neighbors.size()) % neighbors.size();
        const auto next = edgeId.find({v, neighbors[previous]});
        if (next != edgeId.end())
            nextEdge[i] = next->second;
    }

    std::vector<std::vector<long long>> faces;
    for (long long i = 0; i < static_cast<long long>(halfEdges.size()); ++i)
    {
        if (halfEdges[i].visited || nextEdge[i] == -1)
            continue;

        std::vector<long long> face;
        long long current = i;
        while (current != -1 && !halfEdges[current].visited)
        {
            halfEdges[current].visited = true;
            face.push_back(indexToOriginal[halfEdges[current].u]);
            current = nextEdge[current];
        }
        if (!face.empty())
            faces.push_back(std::move(face));
    }
    return faces;
}
