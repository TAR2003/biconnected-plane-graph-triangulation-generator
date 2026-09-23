// =======================================================================
// Simple I/O tool: read one input file (your face-list format), run the
// chosen algorithm, and write every triangulation found to a plain,
// human-readable output .txt file -- one triangulation per line, chords
// listed as (a, b) pairs, vertices renumbered back to your ORIGINAL
// input vertex ids (not the internal 0..n-1 cycle positions).
//
// This is NOT a benchmark or a correctness check -- it's the "just show
// me the actual triangulations" tool, e.g. for manually inspecting a
// small input, sanity-checking a hand-built test case, or generating a
// reference file to diff against later.
// =======================================================================
#include <bits/stdc++.h>
#include <filesystem>
using namespace std;
namespace fs = std::filesystem;

#include "Edge.hpp"
#include "PairHash.hpp"
#include "GraphTriangulation.hpp"

namespace {

enum class Algorithm { Oneconnected, Biconnected };

struct Options {
    Algorithm algorithm = Algorithm::Biconnected;
    fs::path input_path;
    fs::path output_path;
    bool include_boundary_edges = false; // if true, also list the original face boundary edges per face, for context
    bool number_triangulations = true;
};

Options options;

vector<vector<long long>> readFaces(const string &filename) {
    ifstream infile(filename);
    if (!infile.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        exit(1);
    }
    vector<vector<long long>> faces;
    long long faceno;
    if (!(infile >> faceno) || faceno < 1) {
        cerr << "Error: invalid face count in " << filename << endl;
        exit(1);
    }
    for (long long i = 0; i < faceno; i++) {
        long long vertices;
        infile >> vertices;
        vector<long long> face;
        for (long long j = 0; j < vertices; j++) {
            long long vertex;
            infile >> vertex;
            face.push_back(vertex);
        }
        faces.push_back(face);
    }
    return faces;
}

unique_ptr<GraphTriangulation> makeAlgorithm(vector<vector<long long>> &faces) {
    if (options.algorithm == Algorithm::Oneconnected) {
        return make_unique<GraphTriangulationOneconnectedCorrectness>(faces);
    }
    return make_unique<GraphTriangulationBiconnectedCorrectness>(faces);
}

void writeOutput(GraphTriangulation &gt, const vector<vector<long long>> &faces) {
    ofstream out(options.output_path);
    if (!out.is_open()) {
        cerr << "Error: could not open output file " << options.output_path.string() << " for writing." << endl;
        exit(1);
    }

    gt.sortTriangulations();

    out << "# Triangulation results\n";
    out << "# Algorithm: " << (options.algorithm == Algorithm::Oneconnected ? "oneconnected" : "biconnected") << "\n";
    out << "# Input faces: " << faces.size() << "\n";
    out << "# Total triangulations found: " << gt.allTriangulations.size() << "\n";
    out << "#\n";
    out << "# Each line below is one complete triangulation: the set of\n";
    out << "# internal chords that, together with the original face\n";
    out << "# boundaries, triangulate every face. Chords are written as\n";
    out << "# (vertex_a, vertex_b) pairs using YOUR ORIGINAL input vertex\n";
    out << "# ids, in ascending vertex order within each pair.\n";
    out << "#\n";

    if (options.include_boundary_edges) {
        out << "# --- Original face boundaries (for reference) ---\n";
        for (size_t f = 0; f < faces.size(); f++) {
            out << "# Face " << f << ": ";
            for (size_t i = 0; i < faces[f].size(); i++) {
                out << faces[f][i];
                if (i + 1 < faces[f].size()) out << " - ";
            }
            out << " - " << faces[f][0] << " (cycle)\n";
        }
        out << "#\n";
    }

    out << "# --- Triangulations (" << gt.allTriangulations.size() << " total) ---\n\n";

    long long index = 1;
    for (const auto &triangulation : gt.allTriangulations) {
        if (options.number_triangulations) {
            out << "Triangulation " << index++ << " (" << triangulation.size() << " chords):\n";
        }
        out << "  ";
        for (size_t i = 0; i < triangulation.size(); i++) {
            long long a = triangulation[i].first, b = triangulation[i].second;
            if (a > b) swap(a, b);
            out << "(" << a << ", " << b << ")";
            if (i + 1 < triangulation.size()) out << " , ";
        }
        out << "\n";
        if (options.number_triangulations) out << "\n";
    }

    out.close();
    cout << "Wrote " << gt.allTriangulations.size() << " triangulation(s) to "
         << options.output_path.string() << endl;
}

string optionValue(int *index, int argc, char **argv, const string &opt) {
    string arg = argv[*index];
    if (arg.rfind(opt + "=", 0) == 0) return arg.substr(opt.size() + 1);
    if (arg == opt && *index + 1 < argc) return argv[++*index];
    return {};
}

void parseArgs(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            cout <<
R"(triangulate -- run one input file through the algorithm and write every
                triangulation found to a readable output .txt file

  --input=PATH             input file in your face-list format (required)
  --output=PATH             output .txt path (default: <input_basename>_triangulations.txt)
  --algorithm=oneconnected|biconnected   which algorithm to run (default: biconnected)
  --include-boundary        also print the original face boundary edges for reference
  --no-numbering             omit "Triangulation N (k chords):" headers, one triangulation per line only
  -h, --help                 show this message
)";
            exit(0);
        } else if (arg == "--input" || arg.rfind("--input=", 0) == 0) {
            options.input_path = optionValue(&i, argc, argv, "--input");
        } else if (arg == "--output" || arg.rfind("--output=", 0) == 0) {
            options.output_path = optionValue(&i, argc, argv, "--output");
        } else if (arg == "--algorithm" || arg.rfind("--algorithm=", 0) == 0) {
            string v = optionValue(&i, argc, argv, "--algorithm");
            if (v == "oneconnected") options.algorithm = Algorithm::Oneconnected;
            else if (v == "biconnected") options.algorithm = Algorithm::Biconnected;
            else { cerr << "--algorithm must be oneconnected or biconnected\n"; exit(1); }
        } else if (arg == "--include-boundary") {
            options.include_boundary_edges = true;
        } else if (arg == "--no-numbering") {
            options.number_triangulations = false;
        } else {
            cerr << "Unrecognized argument: " << arg << " (use --help)\n";
            exit(1);
        }
    }

    if (options.input_path.empty()) {
        cerr << "Error: --input=PATH is required (use --help)\n";
        exit(1);
    }
    if (options.output_path.empty()) {
        options.output_path = options.input_path.stem().string() + "_triangulations.txt";
    }
}

} // namespace

int main(int argc, char **argv) {
    parseArgs(argc, argv);

    vector<vector<long long>> faces = readFaces(options.input_path.string());
    if (faces.empty()) return 1;

    cout << "Running " << (options.algorithm == Algorithm::Oneconnected ? "oneconnected" : "biconnected")
         << " algorithm on " << options.input_path.string()
         << " (" << faces.size() << " face(s))..." << endl;

    auto gt = makeAlgorithm(faces);
    gt->getAllTriangulations();

    writeOutput(*gt, faces);
    return 0;
}

// =======================================================================
// COMMAND LINE ARGUMENTS
//
//   ./triangulate --input=inputs/hexagon.txt
//       Runs Biconnected (default) on hexagon.txt, writes
//       hexagon_triangulations.txt in the current directory.
//
//   ./triangulate --input=inputs/hexagon.txt --output=results/hex_out.txt
//       Custom output path (creates parent dirs? no -- see note below;
//       make sure results/ already exists, or run through a shell that
//       creates it: mkdir -p results && ./triangulate ...).
//
//   ./triangulate --input=inputs/hexagon.txt --algorithm=oneconnected
//       Use the Oneconnected algorithm instead.
//
//   ./triangulate --input=inputs/hexagon.txt --include-boundary
//       Also print the original face boundary edges at the top of the
//       output file, for context when reading the chord list.
//
//   ./triangulate --input=inputs/hexagon.txt --no-numbering
//       Compact output: one triangulation's chord list per line, no
//       "Triangulation N (k chords):" header lines. Easier to grep/diff.
// =======================================================================
