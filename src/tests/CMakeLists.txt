
#######################
# KCore unit tests
#######################

add_executable(tests_kcore
    test_kcore.cpp
)

target_link_libraries(tests_kcore
    PRIVATE
        kcore
        GTest::gtest_main
)

set_target_properties(tests_kcore
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
)

# Register GoogleTest tests with CTest
include(GoogleTest)
gtest_discover_tests(tests_kcore)

# Test data shipped with BitGraph
target_compile_definitions(tests_kcore
    PRIVATE
        PATH_GRAPH_TESTS_CMAKE_SRC_CODE="${bitgraph_SOURCE_DIR}/src/graph/data/"
        BITGRAPH_GRAPH_TEST_DATA_DIR="${bitgraph_SOURCE_DIR}/src/graph/data/"
)
