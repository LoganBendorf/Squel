
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <sstream>
#include <cstring>
#include <cctype>

enum keyword_enum {
    CREATE, TABLE, SELECT, FROM, INSERT, INTO, VALUES, STRING_LITERAL, INTEGER_LITERAL, OPEN_PAREN, CLOSE_PAREN, SEMICOLON, COMMA, ASTERISK, LINE_END, ILLEGAL
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