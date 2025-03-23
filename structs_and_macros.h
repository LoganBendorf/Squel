
#pragma once

#include "object.h"

#include <string>
#include <vector>

enum token_type {
    CREATE, TABLE, SELECT, FROM, INSERT, INTO, VALUES, STRING_LITERAL, INTEGER_LITERAL, OPEN_PAREN, CLOSE_PAREN, SEMICOLON,
     COMMA, LINE_END, ILLEGAL, NEW_LINE, QUOTE, DOT, TRUE, FALSE, OPEN_BRACKET, CLOSE_BRACKET,
      EQUAL, NOT_EQUAL, LESS_THAN, GREATER_THAN, PLUS, MINUS, SLASH, ASTERISK, BANG
};

typedef struct token {
    token_type type;
    std::string data;
    int line;
    int position;
} token;

typedef struct column_data {
    std::string field_name;
    SQL_data_type_object* data_type;
    std::string default_value;
} column_data;

typedef struct row {
    std::vector<std::string> column_values;
} row;

typedef struct table {
    std::string name;
    std::vector<column_data> column_datas;
    std::vector<row> rows;
} table;

typedef struct display_table {
    bool to_display;
    table tab;
} display_table;

struct test {
    std::string folder_name;
    std::vector<std::string> test_paths;
    int max_tests = 0;
    int current_test_num = 0;
};
