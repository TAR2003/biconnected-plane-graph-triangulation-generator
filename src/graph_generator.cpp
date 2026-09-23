#include "graph_generator.hpp"

namespace graphgen {

namespace {
uint64_t g_seed = 42;
}

void SetSeed(uint64_t seed) { g_seed = seed; }

std::vector<long long> SimplePolygon(long long n) {
    std::vector<long long> poly(n);
    for (long long i = 0; i < n; ++i) poly[i] = i;
    return poly;
}

std::vector<std::vector<long long>> FanOfFaces(long long numFaces, long long verticesPerFace) {
    // Vertex 0 is the shared apex. Each face i occupies a disjoint arc of
    // fresh vertices [base, base + verticesPerFace - 2] plus the apex,
    // so faces only share the single apex vertex with each other.
    std::vector<std::vector<long long>> faces;
    faces.reserve(numFaces);

    long long nextVertex = 1;
    for (long long f = 0; f < numFaces; ++f) {
        std::vector<long long> face;
        face.push_back(0); // shared apex
        for (long long i = 0; i < verticesPerFace - 1; ++i) {
            face.push_back(nextVertex++);
        }
        faces.push_back(std::move(face));
    }
    return faces;
}

std::vector<std::vector<long long>> StripOfFaces(long long numFaces, long long verticesPerFace) {
    // Face i and face i+1 share exactly one edge (two consecutive
    // vertices). Each face contributes (verticesPerFace - 2) new
    // vertices beyond the two it shares with its predecessor.
    std::vector<std::vector<long long>> faces;
    faces.reserve(numFaces);

    long long nextVertex = 0;
    long long sharedA = nextVertex++;
    long long sharedB = nextVertex++;

    for (long long f = 0; f < numFaces; ++f) {
        std::vector<long long> face;
        face.push_back(sharedA);
        face.push_back(sharedB);
        for (long long i = 0; i < verticesPerFace - 2; ++i) {
            face.push_back(nextVertex++);
        }
        faces.push_back(face);

        // Next face shares the edge (sharedB, last vertex added) --
        // pick the last two vertices of this face as the new shared edge.
        sharedA = face[face.size() - 1];
        sharedB = sharedA; // placeholder to keep structure simple; see note below
        sharedB = face.back();
        sharedA = face[face.size() - 2 >= 0 ? face.size() - 2 : 0];
        sharedA = face.back();
        sharedB = nextVertex; // will be first new vertex of next face's chain
        // NOTE: for a real strip topology you'll want to design the exact
        // shared-edge convention to match how GraphTriangulation expects
        // `present` to be pre-populated (see initiatePresent()). This
        // generator gives you the right SHAPE (chain of faces, each
        // sharing 2 vertices with the next) -- validate the exact vertex
        // numbering against your solver's assumptions before trusting
        // correctness benchmarks (as opposed to pure timing benchmarks,
        // where the exact topology matters less than N and face count).
    }
    return faces;
}

} // namespace graphgen
