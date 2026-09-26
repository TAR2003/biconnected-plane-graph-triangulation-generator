# Biconnected Plane Graph Triangulation Generator

C++17 implementations and experiments for enumerating all triangulations of plane graphs. The repository contains performance implementations for biconnected and one-connected inputs, a triconnected implementation used by the development checks, benchmark drivers, graph generators, and plotting utilities.

The main supported build is the CMake project at the repository root. It builds two Google Benchmark executables:

- `bench_total`: measures total enumeration time and peak memory for each input graph.
- `bench_individual`: records the cumulative and incremental time at every generated triangulation.

Enumeration is output-sensitive: the programs visit every triangulation up to the configured limit, so runtime can grow rapidly with the number of triangulations.

## Repository layout

```text
include/          Header-only triangulation algorithms used by the benchmarks
bench/            Google Benchmark drivers and memory tracking support
test-cases/       Python graph generators and generated input cases
averageTimeGraph/ Per-triangulation timing experiment and plotting scripts
dev/              Correctness checks, standalone experiments, and plotting tools
benchmark-results Generated CSV output from the benchmark executables
graphs/           Generated benchmark plots and summaries
graphs_output/    Additional generated plots
build/            Local CMake build directory
```

The checked-in `build/`, `_deps/`, result files, and image directories may contain outputs from previous experiments. Recreate the build directory when a clean build is needed.

## Core headers

| Header | Purpose |
| --- | --- |
| `include/GraphTriangulation.hpp` | Multi-face orchestration and output dispatch |
| `include/FaceTriangulation.hpp` | Base per-face triangulation traversal |
| `include/FaceTriangulationBiconnected.hpp` | Biconnected face enumeration and performance variants |
| `include/FaceTriangulationOneconnected.hpp` | One-connected face enumeration and performance variants |
| `include/GraphTriangulationTriconnected.hpp` | Triconnected generation/refinement path |
| `include/ParvezRahmanNakano.hpp` | Reference algorithm utilities |
| `include/Edge.hpp`, `include/PairHash.hpp` | Edge representation and pair hashing |

The benchmark selects either `GraphTriangulationBiconnectedPerformance` or `GraphTriangulationOneconnectedPerformance`. The individual benchmark selects the corresponding `IndividualPerformance` class so it can record generation times.

## Build

### Requirements

- CMake 3.16 or newer
- A C++17 compiler
- Git and network access on the first configure, because CMake fetches Google Benchmark v1.9.1
- Python 3 plus the plotting dependencies when using the Python tools

The benchmark support and memory tracker are developed for Linux-like environments. WSL2 is the recommended Windows setup. Native Windows builds may work with a compatible C++ toolchain, but the shell scripts and some memory-related experiments are Linux-oriented.

### CMake build

From the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

The same commands are in `build.sh`. CMake builds `bench_total` and `bench_individual` and fetches Google Benchmark into `_deps/` if necessary. Keep any sanitizer build configuration separate from timing builds.

## Input format

Each graph is represented by its faces. The first line is the number of faces. Each following line contains the number of vertices on that face followed by its vertex IDs in boundary order:

```text
<number of faces>
<face size> <v0> <v1> ... <v_k-1>
```

Example:

```text
2
3 0 1 2
4 0 2 3 4
```

Vertex IDs are integers. The benchmark counts distinct IDs to report the graph's vertex count and expects a valid plane-face description.

The standard dataset root is `test-cases/input/`:

```text
test-cases/input/
├── Biconnected/
└── Oneconnected/
```

Inputs are discovered recursively from `.txt` files in these directories.

## Run `bench_total`

The algorithm is required and must be supplied using the `--key=value` form:

```bash
./build/bench_total --algo=biconnected --cases=biconnected
./build/bench_total --algo=oneconnected --cases=oneconnected
./build/bench_total --algo=oneconnected --cases=biconnected
```

Useful options are:

| Option | Meaning | Default |
| --- | --- | --- |
| `--algo=biconnected\|oneconnected` | Algorithm under test; required | none |
| `--cases=biconnected\|oneconnected\|all` | Input families to run | `all` |
| `--runs=N` | Recorded runs per case | `3` |
| `--limit=N` | Maximum triangulations passed to the generator | `10000000` |
| `--input=DIR` | Input root containing the two family directories | repository `test-cases/input` |
| `--output=DIR` | Result root | repository `benchmark-results` |

For example:

```bash
./build/bench_total --algo=biconnected --cases=biconnected --runs=5 --limit=100000 --output=benchmark-results
```

Each run measures graph construction plus enumeration for peak-memory accounting, while the reported benchmark time is the enumeration interval. Results are appended to CSV files under:

```text
benchmark-results/<algorithm>/total/<input-family>__<subdirectory>...csv
```

The benchmark skips a case once its CSV already contains the requested number of runs. Delete or move the relevant result files before a fresh experiment.

## Run `bench_individual`

This driver records one row per generated triangulation, including the run number, triangulation number, cumulative nanoseconds, and delta nanoseconds:

```bash
./build/bench_individual --algo=oneconnected --cases=all --limit=5000
./build/bench_individual --algo=biconnected --cases=biconnected --runs=2 --limit=5000
```

The default is one run per case and a limit of `10000`. Results are stored under:

```text
benchmark-results/<algorithm>/individual/<input-family>__<subdirectory>.../<case>.csv
```

Use `--input=...` and `--output=...` to run a separate dataset or write results elsewhere. Google Benchmark flags such as `--benchmark_list_tests` and `--benchmark_format=json --benchmark_out=results.json` can also be passed to either executable.

The root `run.sh` contains example benchmark commands. It assumes the binaries have already been built.

## Generate test cases

`test-cases/main.py` generates biconnected graph families. It reads `test-cases/config.json` by default and writes generated inputs to `test-cases/input`:

```bash
cd test-cases
python3 main.py
python3 main.py --config config.json --count 25 --out input
```

The generator supports subdivision, Halin, cycle, cycle-union, snowflake, and chord-sequence families. Global limits include face count, face size, total vertices, and random seed; category-specific overrides are available in the JSON configuration.

`visualizer.py` renders graph images and `make-pdf.py` packages generated images. Their outputs are written below `test-cases/output-images/`.

## Analysis and plotting

- `plot.py` reads benchmark-style CSV data and writes plots to `graphs/`. It requires `pandas`, `matplotlib`, and `numpy`.
- `dev/plot_results.py` plots the older aggregate result format.
- `dev/plot-graph.py` provides graph visualization for development experiments.
- `averageTimeGraph/plot_results.py` plots cumulative average timing, while `plot_nth_results.py` focuses on the first N triangulations.

The per-triangulation experiment can be built independently:

```bash
cd averageTimeGraph
g++ -std=c++17 -O3 -o timer main.cpp
./timer
python3 plot_results.py results
python3 plot_nth_results.py results --n 100
```

This experiment uses a local copy of part of the triangulation code and should be treated as a separate analysis tool from the CMake benchmarks.

## Development checks

The `dev/` directory contains standalone correctness and complexity programs. For example, from `dev/`:

```bash
g++ -std=c++17 -O2 -o correctness-biconnected correctnessCheckBiconnected.cpp
./correctness-biconnected

g++ -std=c++17 -O2 -o correctness-oneconnected correctnessCheckOneconnected.cpp
./correctness-oneconnected
```

Other programs include `simpleTest.cpp`, `timeComplexityCheckBiconnected.cpp`, `timeComplexityCheckOneconnected.cpp`, and `modifyInput.cpp`. They use the local headers and input files in `dev/` and are exploratory utilities rather than CMake targets.

## Research notes

Record the compiler, CMake version, operating system, CPU, build type, command line, input revision, run count, and triangulation limit with each experiment. Keep release and sanitizer results separate. Because enumeration is output-sensitive, compare runs using triangulation counts as well as wall time, and avoid interpreting a timeout or truncated run as a completed result.

## License and citations

No license file is currently present in the repository. Add the appropriate license and citation information before redistributing the project or its results.
