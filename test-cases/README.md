# Plane-graph input generators for the triangulation benchmark

Generators for the input files of the Google Benchmark harness (`bench_total.cpp`), which times
`GraphTriangulationBiconnectedPerformance`, `GraphTriangulationBiconnectedWithoutVGSPerformance`,
and `GraphTriangulationOneconnectedPerformance`.

| File | Purpose |
|---|---|
| `planar_common.py` | Rotation-system helpers, **validation**, file writer, CLI driver |
| `gen_biconnected.py` | 15 families of 2-connected plane graphs |
| `gen_oneconnected.py` | 19 families of connected plane graphs with at least one cut vertex |
| `plantri_import.py` | Exhaustive small-n inputs through `plantri` (external program, compile separately) |

---

## 1. Install and run

```bash
pip install networkx scipy numpy          # scipy is only needed for the Delaunay families
<out>/<Kind>_<family>/<family>_n<N>_<i>.txt
```


python gen_biconnected.py  --out inputs/biconnected  --sizes 3 5 10 50 100 500 1000 5000 --count 20 --seed 1
python gen_oneconnected.py --out inputs/oneconnected --sizes 3 5 10 50 100 500 1000 5000 --count 20 --seed 1
python gen_biconnected.py --list          # family names
```

| Option | Default | Meaning |
|---|---|---|
| `--out` | `inputs/<kind>` | Output root |
| `--sizes` | 3 4 5 6 7 8 10 12 16 20 30 50 100 200 500 1000 | Vertex counts (any list) |
| `--count` | 10 | Graphs per (family, size) |
| `--seed` | 12345 | Global seed. Each graph uses `Random("<seed>/<family>/<n>/<i>")`, so a file can be regenerated in isolation |
| `--families` | all | Subset of families |
| `--orientation` | `mixed` | `cw`, `ccw`, or `mixed` (random per **file**, never within one file) |
| `--no-relabel` | off | Keep generator labels. By default labels are shuffled so vertex numbering carries no structure |
| `--base` | 0 | First vertex label, 0 or 1. **Must match what `readAdjacency` expects** |
| `--retries` | 25 | Regeneration attempts if a candidate fails validation |

Total graphs = (families that support the size) x (number of sizes) x `--count`.
Sizes a family cannot produce (e.g. prism needs even n >= 6) are skipped and printed.

### Configure sizes and count in code

The `GENERATION_CONFIG` dictionary near the bottom of
[`gen_biconnected.py`](gen_biconnected.py) and
[`gen_oneconnected.py`](gen_oneconnected.py) provides the same settings as
the corresponding command-line options. Edit `sizes` and `count` there to
change the default run without adding command-line arguments:

```python
"sizes": [10, 100, 1000],
"count": 20,
```

The command-line interface is unchanged; for example, `--sizes 5 50 --count 3`
overrides those code defaults for that invocation. Output path, seed, and
families can also be configured in the same dictionary.

### Output format

```
N
d_0 a a a ...      # d_i = degree of vertex i, then its neighbours in cyclic order
...
```

Every file uses one orientation throughout. `--orientation ccw` is produced by reversing
every neighbour list of an already validated clockwise graph, so within-file consistency
comes from construction.

### Output layout

```
<out>/<Kind>_<family>/n<N>/<family>_n<N>_<i>.txt
<out>/manifest.csv     # file, family, n, m, faces, minDeg, maxDeg, orientation, seed
```

For example, biconnected wheel cases are stored under
`input/Biconnected/Biconnected_wheel/`, while one-connected tree-path cases are stored under
`input/Oneconnected/Oneconnected_tree_path/`. This matches the `<InputFamily>__<sub>` folder
ids in your harness (`c.folderId`), so each family folder becomes one CSV in
`<output>/<Algo>/total/`. Adjust the layout if your `registerCases` expects a different folder depth.

---

## 2. How it works

### Representation
Every graph is a **rotation system**: `rot[v]` is the list of v's neighbours in clockwise
order. This is exactly the input format, and it fully determines a plane embedding.

### Where embeddings come from
1. **networkx `check_planarity`** (LR planarity test) returns an embedding for the abstract graph.
   Used for wheels, prisms, grids, cylinders, outerplanar, series-parallel, stacked triangulations.
2. **Geometry**: for Delaunay graphs the neighbours are sorted by angle around each vertex.
   This is an exact embedding of a straight-line drawing, and is the only source of embeddings
   that are not chosen by networkx's algorithm.
3. **Restriction**: deleting edges from an embedded graph keeps a valid embedding
   (sparsified families reuse the parent's rotation system).
4. **Duals**: face tracing gives the dual rotation system (cubic graphs from triangulations).
5. **Block gluing** (one-connected): to attach block B at vertex u to vertex v of the graph G, u is identified with v, and
   B's rotation at u is inserted into v's rotation at a random position, after a random cyclic
   shift. The shift chooses which face of B contains the rest of G, and the position chooses
   the corner of G where B sits. The result is a valid plane embedding, and different random
   choices produce different embeddings of the same abstract graph.

### Validation (`planar_common.validate`)
Applied to every graph **before** it is written; failures are regenerated:
- labels are exactly `0..n-1`, no loops, no multi-edges, symmetric adjacency;
- connected;
- **Euler check**: faces are traced from the rotation system and `V - E + F = 2` must hold.
  For a connected graph this holds exactly when the rotation system is a planar embedding
  (genus 0), so a wrong neighbour order is caught, not just a non-planar graph;
- biconnected files: `networkx.is_biconnected`; one-connected files: n >= 3 and **not** biconnected
  (guarantees at least one cut vertex).

Not covered by the validator: generation-time bugs that yield valid graphs of an unintended
kind, and how well the families represent a given class (see Limitations).

---

## 3. Families

### Biconnected (`gen_biconnected.py`)

| Family | Structure | Stresses |
|---|---|---|
| `cycle` | n-cycle | 2 faces, minimum edges |
| `wheel` | hub + rim | one degree n-1 vertex |
| `bipyramid` | cycle + 2 apexes | two huge-degree vertices |
| `prism` | circular ladder (even n) | cubic, 3-connected |
| `grid` | r x c grid, random aspect | quad faces; ladders to squares |
| `cylinder` | nested cycles + spokes + random diagonals | nesting depth, mixed face sizes |
| `outerplanar` | polygon + random non-crossing chords | one huge outer face |
| `series_parallel` | subdivisions + parallel ears | many degree-2 vertices |
| `stacked_triangulation` | Apollonian nets, balanced to path-like | maximal planar, all-triangle faces |
| `stacked_dual_cubic` | dual of the above | cubic 3-connected, many faces |
| `delaunay_uniform` / `_clustered` / `_lattice` | Delaunay triangulations | realistic geometry, near-degenerate faces |
| `sparse_delaunay` / `sparse_stacked` | triangulation with random edges removed while biconnected | large and varied face sizes, random density |

### One-connected (`gen_oneconnected.py`)

| Family | Structure |
|---|---|
| `tree_path`, `tree_star`, `tree_binary`, `tree_random`, `tree_deep_random`, `tree_caterpillar`, `tree_spider` | Trees with random rotations (max degree 2 up to n-1) |
| `cactus_random`, `cactus_chain`, `cactus_star` | Cycles/bridges glued at vertices |
| `blocks_random`, `blocks_chain`, `blocks_star`, `blocks_pref` | Random biconnected blocks (any family above) or bridges, glued by random / chain / single-hub / degree-preferential attachment |
| `blocks_bridges` | Block trees where about half the blocks are bridges |
| `core_pendants` | Large biconnected core plus pendant edges |
| `comet` | ~90% of vertices in one stacked triangulation, then a long tail of bridges/triangles |
| `hub_asymmetric` | One cut vertex shared by one block of ~n/2 vertices and O(n) tiny triangles/bridges |

Block sizes, bridge probability and attachment position are re-drawn for every graph.

---

## 4. Small-n exhaustive inputs (`plantri_import.py`)

Random sampling can miss rare small topologies. `plantri` (Brinkmann and McKay) enumerates
planar graphs exhaustively and is compiled from its own source.

```bash
python plantri_import.py --plantri ./plantri --kind biconnected  --nmin 4 --nmax 10 --out inputs/plantri_bicon
python plantri_import.py --plantri ./plantri --kind oneconnected --nmin 3 --nmax 9  --out inputs/plantri_onecon
```

It runs `plantri -pc2m2a N` (biconnected) or `-pc1m1a N` (connected) and parses the ASCII
lines (`<n> bcd,adc,...`, each list a vertex's neighbours in rotation order). Every graph passes
through `validate()`; for `oneconnected`, output graphs that turn out to be biconnected are rejected.

**Status:** parsing and validation were tested on a hand-written K4 line. `plantri` itself was
**not** available in the development environment. Run it for small n and check the reported
counts against the plantri guide. If the flags differ in your build, only the `flags` line changes.
Growth is fast: keep n small (about <= 10-12) or use `--max-files`.

---

## 5. Notes on the external review

An outside review of the suite called it "publication-ready", asked for a few additions, and gave
a benchmark checklist. Where each point ended up:

| Point | Status |
|---|---|
| Comet / lollipop block trees | Added as `comet` |
| One cut vertex joining one huge block and O(n) tiny triangles | Added as `hub_asymmetric` |
| plantri bridge for small n | Added as `plantri_import.py`. The review's suggested input format (`v[u1 u2 ...]`) does not match plantri's ASCII output, so the parser follows the format in section 4 |
| "No extra libraries needed" | Agreed. `subprocess`, `argparse`, `csv` are standard library |
| Verify `readAdjacency` is 0- or 1-indexed | **Your action.** Use `--base` |
| Set an output limit for huge instances | `bench_total.cpp` already defaults `kDefaultLimit` to 10,000,000. Override via whatever flag `parseArgs` exposes (not shown in the file) |
| Orientation consistent within a file | True by construction (see above) |

Claims in that review that this README does **not** adopt:
- "Covers over 95% of topological stress states" has no measurable basis. There is no agreed
  taxonomy of plane graphs, so no coverage percentage can be computed.
- Statements about what specific journals' reviewers expect, and which internal data structures
  (e.g. hash collisions at cut vertices) each family stresses, are speculation. Treat the family
  table above as "structural diversity", not proof of algorithmic worst cases.
- The Euler check certifies a valid plane embedding. It does not certify that the sample is representative.

---

## 6. Limitations and advice

- **Coverage is diversity, not exhaustiveness.** Only `plantri_import.py` is exhaustive, and only for small n.
  Random families are biased in ways typical of their construction (e.g. Delaunay graphs have
  degree about 6 on average).
- **Not covered:** high-connectivity families beyond triangulations and duals, planar graphs with
  very specific degree sequences, and adversarial inputs tailored to your algorithm's
  data structures. If you know its worst cases, add a family in a few lines (`fn(n, rng) -> rot` or `None`,
  then register it in the dict).
- **Runtime:** sparsification re-checks biconnectivity per edge removal; expect it to be slow
  above a few thousand vertices. Lower `--count` for large n.
- **Output volume:** at n >= 500 the triangulation counts can be huge. Keep the enumeration limit on,
  and record in your paper which runs hit the limit (`triangulations == limit`).
- **Statistics:** the benchmark defaults to 3 runs per file. With `--count` graphs per family, report per-family
  distributions rather than single files. Record the `--seed` and the generator's git commit alongside results.
- **Reproducibility:** networkx/scipy versions can change embeddings or tie-breaking. Pin versions
  (`pip freeze > requirements.txt`) and archive `manifest.csv` with the input set.

## 7. Adding a family

```python
def g_myfamily(n, rng):          # rng is a random.Random
    if n < 5: return None        # None means "size unsupported", skipped
    ...
    return {v: [neighbours in clockwise order]}   # labels 0..n-1, exactly n vertices

BICONNECTED["my_family"] = g_myfamily
```
Validation, relabeling, orientation and manifest handling are automatic.
