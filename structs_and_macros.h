
#ifndef STRUCTS_AND_MACROS
#define STRUCTS_AND_MACROS

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <sstream>
#include <cstring>
#include <cctype>

#define push_error_return(x)                \
        std::string error = x;              \
        std::string line_string = "Line: " + std::to_string(token_line_number) + ". Position: " + std::to_string(token_line_position); \
        errors.push_back(error);            \
        return                              \

#define push_error_return_empty_string(x)   \
        std::string error = x;              \
        errors.push_back(error);            \
        return ""                           \


enum keyword_enum {
    CREATE, TABLE, SELECT, FROM, INSERT, INTO, VALUES, STRING_LITERAL, INTEGER_LITERAL, OPEN_PAREN, CLOSE_PAREN, SEMICOLON, COMMA, ASTERISK, LINE_END, ILLEGAL, NEW_LINE
};

typedef struct token {
    keyword_enum keyword;
    std::string data;
} token;

typedef struct column {
    std::string field_name;
    std::string data_type;
    std::string default_value;
    std::string data;
} column;

typedef struct row {
    std::vector<column> columns;
} row;

typedef struct table {
    std::string name;
    std::vector<column> columns;
    std::vector<row> rows;
} table;

typedef struct display_table {
    bool to_display;
    table tab;
} display_table;

#endif
