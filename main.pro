QT += widgets
TEMPLATE = app
SOURCES += main.cpp lexer.cpp parser.cpp evaluator.cpp helpers.cpp test_reader.cpp print.cpp
HEADERS += lexer.h parser.h evaluator.h helpers.h test_reader.h structs_and_macros.h node.h print.h
TARGET = myApp


# Suppress warnings
CONFIG -= warn_on
CONFIG += warn_off

# Debug flags
CONFIG += debug
QMAKE_CXXFLAGS += -g

# Compiler profiler
# QMAKE_CXXFLAGS += -ftime-report

# Resources
RC_ICONS = icons/squel.ico

# Compile speed
QMAKE_PARALLEL = 8
QMAKE_CXX = ccache g++

CONFIG += precompile_header
PRECOMPILED_HEADER = pch.h

    # Linker
QMAKE_LD = lld
QMAKE_LFLAGS += -fuse-ld=lld