QT += widgets
TEMPLATE = app
SOURCES += evaluator.cpp helpers.cpp lexer.cpp main.cpp parser.cpp print.cpp test_reader.cpp
HEADERS += evaluator.h   helpers.h   lexer.h node.h object.h parser.h pch.h print.h structs_and_macros.h test_reader.h token.h
TARGET = squel

DESTDIR = build
OBJECTS_DIR = build



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