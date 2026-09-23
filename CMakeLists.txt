cmake_minimum_required(VERSION 3.16)
project(TriangulationBenchmarks CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Release by default -- benchmarking a Debug build is meaningless.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Fetch Google Benchmark (pulls in googletest too, needed for its own tests --
# we disable GB's internal tests to keep the build fast).
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

add_executable(triangulation_bench
  src/graph_generator.cpp
  src/memory_tracker.cpp
  src/bench_main.cpp
)

add_executable(triangulation_dataset_bench
  src/memory_tracker.cpp
  src/dataset_bench_main.cpp
)

target_include_directories(triangulation_bench PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/include   # <- put your Edge.hpp, GraphTriangulation.hpp etc. here
)

target_link_libraries(triangulation_bench PRIVATE benchmark::benchmark pthread)
target_include_directories(triangulation_dataset_bench PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(triangulation_dataset_bench PRIVATE benchmark::benchmark pthread)

# Sanitizer builds for correctness-under-load sanity checks, not for timing.
option(WITH_ASAN "Build with AddressSanitizer" OFF)
if(WITH_ASAN)
  target_compile_options(triangulation_bench PRIVATE -fsanitize=address,undefined -g)
  target_link_options(triangulation_bench PRIVATE -fsanitize=address,undefined)
  target_compile_options(triangulation_dataset_bench PRIVATE -fsanitize=address,undefined -g)
  target_link_options(triangulation_dataset_bench PRIVATE -fsanitize=address,undefined)
endif()
