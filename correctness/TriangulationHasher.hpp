#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// Portable little-endian encoding for canonical triangulation serialization.
// ---------------------------------------------------------------------------

namespace tri_hash_detail
{

inline void writeLE32(uint8_t *dst, uint32_t value)
{
    dst[0] = static_cast<uint8_t>(value);
    dst[1] = static_cast<uint8_t>(value >> 8);
    dst[2] = static_cast<uint8_t>(value >> 16);
    dst[3] = static_cast<uint8_t>(value >> 24);
}

inline uint64_t readLE64(const uint8_t *src)
{
    uint64_t value = 0;
    for (int i = 0; i < 8; ++i)
        value |= static_cast<uint64_t>(src[i]) << (i * 8);
    return value;
}

inline void loadSha256Words(const uint8_t digest[32], uint64_t words[4])
{
    for (int i = 0; i < 4; ++i)
        words[i] = readLE64(digest + i * 8);
}

inline uint64_t rotl64(uint64_t x, int r)
{
    return (x << r) | (x >> (64 - r));
}

inline void serializeTriangulation(
    const std::vector<std::pair<int, int>> &tri,
    std::vector<uint8_t> &buffer)
{
    buffer.resize(tri.size() * 8);
    for (size_t i = 0; i < tri.size(); ++i)
    {
        writeLE32(buffer.data() + i * 8, static_cast<uint32_t>(tri[i].first));
        writeLE32(buffer.data() + i * 8 + 4, static_cast<uint32_t>(tri[i].second));
    }
}

// ---------------------------------------------------------------------------
// XXHash64
// ---------------------------------------------------------------------------

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
            uint64_t k1 = readLE64(p);
            p += 8;
            uint64_t k2 = readLE64(p);
            p += 8;
            uint64_t k3 = readLE64(p);
            p += 8;
            uint64_t k4 = readLE64(p);
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
        uint64_t k1 = readLE64(p);
        k1 *= PRIME64_2;
        k1 = rotl64(k1, 31);
        k1 *= PRIME64_1;
        h64 ^= k1;
        h64 = rotl64(h64, 27) * PRIME64_1 + PRIME64_4;
        p += 8;
    }

    if (p + 4 <= end)
    {
        uint32_t k1 = static_cast<uint32_t>(p[0]) |
                      (static_cast<uint32_t>(p[1]) << 8) |
                      (static_cast<uint32_t>(p[2]) << 16) |
                      (static_cast<uint32_t>(p[3]) << 24);
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

// ---------------------------------------------------------------------------
// SipHash-2-4
// ---------------------------------------------------------------------------

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
            uint64_t m = readLE64(p);
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

// ---------------------------------------------------------------------------
// SHA-256 (self-contained, public-domain style implementation)
// ---------------------------------------------------------------------------

inline uint32_t rotr32(uint32_t x, int n)
{
    return (x >> n) | (x << (32 - n));
}

inline uint32_t sha256Ch(uint32_t x, uint32_t y, uint32_t z)
{
    return (x & y) ^ (~x & z);
}

inline uint32_t sha256Maj(uint32_t x, uint32_t y, uint32_t z)
{
    return (x & y) ^ (x & z) ^ (y & z);
}

inline uint32_t sha256Sigma0(uint32_t x)
{
    return rotr32(x, 2) ^ rotr32(x, 13) ^ rotr32(x, 22);
}

inline uint32_t sha256Sigma1(uint32_t x)
{
    return rotr32(x, 6) ^ rotr32(x, 11) ^ rotr32(x, 25);
}

inline uint32_t sha256Gamma0(uint32_t x)
{
    return rotr32(x, 7) ^ rotr32(x, 18) ^ (x >> 3);
}

inline uint32_t sha256Gamma1(uint32_t x)
{
    return rotr32(x, 17) ^ rotr32(x, 19) ^ (x >> 10);
}

inline void sha256(const uint8_t *data, size_t len, uint8_t outDigest[32])
{
    static const uint32_t K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

    uint32_t h0 = 0x6a09e667;
    uint32_t h1 = 0xbb67ae85;
    uint32_t h2 = 0x3c6ef372;
    uint32_t h3 = 0xa54ff53a;
    uint32_t h4 = 0x510e527f;
    uint32_t h5 = 0x9b05688c;
    uint32_t h6 = 0x1f83d9ab;
    uint32_t h7 = 0x5be0cd19;

    std::vector<uint8_t> msg;
    msg.assign(data, data + len);
    msg.push_back(0x80);

    while ((msg.size() % 64) != 56)
        msg.push_back(0x00);

    const uint64_t bitLen = static_cast<uint64_t>(len) * 8ULL;
    for (int i = 7; i >= 0; --i)
        msg.push_back(static_cast<uint8_t>((bitLen >> (i * 8)) & 0xFF));

    for (size_t offset = 0; offset < msg.size(); offset += 64)
    {
        uint32_t w[64];
        for (int i = 0; i < 16; ++i)
        {
            const uint8_t *block = msg.data() + offset + i * 4;
            w[i] = (static_cast<uint32_t>(block[0]) << 24) |
                   (static_cast<uint32_t>(block[1]) << 16) |
                   (static_cast<uint32_t>(block[2]) << 8) |
                   static_cast<uint32_t>(block[3]);
        }
        for (int i = 16; i < 64; ++i)
            w[i] = sha256Gamma1(w[i - 2]) + w[i - 7] + sha256Gamma0(w[i - 15]) + w[i - 16];

        uint32_t a = h0;
        uint32_t b = h1;
        uint32_t c = h2;
        uint32_t d = h3;
        uint32_t e = h4;
        uint32_t f = h5;
        uint32_t g = h6;
        uint32_t h = h7;

        for (int i = 0; i < 64; ++i)
        {
            const uint32_t t1 = h + sha256Sigma1(e) + sha256Ch(e, f, g) + K[i] + w[i];
            const uint32_t t2 = sha256Sigma0(a) + sha256Maj(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }

        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
        h5 += f;
        h6 += g;
        h7 += h;
    }

    const uint32_t hs[8] = {h0, h1, h2, h3, h4, h5, h6, h7};
    for (int i = 0; i < 8; ++i)
    {
        outDigest[i * 4 + 0] = static_cast<uint8_t>((hs[i] >> 24) & 0xFF);
        outDigest[i * 4 + 1] = static_cast<uint8_t>((hs[i] >> 16) & 0xFF);
        outDigest[i * 4 + 2] = static_cast<uint8_t>((hs[i] >> 8) & 0xFF);
        outDigest[i * 4 + 3] = static_cast<uint8_t>(hs[i] & 0xFF);
    }
}

} // namespace tri_hash_detail

// ---------------------------------------------------------------------------
// Per-triangulation fingerprint
// ---------------------------------------------------------------------------

struct TriangulationFingerprint
{
    uint64_t xxHash = 0;
    uint64_t sipHash = 0;
    std::array<uint8_t, 32> sha256{};
};

inline TriangulationFingerprint hashTriangulation(
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

    TriangulationFingerprint result;
    result.xxHash = tri_hash_detail::xxHash64(
        buffer.data(), buffer.size(), 0xDEADBEEFCAFEBABEULL);
    result.sipHash = tri_hash_detail::sipHash24(
        buffer.data(), buffer.size(),
        0xA5A5A5A5A5A5A5A5ULL, 0x5A5A5A5A5A5A5A5AULL);
    tri_hash_detail::sha256(buffer.data(), buffer.size(), result.sha256.data());
    return result;
}

// ---------------------------------------------------------------------------
// O(1)-memory aggregate statistics
// ---------------------------------------------------------------------------

struct TriangulationRunStats
{
    static constexpr int SHA_WORDS = 4;

    uint64_t totalTriangulationCount = 0;

    uint64_t hashXxXor = 0;
    uint64_t hashXxSum = 0;
    uint64_t hashSipXor = 0;
    uint64_t hashSipSum = 0;
    uint64_t sha256Xor[SHA_WORDS] = {};
    uint64_t sha256Sum[SHA_WORDS] = {};

    size_t storedTriangulationCount = 0;
    size_t estimatedStoredBytes = 0;
    bool storageStopped = false;

    size_t memoryLimitBytes = 2ULL * 1024ULL * 1024ULL * 1024ULL;

    static size_t estimateTriangulationBytes(const std::vector<std::pair<int, int>> &tri)
    {
        return sizeof(std::vector<std::pair<int, int>>) +
               tri.size() * sizeof(std::pair<int, int>);
    }

    void accumulateFingerprint(const TriangulationFingerprint &fp)
    {
        hashXxXor ^= fp.xxHash;
        hashXxSum += fp.xxHash;
        hashSipXor ^= fp.sipHash;
        hashSipSum += fp.sipHash;

        uint64_t words[SHA_WORDS];
        tri_hash_detail::loadSha256Words(fp.sha256.data(), words);
        for (int i = 0; i < SHA_WORDS; ++i)
        {
            sha256Xor[i] ^= words[i];
            sha256Sum[i] += words[i];
        }
    }

    void recordTriangulation(
        const std::vector<std::pair<int, int>> &tri,
        std::vector<std::vector<std::pair<int, int>>> *storage)
    {
        ++totalTriangulationCount;
        accumulateFingerprint(hashTriangulation(tri));

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

    bool aggregatesEqual(const TriangulationRunStats &other) const
    {
        if (hashXxXor != other.hashXxXor || hashXxSum != other.hashXxSum)
            return false;
        if (hashSipXor != other.hashSipXor || hashSipSum != other.hashSipSum)
            return false;
        for (int i = 0; i < SHA_WORDS; ++i)
        {
            if (sha256Xor[i] != other.sha256Xor[i] || sha256Sum[i] != other.sha256Sum[i])
                return false;
        }
        return true;
    }
};

inline std::string formatHashHex(uint64_t value)
{
    char buf[19];
    std::snprintf(buf, sizeof(buf), "0x%016llX",
                  static_cast<unsigned long long>(value));
    return std::string(buf);
}

inline std::string formatSha256WordsHex(const uint64_t words[TriangulationRunStats::SHA_WORDS])
{
    std::string out;
    for (int i = 0; i < TriangulationRunStats::SHA_WORDS; ++i)
    {
        if (i > 0)
            out += '|';
        out += formatHashHex(words[i]);
    }
    return out;
}
