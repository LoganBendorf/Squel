

#include "helpers.h"

#include "object.h"

#include <vector>
#include <string>
#include <span>

extern std::vector<std::string> warnings;

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


object* can_insert(object* insert_obj, SQL_data_type_object* data_type) {

    error_object* err_obj = new error_object();

    switch (data_type->data_type) {
    case INT:
        switch (insert_obj->type()) {
        case INTEGER_OBJ:
            return insert_obj; 
            break;
        case DECIMAL_OBJ:
            warnings.push_back("Decimal implicitly converted to INT");
            return new integer_object(insert_obj->data());
            break;
        default:
            err_obj->value = "can_insert(): Value: (" + insert_obj->data() + ") has mismatching type with column (" + data_type->inspect() + ")";
            return err_obj;
            break;
        }
        break;
    case FLOAT:
        switch (insert_obj->type()) {
        case INTEGER_OBJ:
            warnings.push_back("Integer implicitly converted to FLOAT");
            return new decimal_object(insert_obj->data());
            break;
        case DECIMAL_OBJ:
            return insert_obj; 
            break;
        default:
            err_obj->value = "can_insert(): Value: (" + insert_obj->data() + ") has mismatching type with column (" + data_type->inspect() + ")";
            return err_obj;
            break;
        }
        break;
    case DOUBLE:
    switch (insert_obj->type()) {
        case INTEGER_OBJ:
            warnings.push_back("Integer implicitly converted to DOUBLE");
            return new decimal_object(insert_obj->data());
            break;
        case DECIMAL_OBJ:
            return insert_obj; 
            break;
        default:
            err_obj->value = "can_insert(): Value: (" + insert_obj->data() + ") has mismatching type with column (" + data_type->inspect() + ")";
            return err_obj;
            break;
        }
        break;
    case VARCHAR:
        switch (insert_obj->type()) {
        case INTEGER_OBJ:
            warnings.push_back("Integer implicitly converted to VARCHAR");
            return new string_object(insert_obj->data());
            break;
        case DECIMAL_OBJ:
            warnings.push_back("Decimal implicitly converted to VARCHAR");
            return new string_object(insert_obj->data());
            break;
        case STRING_OBJ: {
            if (data_type->parameter->type() != INTEGER_OBJ) {
                err_obj->value = "can_insert(): varchar cannot be inserted into data type with non-integer parameter";
                return err_obj; }
            int max_length = static_cast<integer_object*>(data_type->parameter)->value;
            int insert_length = insert_obj->data().length();
            if (insert_length > max_length) {
                err_obj->value = "can_insert(): Value: (" + insert_obj->data() + ") excedes column's max length (" + data_type->parameter->inspect() + ")";
                return err_obj;}
            return insert_obj;
        } break;
        default:
            err_obj->value = "can_insert(): Value: (" + insert_obj->data() + ") has mismatching type with column (" + data_type->inspect() + ")";
            return err_obj;
            break;
        }
        break;
    default:
        err_obj->value = "can_insert(): " + data_type->inspect() + " is not supported YET";
        return err_obj;
        break;
    }
    
}