#pragma once
// -----------------------------------------------------------------------
// Generates synthetic face structures to benchmark against, so you can
// sweep over N (vertices per face) and structural parameters without
// hand-authoring test graphs.
// -----------------------------------------------------------------------
#include <vector>
#include <cstdint>

namespace graphgen {

// A single convex-position N-gon: vertices 0..N-1 in cyclic order, no
// chords marked present beyond the boundary. This is the "one big face"
// case -- the worst case for combinatorial explosion since it has the
// maximum number of triangulations (Catalan number growth).
std::vector<long long> SimplePolygon(long long n);

// A "fan" structure: k faces sharing a common apex vertex, each face
// with `verticesPerFace` vertices. Useful for stressing the multi-face
// GraphTriangulation path (present-set sharing across faces) rather
// than a single FaceTriangulation in isolation.
std::vector<std::vector<long long>> FanOfFaces(long long numFaces, long long verticesPerFace);

// A "strip" structure: a chain of k faces, each sharing one edge with
// the next, verticesPerFace vertices each. Stresses boundary-sharing
// logic differently from the fan (shared edges rather than a shared
// vertex), and is a more realistic proxy for a planar-subdivision input.
std::vector<std::vector<long long>> StripOfFaces(long long numFaces, long long verticesPerFace);

// Deterministic seed so results are reproducible across benchmark runs --
// important if you ever add a randomized generator (e.g. random chord
// pre-population to study how `present`-set density affects flip cost).
void SetSeed(uint64_t seed);

} // namespace graphgen
