#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

if [[ -f "$ROOT_DIR/plot.py" ]]; then
    cd "$ROOT_DIR"
    uv run python plot.py
elif [[ -f "$ROOT_DIR/test-cases/plot.py" ]]; then
    cd "$ROOT_DIR/test-cases"
    uv run python plot.py
else
    echo "Warning: plot.py was not found in the project root or test-cases/." >&2
    exit 1
fi

echo "[+] Performance plots generated."
