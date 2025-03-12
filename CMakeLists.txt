cmake_minimum_required(VERSION 3.16)
project(MyQtApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt automation (MOC, UIC, RCC)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC OFF)  # Disable if you do not use .qrc files
set(CMAKE_AUTOUIC OFF)  # Disable if you do not use .ui files

# Use CCache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Use lld as the linker
set(CMAKE_LINKER "lld")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -j${CMAKE_BUILD_PARALLEL_LEVEL}")


# Split stable code into a library
add_library(MyQtLib STATIC 
  lexer.cpp 
  parser.cpp 
  evaluator.cpp 
  helpers.cpp 
  test_reader.cpp
)
target_link_libraries(MyQtLib PUBLIC Qt6::Widgets)

# Apply precompiled headers
target_precompile_headers(MyQtLib PRIVATE   
  <vector> 
  "pch.h"
)

# Main executable (only contains main.cpp)
add_executable(MyQtApp main.cpp)
target_link_libraries(MyQtApp PRIVATE MyQtLib Qt6::Widgets)

# Enable compiler optimizations and LTO
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -flto")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
