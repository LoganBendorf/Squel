
#include "helpers.h"
#include "object.h"

#include <vector>
#include <string>

extern std::vector<std::string> warnings;

std::string token_type_to_string(token_type index) {
    constexpr const char* token_type_to_string[] =  
    {"ERROR", "CREATE", "TABLE", "SELECT", "FROM", "INSERT", "INTO", "VALUES", 
       "STRING_LITERAL", "INTEGER_LITERAL", "OPEN_PAREN", "CLOSE_PAREN",
         "SEMICOLON", "COMMA", "LINE_END", "ILLEGAL", "NEW_LINE",
           "QUOTE", "DOT", "TRUE", "FALSE", "OPEN_BRACKET", "CLOSE_BRACKET",
           "EQUAL", "NOT_EQUAL", "LESS_THAN", "GREATER_THAN", "PLUS", "MINUS", "SLASH", "ASTERISK", "BANG", "WHERE", "ALTER", "ADD", "COLUMN", "DEFAULT"};
    return std::string(token_type_to_string[index]);
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
    case DECIMAL_OBJ:
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

bool is_listable(object* obj) {
    if (is_numeric_object(obj) ||
        is_string_object(obj)) {
        return true;
    }
    return false;
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
            int max_length = data_type->parameter_value;
            int insert_length = insert_obj->data().length();
            if (insert_length > max_length) {
                err_obj->value = "can_insert(): Value: (" + insert_obj->data() + ") excedes max column length(" + data_type->inspect() + ")";
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