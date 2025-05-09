

#include "helpers.h"

#include "object.h"

#include <vector>
#include <string>
#include <span>


std::span<const char* const> token_type_span() {
    static constexpr const char* token_type_to_string[] = {
        "ERROR", "CREATE", "TABLE", "SELECT", "FROM", "INSERT", "INTO", "VALUES", "STRING_LITERAL", "INTEGER_LITERAL", "OPEN_PAREN", "CLOSE_PAREN", "SEMICOLON",
          "COMMA", "LINE_END", "ILLEGAL", "NEW_LINE", "QUOTE", "DOT", "TRUE", "FALSE", "OPEN_BRACKET", "CLOSE_BRACKET",
            "EQUAL", "NOT_EQUAL", "LESS_THAN", "GREATER_THAN", "PLUS", "MINUS", "SLASH", "ASTERISK", "BANG", "WHERE", "ALTER", "ADD", "COLUMN", "DEFAULT",

            "OR", "REPLACE", "FUNCTION", "RETURNS", "BEGIN", "RETURN", "END", "IF", "THEN", "ELSIF", "ELSE","$$", "AS",

            "CHAR", "VARCHAR", "BOOL", "BOOLEAN", "DATE", "YEAR", "SET", "BIT", "INT", "INTEGER", "FLOAT", "DOUBLE", "NONE", "UNSIGNED", "ZEROFILL",
            "TINYBLOB", "TINYTEXT", "MEDIUMTEXT", "MEDIUMBLOB", "LONGTEXT", "LONGBLOB", "DEC", "DECIMAL", 
            
            "NULL_TOKEN",
    };
    return token_type_to_string;
}

std::string token_type_to_string(token_type index) {
    return std::string(token_type_span()[index]);
}


bool is_numeric_data_type(SQL_data_type_object* data_type) {
    if (data_type->data_type == INT  ||
        data_type->data_type == FLOAT ||
        data_type->data_type == DOUBLE ||
        data_type->data_type == BOOL ) {
            return true;
    }
    return false;
}

bool is_string_data_type(SQL_data_type_object* data_type) {
    if (data_type->data_type == CHAR ||
        data_type->data_type == VARCHAR ) {
            return true;
    }
    return false;
}

bool is_sql_data_type_token(token tok) {
    switch (tok.type) {
    case CHAR: case VARCHAR: case BOOL: case BOOLEAN: case DATE: case YEAR: case SET: case BIT: case INT: case INTEGER: case FLOAT: case DOUBLE: case NONE: case UNSIGNED: case ZEROFILL: case
    TINYBLOB: case TINYTEXT: case MEDIUMTEXT: case MEDIUMBLOB: case LONGTEXT: case LONGBLOB: case DEC: case DECIMAL: 
        return true;
    default:
        return false;
    }
}

// Returns false for plus and minus signs
bool is_numeric_token(token tok) {
    switch (tok.type) {
    case DOT:
        return true;
    case INTEGER_LITERAL:
        return true;
    default:
        return false;
    }
}


bool is_string_object(object* obj) {
    switch (obj->type()) {
    case STRING_OBJ:
        return true;
    default:
        return false;
    }
}

bool is_numeric_object(object* obj) {
    switch(obj->type()) {
    case INTEGER_OBJ: case DECIMAL_OBJ:
        return true;
    default:
        return false;
    }
}

bool is_conditional_object(object* obj) {
    switch (obj->type()) {
    case INFIX_EXPRESSION_OBJ:
        return true;
    default:
        return false;
    }
}
