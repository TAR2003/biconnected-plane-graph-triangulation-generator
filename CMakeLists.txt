cmake_minimum_required(VERSION 3.16)
project(PlaneGraphTriangulationBenchmark LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# --- Google Benchmark (pinned for reproducibility) ---------------------------
include(FetchContent)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.9.1
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(benchmark)

# --- Shared harness (object library so the operator new/delete overrides are
# always linked into the executables) --------------------------------------
add_library(bench_common OBJECT bench/MemoryTracker.cpp)
target_include_directories(bench_common PUBLIC ${CMAKE_SOURCE_DIR}/bench ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(bench_common PUBLIC BENCH_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
target_link_libraries(bench_common PUBLIC benchmark::benchmark)

# --- Executables --------------------------------------------------------------
foreach(target bench_total bench_individual)
  add_executable(${target} bench/${target}.cpp)
  target_link_libraries(${target} PRIVATE bench_common)
endforeach()