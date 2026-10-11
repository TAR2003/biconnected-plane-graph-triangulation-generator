#!/usr/bin/env bash
set -e  # Exit immediately if any command fails

echo "=== 1. Installing uv (if not already installed) ==="
if ! command -v uv &> /dev/null; then
    curl -LsSf https://astral.sh/uv/install.sh | sh
    # Add uv to the current shell session path
    export PATH="$HOME/.local/bin:$PATH"
fi

echo "=== 2. Setting up Python virtual environment and installing dependencies via uv ==="
# uv venv automatically creates a .venv if it doesn't exist
uv venv .venv --python 3.10  # or python 3.11/3.12 depending on your target
uv pip install networkx numpy scipy matplotlib Pillow reportlab

echo "=== 3. Preparing input directories ==="
rm -rf input/
mkdir -p input/Biconnected
mkdir -p input/Oneconnected

# Safe checks before copying from local development folders
if [ -d "../dev/input/Biconnected/9_general_biconnected" ]; then
    cp -r ../dev/input/Biconnected/9_general_biconnected ./input/Biconnected/
fi
if [ -d "../dev/input/Oneconnected" ]; then
    cp -r ../dev/input/Oneconnected/* ./input/Oneconnected/
fi

echo "=== 4. Running main and generation scripts ==="
uv run python main.py

uv run python gen_biconnected.py --out input/Biconnected \
    --sizes 8 16 32 64 128 256 384 512 640 768 896 1024 1280 1536 1792 2048 \
    --count 3 --seed 1

uv run python gen_oneconnected.py --out input/Oneconnected \
    --sizes 8 16 32 64 128 256 384 512 640 768 896 1024 1280 1536 1792 2048 \
    --count 3 --seed 1

echo "=== 5. Compiling Plantri ==="
sudo apt update && sudo apt install -y build-essential g++ make wget
wget https://users.cecs.anu.edu.au/~bdm/plantri/plantri52.tar.gz
tar -xzf plantri52.tar.gz
cd plantri52
gcc -O3 -o plantri plantri.c
mv plantri ..
cd ..
rm -rf plantri52 plantri52.tar.gz

echo "=== 6. Importing Plantri datasets ==="
uv run python plantri_import.py --plantri ./plantri --kind biconnected --nmin 4 --nmax 9 --out input/Biconnected/Biconnected_plantri --seed 42
uv run python plantri_import.py --plantri ./plantri --kind oneconnected --nmin 3 --nmax 8 --out input/Oneconnected/Oneconnected_plantri --seed 42

echo "=== Setup and execution pipeline completed successfully! ==="