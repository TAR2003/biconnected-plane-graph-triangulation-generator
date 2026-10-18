# Biconnected plane graph triangulation generator

Header-only C++17 algorithms that enumerate triangulations of a **plane graph** given as a **rotation system** (cyclic neighbor order at each vertex). The repository ships a Google Benchmark harness, Python input generators, correctness utilities under `dev/`, and plotting for benchmark CSVs.

Three enumeration backends live in `src/` and are wired through `bench/` adapters:

| Algorithm | Class | Typical input |
| --- | --- | --- |
| **BiconnectedWithVGS** | `GenerateBiconnectedTriangulationsWithVGS` | 2-connected plane graphs |
| **BiconnectedWithoutVGS** | `GenerateBiconnectedTriangulationsWithoutVGS` | same biconnected inputs |
| **Oneconnected** | `GenerateOneconnectedTriangulations` | connected graphs with cut vertices |

`BiconnectedWithoutVGS` and **Oneconnected** record **check** counters during search; **Oneconnected** also records **traversal** counters. **BiconnectedWithVGS** does not expose those metrics in the benchmark CSV (fields are left empty).

---

## Quick start (full pipeline)

On Linux or WSL, from the repository root:

```bash
chmod +x run_all.sh scripts/*.sh
./run_all.sh
```

`run_all.sh` runs the end-to-end workflow:

1. Creates a Python 3.10 virtual environment with [uv](https://github.com/astral-sh/uv) and installs `networkx`, `numpy`, `scipy`, `matplotlib`, `pandas`, `Pillow`, and `reportlab`.
2. **`scripts/1_build_cpp.sh`** — clean CMake configure/build of `bench_time` and `bench_memory`.
3. **`scripts/2_generate_test_cases.sh`** — rebuilds `test-cases/input/` (generators, optional Plantri import, dev seed cases).
4. **`scripts/3_run_benchmarks.sh`** — runs `bench_time` on all three algorithms. The biconnected algorithms use only `test-cases/input/Biconnected/`; **Oneconnected** uses inputs from both `Biconnected/` and `Oneconnected/`.
5. **`scripts/4_generate_plots.sh`** — runs root **`plot.py`** and writes PNGs under `graph-biconnected-out/`.

Individual stages can be run by executing the corresponding script under `scripts/` after dependencies are installed.

---

## Repository layout

| Path | Role |
| --- | --- |
| `src/` | Core headers: `Common.hpp`, `GenerateTriangulations.hpp`, biconnected and one-connected generators |
| `bench/` | `bench_time.cpp`, `bench_memory.cpp`, `BenchCommon.hpp`, algorithm adapters |
| `test-cases/` | Input generators (`gen_biconnected.py`, `gen_oneconnected.py`, …) — see [test-cases/README.md](test-cases/README.md) |
| `dev/` | Correctness checks, legacy benchmarks, notebooks, visualizers (not part of the default CMake targets) |
| `plot.py` | Comparison plots from `benchmark-results/` |
| `run_all.sh` | Orchestrates build → inputs → benchmark → plots |
| `scripts/` | Shell modules invoked by `run_all.sh` |
| `benchmark-results/` | CSV output (created by benchmarks; may be empty in a fresh clone) |
| `graph-biconnected-out/` | PNG plots (created by `plot.py`) |
| `build/` | CMake build tree (created by `scripts/1_build_cpp.sh`) |

---

## Requirements

- **CMake** ≥ 3.16, **Git** (Google Benchmark is fetched with `FetchContent`)
- **C++17** compiler (GCC/Clang; MSVC with a suitable developer environment on Windows)
- **bash**, **curl**, and **uv** (or let `run_all.sh` install uv)
- **Python 3.10+** with packages listed above for generators and plotting
- Optional: **plantri** (built automatically by `2_generate_test_cases.sh` when possible) for exhaustive small instances

---

## Build (manual)

```bash
cmake -S . -B build -DTRI_SRC_DIR=./src
cmake --build build --parallel
```

Executables:

- `build/bench_time` — CPU/real time per full enumeration (primary benchmark)
- `build/bench_memory` — peak RSS during enumeration

CMake options include `TRI_NATIVE`, `TRI_LTO`, `TRI_PGO`, and `TRI_ALLOCATOR` (see `CMakeLists.txt`).

---

## Input format

Each benchmark file is a rotation system:

```text
N
degree_0 neighbor_0 neighbor_1 ...
degree_1 neighbor_0 neighbor_1 ...
...
```

`N` is the vertex count. Line `i+1` lists the degree of vertex `i` and its neighbors in **cyclic order** around that vertex. Labels are typically `0 … N-1` and must match what the generators emit.

On disk, inputs are grouped as:

```text
test-cases/input/Biconnected/<category>/…/*.txt
test-cases/input/Oneconnected/<category>/…/*.txt
```

Both biconnected algorithms read only from `Biconnected/`. **Oneconnected** reads from both `Biconnected/` and `Oneconnected/`, since biconnected graphs are also valid connected inputs.

---

## Benchmark CLI

Shared flags (see `bench/BenchCommon.hpp`):

| Flag | Meaning |
| --- | --- |
| `--tri_input=DIR` | Root input directory (default `input`; pipeline uses `test-cases/input`) |
| `--tri_algos=A,B,C` | `BiconnectedWithoutVGS`, `BiconnectedWithVGS`, `Oneconnected` |
| `--tri_limits=…` | Triangulation caps tried outermost (resume skips completed `(algo, category, file, limit)` rows) |
| `--tri_categories=…` | Optional filter |
| `--tri_csv=FILE` | Base CSV path; per-algorithm/per-category files are derived automatically |
| `--tri_cpu=N` | Pin benchmark thread to CPU `N` when supported |

Example:

```bash
./build/bench_time \
  --tri_algos=BiconnectedWithoutVGS,BiconnectedWithVGS,Oneconnected \
  --tri_input=./test-cases/input \
  --tri_csv=./benchmark-results/results_time.csv \
  --tri_limits=10000000
```

### CSV layout

Rows are appended under:

```text
benchmark-results/<AlgorithmName>Algo/results_time_<category>.csv
```

Each algorithm gets its own results folder. The biconnected result folders contain CSVs only for biconnected input categories; the Oneconnected folder contains CSVs for categories from both input trees. If a category name exists in both trees, the Oneconnected CSV category is prefixed with its input folder name to keep results separate.

Timing CSV columns include:

`algorithm`, `category`, `filename`, `limit`, `triangulationLimit`, `vertices`, `cpuTimeSeconds`, `realTimeSeconds`, `triangulations`, `status`, `totalChecks`, `successfulChecks`, `unsuccessfulChecks`, `checkSuccessRate`, `totalTraversals`, `successfulTraversals`, `unsuccessfulTraversals`, `traversalSuccessRate`, `timestamp`.

Check and traversal fields are populated for **BiconnectedWithoutVGS** and **Oneconnected** only. A run that hits the triangulation limit is marked accordingly and represents a **truncated** enumeration, not a full count.

Default limits and paths are also editable in the `cfg` block at the top of `main()` in `bench/bench_time.cpp` and `bench/bench_memory.cpp`; CLI flags override those defaults.

---

## Plotting

After benchmarks finish:

```bash
python plot.py
```

(or `uv run python plot.py` as in `scripts/4_generate_plots.sh`)

**Inputs:** `benchmark-results/BiconnectedWithoutVGSAlgo/`, `BiconnectedWithVGSAlgo/`, and `OneconnectedAlgo/` (`results_time_*.csv`).

**Outputs:**

- `graph-biconnected-out/<category>/` — per-category charts (plots `1`–`5b` compare all three algorithms; optional `6`–`7` for metric views on one-connected categories)
- `graph-biconnected-out/_combined/` — all categories overlaid
- `graph-biconnected-out/_metrics/` — **additional** charts (existing `1`–`5b` filenames unchanged):
  - check success rate vs average time per triangulation (**Without VGS** and **Oneconnected**)
  - check success rate vs time ratio to **With VGS** (matched biconnected cases; one-connected where the same `(category, file)` exists in both trees)
  - **Oneconnected** traversal success rate vs average time per triangulation

Algorithm colours in plots: **Without VGS** blue, **With VGS** orange, **Oneconnected** green.

Configure `TIME_COL`, log axes, and limit filtering at the top of `plot.py`.

---

## Algorithms (high level)

All variants extend `GenerateTriangulations`, which builds faces from the rotation system and searches chord insertions subject to a triangulation limit.

- **With VGS** uses virtual good-substructure (VGS) bookkeeping for faster pruning on biconnected instances.
- **Without VGS** uses explicit validity checks (`isValidChildTriangulation`); the benchmark records how often those checks succeed.
- **Oneconnected** handles graphs with cut vertices; it records both check and internal traversal outcomes.

Implementation details and shared chord/face structures are in `src/GenerateTriangulations.hpp` and the specialized headers.

---

## Correctness and development

Programs under `dev/` (e.g. `correctnessCheckBiconnected.cpp`, `correctnessCheckOneconnected.cpp`) compare enumerator output against reference constructions. They are **not** built by default; compile manually with `-I src` (and `-I dev` where needed).

When diffing triangulations, canonicalize undirected edges as `(min(u,v), max(u,v))`, sort edges within each triangulation, and sort the list of triangulations so order-only differences do not fail the comparison.

---

## Test-case generators

From `test-cases/`:

```bash
python gen_biconnected.py --out input/Biconnected --seed 42
python gen_oneconnected.py --out input/Oneconnected --seed 42
```

Family lists, validation rules, and Plantri import are documented in [test-cases/README.md](test-cases/README.md). The automated pipeline uses fixed size grids and counts defined in `scripts/2_generate_test_cases.sh`.

---

## Interpreting results

- **Average time per triangulation** = `cpuTimeSeconds / triangulations` (primary scaling metric in plots).
- **Check success rate** = `100 × successfulChecks / totalChecks` when checks are recorded; higher rates mean fewer pruned branches failed late in the search.
- **Traversal success rate** (**Oneconnected** only) reflects how often internal traversals complete successfully; lower rates often correlate with harder search structure and can track higher per-triangulation cost on the same inputs.
- Timing depends on compiler flags, CPU, and I/O; use the same build and `--tri_cpu` pinning for reproducible comparisons.

---

## License and attribution

See repository history and upstream references in `dev/` and generator scripts. For thesis-related experiments, see `dev/thesis-triangulation-generation.ipynb`.
