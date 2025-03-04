
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

enum keyword_enum {
    CREATE, TABLE, SELECT, FROM, INSERT, INTO, VALUES, STRING_LITERAL, INTEGER_LITERAL, OPEN_PAREN, CLOSE_PAREN, SEMICOLON,
     COMMA, ASTERISK, LINE_END, ILLEGAL, NEW_LINE, DATA, QUOTE, BOOL
};

typedef struct token {
    keyword_enum keyword;
    std::string data;
    int line;
    int position;
} token;

typedef struct column_data {
    std::string field_name;
    std::string data_type;
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

#endif
