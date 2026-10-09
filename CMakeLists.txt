cmake_minimum_required(VERSION 3.16)
project(TriangulationBench CXX)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard (raise if your algorithm headers need it)")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# ---- where the triangulation algorithm headers live
set(TRI_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src" CACHE PATH "Directory containing the triangulation algorithm headers")
if(NOT TRI_SRC_DIR)
  set(TRI_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
elseif(NOT IS_ABSOLUTE "${TRI_SRC_DIR}")
  get_filename_component(TRI_SRC_DIR "${TRI_SRC_DIR}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
set(TRI_SRC_DIR "${TRI_SRC_DIR}" CACHE PATH "Directory containing the triangulation algorithm headers" FORCE)
if(NOT EXISTS "${TRI_SRC_DIR}/GenerateTriangulations.hpp" OR
   NOT EXISTS "${TRI_SRC_DIR}/GenerateBiconnectedTriangulations.hpp" OR
   NOT EXISTS "${TRI_SRC_DIR}/GenerateOneconnectedTriangulations.hpp")
  message(FATAL_ERROR "TRI_SRC_DIR must contain the current triangulation generator headers")
endif()

# ---- speed options
option(TRI_NATIVE "Tune for this CPU (-march=native); binary is NOT portable" ON)
option(TRI_LTO    "Link-time optimisation" ON)
option(TRI_STATIC "Link statically (-static), removes PLT/dynamic-loader overhead" OFF)
set(TRI_PGO "OFF" CACHE STRING "Profile-guided optimisation: OFF | GEN | USE")
set(TRI_ALLOCATOR "system" CACHE STRING "Allocator: system | jemalloc | tcmalloc | mimalloc (must be installed)")

# ---- Google Benchmark (built Release, no tests, LTO)
include(FetchContent)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_LTO ${TRI_LTO} CACHE BOOL "" FORCE)
FetchContent_Declare(benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.9.1
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(benchmark)

# ---- compile / link flags
set(TRI_COPTS "")
set(TRI_LOPTS "")
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  list(APPEND TRI_COPTS -O3 -DNDEBUG -fomit-frame-pointer -fno-plt)
  if(TRI_NATIVE)
    list(APPEND TRI_COPTS -march=native -mtune=native)
  endif()
  if(TRI_PGO STREQUAL "GEN")
    list(APPEND TRI_COPTS -fprofile-generate)
    list(APPEND TRI_LOPTS -fprofile-generate)
  elseif(TRI_PGO STREQUAL "USE")
    list(APPEND TRI_COPTS -fprofile-use -fprofile-correction -Wno-missing-profile)
    list(APPEND TRI_LOPTS -fprofile-use)
  endif()
  if(TRI_STATIC)
    list(APPEND TRI_LOPTS -static)
  endif()
elseif(MSVC)
  list(APPEND TRI_COPTS /O2 /Ob3 /DNDEBUG)
endif()

set(TRI_ALLOC_LIB "")
if(NOT TRI_ALLOCATOR STREQUAL "system")
  if(TRI_ALLOCATOR STREQUAL "tcmalloc")
    set(_alloc_name tcmalloc_minimal)
  else()
    set(_alloc_name ${TRI_ALLOCATOR})
  endif()
  find_library(TRI_ALLOC_LIB NAMES ${_alloc_name} REQUIRED)
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT TRI_IPO_OK OUTPUT TRI_IPO_MSG)

function(tri_target tgt)
  target_include_directories(${tgt} PRIVATE ${TRI_SRC_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/bench)
  target_compile_options(${tgt} PRIVATE ${TRI_COPTS})
  target_link_options(${tgt} PRIVATE ${TRI_LOPTS})
  if(TRI_ALLOC_LIB)
    target_link_libraries(${tgt} PRIVATE ${TRI_ALLOC_LIB})
  endif()
  if(TRI_LTO AND TRI_IPO_OK)
    set_property(TARGET ${tgt} PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)
  endif()
endfunction()

add_library(triangulation_adapters STATIC
  bench/AlgorithmAdaptersBiconnected.cpp
  bench/AlgorithmAdapterOneconnected.cpp)
target_include_directories(triangulation_adapters PRIVATE ${TRI_SRC_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/bench)
target_compile_options(triangulation_adapters PRIVATE ${TRI_COPTS})
if(TRI_LTO AND TRI_IPO_OK)
  set_property(TARGET triangulation_adapters PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)
endif()

add_executable(bench_time bench/bench_time.cpp)
target_link_libraries(bench_time PRIVATE benchmark::benchmark triangulation_adapters)
tri_target(bench_time)

add_executable(bench_memory bench/bench_memory.cpp)
target_link_libraries(bench_memory PRIVATE triangulation_adapters)
tri_target(bench_memory)
if(WIN32)
  target_link_libraries(bench_memory PRIVATE psapi)
endif()
