
cmake_minimum_required(VERSION 3.28)

# Use Clang 19 compiler
set(CMAKE_CXX_COMPILER clang++-19)
set(CMAKE_C_COMPILER clang-19)

project(squel LANGUAGES CXX)

# C++ Version
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS /usr/bin/clang-scan-deps-19)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_MODULE_STD 1)

# Use libc++ and mold
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi -fuse-ld=mold ")


# Flags

# Sanitizer flags (need for compile and link)
set(SANITIZER_FLAGS
    -fsanitize=address,undefined
    -fsanitize=signed-integer-overflow
)

set(COMPILE_FLAGS
    -fcolor-diagnostics
    -O0
    -Wno-deprecated-declarations
    -Wall -Wextra -Wpedantic -Wconversion -Wdouble-promotion
    -Wcast-align -Wnon-virtual-dtor -Wunused -Woverloaded-virtual
    -Wold-style-cast -Wuninitialized -Wshadow -Wpointer-arith
    -Wformat=2 -Wsign-conversion -Wnull-dereference
    -Wmisleading-indentation -Wimplicit-fallthrough
    -Wno-dollar-in-identifier-extension
    -Werror -pedantic-errors
    # Sanitzer
    ${SANITIZER_FLAGS}
    # Debug
    -g 
    #-fno-omit-frame-pointer 
    #-fno-optimize-sibling-calls
    #-funwind-tables 
)

# Files
add_executable(squel main.cpp)
target_sources(squel PUBLIC
        ${PROJECT_SOURCE_DIR}/main.cpp
        ${PROJECT_SOURCE_DIR}/test_reader.cpp
        # Module implementations
            ${PROJECT_SOURCE_DIR}/object.cpp
            ${PROJECT_SOURCE_DIR}/helpers.cpp
            ${PROJECT_SOURCE_DIR}/node.cpp
            ${PROJECT_SOURCE_DIR}/environment.cpp
            ${PROJECT_SOURCE_DIR}/evaluator.cpp
            ${PROJECT_SOURCE_DIR}/parser.cpp
            ${PROJECT_SOURCE_DIR}/print.cpp
            ${PROJECT_SOURCE_DIR}/lexer.cpp
            ${PROJECT_SOURCE_DIR}/execute.cpp
    PUBLIC
    FILE_SET CXX_MODULES
    FILES
        ${PROJECT_SOURCE_DIR}/object.cppm
        ${PROJECT_SOURCE_DIR}/helpers.cppm
        ${PROJECT_SOURCE_DIR}/node.cppm
        ${PROJECT_SOURCE_DIR}/environment.cppm
        ${PROJECT_SOURCE_DIR}/evaluator.cppm
        ${PROJECT_SOURCE_DIR}/parser.cppm
        ${PROJECT_SOURCE_DIR}/print.cppm
        ${PROJECT_SOURCE_DIR}/lexer.cppm
        ${PROJECT_SOURCE_DIR}/execute.cppm
)

target_compile_options(squel PUBLIC
    ${COMPILE_FLAGS}
)

# Apply sanitizer flags to linker as well
target_link_options(squel PUBLIC ${SANITIZER_FLAGS})

target_include_directories(squel PUBLIC 
    ${CMAKE_SOURCE_DIR}/Allocators
)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets)
target_link_libraries(squel 
    Qt5::Core Qt5::Widgets
)
target_precompile_headers(squel PUBLIC pch.h)