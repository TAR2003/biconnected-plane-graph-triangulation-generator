#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
TEST_CASES_DIR="$ROOT_DIR/test-cases"
INPUT_DIR="$TEST_CASES_DIR/input"
PLANTRI="$TEST_CASES_DIR/plantri"
SIZES=(8 16 32 64 128 256 384 512 640 768 896 1024 1280 1536 1792 2048)

for tool in uv python3; do
    command -v "$tool" >/dev/null 2>&1 || {
        echo "$tool is required to generate test cases." >&2
        exit 1
    }
done

echo "[-] Recreating test-case input directories..."
rm -rf "$INPUT_DIR"
mkdir -p "$INPUT_DIR/Biconnected" "$INPUT_DIR/Oneconnected"

if [[ -d "$ROOT_DIR/dev/input/Biconnected/9_general_biconnected" ]]; then
    cp -R "$ROOT_DIR/dev/input/Biconnected/9_general_biconnected" "$INPUT_DIR/Biconnected/"
fi
if [[ -d "$ROOT_DIR/dev/input/Oneconnected" ]]; then
    cp -R "$ROOT_DIR/dev/input/Oneconnected/." "$INPUT_DIR/Oneconnected/"
fi

cd "$TEST_CASES_DIR"

echo "[-] Running the configured biconnected input generator..."
uv run python main.py

echo "[-] Generating random biconnected and one-connected inputs..."
uv run python gen_biconnected.py --out input/Biconnected \
    --sizes "${SIZES[@]}" --count 3 --seed 1
uv run python gen_oneconnected.py --out input/Oneconnected \
    --sizes "${SIZES[@]}" --count 3 --seed 1

if [[ ! -x "$PLANTRI" ]]; then
    missing_tools=()
    for tool in gcc wget tar; do
        if ! command -v "$tool" >/dev/null 2>&1; then
            missing_tools+=("$tool")
        fi
    done
    if (( ${#missing_tools[@]} > 0 )); then
        if command -v apt-get >/dev/null 2>&1 && command -v sudo >/dev/null 2>&1; then
            echo "[-] Installing Plantri build prerequisites with apt..."
            sudo apt-get update
            sudo apt-get install -y build-essential g++ make wget
        else
            printf 'Missing Plantri build prerequisite(s): %s\n' "${missing_tools[*]}" >&2
            echo "Install the missing tools and rerun this module." >&2
            exit 1
        fi
    fi

    for tool in gcc wget tar; do
        command -v "$tool" >/dev/null 2>&1 || {
            echo "$tool is required to build Plantri." >&2
            exit 1
        }
    done

    plantri_tmp="$(mktemp -d "$TEST_CASES_DIR/.plantri-build.XXXXXX")"
    trap 'rm -rf "$plantri_tmp"' EXIT

    echo "[-] Downloading and compiling Plantri..."
    wget -O "$plantri_tmp/plantri52.tar.gz" \
        https://users.cecs.anu.edu.au/~bdm/plantri/plantri52.tar.gz
    tar -xzf "$plantri_tmp/plantri52.tar.gz" -C "$plantri_tmp"
    plantri_source="$(find "$plantri_tmp" -type f -name plantri.c -print -quit)"
    if [[ -z "$plantri_source" ]]; then
        echo "Could not find plantri.c in the downloaded archive." >&2
        exit 1
    fi
    gcc -O3 -o "$PLANTRI" "$plantri_source"
    rm -rf "$plantri_tmp"
    trap - EXIT
fi

echo "[-] Importing Plantri benchmark datasets..."
uv run python plantri_import.py --plantri "$PLANTRI" --kind biconnected \
    --nmin 4 --nmax 7 --out input/Biconnected/Biconnected_plantri --seed 42
uv run python plantri_import.py --plantri "$PLANTRI" --kind oneconnected \
    --nmin 3 --nmax 7 --out input/Oneconnected/Oneconnected_plantri --seed 42

# if [[ -f "$TEST_CASES_DIR/visualizer.py" ]]; then
#     echo "[-] Rendering test-case visualizations..."
#     uv run python visualizer.py --input input --output output-images
# fi

echo "[+] Test-case generation and visualization complete."
