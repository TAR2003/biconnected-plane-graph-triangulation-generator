cmake_minimum_required(VERSION 3.16)
project(biconnected_triangulation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(REPO_INCLUDE "${CMAKE_SOURCE_DIR}/include")
set(BENCHMARK_LIB "${CMAKE_SOURCE_DIR}/lib/benchmark")

function(add_triangulation_exe target source)
    add_executable(${target} ${source})
    target_include_directories(${target} PRIVATE
        "${REPO_INCLUDE}/triangulation"
        "${BENCHMARK_LIB}"
    )
    if(WIN32)
        target_link_libraries(${target} PRIVATE psapi)
    endif()
endfunction()

function(add_one_connected_exe target source)
    add_executable(${target} ${source})
    target_include_directories(${target} PRIVATE
        "${REPO_INCLUDE}/triangulation/one_connected"
        "${BENCHMARK_LIB}"
    )
    if(WIN32)
        target_link_libraries(${target} PRIVATE psapi)
    endif()
endfunction()

function(add_correctness_exe target source)
    add_executable(${target} ${source})
    target_include_directories(${target} PRIVATE
        "${REPO_INCLUDE}/triangulation"
        "${REPO_INCLUDE}/reference"
    )
endfunction()

add_triangulation_exe(generate "${CMAKE_SOURCE_DIR}/main.cpp")
add_correctness_exe(verify "${CMAKE_SOURCE_DIR}/correctness/main.cpp")
add_triangulation_exe(benchmark_aggregate "${CMAKE_SOURCE_DIR}/time_complexity/main.cpp")
add_triangulation_exe(average_time_profiler "${CMAKE_SOURCE_DIR}/averageTimeGraph/main.cpp")

add_one_connected_exe(benchmark_one_connected_biconnected
    "${CMAKE_SOURCE_DIR}/one-connected-code-biconnected-time-complexity/main.cpp")
add_one_connected_exe(benchmark_one_connected
    "${CMAKE_SOURCE_DIR}/one-connected-time-complexity/main.cpp")
add_one_connected_exe(one_connected_cli
    "${CMAKE_SOURCE_DIR}/one-connected/main.cpp")
add_one_connected_exe(one_connected_verify
    "${CMAKE_SOURCE_DIR}/one-connected-code-test-on-biconnected/main.cpp")
