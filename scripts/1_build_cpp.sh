#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

command -v cmake >/dev/null 2>&1 || {
    echo "cmake is required to build this project." >&2
    exit 1
}

if command -v nproc >/dev/null 2>&1; then
    BUILD_JOBS="$(nproc)"
elif command -v getconf >/dev/null 2>&1; then
    BUILD_JOBS="$(getconf _NPROCESSORS_ONLN)"
else
    BUILD_JOBS=1
fi

echo "[-] Cleaning the previous CMake build..."
rm -rf "$ROOT_DIR/build"

echo "[-] Configuring CMake..."
cmake -S "$ROOT_DIR" -B "$ROOT_DIR/build" -DTRI_SRC_DIR="$ROOT_DIR/src"

echo "[-] Building with $BUILD_JOBS job(s)..."
cmake --build "$ROOT_DIR/build" --parallel "$BUILD_JOBS"

echo "[+] C++ build complete."
