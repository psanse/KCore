# KCore executable

file(GLOB HEADER_FILES CONFIGURE_DEPENDS "*.h")
file(GLOB SRC_FILES CONFIGURE_DEPENDS "*.cpp")

add_executable(kcore
	${HEADER_FILES}
	${SRC_FILES}
)

# Link BitGraph libraries
target_link_libraries(kcore
	PRIVATE
		graph
		bitscan
		utils
)

# BitGraph headers
target_include_directories(kcore
    PRIVATE
        ${bitgraph_SOURCE_DIR}/src
        ${bitgraph_SOURCE_DIR}/src/graph/algorithms
)

# Require C++14
target_compile_features(kcore
    PRIVATE cxx_std_14
)

# Executable output directory
set_target_properties(kcore
	PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

# Unit tests
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
