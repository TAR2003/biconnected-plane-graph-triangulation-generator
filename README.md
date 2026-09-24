# Biconnected Plane Graph Triangulation Generator

Algorithms and experiments for generating all triangulations of biconnected (and related) plane graphs. The **current** C++ library lives under `include/`; the **Google Benchmark** harness is built from the repo root via CMake. Older standalone drivers and notebooks are kept in `oldCodes/`.

---

## Table of contents

- [Repository layout](#repository-layout)
- [Core library (`include/`)](#core-library-include)
- [Benchmark harness (`src/` + CMake)](#benchmark-harness-src--cmake)
- [Build the benchmarks](#build-the-benchmarks)
- [Run the benchmarks](#run-the-benchmarks)
- [Benchmark cases](#benchmark-cases)
- [Input graph format](#input-graph-format)
- [Benchmark datasets (`inputs/`)](#benchmark-datasets-inputs)
- [Test case generator (`test-case-generator/`)](#test-case-generator-test-case-generator)
- [Analysis outputs (`analyze-info/`)](#analysis-outputs-analyze-info)
- [Per-triangulation timing (`averageTimeGraph/`)](#per-triangulation-timing-averagetimegraph)
- [Legacy code (`oldCodes/`)](#legacy-code-oldcodes)
- [Platform notes](#platform-notes)

---

## Repository layout

```
biconnected-plane-graph-triangulation-generator/
│
├── CMakeLists.txt              # Builds triangulation_bench (Google Benchmark)
├── run_sweep.sh                # Linux: run each benchmark case in its own process + timeout
├── README.md                   # This file
├── .gitignore
├── .gitattributes
│
├── include/                    # Header-only triangulation engine (used by benchmarks)
├── src/                        # Benchmark driver, synthetic graphs, memory/timeout helpers
├── build/                      # Local CMake build directory (created by you; often gitignored)
│
├── test-cases/input/           # Real graph instances used by the dataset benchmark
│   ├── Biconnected/            # Biconnected cases, recursively grouped by family
│   └── Oneconnected/           # One-connected cases, recursively grouped by family
│
├── test-case-generator/        # Python generators + visualizations for synthetic families
├── benchmark-results/          # Generated per-category CSV files (created at runtime)
├── analyze-info/               # CSV results and plotting scripts for thesis experiments
├── averageTimeGraph/           # Records cumulative average time at every triangulation
├── oldCodes/                   # Previous layout: correctness/time checks, plots, notebook
└── .vscode/                    # Editor settings (optional)
```

---

## Core library (`include/`)

| File | Role |
|------|------|
| `Edge.hpp` | Canonical undirected edges as normalized vertex pairs |
| `PairHash.hpp` | Hash for edge pairs in unordered containers |
| `FaceTriangulation.hpp` | Base per-face genealogical tree traversal |
| `FaceTriangulationBiconnected.hpp` | Biconnected face enumeration (including performance variants) |
| `FaceTriangulationOneconnected.hpp` | One-connected face enumeration |
| `GraphTriangulation.hpp` | Multi-face orchestration, present-set, output dispatch |
| `GraphTriangulationTriconnected.hpp` | Triconnected path (PRN-style combination) |
| `ParvezRahmanNakano.hpp` | Reference utilities for the Parvez–Rahman–Nakano algorithm |

These headers are included by `src/bench_main.cpp` and by the legacy programs in `oldCodes/`.

---

## Benchmark harness (`src/` + CMake)

| File | Role |
|------|------|
| `bench_main.cpp` | Synthetic Google Benchmark cases (timing + custom counters) |
| `dataset_bench_main.cpp` | Recursive file-dataset benchmark with algorithm selection and CSV output |
| `graph_generator.hpp` / `graph_generator.cpp` | Synthetic inputs: simple polygon, fan of faces, strip of faces |
| `memory_tracker.hpp` / `memory_tracker.cpp` | Peak RSS and malloc-interposition counters (Linux-oriented) |
| `timeout_guard.hpp` | Watchdog used by the synthetic benchmark harness |

**CMake** (`CMakeLists.txt`):

- C++17, **Release** by default (`-O3 -DNDEBUG`)
- Fetches [Google Benchmark](https://github.com/google/benchmark) v1.9.1 via `FetchContent`
- Target executable: **`triangulation_bench`**
- Optional: `-DWITH_ASAN=ON` for AddressSanitizer (debugging only, not for timing)

---

## Build the benchmarks

### Prerequisites

- **CMake** 3.16+
- **C++17** compiler (`g++`, `clang++`, or MinGW on Windows)
- **Git** (first configure clones Google Benchmark)
- Network access on the first configure

**Recommended:** Linux or **WSL2**. The memory tracker and `run_sweep.sh` assume a Linux-like environment.

### Linux / macOS / WSL

From the repository root:

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
```

The binary is `build/triangulation_bench`.

### Windows (MinGW / MSYS2)

Open an **MSYS2 MinGW64** shell so `g++`, `cmake`, and `mingw32-make` are on `PATH`:

```bash
cd /c/Users/YOU/biconnected-plane-graph-triangulation-generator
rm -rf build && mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
mingw32-make -j
```

If `FetchContent` fails when cloning Google Benchmark (for example SSL certificate errors), fix Git/CA settings or install `google-benchmark` from MSYS2 and adjust `CMakeLists.txt` to link against the system package instead of fetching.

---

## Run the benchmarks

All commands assume you are in the `build/` directory (or pass the full path to the executable).

### Run the full suite

```bash
./triangulation_bench
```

Windows:

```powershell
.\triangulation_bench.exe
```

### Filter by name (regex)

```bash
./triangulation_bench --benchmark_filter=Biconnected
./triangulation_bench --benchmark_filter=Oneconnected
./triangulation_bench --benchmark_filter=Triconnected
```

### Export results (for papers / plots)

```bash
./triangulation_bench --benchmark_format=json --benchmark_out=results.json
./triangulation_bench --benchmark_format=csv  --benchmark_out=results.csv
```

### Repeat cases for statistics

Dataset runs are isolated by default: every repetition is a fresh child process, so peak RSS starts at zero for each observation. The default is three repetitions; choose five for a larger sample:

```bash
./dataset_bench --runs-per-case=5 --timeout=300 --cpu=0
```

`--timeout=300` kills a case after 300 seconds and records `timed_out_partial` when a checkpoint was written, or `timed_out_no_data` otherwise. Omit `--timeout` for an isolated run with no deadline. Do not use Google Benchmark's `--benchmark_min_time` for dataset measurements: each child is registered with exactly one manual iteration, so long enumerations are never auto-repeated.

### Reproducible experimental environment

Record the CPU model, base/boost frequency, RAM capacity/speed, L3 cache, OS/kernel, compiler version, CMake version, and the exact commit used for each experiment. Build the release binary with the explicit flags used by the paper:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DTRIANGULATION_NATIVE_ARCH=ON
cmake --build build --target dataset_bench -j
g++ --version
cmake --version
```

On Linux, run on a quiet machine with the CPU governor set to `performance`, Turbo Boost disabled for the experiment, and a pinned core. For example, the benchmark command above uses core 0 through the portable harness option `--cpu=0` (equivalent in purpose to `taskset -c 0`). These controls are Linux-only; Windows results should report the scheduler and power-plan settings instead of claiming CPU pinning.

Report wall-clock seconds, triangulations per second, check-success percentage, and peak RSS in MB. State the timeout and repetition count, and mark timed-out partial rows separately from completed rows in plots and tables.

### Minimum time per case (not for dataset measurements)

```bash
./triangulation_bench --benchmark_min_time=2s
```

### List registered test names

```bash
./triangulation_bench --benchmark_list_tests
```

Useful for scripting and for `run_sweep.sh`.

### Full sweep with per-case OS timeout (Linux)

For combinatorially heavy cases, run **one benchmark name per process** so a hung case can be killed without stopping the whole suite:

```bash
chmod +x ../run_sweep.sh
../run_sweep.sh ./triangulation_bench 30 ../results/
```

Arguments: `<binary> <timeout_seconds> <output_dir>`. Each case writes JSON under `output_dir/` and a `sweep_summary.csv`.

---

## Benchmark cases

Registered in `src/bench_main.cpp` (names appear as `TriangulationFixture/<CaseName>` in `--benchmark_list_tests`):

| Case | What it measures |
|------|------------------|
| **BiconnectedSingleFace** | One convex `N`-gon face; `N` swept with `DenseRange(4, 16, 2)` |
| **OneconnectedSingleFace** | Same polygon setup with the one-connected performance class; extra counters for checks / traversals |
| **BiconnectedFanOfFaces** | Multiple faces sharing an apex; face count swept with `Range(1, 32)` (×2 multiplier) |
| **TriconnectedFanOfFaces** | Fan structure via `GraphTriangulationTriconnected` (generate + refine) |

**Custom counters** (extra columns in console / JSON / CSV):

- `peak_rss_KB`, `bytes_allocated`, `alloc_count` (from `memory_tracker`)
- `triangulations_found` (and one-connected diagnostic counters where applicable)

The synthetic benchmark harness keeps its existing watchdog behavior. For dataset runs, use the runner-level timeout below: each case is launched in a separate child process, so a timeout kills only that case and the parent continues with the next one.

> **Catalan growth:** triangulation count on a convex `N`-gon grows like Catalan numbers. Extending `N` far beyond ~16–20 can make single cases run for minutes or hours even in “performance” (discard) mode, because enumeration still visits every triangulation.

---

## Input graph format

Shared by dataset files under `inputs/`, legacy drivers in `oldCodes/`, and `averageTimeGraph/`:

```
<number_of_faces>
<face_size> <v0> <v1> ... <v_{k-1}>
...
```

- Vertices are **0-indexed** integers.
- Each face is listed in **counter-clockwise** order on the outer boundary.

Example (two faces sharing edge `(0, 2)`):

```
2
3 0 1 2
4 0 2 3 4
```

The `triangulation_bench` binary uses synthetic graphs. The `triangulation_dataset_bench` binary reads the real cases under `test-cases/input/` recursively.

### File-dataset benchmark

Build and list the discovered cases:

```bash
cmake --build build --target triangulation_dataset_bench -j
./build/triangulation_dataset_bench --algorithm biconnected --list-cases
./build/triangulation_dataset_bench --algorithm oneconnected --list-cases
```

Select the algorithm explicitly with `--algorithm biconnected` or `--algorithm oneconnected`. The benchmark constructs the matching performance class from `GraphTriangulation.hpp`:

- `biconnected` -> `GraphTriangulationBiconnectedPerformance`
- `oneconnected` -> `GraphTriangulationOneconnectedPerformance`

### Dataset timeout

Use `--timeout=30s` (or `--timeout 30s`) to enforce a hard per-case process timeout:

```bash
./build/triangulation_dataset_bench \
	--algorithm biconnected \
	--timeout=30s \
	--runs-per-case=1
```

The parent runner launches one child per case. A completed child writes the normal CSV row. A child that exceeds the timeout is terminated, receives a `timed_out` CSV row with its input vertex count and elapsed wall time, and the next case starts. Counters that exist only inside the killed process cannot be recovered and are recorded as zero; the graph algorithms are not instrumented or modified.

### How many times does one case run?

There are two Google Benchmark controls:

- `--benchmark_repetitions=N` repeats the measured benchmark `N` times. Use this for independent timing samples.
- `--runs-per-case=N` fixes the number of algorithm executions in each repetition. The default is one. This is an application option implemented with Google Benchmark's fixed-iteration API.

For a clear research run, use `--runs-per-case=1 --benchmark_repetitions=10`. This produces ten measured CSV rows for each case, unless the case is skipped because that filename already exists in its category CSV.

The default is **resume mode**. Before registering a case, the program checks the category CSV. If the filename is already present, it prints `[SKIP]` and does not run it again. This check is per algorithm output directory, so biconnected and one-connected results are kept separate. Resume mode treats any existing row as complete; it does not try to infer whether a partially completed set of repetitions needs more rows.

Use `--rerun-existing` to ignore the CSV and run selected cases again. New rows are appended, not overwritten.

### Common commands

Run one biconnected algorithm case on one biconnected input:

```bash
./build/triangulation_dataset_bench \
	--algorithm biconnected \
	--case-filter=4_halin/case_001.txt \
	--runs-per-case=1 --benchmark_repetitions=10
```

Run the one-connected algorithm on one biconnected input:

```bash
./build/triangulation_dataset_bench \
	--algorithm oneconnected \
	--input-root=test-cases/input/Biconnected \
	--case-filter=4_halin/case_001.txt \
	--runs-per-case=1 --benchmark_repetitions=10
```

Run one biconnected algorithm case on one one-connected input:

```bash
./build/triangulation_dataset_bench \
	--algorithm biconnected \
	--input-root=test-cases/input/Oneconnected \
	--case-filter=04_others/bridge.txt \
	--runs-per-case=1 --benchmark_repetitions=10
```

Run one one-connected algorithm case on one one-connected input:

```bash
./build/triangulation_dataset_bench \
	--algorithm oneconnected \
	--case-filter=04_others/bridge.txt \
	--runs-per-case=1 --benchmark_repetitions=10
```

Run all biconnected inputs with the biconnected algorithm. Omit `--case-filter`:

```bash
./build/triangulation_dataset_bench \
	--algorithm biconnected \
	--runs-per-case=1 --benchmark_repetitions=10 \
	--benchmark_report_aggregates_only=true
```

Run all one-connected inputs with the one-connected algorithm:

```bash
./build/triangulation_dataset_bench \
	--algorithm oneconnected \
	--input-root=test-cases/input/Oneconnected \
	--runs-per-case=1 --benchmark_repetitions=10 \
	--benchmark_report_aggregates_only=true
```

Run all biconnected inputs with the one-connected algorithm:

```bash
./build/triangulation_dataset_bench \
	--algorithm oneconnected \
	--input-root=test-cases/input/Biconnected \
	--runs-per-case=1 --benchmark_repetitions=10
```

Run all one-connected inputs with the biconnected algorithm:

```bash
./build/triangulation_dataset_bench \
	--algorithm biconnected \
	--input-root=test-cases/input/Oneconnected \
	--runs-per-case=1 --benchmark_repetitions=10
```

Force a rerun even when the selected filenames already exist in CSV:

```bash
./build/triangulation_dataset_bench \
	--algorithm biconnected --case-filter=4_halin \
	--rerun-existing --runs-per-case=1 --benchmark_repetitions=10
```

The `--input-root` and `--case-filter` options combine: the filter is matched against the path relative to the selected input root. `--case-filter=4_halin` selects a whole family; `--case-filter=4_halin/case_001.txt` selects one file.

List every matching input without running it or applying resume skipping:

```bash
./build/triangulation_dataset_bench --algorithm biconnected --list-cases
```

The program prints progress during execution:

```text
[SKIP] Biconnected/4_halin/case_001.txt | already present in benchmark-results/biconnected/Biconnected/4_halin.csv
[RUNNING] Biconnected/4_halin/case_002.txt | algorithm=biconnected | iteration=1
[DONE] Biconnected/4_halin/case_002.txt | run=1 | 0.012345 s | triangulations=42 | csv=benchmark-results/biconnected/Biconnected/4_halin.csv
```

The `[RUNNING]` and `[DONE]` messages are printed once per measured iteration. Google Benchmark's summary is printed after the cases finish.

The older `--benchmark_min_time=1s` style is also valid, but it allows Google Benchmark to choose the number of iterations. Use `--runs-per-case=1` when each CSV row must represent exactly one algorithm execution.

On Windows, use the repository's WSL2 build and run the Linux commands above from WSL. The current memory tracker and `pthread` linkage are Linux-oriented; native Visual Studio builds are not the supported research configuration.

Each `.txt` file is registered as one Google Benchmark case. Input parsing and graph construction are paused out of the measured interval; only `getAllTriangulations()` is timed. Results are appended to a CSV whose path mirrors the input category, for example:

```text
benchmark-results/biconnected/Biconnected/04_halin.csv
benchmark-results/oneconnected/Oneconnected/04_others.csv
```

The CSV contains the legacy experiment fields: filename, run index, distinct vertices, triangulation count, seconds, peak RSS, memory per vertex, timestamps, status, total/successful/failed checks, check success rate, invalid traversals, extended traversal count, and traversal success rate. Google Benchmark's own JSON/CSV report can be emitted separately with `--benchmark_format=json --benchmark_out=benchmark-results/google-benchmark.json`.

For research runs, pin the executable build to Release, record the command line and machine details, use multiple `--benchmark_repetitions`, and keep `--benchmark_report_aggregates_only=true` for aggregate summaries. The generator enumerates every triangulation, so use `--benchmark_filter` or `--case-filter` to isolate expensive cases. For a hard kill boundary, run one case per process with an external OS timeout; an in-process timeout would terminate the complete benchmark run.

---

## Benchmark datasets (`inputs/`)

### `inputs/Biconnected/`

Graph families used for biconnected experiments (hundreds of `.txt` files):

| Folder | Description |
|--------|-------------|
| `1_triangulation_subdivide_1/` | Subdivision-style cases (family 1) |
| `2_triangulation_subdivide_2/` | Subdivision-style cases (family 2) |
| `3_triangulation_subdivide_3/` | Subdivision-style cases (family 3) |
| `4_halin/` | Halin-graph-style instances |
| `5_cycles/` | Cycle-dominated structures |
| `6_cycle_union/` | Unions of cycles |
| `7_snowflake/` | Snowflake constructions |
| `8_chord_sequence/` | Chord-sequence graphs |
| `9_general_biconnected/` | General instances (including parametric θ-graphs, cycles, grids) |

Many categories include a `case_manifest.txt` listing cases in that folder.

### `inputs/Oneconnected/`

| Folder | Description |
|--------|-------------|
| `01_path/` | Path graphs (`path05.txt` … `path60.txt`, etc.) |
| `02_star/` | Stars at various sizes |
| `03_tree/` | Trees (binary, caterpillar, broom, comb, …) |
| `04_others/` | Bridges, bowties, cactus, multi-face misc. |

---

## Test case generator (`test-case-generator/`)

Python tooling to **generate** and **visualize** graph families (mirrors many `inputs/Biconnected` categories).

| Script | Purpose |
|--------|---------|
| `main.py` | Entry point for generation workflows |
| `common.py` | Shared helpers |
| `gen_triangulation_subdivide.py` | Subdivision families |
| `gen_halin.py` | Halin-style graphs |
| `gen_cycles.py` | Cycle cases |
| `gen_snowflake.py` | Snowflake cases |
| `gen_chord_sequence.py` | Chord-sequence cases |
| `visualizer.py` | Render graphs to images |
| `make-pdf.py` | Bundle images into PDFs |

Typical layout:

- `input/<category>/case_XXX.txt` — generated graph files  
- `output-images/<category>/` — PNG previews  
- `output-images/pdfs/` — per-category PDFs  

Requires **Python 3** and dependencies used by those scripts (see imports in each file; commonly `matplotlib` for visualization).

---

## Analysis outputs (`analyze-info/`)

Experiment results and figures comparing algorithm variants (for example with vs. without vertex-group shortcuts, “vgs”):

| Path | Contents |
|------|----------|
| `results-with-vgs/` | CSV result tables per graph category |
| `results-without-vgs/` | Same categories without VGS |
| `plots_results-with-vgs/` | Aggregate plots (timing, memory, throughput, …) |
| `plots_results-without-vgs/` | Same plot set for the without-VGS runs |
| `plots_comparison/` | Side-by-side comparisons across datasets and categories |
| `plot.py` | Script used to generate many of the figures |

Categories align with dataset folders (e.g. `1_triangulation_subdivide_1`, `4_halin`, `5_cycles`).

---

## Per-triangulation timing (`averageTimeGraph/`)

Separate from the CMake benchmark binary: records a timestamp **after every triangulation** to plot running cumulative average time (validates O(1) amortized behavior within a single long run).

| Item | Role |
|------|------|
| `main.cpp` | Driver with `onTriangulationGenerated` hook |
| `biconnected.hpp`, `FaceTriangulation.hpp`, `Edge.hpp`, `pairHash.hpp` | Local copy of the engine for this experiment |
| `input/small/`, `input/medium/`, `input/test_single/` | Graph inputs |
| `plot_results.py` | Full-run cumulative-average plots → `graphs/` |
| `plot_nth_results.py` | First-N triangulation zoom → `nthGraphs/` |
| `results.csv`, `results.html`, `results.txt` | Example outputs |

Build and run (from `averageTimeGraph/`):

```bash
g++ -std=c++17 -O3 -o timer main.cpp
./timer
python3 plot_results.py results
python3 plot_nth_results.py results --n 100
```

---

## Legacy code (`oldCodes/`)

Previous monolithic layout before `include/` + CMake benchmarks:

| Item | Role |
|------|------|
| `correctnessCheckBiconnected.cpp` / `correctnessCheckOneconnected.cpp` | Cross-check outputs |
| `timeComplexityCheckBiconnected.cpp` / `timeComplexityCheckOneconnected.cpp` | Batch timing on file inputs |
| `plot-graph.py`, `plot_results.py` | Plotting helpers |
| `input.txt`, `output.txt`, `results_*.csv` | Sample I/O and results |
| `run` | Shell helper to compile/run checks |
| `thesis-triangulation-generation.ipynb` | Notebook |
| `README.md` | Older thesis-oriented documentation (multi-module layout) |

Example (manual compile):

```bash
cd oldCodes
g++ -std=c++17 -O3 -o check_bicon correctnessCheckBiconnected.cpp
./check_bicon
```

Adjust includes if you compile from `oldCodes/` while headers live in `../include/`.

---

## Platform notes

| Component | Linux / WSL | Native Windows |
|-----------|-------------|----------------|
| `triangulation_bench` (CMake) | Supported | MinGW possible; fix FetchContent/Git SSL if needed |
| `memory_tracker.cpp` | Full RSS + malloc hooks | Not supported as-is (uses `getrusage`, `dlfcn`) |
| `run_sweep.sh` | Supported | Use WSL or run filtered invocations manually |
| `averageTimeGraph/` | Typical `g++` build | Same as any single-file C++17 build |

For reproducible timing, always use **`Release`** builds (`-DCMAKE_BUILD_TYPE=Release`). Do not compare Release numbers against Debug or ASan builds.

---

## Quick reference

```bash
# Configure & build (from repo root)
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j

# Run all benchmarks
./triangulation_bench

# JSON export
./triangulation_bench --benchmark_format=json --benchmark_out=results.json

# One family only
./triangulation_bench --benchmark_filter=Biconnected
```

---

*B.Sc. thesis work — generating all triangulations of biconnected plane graphs. For the full theoretical treatment and older multi-module README, see `oldCodes/README.md` and thesis materials referenced there.*
