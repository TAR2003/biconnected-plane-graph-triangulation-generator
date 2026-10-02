rm -rf build
cmake -S . -B build -DTRI_SRC_DIR=./src
cmake --build build -j"$(nproc)"
