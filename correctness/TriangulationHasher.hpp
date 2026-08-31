#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// Strong, independent 64-bit hash primitives for research-grade fingerprinting.
// Three unrelated algorithms minimise correlated collision risk.
// ---------------------------------------------------------------------------

namespace tri_hash_detail
{

inline uint64_t rotl64(uint64_t x, int r)
{
    return (x << r) | (x >> (64 - r));
}

// XXHash64 (Yann Collet) — fast, excellent avalanche properties.
inline uint64_t xxHash64(const void *data, size_t len, uint64_t seed)
{
    static constexpr uint64_t PRIME64_1 = 0x9E3779B185EBCA87ULL;
    static constexpr uint64_t PRIME64_2 = 0xC2B2AE3D27D4EB4FULL;
    static constexpr uint64_t PRIME64_3 = 0x165667B19E3779F9ULL;
    static constexpr uint64_t PRIME64_4 = 0x85EBCA77C2B2AE63ULL;
    static constexpr uint64_t PRIME64_5 = 0x27D4EB2F165667C5ULL;

    const uint8_t *p = static_cast<const uint8_t *>(data);
    const uint8_t *const end = p + len;
    uint64_t h64;

    if (len >= 32)
    {
        const uint8_t *const limit = end - 32;
        uint64_t v1 = seed + PRIME64_1 + PRIME64_2;
        uint64_t v2 = seed + PRIME64_2;
        uint64_t v3 = seed + 0;
        uint64_t v4 = seed - PRIME64_1;

        do
        {
            uint64_t k1, k2, k3, k4;
            std::memcpy(&k1, p, 8);
            p += 8;
            std::memcpy(&k2, p, 8);
            p += 8;
            std::memcpy(&k3, p, 8);
            p += 8;
            std::memcpy(&k4, p, 8);
            p += 8;

            v1 = rotl64(v1 + k1 * PRIME64_2, 31) * PRIME64_1;
            v2 = rotl64(v2 + k2 * PRIME64_2, 31) * PRIME64_1;
            v3 = rotl64(v3 + k3 * PRIME64_2, 31) * PRIME64_1;
            v4 = rotl64(v4 + k4 * PRIME64_2, 31) * PRIME64_1;
        } while (p <= limit);

        h64 = rotl64(v1, 1) + rotl64(v2, 7) + rotl64(v3, 12) + rotl64(v4, 18);
    }
    else
    {
        h64 = seed + PRIME64_5;
    }

    h64 += static_cast<uint64_t>(len);

    while (p + 8 <= end)
    {
        uint64_t k1;
        std::memcpy(&k1, p, 8);
        k1 *= PRIME64_2;
        k1 = rotl64(k1, 31);
        k1 *= PRIME64_1;
        h64 ^= k1;
        h64 = rotl64(h64, 27) * PRIME64_1 + PRIME64_4;
        p += 8;
    }

    if (p + 4 <= end)
    {
        uint32_t k1;
        std::memcpy(&k1, p, 4);
        h64 ^= static_cast<uint64_t>(k1) * PRIME64_1;
        h64 = rotl64(h64, 23) * PRIME64_2 + PRIME64_3;
        p += 4;
    }

    while (p < end)
    {
        h64 ^= static_cast<uint64_t>(*p) * PRIME64_5;
        h64 = rotl64(h64, 11) * PRIME64_1;
        ++p;
    }

    h64 ^= h64 >> 33;
    h64 *= PRIME64_2;
    h64 ^= h64 >> 29;
    h64 *= PRIME64_3;
    h64 ^= h64 >> 32;
    return h64;
}

// MurmurHash3 x64 128-bit, folded to 64 bits — widely used in research.
inline uint64_t murmurHash3_x64_128_folded(const void *key, size_t len, uint64_t seed)
{
    static constexpr uint64_t C1 = 0x87C37B91114253D5ULL;
    static constexpr uint64_t C2 = 0x4CF5AD432745937FULL;

    const uint8_t *data = static_cast<const uint8_t *>(key);
    const int nblocks = static_cast<int>(len / 16);

    uint64_t h1 = seed;
    uint64_t h2 = seed;

    for (int i = 0; i < nblocks; ++i)
    {
        const uint8_t *block = data + i * 16;
        uint64_t k1, k2;
        std::memcpy(&k1, block, 8);
        std::memcpy(&k2, block + 8, 8);

        k1 *= C1;
        k1 = rotl64(k1, 31);
        k1 *= C2;
        h1 ^= k1;
        h1 = rotl64(h1, 27);
        h1 += h2;
        h1 = h1 * 5 + 0x52DCE729;

        k2 *= C2;
        k2 = rotl64(k2, 33);
        k2 *= C1;
        h2 ^= k2;
        h2 = rotl64(h2, 31);
        h2 += h1;
        h2 = h2 * 5 + 0x38495AB5;
    }

    const uint8_t *tail = data + nblocks * 16;
    uint64_t k1 = 0, k2 = 0;
    switch (len & 15)
    {
    case 15:
        k2 ^= static_cast<uint64_t>(tail[14]) << 48;
        [[fallthrough]];
    case 14:
        k2 ^= static_cast<uint64_t>(tail[13]) << 40;
        [[fallthrough]];
    case 13:
        k2 ^= static_cast<uint64_t>(tail[12]) << 32;
        [[fallthrough]];
    case 12:
        k2 ^= static_cast<uint64_t>(tail[11]) << 24;
        [[fallthrough]];
    case 11:
        k2 ^= static_cast<uint64_t>(tail[10]) << 16;
        [[fallthrough]];
    case 10:
        k2 ^= static_cast<uint64_t>(tail[9]) << 8;
        [[fallthrough]];
    case 9:
        k2 ^= static_cast<uint64_t>(tail[8]);
        k2 *= C2;
        k2 = rotl64(k2, 33);
        k2 *= C1;
        h2 ^= k2;
        [[fallthrough]];
    case 8:
        k1 ^= static_cast<uint64_t>(tail[7]) << 56;
        [[fallthrough]];
    case 7:
        k1 ^= static_cast<uint64_t>(tail[6]) << 48;
        [[fallthrough]];
    case 6:
        k1 ^= static_cast<uint64_t>(tail[5]) << 40;
        [[fallthrough]];
    case 5:
        k1 ^= static_cast<uint64_t>(tail[4]) << 32;
        [[fallthrough]];
    case 4:
        k1 ^= static_cast<uint64_t>(tail[3]) << 24;
        [[fallthrough]];
    case 3:
        k1 ^= static_cast<uint64_t>(tail[2]) << 16;
        [[fallthrough]];
    case 2:
        k1 ^= static_cast<uint64_t>(tail[1]) << 8;
        [[fallthrough]];
    case 1:
        k1 ^= static_cast<uint64_t>(tail[0]);
        k1 *= C1;
        k1 = rotl64(k1, 31);
        k1 *= C2;
        h1 ^= k1;
    }

    h1 ^= static_cast<uint64_t>(len);
    h2 ^= static_cast<uint64_t>(len);
    h1 += h2;
    h2 += h1;
    h1 = (h1 ^ (h1 >> 33)) * 0xFF51AFD7ED558CCDULL;
    h1 = (h1 ^ (h1 >> 33)) * 0xC4CEB9FE1A85EC53ULL;
    h1 ^= h1 >> 33;
    h2 = (h2 ^ (h2 >> 33)) * 0xFF51AFD7ED558CCDULL;
    h2 = (h2 ^ (h2 >> 33)) * 0xC4CEB9FE1A85EC53ULL;
    h2 ^= h2 >> 33;
    h1 += h2;
    h2 += h1;
    return h1 ^ h2;
}

// SipHash-2-4 — cryptographic-strength short-message hash (64-bit output).
inline uint64_t sipHash24(const void *data, size_t len, uint64_t seed0, uint64_t seed1)
{
    uint64_t v0 = seed0 ^ 0x736F6D6570736575ULL;
    uint64_t v1 = seed1 ^ 0x646F72616E646F6DULL;
    uint64_t v2 = seed0 ^ 0x6C7967656E657261ULL;
    uint64_t v3 = seed1 ^ 0x7465646279746573ULL;

    const uint8_t *p = static_cast<const uint8_t *>(data);
    const uint8_t *end = p + (len & ~static_cast<size_t>(7));
    const int left = static_cast<int>(len & 7);
    uint64_t b = static_cast<uint64_t>(len) << 56;

    if (len)
    {
        for (; p != end; p += 8)
        {
            uint64_t m;
            std::memcpy(&m, p, 8);
            v3 ^= m;
            for (int r = 0; r < 2; ++r)
            {
                v0 += v1;
                v1 = rotl64(v1, 13);
                v1 ^= v0;
                v0 = rotl64(v0, 32);
                v2 += v3;
                v3 = rotl64(v3, 16);
                v3 ^= v2;
                v0 += v3;
                v3 = rotl64(v3, 21);
                v3 ^= v0;
                v2 += v1;
                v1 = rotl64(v1, 17);
                v1 ^= v2;
                v2 = rotl64(v2, 32);
            }
            v0 ^= m;
        }

        uint64_t t = 0;
        switch (left)
        {
        case 7:
            t ^= static_cast<uint64_t>(p[6]) << 48;
            [[fallthrough]];
        case 6:
            t ^= static_cast<uint64_t>(p[5]) << 40;
            [[fallthrough]];
        case 5:
            t ^= static_cast<uint64_t>(p[4]) << 32;
            [[fallthrough]];
        case 4:
            t ^= static_cast<uint64_t>(p[3]) << 24;
            [[fallthrough]];
        case 3:
            t ^= static_cast<uint64_t>(p[2]) << 16;
            [[fallthrough]];
        case 2:
            t ^= static_cast<uint64_t>(p[1]) << 8;
            [[fallthrough]];
        case 1:
            t ^= static_cast<uint64_t>(p[0]);
        }
        b |= t;
    }

    v3 ^= b;
    for (int r = 0; r < 2; ++r)
    {
        v0 += v1;
        v1 = rotl64(v1, 13);
        v1 ^= v0;
        v0 = rotl64(v0, 32);
        v2 += v3;
        v3 = rotl64(v3, 16);
        v3 ^= v2;
        v0 += v3;
        v3 = rotl64(v3, 21);
        v3 ^= v0;
        v2 += v1;
        v1 = rotl64(v1, 17);
        v1 ^= v2;
        v2 = rotl64(v2, 32);
    }
    v0 ^= b;
    v2 ^= 0xFF;
    for (int r = 0; r < 4; ++r)
    {
        v0 += v1;
        v1 = rotl64(v1, 13);
        v1 ^= v0;
        v0 = rotl64(v0, 32);
        v2 += v3;
        v3 = rotl64(v3, 16);
        v3 ^= v2;
        v0 += v3;
        v3 = rotl64(v3, 21);
        v3 ^= v0;
        v2 += v1;
        v1 = rotl64(v1, 17);
        v1 ^= v2;
        v2 = rotl64(v2, 32);
    }
    return v0 ^ v1 ^ v2 ^ v3;
}

inline void serializeTriangulation(
    const std::vector<std::pair<int, int>> &tri,
    std::vector<uint8_t> &buffer)
{
    buffer.resize(tri.size() * 2 * sizeof(int32_t));
    for (size_t i = 0; i < tri.size(); ++i)
    {
        int32_t a = tri[i].first;
        int32_t b = tri[i].second;
        std::memcpy(buffer.data() + i * 8, &a, 4);
        std::memcpy(buffer.data() + i * 8 + 4, &b, 4);
    }
}

} // namespace tri_hash_detail

// ---------------------------------------------------------------------------
// Per-triangulation and multiset (XOR-aggregated) hashing.
// ---------------------------------------------------------------------------

struct TriangulationHashTriple
{
    uint64_t xxHash = 0;
    uint64_t murmurHash = 0;
    uint64_t sipHash = 0;
};

inline TriangulationHashTriple hashTriangulation(
    const std::vector<std::pair<int, int>> &tri)
{
    std::vector<std::pair<int, int>> canonical = tri;
    for (auto &p : canonical)
    {
        if (p.first > p.second)
            std::swap(p.first, p.second);
    }
    std::sort(canonical.begin(), canonical.end());

    std::vector<uint8_t> buffer;
    tri_hash_detail::serializeTriangulation(canonical, buffer);

    TriangulationHashTriple result;
    result.xxHash = tri_hash_detail::xxHash64(
        buffer.data(), buffer.size(), 0xDEADBEEFCAFEBABEULL);
    result.murmurHash = tri_hash_detail::murmurHash3_x64_128_folded(
        buffer.data(), buffer.size(), 0x0123456789ABCDEFULL);
    result.sipHash = tri_hash_detail::sipHash24(
        buffer.data(), buffer.size(),
        0xA5A5A5A5A5A5A5A5ULL, 0x5A5A5A5A5A5A5A5AULL);
    return result;
}

// ---------------------------------------------------------------------------
// Accumulator used by both algorithms during generation.
// Always counts and hashes; optionally stores triangulations until the
// memory budget is exhausted.
// ---------------------------------------------------------------------------

struct TriangulationRunStats
{
    uint64_t totalTriangulationCount = 0;
    uint64_t hashXxXor = 0;
    uint64_t hashMurmurXor = 0;
    uint64_t hashSipXor = 0;

    size_t storedTriangulationCount = 0;
    size_t estimatedStoredBytes = 0;
    bool storageStopped = false;

    size_t memoryLimitBytes = 2ULL * 1024ULL * 1024ULL * 1024ULL;

    static size_t estimateTriangulationBytes(const std::vector<std::pair<int, int>> &tri)
    {
        return sizeof(std::vector<std::pair<int, int>>) +
               tri.size() * sizeof(std::pair<int, int>);
    }

    void recordTriangulation(
        const std::vector<std::pair<int, int>> &tri,
        std::vector<std::vector<std::pair<int, int>>> *storage)
    {
        ++totalTriangulationCount;

        TriangulationHashTriple h = hashTriangulation(tri);
        hashXxXor ^= h.xxHash;
        hashMurmurXor ^= h.murmurHash;
        hashSipXor ^= h.sipHash;

        if (storage == nullptr)
            return;

        if (!storageStopped)
        {
            const size_t needed = estimateTriangulationBytes(tri);
            if (estimatedStoredBytes + needed <= memoryLimitBytes)
            {
                storage->push_back(tri);
                ++storedTriangulationCount;
                estimatedStoredBytes += needed;
            }
            else
            {
                storageStopped = true;
            }
        }
    }
};

inline std::string formatHashHex(uint64_t value)
{
    char buf[19];
    std::snprintf(buf, sizeof(buf), "0x%016llX",
                  static_cast<unsigned long long>(value));
    return std::string(buf);
}
