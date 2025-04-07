#pragma once

#include <string>

enum token_type {
    ERROR, CREATE, TABLE, SELECT, FROM, INSERT, INTO, VALUES, STRING_LITERAL, INTEGER_LITERAL, OPEN_PAREN, CLOSE_PAREN, SEMICOLON,
     COMMA, LINE_END, ILLEGAL, NEW_LINE, QUOTE, DOT, TRUE, FALSE, OPEN_BRACKET, CLOSE_BRACKET,
      EQUAL, NOT_EQUAL, LESS_THAN, GREATER_THAN, PLUS, MINUS, SLASH, ASTERISK, BANG, WHERE, ALTER, ADD, COLUMN, DEFAULT
    
};

typedef struct token {
    token_type type;
    std::string data;
    int line;
    int position;
} token;