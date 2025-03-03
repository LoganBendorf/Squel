QT += widgets
TEMPLATE = app
SOURCES += main.cpp lexer.cpp parser.cpp evaluator.cpp test_reader.cpp
HEADERS += lexer.h parser.h evaluator.h test_reader.h structs_and_macros.h
TARGET = myApp


# Suppress warnings
QMAKE_CFLAGS += -w
QMAKE_CXXFLAGS += -w


# Add debug flags
CONFIG += debug
QMAKE_CXXFLAGS += -g

# Resources
RC_ICONS = icons/squel.ico
