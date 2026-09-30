# Plane Graph Triangulation Generator

A C++17 implementation and benchmark suite for enumerating triangulations of plane graphs. The project works directly from a rotation system: each vertex is described by its neighbors in cyclic order, so the embedding is part of the input rather than reconstructed from an abstract graph.

The repository contains three independently selectable enumeration algorithms:

- **Biconnected**: `GraphTriangulationBiconnected`, using `FaceTriangulationBiconnected` and its VGS bookkeeping.
- **BiconnectedWithoutVGS**: `GraphTriangulationBiconnectedWithoutVGS`, using the alternative face traversal that does not maintain the valid-generating-set list.
- **Oneconnected**: `GraphTriangulationOneconnected`, using the additional conflict checks required when faces share cut vertices.

All three algorithms use the same input cases, result schema, benchmark drivers, and plotting pipeline. Their outputs are kept in separate result directories so runs can be resumed independently.

## Repository Layout

| Path | Purpose |
| --- | --- |
| `include/` | Header-only triangulation algorithms and shared graph data structures. |
| `bench/` | Google Benchmark drivers for total-run and per-triangulation timing. |
| `test-cases/` | Python input generators, validation, and generator documentation. |
| `dev/` | Correctness checks, experimental programs, visualizers, and development utilities. |
| `plot.py` | Generates comparison graphs from benchmark CSV files. |
| `CMakeLists.txt` | CMake project definition and pinned Google Benchmark dependency. |
| `run.sh` | Example benchmark commands for the three algorithms. |
| `benchmark-results/` | Benchmark CSV output. |
| `graphs_output/` | Generated PNG graphs. |

Generated build and result directories are working data, not required source dependencies.

## Requirements

- CMake 3.16 or newer.
- A C++17 compiler.
- Git, because CMake fetches Google Benchmark v1.9.1 with `FetchContent`.
- Python 3 for plotting.
- Python packages for plotting: `numpy`, `pandas`, `matplotlib`, and `scipy`.
- Python package `networkx` for most input generators.
- Optional: `plantri` for exhaustive small-input generation.

On Windows, use a configured Visual Studio developer prompt, or another CMake generator whose compiler is available. A MinGW build may require disabling or adjusting warning-as-error settings in the vendored Google Benchmark dependency, depending on the compiler version.

## Build

From the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The build creates:

- `bench_total`: total enumeration time and peak allocation measurements.
- `bench_individual`: timestamps for every generated triangulation.

For a clean rebuild, remove the build directory and run the commands again. Do not reuse a CMake cache created from a different operating-system path or generator.

## Input Format

Each input file is a rotation system:

```text
N
degree_0 neighbor_0 neighbor_1 ...
degree_1 neighbor_0 neighbor_1 ...
...
degree_N-1 neighbor_0 neighbor_1 ...
```

`N` is the number of vertices. Each following line gives the degree and the cyclic neighbor order for one vertex. Neighbor labels must match the convention used by the generator, normally `0` through `N - 1`.

The shared reader validates the file shape while `rotationSystemToFaces` traces the embedded faces. The algorithms then work on those faces and a shared multiset of already-present boundary edges.

## Generate Test Cases

The generator package lives under `test-cases/`. A typical setup is:

```bash
cd test-cases
python main.py
python gen_biconnected.py --out input/Biconnected --seed 42
python gen_oneconnected.py --out input/Oneconnected --seed 42
```

The generated layout is compatible with the benchmark harness:

```text
input/
  Biconnected/<category>/<case>.txt
  Oneconnected/<category>/<case>.txt
```

`gen_biconnected.py` provides biconnected stress families such as cycles, wheels, grids, prisms, outerplanar graphs, stacked triangulations, and Delaunay-derived graphs. `gen_oneconnected.py` provides trees, cacti, block trees, pendant structures, and other graphs with cut vertices. See [test-cases/README.md](test-cases/README.md) for generator options, validation rules, and family details.

## Benchmark CLI

Both benchmark executables accept the same algorithm and case-selection options:

```text
--algo=biconnected
--algo=biconnected-without-vgs
--algo=oneconnected
--cases=biconnected|oneconnected|all
--runs=N
--limit=N
--input=DIR
--output=DIR
```

`--algo` is required. The aliases `biconnected_without_vgs` and `biconnectedWithoutVGS` are also accepted for the third algorithm.

`--cases` controls which input folders are discovered; it does not select the algorithm. This makes it possible to compare every algorithm on the same biconnected inputs, the same one-connected inputs, or both.

### Total benchmark

```bash
./build/bench_total --algo=biconnected --cases=all --runs=3 --limit=10000000
./build/bench_total --algo=biconnected-without-vgs --cases=all --runs=3 --limit=10000000
./build/bench_total --algo=oneconnected --cases=all --runs=3 --limit=10000000
```

Each run records total triangulations, elapsed seconds, peak tracked allocation, memory per vertex, check counters, traversal counters, timestamps, and status. Output is stored under:

```text
benchmark-results/<Algorithm>/total/<InputFamily>__<category>.csv
```

The benchmark is resumable. Existing rows are counted per case and only missing repetitions are registered.

### Individual benchmark

```bash
./build/bench_individual --algo=biconnected --cases=all --limit=5000
./build/bench_individual --algo=biconnected-without-vgs --cases=all --limit=5000
./build/bench_individual --algo=oneconnected --cases=all --limit=5000
```

This records `run`, `triangulation`, `cumulativeNs`, and `deltaNs` for every generated triangulation. Output is stored under:

```text
benchmark-results/<Algorithm>/individual/<InputFamily>__<category>/<case>.csv
```

The three algorithm names in the directory structure are:

```text
Biconnected/
BiconnectedWithoutVGS/
Oneconnected/
```

## Plotting

After benchmark CSVs exist, generate the complete comparison suite:

```bash
python plot.py --root ./benchmark-results --outdir ./graphs_output
```

The plotting code discovers all three result directories and includes them in:

- overall scaling, memory, timing, checks, traversal, and status charts;
- per-category comparisons;
- per-case individual timing charts;
- algorithm-scope overlays;
- pairwise scatter and Bland-Altman comparisons;
- pairwise triangulation-count and speed-ratio charts.

The pairwise charts compare every combination: Biconnected versus BiconnectedWithoutVGS, Biconnected versus Oneconnected, and BiconnectedWithoutVGS versus Oneconnected. Missing data is skipped without preventing plots for pairs that are available.

## Algorithm Structure

`GraphTriangulation` owns the graph-level traversal. It traces faces, initializes the existing-edge multiset, creates one face triangulator at a time, and forwards completed combinations to `storeTriangulation()`.

The graph-level subclasses select the face implementation:

- `GraphTriangulationBiconnected` -> `FaceTriangulationBiconnected`.
- `GraphTriangulationBiconnectedWithoutVGS` -> `FaceTriangulationBiconnectedWithoutVGS`.
- `GraphTriangulationOneconnected` -> `FaceTriangulationOneconnected`.

Performance subclasses override `storeTriangulation()` with an empty method so enumeration cost is measured without materializing every result. Correctness subclasses call `addTriangulation()` so complete triangulations can be compared. Individual-performance subclasses record timestamps through `GenerationTimeline`.

The face triangulators use a rooted polygon representation. Chords are flipped recursively, and `Edge` stores the neighboring endpoints needed to update opposite edges after a flip. The VGS variant maintains a list of currently valid generating-set edges. The WithoutVGS variant keeps only the generating set and performs the corresponding traversal without maintaining that auxiliary list. The Oneconnected variant additionally tracks conflicts within the current face and the graph-wide present-edge multiset.

## Correctness Checks

The development correctness programs compare generated triangulations against `GraphTriangulationTriconnected`, which uses `ParvezRahmanNakano` to enumerate local polygon triangulations and then filters invalid combinations.

The reusable correctness classes are:

- `GraphTriangulationBiconnectedCorrectness`.
- `GraphTriangulationBiconnectedWithoutVGSCorrectness`.
- `GraphTriangulationOneconnectedCorrectness`.

The correctness programs are development utilities rather than CMake targets. Compile them with the project include directory and run them from a directory containing the expected `input/` tree. The existing biconnected and one-connected checkers can be extended or invoked with the corresponding correctness class when validating a new case.

When comparing results, canonicalize every edge as `(min(u, v), max(u, v))`, sort the edges inside each triangulation, and sort the triangulation list. This avoids differences caused only by traversal order or edge orientation.

## Metrics and Interpretation

Total benchmark CSV columns include:

- `triangulations`: completed valid triangulations.
- `timeSeconds`: measured enumeration duration.
- `peakMemoryBytes`, `memoryPerVertex`: allocation-window measurements.
- `totalChecks`, `successfulChecks`, `failedChecks`, `checkSuccessRate`: face-level candidate checks.
- `invalidTraversals`, `totalTraversalsExtended`, `traversalSuccessRate`: accepted versus rejected graph-level traversals.
- `startTime`, `endTime`, `status`: run bookkeeping.

A triangulation limit bounds enumeration for large cases. A run reaching the limit is not a complete count of all triangulations and should be reported as truncated in analysis.

Peak allocation is measured by the benchmark memory tracker and should be interpreted as the tracker window's peak, not a complete operating-system memory profile. Timing results are sensitive to compiler, optimization level, CPU frequency, filesystem state, and input ordering.

## Development Notes

- Public implementations are header-only under `include/`; keep changes consistent with the existing include order and virtual dispatch model.
- Use separate result directories for algorithm variants. Do not mix CSV schemas from unrelated experiments.
- Keep generated benchmark output out of source edits unless a result artifact is intentionally being updated.
- For reproducible experiments, record the generator seed, input manifest, compiler, build type, algorithm name, limit, and run count.
- Large triangulation counts can grow rapidly. Use a finite `--limit` and isolated runs for stress cases.

## Validation Commands

Useful lightweight checks are:

```bash
python -m py_compile plot.py
g++ -std=c++17 -fsyntax-only -Iinclude -Ibench bench/bench_total.cpp
g++ -std=c++17 -fsyntax-only -Iinclude -Ibench bench/bench_individual.cpp
```

The normal end-to-end check is a CMake build followed by a small benchmark invocation against a small generated input directory. The build must use a compiler and generator supported by the host environment.
