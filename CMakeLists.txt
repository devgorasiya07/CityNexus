cmake_minimum_required(VERSION 3.16)
project(CityNexus VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Collect all source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Main executable
add_library(citynexus SHARED ${SOURCES})

# Platform-specific linking
if(WIN32)
    # Nothing extra needed for Windows
elseif(UNIX)
    target_link_libraries(citynexus pthread)
endif()

# Output directory
set_target_properties(citynexus PROPERTIES
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

message(STATUS "CityNexus v${PROJECT_VERSION} - Smart City Infrastructure Planner")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
