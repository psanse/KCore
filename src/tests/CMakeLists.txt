
#######################
# KCore unit tests
#######################

add_executable(tests_kcore
    test_kcore.cpp
)

# Link dependencies
target_link_libraries(tests_kcore
    PRIVATE
        graph
        bitscan
        utils
        GTest::gtest_main
)

# Include directories
target_include_directories(tests_kcore
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${bitgraph_SOURCE_DIR}/src
        ${bitgraph_SOURCE_DIR}/src/graph/algorithms
)

# Require C++14
target_compile_features(tests_kcore
    PRIVATE cxx_std_14
)

# Output directory
set_target_properties(tests_kcore
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
)

# Test data shipped with BitGraph
target_compile_definitions(tests_kcore
    PRIVATE
        PATH_GRAPH_TESTS_CMAKE_SRC_CODE="${bitgraph_SOURCE_DIR}/src/graph/data/"
        BITGRAPH_GRAPH_TEST_DATA_DIR="${bitgraph_SOURCE_DIR}/src/graph/data/"
)

# Register GoogleTest tests with CTest
include(GoogleTest)
gtest_discover_tests(tests_kcore)
