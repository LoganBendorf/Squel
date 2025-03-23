
#include "helpers.h"
#include "object.h"

std::string token_type_to_string(token_type index) {
    constexpr const char* token_type_to_string[] =  
    {"CREATE", "TABLE", "SELECT", "FROM", "INSERT", "INTO", "VALUES", 
       "STRING_LITERAL", "INTEGER_LITERAL", "OPEN_PAREN", "CLOSE_PAREN",
         "SEMICOLON", "COMMA", "LINE_END", "ILLEGAL", "NEW_LINE",
           "QUOTE", "DOT", "TRUE", "FALSE", "OPEN_BRACKET", "CLOSE_BRACKET",
           "EQUAL", "NOT_EQUAL", "LESS_THAN", "GREATER_THAN", "PLUS", "MINUS", "SLASH", "ASTERISK", "BANG"};
    return std::string(token_type_to_string[index]);
}


bool is_integer_data_type(SQL_data_type_object* data_type) {
    if (data_type->data_type == INT ||
        data_type->data_type == BOOL ) {
            return true;
    }
    return false;
}

bool is_decimal_data_type(std::string data_type) {
    if (data_type.find("DECIMAL") == std::string::npos
    ) {
        return false;
    }
    return true;
}

bool is_boolean_data_type(std::string data_type) {
    if (data_type.find("BOOL") == std::string::npos) {
        return false;
    }
    return true;
}

bool is_string_data_type(SQL_data_type_object* data_type) {
    if (data_type->data_type == CHAR ||
        data_type->data_type == VARCHAR ) {
            return true;
    }
    return false;
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

bool is_numeric_object(object* obj) {
    switch (obj->type()) {
    case INTEGER_OBJ:
        return true;
    default:
        return false;
    }
}
