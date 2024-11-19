cmake_minimum_required(VERSION 3.10)
project(AEX)

set(AEX_VERSION_MAJOR "1")
set(AEX_VERSION_MINOR "0")
set(AEX_VERSION_PATCH "0")
set(AEX_VERSION_STRING ${AEX_VERSION_MAJOR}.${AEX_VERSION_MINOR}.${AEX_VERSION_PATCH})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-Wall -march=native -fopenmp -ffast-math -fmax-errors=3 -mavx -mavx2 -O0 -g -DDEBUG -lpthread")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -march=native -fopenmp -ffast-math -fmax-errors=3 -mavx -mavx2 -O3 -lpthread")

#set(CMAKE_CXX_FLAGS "-g -Wall -march=native -fopenmp -fmax-errors=3 -std=c++17")
#
#if (CMAKE_BUILD_TYPE STREQUAL "Debug")
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
#elseif(CMKAE_BUILD_TYPE STREQUAL "Release")
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
#endif()

#message(STATUS "BUILD TYPE:" ${CMAKE_BUILD_TYPE})
#message(STATUS "FLAGS:" ${CMAKE_CXX_FLAGS})

include_directories(${CMAKE_SOURCE_DIR})

#add_executable(example ${CMAKE_SOURCE_DIR}/examples/aex.cpp)
#add_executable(example_con ${CMAKE_SOURCE_DIR}/examples/aex_con.cpp)

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

# LIPP
set(LIPP_DIR ${CMAKE_SOURCE_DIR}/thirdparty/lipp)
include_directories(${LIPP_DIR}/src/core)

# benchmark
set(BENCHMARK_DIR ${CMAKE_SOURCE_DIR}/benchmark)
file(GLOB_RECURSE BENCHMARK_SRC ${CMAKE_SOURCE_DIR}/benchmark/benchmark_*.cpp ${CMAKE_SOURCE_DIR}/benchmark/benchmark.cpp)
find_library(JemallocLib jemalloc)
add_executable(benchmark ${BENCHMARK_SRC})
target_link_libraries(benchmark PRIVATE ${JemallocLib})

# generate dataset
add_executable(generate_dataset ${BENCHMARK_DIR}/generate_dataset.cpp)

# test
set(TEST_DIR ${CMAKE_SOURCE_DIR}/test)
# file(GLOB_RECURSE TEST_SRC ${TEST_DIR}/*.cpp)
add_executable(unit_test ${TEST_DIR}/test.cpp)
