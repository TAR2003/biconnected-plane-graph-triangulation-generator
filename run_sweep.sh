#!/usr/bin/env bash
# =======================================================================
# Runs each benchmark case as its OWN process invocation, wrapped in the
# coreutils `timeout` command. This is the robust way to sweep over a
# range where some cases might be combinatorially explosive and you
# don't know in advance which ones -- a per-process OS-level timeout
# will SIGKILL a hung case and let the sweep continue, which the
# in-process Watchdog (timeout_guard.hpp) cannot do (it can only abort
# the whole binary, taking down whatever else is running in-process).
#
# Usage:
#   ./run_sweep.sh <benchmark_binary> <per_case_timeout_seconds> <output_dir>
#
# Example:
#   ./run_sweep.sh ./build/triangulation_bench 30 results/
# =======================================================================
set -uo pipefail

BINARY="${1:?Usage: $0 <binary> <timeout_seconds> <output_dir>}"
TIMEOUT_S="${2:?Missing timeout in seconds}"
OUTDIR="${3:?Missing output directory}"

mkdir -p "$OUTDIR"

# List every registered benchmark name without running them.
mapfile -t CASES < <("$BINARY" --benchmark_list_tests)

SUMMARY="$OUTDIR/sweep_summary.csv"
echo "case,exit_code,status,json_file" > "$SUMMARY"

for case_name in "${CASES[@]}"; do
    # Sanitize case name for use as a filename.
    safe_name=$(echo "$case_name" | tr '/:' '__')
    out_json="$OUTDIR/${safe_name}.json"

    echo ">> Running: $case_name (timeout ${TIMEOUT_S}s)"

    timeout --signal=KILL "${TIMEOUT_S}s" \
        "$BINARY" \
        --benchmark_filter="^${case_name}$" \
        --benchmark_format=json \
        --benchmark_out="$out_json" \
        --benchmark_out_format=json \
        > "$OUTDIR/${safe_name}.stdout.log" 2>&1

    exit_code=$?

    if [ "$exit_code" -eq 0 ]; then
        status="OK"
    elif [ "$exit_code" -eq 137 ] || [ "$exit_code" -eq 124 ]; then
        status="TIMEOUT"
        echo "   !! TIMED OUT after ${TIMEOUT_S}s -- see ${safe_name}.stdout.log"
    else
        status="ERROR($exit_code)"
        echo "   !! FAILED with exit code $exit_code -- see ${safe_name}.stdout.log"
    fi

    echo "${case_name},${exit_code},${status},${out_json}" >> "$SUMMARY"
done

echo
echo "Sweep complete. Summary: $SUMMARY"
echo "Cases that timed out or errored will have NO json result file --"
echo "treat them as missing data points, not zero, when you plot this."
