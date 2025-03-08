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
set(CMAKE_CXX_FLAGS "-Wall -fopenmp -ffast-math -fmax-errors=3 -march=native -mfma -msse -mavx2 -mavx512bw -mavx512vl -mavx512f -mavx512cd -mavx512dq -lpthread")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

#set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "/home/zzr/lib/lib")

find_package(Boost REQUIRED)
find_path(TBB_INCLUDE_DIR tbb)
find_library(Tbblib          tbb)
# find_library(Tbbbind         tbbbind_2_0)
# find_library(Tbbmalloc       tbbmalloc)
# find_library(Tbbmalloc_proxy tbbmalloc_proxy)

#set(Tbbpack ${Tbblib} ${Tbbbind} ${Tbbmalloc} ${Tbbmalloc_proxy})
#message(${Tbbmalloc})

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
include_directories(${CMAKE_SOURCE_DIR}/aex)
include_directories(${TBB_INCLUDE_DIR})
message(${Boost_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})


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
#target_link_libraries(benchmark PUBLIC ${JemallocLib} ${Tbbpack})

# generate dataset
add_executable(generate_dataset ${BENCHMARK_DIR}/generate_dataset.cpp)
#target_link_libraries(generate_dataset PUBLIC ${JemallocLib} ${Tbbpack})

# test
set(TEST_DIR ${CMAKE_SOURCE_DIR}/test)
add_executable(unit_test ${TEST_DIR}/test.cpp)
#target_link_libraries(unit_test PUBLIC ${JemallocLib} ${Tbbpack})

add_executable(demo examples/example.cpp)
#target_link_libraries(demo PUBLIC ${JemallocLib} ${Tbbpack})

add_executable(demo_con examples/example_concurrency.cpp)
#target_link_libraries(demo_con PUBLIC ${JemallocLib} ${Tbbpack})
#target_link_libraries(benchmark PUBLIC  ${Tbbpack})
#target_link_libraries(generate_dataset PUBLIC ${Tbbpack})
#target_link_libraries(unit_test PUBLIC ${Tbbpack})
#target_link_libraries(demo PUBLIC ${Tbbpack})
#target_link_libraries(demo_con PUBLIC ${Tbbpack})

# target_link_libraries(benchmark        PUBLIC ${JemallocLib} ${Tbblib} ${Tbbbind} ${Tbbmalloc} ${Tbbmalloc_proxy})
# target_link_libraries(generate_dataset PUBLIC ${JemallocLib} ${Tbblib} ${Tbbbind} ${Tbbmalloc} ${Tbbmalloc_proxy})
# target_link_libraries(unit_test        PUBLIC ${JemallocLib} ${Tbblib} ${Tbbbind} ${Tbbmalloc} ${Tbbmalloc_proxy})
# target_link_libraries(demo             PUBLIC ${JemallocLib} ${Tbblib} ${Tbbbind} ${Tbbmalloc} ${Tbbmalloc_proxy})
# target_link_libraries(demo_con         PUBLIC ${JemallocLib} ${Tbblib} ${Tbbbind} ${Tbbmalloc} ${Tbbmalloc_proxy})
target_link_libraries(benchmark        PUBLIC ${JemallocLib} ${Tbblib})
target_link_libraries(generate_dataset PUBLIC ${JemallocLib} ${Tbblib})
target_link_libraries(unit_test        PUBLIC ${JemallocLib} ${Tbblib})
target_link_libraries(demo             PUBLIC ${JemallocLib} ${Tbblib})
target_link_libraries(demo_con         PUBLIC ${JemallocLib} ${Tbblib})