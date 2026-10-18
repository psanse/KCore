cmake_minimum_required(VERSION 3.14)

project(KCORE LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
include(CTest)

#######################
# BitGraph dependency
#######################

FetchContent_Declare(
  bitgraph
  GIT_REPOSITORY https://github.com/psanse/BitGraph
  GIT_TAG v2.0.0 				# latest release
)
FetchContent_MakeAvailable(bitgraph)

#######################
# GoogleTest dependency
#######################

if(BUILD_TESTING)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
    )

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(googletest)

    message(STATUS "GoogleTest available")
endif()

#######################
# Project
#######################

add_subdirectory(src)
