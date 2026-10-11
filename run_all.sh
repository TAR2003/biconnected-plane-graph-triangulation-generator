#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
SCRIPTS_DIR="$ROOT_DIR/scripts"

trap 'status=$?; printf "[!] Pipeline failed (exit %s) while running: %s\n" "$status" "$BASH_COMMAND" >&2; exit "$status"' ERR

print_stage() {
    printf '\n==================================================================\n'
    printf '=== %s\n' "$1"
    printf '==================================================================\n'
}

if ! command -v uv >/dev/null 2>&1; then
    command -v curl >/dev/null 2>&1 || {
        echo "curl is required to install uv." >&2
        exit 1
    }
    echo "[-] Installing uv..."
    curl -LsSf https://astral.sh/uv/install.sh | sh
    export PATH="$HOME/.local/bin:$HOME/.cargo/bin:$PATH"
fi

print_stage "SETTING UP THE PYTHON ENVIRONMENT"
uv venv "$ROOT_DIR/.venv" --python 3.10
uv pip install --python "$ROOT_DIR/.venv/bin/python" \
    networkx numpy scipy matplotlib Pillow reportlab pandas

print_stage "MODULE 1: BUILDING THE C++ CORE"
bash "$SCRIPTS_DIR/1_build_cpp.sh"

print_stage "MODULE 2: GENERATING TEST CASES"
bash "$SCRIPTS_DIR/2_generate_test_cases.sh"

print_stage "MODULE 3: RUNNING BENCHMARKS"
bash "$SCRIPTS_DIR/3_run_benchmarks.sh"

print_stage "MODULE 4: GENERATING PLOTS"
bash "$SCRIPTS_DIR/4_generate_plots.sh"

print_stage "ALL PIPELINE STAGES COMPLETED SUCCESSFULLY"
