# KCore is header-only: expose it as an interface library

add_library(kcore INTERFACE)

target_include_directories(kcore
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${bitgraph_SOURCE_DIR}/src
        ${bitgraph_SOURCE_DIR}/src/graph/algorithms
)

target_link_libraries(kcore
    INTERFACE
        graph
        bitscan
        utils
)

target_compile_features(kcore INTERFACE cxx_std_14)

# Unit tests
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
