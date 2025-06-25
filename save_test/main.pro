
QT += widgets
TEMPLATE = app

INCLUDEPATH += ../Allocators
INCLUDEPATH += ..
DEPENDPATH += ../Allocators
DEPENDPATH += ..
SOURCES += save.cpp
SOURCES += ../evaluator.cpp ../object.cpp ../parser.cpp ../environment.cpp ../helpers.cpp ../node.cpp ../lexer.cpp ../print.cpp ../test_reader.cpp
HEADERS += ../pch.h ../evaluator.h   ../helpers.h   ../lexer.h ../node.h ../object.h ../parser.h ../pch.h ../print.h ../structs_and_macros.h ../test_reader.h ../token.h ../environment.h
HEADERS += ../Allocators/allocators.h  ../Allocators/allocator_aliases.h  ../Allocators/allocator_string.h
TARGET = test

DESTDIR = build
OBJECTS_DIR = build

# Always emit color!
QMAKE_CXXFLAGS += -fdiagnostics-color=always
QMAKE_LDFLAGS  += -fdiagnostics-color=always

# C++ 23
QMAKE_CXXFLAGS += -std=c++23

# Optimize. O3 has a bunch of warnings that can't be fixed, so stick with O2. Both are only about 2x faster so not worth the compile time.
QMAKE_CXXFLAGS += -O0

# Sanitizers
QMAKE_LFLAGS += -fsanitize=address
QMAKE_LFLAGS += -fsanitize=signed-integer-overflow


# Warning stuff
#CONFIG -= warn_on
#CONFIG += warn_off
#QMAKE_CXXFLAGS -= Wsign-compare
#QMAKE_CXXFLAGS += -Wno-sign-compare

QMAKE_CXXFLAGS += -Wall #Core warnings, but misses many important ones.
QMAKE_CXXFLAGS += -Wextra #Adds more strict checks (e.g. unused parameters).
QMAKE_CXXFLAGS += -Wpedantic # Warn on non-standard C++ behavior.
QMAKE_CXXFLAGS += -Wconversion #Warn on implicit conversions that might lose data.
QMAKE_CXXFLAGS += -Wdouble-promotion #Implicit float → double.
QMAKE_CXXFLAGS += -Wcast-align # Casts that might misalign pointers.
QMAKE_CXXFLAGS += -Wnon-virtual-dtor #When base classes lack virtual destructors.
QMAKE_CXXFLAGS += -Wunused 
QMAKE_CXXFLAGS += -Woverloaded-virtual
QMAKE_CXXFLAGS += -Wlogical-op # Suspicious logical expressions.
QMAKE_CXXFLAGS += -Wuseless-cast
QMAKE_CXXFLAGS += -Wold-style-cast # C-style casts in C++.
QMAKE_CXXFLAGS += -Wuninitialized

QMAKE_CXXFLAGS += -Wconversion               # warn on implicit type conversions
QMAKE_CXXFLAGS += -Wshadow                   # warn when a local variable shadows another
QMAKE_CXXFLAGS += -Wpointer-arith            # warn about questionable pointer arithmetic
QMAKE_CXXFLAGS += -Wlogical-op               # warn about suspicious combinations of logical operators
QMAKE_CXXFLAGS += -Wformat=2                 # tighten up printf/scanf format checking
QMAKE_CXXFLAGS += -Wold-style-cast           # discourage C‐style casts in C++
QMAKE_CXXFLAGS += -Wsign-conversion          # warn on implicit sign conversions
QMAKE_CXXFLAGS += -Wnull-dereference         # warn if dereferencing nullptr
QMAKE_CXXFLAGS += -Wduplicated-cond          # warn if the same condition appears twice
QMAKE_CXXFLAGS += -Wduplicated-branches      # warn if the same code appears in both branches of an if/else
QMAKE_CXXFLAGS += -Wmisleading-indentation   # warn if indentation implies a different scope
QMAKE_CXXFLAGS += -Wstrict-overflow=5        # warn on optimizations that assume no strict‐overflow UB
QMAKE_CXXFLAGS += -Wimplicit-fallthrough    # warn on any fall-through
#QMAKE_CXXFLAGS += -Wimplicit-fallthrough=3  # stricter (C++17 [[fallthrough]] attribute)
QMAKE_CXXFLAGS += -Werror                   # treat all warnings as errors
QMAKE_CXXFLAGS += -pedantic-errors


# Debug flags
CONFIG += debug
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
QMAKE_CXX = ccache g++

CONFIG += precompile_header
PRECOMPILE_HEADER = pch.h

    # Linker
QMAKE_LD = lld
QMAKE_LFLAGS += -fuse-ld=lld