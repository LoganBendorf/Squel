QT += widgets
TEMPLATE = app
INCLUDEPATH += $$PWD/Allocators
SOURCES += object.cpp node.cpp environment.cpp evaluator.cpp parser.cpp main.cpp helpers.cpp lexer.cpp print.cpp test_reader.cpp
#SOURCES += Allocators/allocators.cpp
HEADERS += pch.h evaluator.h   helpers.h   lexer.h node.h object.h parser.h pch.h print.h structs_and_macros.h test_reader.h token.h environment.h
HEADERS += Allocators/allocators.h  Allocators/allocator_aliases.h  Allocators/allocator_string.h
TARGET = squel
DESTDIR = build
OBJECTS_DIR = build

# Use Clang compiler
QMAKE_CXX = ccache clang++
QMAKE_CC = ccache clang
QMAKE_LINK = clang++

# Use new library versions
QMAKE_CXXFLAGS += -stdlib=libc++
QMAKE_LFLAGS += -stdlib=libc++

# Always emit color!
QMAKE_CXXFLAGS += -fcolor-diagnostics
QMAKE_LDFLAGS  += -fcolor-diagnostics

# C++ 23
QMAKE_CXXFLAGS += -std=c++23

# Optimize. O3 has a bunch of warnings that can't be fixed, so stick with O2. Both are only about 2x faster so not worth the compile time.
QMAKE_CXXFLAGS += -O0

# Sanitizers
QMAKE_LFLAGS += -fsanitize=address
QMAKE_LFLAGS += -fsanitize=signed-integer-overflow

# Warning flags for Clang
QMAKE_CXXFLAGS += -Wall                # Core warnings
QMAKE_CXXFLAGS += -Wextra              # More strict checks (e.g. unused parameters)
QMAKE_CXXFLAGS += -Wpedantic           # Warn on non‑standard C++ behavior
QMAKE_CXXFLAGS += -Wconversion         # Warn on implicit conversions that might lose data
QMAKE_CXXFLAGS += -Wdouble-promotion   # Implicit float -> double promotions
QMAKE_CXXFLAGS += -Wcast-align         # Casts that might misalign pointers
QMAKE_CXXFLAGS += -Wnon-virtual-dtor   # Base classes lacking virtual destructors
QMAKE_CXXFLAGS += -Wunused             # Unused variables/functions/etc.
QMAKE_CXXFLAGS += -Woverloaded-virtual # Overriding virtual functions with different signatures
QMAKE_CXXFLAGS += -Wold-style-cast     # Discourage C‑style casts in C++
QMAKE_CXXFLAGS += -Wuninitialized      # Use of uninitialized variables
QMAKE_CXXFLAGS += -Wshadow             # Local variable shadows another
QMAKE_CXXFLAGS += -Wpointer-arith      # Questionable pointer arithmetic
QMAKE_CXXFLAGS += -Wformat=2           # Tighten printf/scanf format checking
QMAKE_CXXFLAGS += -Wsign-conversion    # Implicit sign conversions
QMAKE_CXXFLAGS += -Wnull-dereference   # Dereferencing nullptr
QMAKE_CXXFLAGS += -Wmisleading-indentation  # Indentation implies different scope
QMAKE_CXXFLAGS += -Wimplicit-fallthrough   # Any fall‑through in switch
QMAKE_CXXFLAGS += -Werror              # Treat all warnings as errors
QMAKE_CXXFLAGS += -pedantic-errors     # Errors for pedantic issues

# Disable
QMAKE_CXXFLAGS += -Wno-dollar-in-identifier-extension



# Debug flags
#CONFIG += debug
#QMAKE_CXXFLAGS += -g
#QMAKE_CXXFLAGS += -g3
#QMAKE_CXXFLAGS += -fno-omit-frame-pointer 
#QMAKE_CXXFLAGS += -funwind-tables
#QMAKE_CXXFLAGS += -ggdb3

# Compiler profiler
#QMAKE_CXXFLAGS += -ftime-report

# Resources
RC_ICONS = icons/squel.ico

# Compile speed
QMAKE_PARALLEL = 8
CONFIG += precompile_header
PRECOMPILE_HEADER = pch.h

# Linker - use LLD with Clang
QMAKE_LFLAGS += -fuse-ld=lld
