cmake_minimum_required(VERSION 2.8)
project(AEX)

set(AEX_VERSION_MAJOR "1")
set(AEX_VERSION_MINOR "1")
set(AEX_VERSION_PATCH "3")
set(AEX_VERSION_STRING ${AEX_VERSION_MAJOR}.${AEX_VERSION_MINOR}.${AEX_VERSION_PATCH})

#set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "-O3 -Wall -march=native -fopenmp -fmax-errors=3 -std=c++17")
#set(CMAKE_CXX_FLAGS "-g -Wall -march=native -fopenmp -fmax-errors=3 -std=c++17")

include_directories(${CMAKE_SOURCE_DIR})

#add_executable(example ${CMAKE_SOURCE_DIR}/examples/main.cpp)

# add_executable(benchmark_query ${CMAKE_SOURCE_DIR}/benchmark/benchmark_query.cpp)

# stx-btree
set(STX_BTREE_DIR ${CMAKE_SOURCE_DIR}/thirdparty/stx-btree-0.9)
include_directories(${STX_BTREE_DIR}/include)

# ALEX
set(ALEX_DIR ${CMAKE_SOURCE_DIR}/thirdparty/ALEX/src/core)
include_directories(${ALEX_DIR})

# PGM
set(PGM_DIR ${CMAKE_SOURCE_DIR}/thirdparty/PGM-index-master)
include_directories(${PGM_DIR}/include/pgm)

# benchmark
set(BENCHMARK_DIR ${CMAKE_SOURCE_DIR}/benchmark)
file(GLOB_RECURSE BENCHMARK_SRC ${CMAKE_SOURCE_DIR}/benchmark/benchmark_*.cpp ${CMAKE_SOURCE_DIR}/benchmark/benchmark.cpp)
add_executable(benchmark ${BENCHMARK_SRC})

# generate dataset
add_executable(generate_dataset ${BENCHMARK_DIR}/generate_dataset.cpp)

# test
set(TEST_DIR ${CMAKE_SOURCE_DIR}/test)
# file(GLOB_RECURSE TEST_SRC ${TEST_DIR}/*.cpp)
add_executable(unit_test ${TEST_DIR}/test.cpp)
