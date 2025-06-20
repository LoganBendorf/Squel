#pragma once

#include "pch.h"

enum token_type {
    ERROR_TOKEN, CREATE, TABLE, SELECT, FROM, INSERT, INTO, VALUES, STRING_LITERAL, INTEGER_LITERAL, OPEN_PAREN, CLOSE_PAREN, SEMICOLON,
     COMMA, LINE_END, ILLEGAL, NEW_LINE, QUOTE, DOT, TRUE, FALSE, OPEN_BRACKET, CLOSE_BRACKET,
      EQUAL, NOT_EQUAL, LESS_THAN, GREATER_THAN, PLUS, MINUS, SLASH, ASTERISK, BANG, WHERE, ALTER, ADD, COLUMN, DEFAULT,
    // Function stuff
        OR, REPLACE, FUNCTION, RETURNS, BEGIN, RETURN, END, IF, THEN, ELSIF, ELSE, $$, AS, LEFT, JOIN, GROUP, ORDER, BY, ON,
    // Built-in functions
        COUNT,
    // Data types
        CHAR, VARCHAR, BOOL, BOOLEAN, DATE, YEAR, SET, BIT, INT, INTEGER, FLOAT, DOUBLE, NONE, UNSIGNED, ZEROFILL,
        TINYBLOB, TINYTEXT, MEDIUMTEXT, MEDIUMBLOB, LONGTEXT, LONGBLOB, DEC, DECIMAL, 
        
        NULL_TOKEN,

    // Custom
        ASSERT
};

typedef struct token {
    token_type type;
    std::string data;
    size_t line;
    size_t position;
} token;