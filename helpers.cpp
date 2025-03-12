
#include "helpers.h"

std::string keyword_enum_to_string(keyword_enum index) {
    constexpr const char* keyword_enum_to_string[] =  
    {"CREATE", "TABLE", "SELECT", "FROM", "INSERT", "INTO", "VALUES", 
       "STRING_LITERAL", "INTEGER_LITERAL", "OPEN_PAREN", "CLOSE_PAREN",
         "SEMICOLON", "COMMA", "ASTERISK", "LINE_END", "ILLEGAL", "NEW_LINE",
           "DATA", "QUOTE", "BOOL", "MINUS", "DOT"};
    return std::string(keyword_enum_to_string[index]);
}


bool is_integer_data_type(std::string data_type) {
    if (data_type.find("INT") == std::string::npos && // covers alot
        data_type.find("BOOL") == std::string::npos
    ) {
        return false;
    }
    return true;
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

bool is_string_data_type(std::string data_type) {
    if (data_type.find("CHAR") == std::string::npos // covers char and varchar
    ) {
        return false;
    }
    return true;
}
