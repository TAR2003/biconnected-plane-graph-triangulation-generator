cmake_minimum_required(VERSION 3.16)
project(TriangulationTools CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(TRIANGULATION_NATIVE_ARCH "Enable -march=native for machine-specific benchmark binaries" OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
if(TRIANGULATION_NATIVE_ARCH AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-march=native)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

# ---------------------------------------------------------------------
# 1. correctness_check -- validate fast algorithm vs brute-force truth
# ---------------------------------------------------------------------
add_executable(correctness_check src/correctness_check.cpp)

# ---------------------------------------------------------------------
# 2. triangulate -- write all triangulations of one input to a text file
# ---------------------------------------------------------------------
add_executable(triangulate src/triangulate.cpp)

# ---------------------------------------------------------------------
# 3. dataset_bench -- Google-Benchmark-based timing/memory benchmark
#    over your own input files, with CSV output and timeout support.
#    Requires Google Benchmark -- fetched automatically.
# ---------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.9.1
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)

add_executable(dataset_bench src/dataset_bench_main.cpp src/memory_tracker.cpp)
if(WIN32)
  target_link_libraries(dataset_bench PRIVATE benchmark::benchmark)
else()
  target_link_libraries(dataset_bench PRIVATE benchmark::benchmark pthread dl)
endif()
