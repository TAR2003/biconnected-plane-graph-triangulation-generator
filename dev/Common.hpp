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
    uint64_t count = 0;
    uint32_t faceHead = EMPTY;
};

struct ChordFaceCount
{
    int32_t faceIndex;
    uint32_t next;
    uint64_t count;
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
    vector<ChordFaceCount> faceCounts;
    size_t mask;
    int shift;
    uint32_t freeFaceCount = ChordEntry::EMPTY;

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

    uint32_t findFaceCount(const ChordEntry &entry, int faceIndex) const
    {
        uint32_t i = entry.faceHead;
        while (i != ChordEntry::EMPTY)
        {
            if (faceCounts[i].faceIndex == faceIndex)
                return i;
            i = faceCounts[i].next;
        }
        return ChordEntry::EMPTY;
    }

    uint32_t allocateFaceCount(int faceIndex, uint32_t next)
    {
        uint32_t i;
        if (freeFaceCount != ChordEntry::EMPTY)
        {
            i = freeFaceCount;
            freeFaceCount = faceCounts[i].next;
            faceCounts[i] = {faceIndex, next, 0};
        }
        else
        {
            if (faceCounts.size() >= ChordEntry::EMPTY)
                throw length_error("FlatChordMap face-count storage exhausted");
            i = static_cast<uint32_t>(faceCounts.size());
            faceCounts.push_back({faceIndex, next, 0});
        }
        return i;
    }

    void releaseFaceCount(uint32_t i)
    {
        faceCounts[i].next = freeFaceCount;
        freeFaceCount = i;
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
            uint32_t faceCount = findFaceCount(*e, face);
            if (faceCount == ChordEntry::EMPTY)
            {
                faceCount = allocateFaceCount(face, e->faceHead);
                e->faceHead = faceCount;
            }
            ++faceCounts[faceCount].count;
            ++e->count;
            return;
        }
        size_t i = home(u, v);
        while (t[i].u != ChordEntry::EMPTY)
            i = (i + 1) & mask;
        uint32_t faceCount = allocateFaceCount(face, ChordEntry::EMPTY);
        faceCounts[faceCount].count = 1;
        t[i].u = u;
        t[i].v = v;
        t[i].faceHead = faceCount;
        t[i].count = 1;
    }
    void erase(pair<long long, long long> p, int face = -1)
    {
        uint32_t u, v;
        normPair(p.first, p.second, u, v);
        ChordEntry *e = locate(u, v);
        if (e == nullptr)
            return;

        uint32_t *link = &e->faceHead;
        while (*link != ChordEntry::EMPTY)
        {
            uint32_t faceCount = *link;
            ChordFaceCount &entry = faceCounts[faceCount];
            if (entry.faceIndex == face)
            {
                --entry.count;
                --e->count;
                if (entry.count == 0)
                {
                    *link = entry.next;
                    releaseFaceCount(faceCount);
                }
                if (e->count == 0)
                {
                    size_t index = static_cast<size_t>(e - t.data());
                    removeAt(index);
                }
                return;
            }
            link = &entry.next;
        }
    }
    uint64_t find(pair<long long, long long> p)
    {
        uint32_t u, v;
        normPair(p.first, p.second, u, v);
        ChordEntry *e = locate(u, v);
        return e == nullptr ? 0 : e->count;
    }
    uint64_t find(pair<long long, long long> p, int faceIndex)
    {
        uint32_t u, v;
        normPair(p.first, p.second, u, v);
        ChordEntry *e = locate(u, v);
        if (e == nullptr)
            return 0;
        uint32_t faceCount = findFaceCount(*e, faceIndex);
        return faceCount == ChordEntry::EMPTY ? 0 : faceCounts[faceCount].count;
    }
    // Copies up to two face ids, repeating an id when it has multiple occurrences.
    int faces(pair<long long, long long> p, int out[2])
    {
        uint32_t u, v;
        normPair(p.first, p.second, u, v);
        ChordEntry *e = locate(u, v);
        if (!e)
            return 0;
        int copied = 0;
        for (uint32_t i = e->faceHead; i != ChordEntry::EMPTY && copied < 2; i = faceCounts[i].next)
        {
            int occurrences = static_cast<int>(min<uint64_t>(faceCounts[i].count, 2 - copied));
            while (occurrences-- > 0)
                out[copied++] = faceCounts[i].faceIndex;
        }
        return copied;
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
