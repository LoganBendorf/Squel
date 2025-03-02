
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

#define push_error_return(x)                    \
        token curTok;                           \
        if (token_position >= tokens.size()) {  \
            curTok.keyword = LINE_END; curTok.data = ""; curTok.line = -1; curTok.position = -1; \
        } else {                                \
            curTok = tokens[token_position];}   \
        std::string error = x;                  \
        error = error + ". Line = " + std::to_string(curTok.line) + ", position = " + std::to_string(curTok.position);\
        errors.push_back(error);                \
        return                                  \

#define push_error_return_empty_string(x)       \
        token curTok;                           \
        if (token_position >= tokens.size()) {  \
            curTok.keyword = LINE_END; curTok.data = ""; curTok.line = -1; curTok.position = -1; \
        } else {                                \
            curTok = tokens[token_position];}   \
        std::string error = x;                  \
        error = error + ". Line = " + std::to_string(curTok.line) + ", position = " + std::to_string(curTok.position);\
        errors.push_back(error);                \
        return ""                               \


enum keyword_enum {
    CREATE, TABLE, SELECT, FROM, INSERT, INTO, VALUES, STRING_LITERAL, INTEGER_LITERAL, OPEN_PAREN, CLOSE_PAREN, SEMICOLON, COMMA, ASTERISK, LINE_END, ILLEGAL, NEW_LINE, DATA
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

#endif
