#pragma once
#include <bits/stdc++.h>
#include <algorithm>
#include <map>
#include <set>
#include <vector>
using namespace std;

class Chord
{
public:
    long long first = -1, second = -1, oppositeFirst = -1, oppositeSecond = -1;
    Chord *nextChord = nullptr, *prevChord = nullptr;
    Chord *nextGS = nullptr, *prevGS = nullptr;
    Chord *nextVGS = nullptr, *prevVGS = nullptr;
    long long faceIndex = -1;
    bool isValid = false;
    Chord(long long a = 0, long long b = 0, long long c = 0, long long d = 0) : first(a), second(b), oppositeFirst(c), oppositeSecond(d) {}
    // ~Chord() {

    // }
    void flip()
    {
        swap(first, oppositeFirst);
        swap(second, oppositeSecond);
    }
    void clearValues()
    {
        first = second = oppositeFirst = oppositeSecond = -1;
        nextGS = prevGS = nextVGS = prevVGS = nullptr;
        isValid = false;
    }
};

struct ChordEntry
{
    static constexpr uint32_t EMPTY = UINT32_MAX;
    uint32_t u = EMPTY, v = 0;
    int32_t face[2] = {-1, -1};
    uint8_t cnt = 0; // 0, 1 or 2 occurrences

    void addFace(int f)
    {
        if (cnt == 0)
            face[0] = f;
        else
            face[1] = f;
        ++cnt;
    }
    void removeFace(int f)
    { // returns with cnt decremented
        if (face[1] == f)
            face[1] = -1;
        else
        {
            face[0] = face[1];
            face[1] = -1;
        }
        if (--cnt == 0)
            face[0] = -1;
    }
};

inline void normPair(long long a, long long b, uint32_t &u, uint32_t &v)
{
    if (a > b)
        swap(a, b);
    u = (uint32_t)a;
    v = (uint32_t)b;
}
inline uint64_t packKey(uint32_t u, uint32_t v) { return ((uint64_t)u << 32) | v; }

class FlatChordMap
{
    vector<ChordEntry> t;
    size_t mask;
    int shift;

    size_t home(uint32_t u, uint32_t v) const
    {
        return (packKey(u, v) * 0x9E3779B97F4A7C15ULL) >> shift;
    }
    ChordEntry *locate(uint32_t u, uint32_t v)
    {
        size_t i = home(u, v);
        while (t[i].u != ChordEntry::EMPTY)
        {
            if (t[i].u == u && t[i].v == v)
                return &t[i];
            i = (i + 1) & mask;
        }
        return nullptr;
    }
    void removeAt(size_t i)
    { // backward-shift deletion
        size_t j = i;
        for (;;)
        {
            j = (j + 1) & mask;
            if (t[j].u == ChordEntry::EMPTY)
                break;
            size_t h = home(t[j].u, t[j].v);
            bool stays = (i <= j) ? (i < h && h <= j) : (i < h || h <= j);
            if (stays)
                continue;
            t[i] = t[j];
            i = j;
        }
        t[i] = ChordEntry();
    }

public:
    explicit FlatChordMap(int n)
    {
        size_t maxChords = max(3 * (size_t)n, (size_t)8);
        int bits = 1;
        while ((1ULL << bits) < 2 * maxChords)
            ++bits;
        t.assign(1ULL << bits, ChordEntry());
        mask = (1ULL << bits) - 1;
        shift = 64 - bits;
    }
    void insert(pair<long long, long long> p, int face = -1)
    {
        uint32_t u, v;
        normPair(p.first, p.second, u, v);
        if (ChordEntry *e = locate(u, v))
        {
            e->addFace(face);
            return;
        }
        size_t i = home(u, v);
        while (t[i].u != ChordEntry::EMPTY)
            i = (i + 1) & mask;
        t[i].u = u;
        t[i].v = v;
        t[i].addFace(face);
    }
    void erase(pair<long long, long long> p, int face = -1)
    {
        uint32_t u, v;
        normPair(p.first, p.second, u, v);
        size_t i = home(u, v);
        while (t[i].u != ChordEntry::EMPTY)
        {
            if (t[i].u == u && t[i].v == v)
            {
                t[i].removeFace(face);
                if (t[i].cnt == 0)
                    removeAt(i);
                return;
            }
            i = (i + 1) & mask;
        }
    }
    bool find(pair<long long, long long> p)
    {
        uint32_t u, v;
        normPair(p.first, p.second, u, v);
        return locate(u, v) != nullptr;
    }
    // up to two face ids; returns how many
    int faces(pair<long long, long long> p, int out[2])
    {
        uint32_t u, v;
        normPair(p.first, p.second, u, v);
        ChordEntry *e = locate(u, v);
        if (!e)
            return 0;
        out[0] = e->face[0];
        out[1] = e->face[1];
        return e->cnt;
    }
    void print()
    {
        for (auto &e : t)
            if (e.u != ChordEntry::EMPTY)
                cout << "(" << e.u << ", " << e.v << ") ";
        cout << endl;
    }
};



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
