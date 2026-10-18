#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BENCHMARK="$ROOT_DIR/build/bench_time"

if [[ ! -x "$BENCHMARK" ]]; then
    echo "Benchmark executable not found at $BENCHMARK. Run scripts/1_build_cpp.sh first." >&2
    exit 1
fi
if [[ ! -d "$ROOT_DIR/test-cases/input" ]]; then
    echo "Test-case input directory not found. Run scripts/2_generate_test_cases.sh first." >&2
    exit 1
fi

mkdir -p "$ROOT_DIR/benchmark-results"

echo "[-] Running time and state benchmarks..."
"$BENCHMARK" \
    --tri_algos=BiconnectedWithoutVGS,BiconnectedWithVGS \
    --tri_input="$ROOT_DIR/test-cases/input/Biconnected" \
    --tri_cpu=2 \
    --tri_csv="$ROOT_DIR/benchmark-results/results_time.csv" \
    --tri_limits=10000000

"$BENCHMARK" \
    --tri_algos=Oneconnected \
    --tri_input="$ROOT_DIR/test-cases/input" \
    --tri_cpu=2 \
    --tri_csv="$ROOT_DIR/benchmark-results/results_time.csv" \
    --tri_limits=10000000



BENCHMARK="$ROOT_DIR/build/bench_memory"

echo "[-] Running time and state benchmarks..."
"$BENCHMARK" \
    --tri_algos=BiconnectedWithoutVGS,BiconnectedWithVGS \
    --tri_input="$ROOT_DIR/test-cases/input/Biconnected" \
    --tri_cpu=2 \
    --tri_csv="$ROOT_DIR/benchmark-results-memory/results_time.csv" \
    --tri_limits=10000000

"$BENCHMARK" \
    --tri_algos=Oneconnected \
    --tri_input="$ROOT_DIR/test-cases/input" \
    --tri_cpu=2 \
    --tri_csv="$ROOT_DIR/benchmark-results-memory/results_time.csv" \
    --tri_limits=10000000


    

echo "[+] Benchmarking complete. Results saved under benchmark-results/."
