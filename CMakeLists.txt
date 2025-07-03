cmake_minimum_required(VERSION 3.28)

# Use Clang 19 compiler
set(CMAKE_CXX_COMPILER clang++-19)
set(CMAKE_C_COMPILER clang-19)

project(squel LANGUAGES CXX)

# C++ Version
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Module setup
# set(CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS /usr/bin/clang-scan-deps-19)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_CXX_MODULE_STD 1)

# Use libc++ and mold
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi -fuse-ld=mold ")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Flags

# Sanitizer flags (need for compile and link)
set(SANITIZER_FLAGS
    # -fsanitize=address,undefined
    # -fsanitize=signed-integer-overflow
)

set(COMPILE_FLAGS
    # -fcolor-diagnostics
    # -O0
    # -Wno-deprecated-declarations
    # -Wall -Wextra -Wpedantic -Wconversion -Wdouble-promotion
    # -Wcast-align -Wnon-virtual-dtor -Wunused -Woverloaded-virtual
    # -Wold-style-cast -Wuninitialized -Wshadow -Wpointer-arith
    # -Wformat=2 -Wsign-conversion -Wnull-dereference
    # -Wmisleading-indentation -Wimplicit-fallthrough
    # -Wno-dollar-in-identifier-extension
    # -Werror -pedantic-errors
    # Sanitzer
    # ${SANITIZER_FLAGS}
    # Debug
    # -g 
    #-fno-omit-frame-pointer 
    #-fno-optimize-sibling-calls
    #-funwind-tables 
)



# Files
set(SOURCE_DIR "${PROJECT_SOURCE_DIR}/source")

set(MODULE_IMPLEMENTATIONS
    ${SOURCE_DIR}/object.cpp
    ${SOURCE_DIR}/helpers.cpp
    ${SOURCE_DIR}/node.cpp
    ${SOURCE_DIR}/environment.cpp
    ${SOURCE_DIR}/evaluator.cpp
    ${SOURCE_DIR}/parser.cpp
    ${SOURCE_DIR}/print.cpp
    ${SOURCE_DIR}/lexer.cpp
    ${SOURCE_DIR}/execute.cpp
    ${SOURCE_DIR}/test_reader.cpp
)

set(MODULES
    ${SOURCE_DIR}/token.cppm
    ${SOURCE_DIR}/object.cppm
    ${SOURCE_DIR}/helpers.cppm
    ${SOURCE_DIR}/node.cppm
    ${SOURCE_DIR}/structs.cppm
    ${SOURCE_DIR}/environment.cppm
    ${SOURCE_DIR}/evaluator.cppm
    ${SOURCE_DIR}/parser.cppm
    ${SOURCE_DIR}/print.cppm
    ${SOURCE_DIR}/lexer.cppm
    ${SOURCE_DIR}/execute.cppm
    ${SOURCE_DIR}/test_reader.cppm
)

# Find Qt5
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

# Create static library of shared code
add_library(squel_core STATIC)
target_sources(squel_core PUBLIC
        ${MODULE_IMPLEMENTATIONS}
    PUBLIC
    FILE_SET CXX_MODULES
    FILES
        ${MODULES}
)

target_compile_options(squel_core PUBLIC ${COMPILE_FLAGS})
target_link_options(squel_core PUBLIC ${SANITIZER_FLAGS})
target_include_directories(squel_core PUBLIC 
    ${SOURCE_DIR}/Allocators
)
target_link_libraries(squel_core PUBLIC Qt5::Core Qt5::Widgets)
target_precompile_headers(squel_core PUBLIC ${SOURCE_DIR}/pch.h)

# Main executable
add_executable(squel ${SOURCE_DIR}/main.cpp)
target_link_libraries(squel PRIVATE squel_core)
target_precompile_headers(squel REUSE_FROM squel_core)

# Tests
set(SAVE_TEST_DIR "${PROJECT_SOURCE_DIR}/tests/save_test")
add_executable(save_test ${SAVE_TEST_DIR}/main.cpp)
target_link_libraries(save_test PRIVATE squel_core)
target_precompile_headers(save_test REUSE_FROM squel_core)
set_target_properties(save_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY tests
)