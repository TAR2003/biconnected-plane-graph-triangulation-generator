# Plane Graph Triangulation Generator

A C++17 implementation and benchmark suite for enumerating triangulations of plane graphs. The project works directly from a rotation system: each vertex is described by its neighbors in cyclic order, so the embedding is part of the input rather than reconstructed from an abstract graph.

The repository benchmarks three independently selectable enumeration algorithms from `src/`:

- **BiconnectedWithVGS**: `GenerateBiconnectedTriangulationsWithVGS`.
- **BiconnectedWithoutVGS**: `GenerateBiconnectedTriangulationsWithoutVGS`.
- **Oneconnected**: `GenerateOneconnectedTriangulations`.

The benchmark adapters live under `bench/`; the algorithm headers in `src/` are used directly and are not modified by the benchmark.

## Repository Layout

| Path | Purpose |
| --- | --- |
| `src/` | Header-only triangulation algorithms and shared graph data structures. |
| `bench/` | Google Benchmark timing driver, memory driver, and adapters for the `src/` algorithms. |
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

- `bench_time`: total enumeration time measurements.
- `bench_memory`: peak resident-memory measurements.

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

Both benchmark executables accept:

```text
--tri_input=DIR
--tri_algos=BiconnectedWithoutVGS,BiconnectedWithVGS,Oneconnected
--tri_limits=10,100,...
--tri_categories=category1,category2
--tri_csv=FILE
--tri_cpu=N
```

For source-configured runs, edit the settings block near the start of `main()` in
[`bench/bench_time.cpp`](bench/bench_time.cpp) or
[`bench/bench_memory.cpp`](bench/bench_memory.cpp). For example, set
`cfg.limits = {100, 1000};` to run those two triangulation limits, or adjust
`cfg.inputRoot`, `cfg.algos`, and `cfg.csv` there. The existing `--tri_*` flags
remain available and override the corresponding code-configured values.

By default `--tri_input=input` discovers `input/Biconnected` for both biconnected algorithms and `input/Oneconnected` for the one-connected algorithm. If `--tri_input` already names one of those algorithm-specific directories, that directory is used as-is. Existing benchmark results are used to resume incomplete runs.

### Timing benchmark
```bash
./build/bench_time --tri_algos=BiconnectedWithoutVGS,BiconnectedWithVGS,Oneconnected --tri_input=./input --tri_limits=1000000
```

### Memory benchmark
```bash
./build/bench_memory --tri_algos=BiconnectedWithoutVGS,BiconnectedWithVGS,Oneconnected --tri_input=./input --tri_limits=1000000
```

CSV output is split by algorithm and input category under:

```text
benchmark-results/<Algorithm>Algo/results_<category>.csv
```

The actual file name preserves the requested CSV stem, for example
`results_time_<category>.csv` or `results_memory_<category>.csv`.

The algorithm names are also the accepted `--tri_algos` values:

```text
BiconnectedWithoutVGS/
BiconnectedWithVGS/
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

The plotting scripts may require adaptation to consume the current benchmark CSV schema and algorithm names.

## Algorithm Structure

The benchmark uses separate adapter translation units because the algorithm headers in `src/` include their shared base header directly. The adapters expose the common `generateAllTriangulations()` entry point to both benchmark executables without changing those algorithms.

## Correctness Checks

The development correctness programs compare generated triangulations against `GraphTriangulationTriconnected`, which uses `ParvezRahmanNakano` to enumerate local polygon triangulations and then filters invalid combinations.

The reusable correctness classes are:

- `GraphTriangulationBiconnectedCorrectness`.
- `GraphTriangulationBiconnectedWithoutVGSCorrectness`.
- `GraphTriangulationOneconnectedCorrectness`.

The correctness programs are development utilities rather than CMake targets. Compile them with the project include directory and run them from a directory containing the expected `input/` tree. The existing biconnected and one-connected checkers can be extended or invoked with the corresponding correctness class when validating a new case.

When comparing results, canonicalize every edge as `(min(u, v), max(u, v))`, sort the edges inside each triangulation, and sort the triangulation list. This avoids differences caused only by traversal order or edge orientation.

## Metrics and Interpretation

The timing and memory CSVs both record `limit` and `triangulationLimit` (the
configured maximum for that run). The timing CSV records CPU and real time,
triangulation count, limit status, and check counters where the selected source algorithm exposes them. Unsupported
check and traversal metrics are left blank rather than reported as zero. The
memory CSV records peak RSS, baseline RSS, and peak delta; child RSS is only
available on platforms where the process runner reports it.

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
