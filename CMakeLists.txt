cmake_minimum_required(VERSION 3.28)

# Use Clang 19 compiler
set(CMAKE_CXX_COMPILER clang++-19)
set(CMAKE_C_COMPILER clang-19)

project(squel LANGUAGES CXX)

# No modules
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# C++ Version
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


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

# Enables back trace symbols if ON
set(CMAKE_ENABLE_EXPORTS OFF) 

set(COMPILE_FLAGS
    -fdiagnostics-color=always
    -O0

    # -Werror
    # -Wno-deprecated-declarations
    # -Wall -Wextra -Wpedantic -Wconversion -Wdouble-promotion
    # -Wcast-align -Wnon-virtual-dtor -Wunused -Woverloaded-virtual
    # -Wold-style-cast -Wuninitialized -Wshadow -Wpointer-arith
    # -Wformat=2 -Wsign-conversion -Wnull-dereference
    # -Wmisleading-indentation -Wimplicit-fallthrough -Wunused-label
    # -Wno-dollar-in-identifier-extension
    # -pedantic-errors

    # Sanitzer
    ${SANITIZER_FLAGS}

    # Debug
    -g 
    #-fno-omit-frame-pointer 
    #-fno-optimize-sibling-calls
    #-funwind-tables 
)



# Files
set(SOURCE_DIR "${PROJECT_SOURCE_DIR}/source")

set(CPP_FILES
    ${SOURCE_DIR}/token.cpp
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
    ${SOURCE_DIR}/files.cpp
)


# Find Qt5
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

# Create static library of shared code
add_library(squel_core STATIC)
target_sources(squel_core PUBLIC
    ${CPP_FILES}
)

target_compile_options(squel_core PUBLIC ${COMPILE_FLAGS})
target_link_options(squel_core PUBLIC ${SANITIZER_FLAGS})
target_include_directories(squel_core PUBLIC 
    ${SOURCE_DIR}
    ${SOURCE_DIR}/Allocators
)
target_link_libraries(squel_core PUBLIC Qt5::Core Qt5::Widgets)
target_precompile_headers(squel_core PUBLIC ${SOURCE_DIR}/pch.h)

# Generate macro
get_filename_component(PROJECT_ROOT_DIR ${CMAKE_SOURCE_DIR} ABSOLUTE)

target_compile_definitions(squel_core PRIVATE 
    PROJECT_ROOT="${PROJECT_ROOT_DIR}"
)

# Main executable
add_executable(squel ${SOURCE_DIR}/main.cpp)
target_link_libraries(squel PRIVATE squel_core)
target_precompile_headers(squel REUSE_FROM squel_core)

# Tests
set(TEST_DIR "${PROJECT_SOURCE_DIR}/tests")

add_executable(save_test ${TEST_DIR}/save_test.cpp)
target_link_libraries(save_test PRIVATE squel_core)
target_precompile_headers(save_test REUSE_FROM squel_core)
set_target_properties(save_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY tests
)

add_executable(file_reader_test ${TEST_DIR}/file_reader_test.cpp)
target_link_libraries(file_reader_test PRIVATE squel_core)
target_precompile_headers(file_reader_test REUSE_FROM squel_core)
set_target_properties(file_reader_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY tests
)


