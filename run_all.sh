#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
SCRIPTS_DIR="$ROOT_DIR/scripts"
PYTHON_ENV_DIR="$ROOT_DIR/.venv-wsl"
export UV_PROJECT_ENVIRONMENT="$PYTHON_ENV_DIR"

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
if [[ ! -x "$PYTHON_ENV_DIR/bin/python" ]]; then
    if [[ -e "$PYTHON_ENV_DIR" ]]; then
        echo "Python environment exists but has no executable at $PYTHON_ENV_DIR/bin/python." >&2
        echo "Move or remove that specific directory, then rerun the pipeline." >&2
        exit 1
    fi
    uv venv "$PYTHON_ENV_DIR" --python 3.10
fi
export VIRTUAL_ENV="$PYTHON_ENV_DIR"
export PATH="$PYTHON_ENV_DIR/bin:$PATH"
uv pip install --python "$PYTHON_ENV_DIR/bin/python" \
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
