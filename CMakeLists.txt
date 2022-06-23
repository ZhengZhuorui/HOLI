cmake_minimum_required(VERSION 2.8)
project(AEX)

set(AEX_VERSION_MAJOR "1")
set(AEX_VERSION_MINOR "1")
set(AEX_VERSION_PATCH "3")
set(AEX_VERSION_STRING ${RAPID1vN_VERSION_MAJOR}.${RAPID1vN_VERSION_MINOR}.${RAPID1vN_VERSION_PATCH})

include_directories(${CMAKE_SOURCE_DIR}/aex)
include_directories(${CMAKE_SOURCE_DIR})

#add_executable(example ${CMAKE_SOURCE_DIR}/examples/main.cpp)
add_executable(test ${CMAKE_SOURCE_DIR}/examples/test.cpp)
