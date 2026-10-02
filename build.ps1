# Remove existing build directory if it exists
if (Test-Path build) { 
    Remove-Item -Recurse -Force build 
}

# Configure project with CMake
cmake -S . -B build -DTRI_SRC_DIR=./src

# Build the project using all available CPU threads
cmake --build build --parallel