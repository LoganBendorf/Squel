
#include "pch.h"

#include "helpers.h"

#include "object.h"

#include <bit>
#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <memory>

[[maybe_unused]] bool is_numeric_data_type(SQL_data_type_object* data_type) {
    if (data_type->data_type == INT  ||
        data_type->data_type == FLOAT ||
        data_type->data_type == DOUBLE ||
        data_type->data_type == BOOL ) {
            return true;
    }
    return false;
}

bool is_string_data_type(token_type data_type) {
    if (data_type == CHAR ||
        data_type == VARCHAR ) {
            return true;
    }
    return false;
}

bool is_string_data_type(SQL_data_type_object* data_type) {
    return is_string_data_type(data_type->data_type);
}

bool is_sql_data_type_token(token tok) {
    switch (tok.type) {
    case CHAR: case VARCHAR: case BOOL: case BOOLEAN: case DATE: case YEAR: case SET: case BIT: case INT: case INTEGER: case FLOAT: case DOUBLE: case NONE: 
    case UNSIGNED: case ZEROFILL: case TINYBLOB: case TINYTEXT: case MEDIUMTEXT: case MEDIUMBLOB: case LONGTEXT: case LONGBLOB: case DEC: case DECIMAL:
    case TIMESTAMP:
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


static bool is_string_object(const object* obj) {
    switch (obj->type()) {
        case STRING_OBJ:
        return true;
    default:
        return false;
    }
}

bool is_string_object(const UP<object>& obj) {
    return is_string_object(static_cast<const object*>(obj.get())); }

bool is_string_object(const UP<evaluated>& obj) {
    return is_string_object(static_cast<const object*>(obj.get())); }

bool is_string_object(const UP<serializable>& obj) {
    return is_string_object(static_cast<const object*>(obj.get())); }


static bool is_numeric_object(const object* obj) {
    switch(obj->type()) {
    case INTEGER_OBJ: case DECIMAL_OBJ:
        return true;
    default:
        return false;
    }
}

bool is_numeric_object(const UP<object>& obj) {
    return is_numeric_object(static_cast<const object*>(obj.get())); }

bool is_numeric_object(const UP<evaluated>& obj) {
    return is_numeric_object(static_cast<const object*>(obj.get())); }

bool is_numeric_object(const UP<serializable>& obj) {
    return is_numeric_object(static_cast<const object*>(obj.get())); }


bool is_conditional_object(const UP<object>& obj) {
    switch (obj->type()) {
    case INFIX_EXPRESSION_OBJ:
        return true;
    default:
        return false;
    }
}

bool is_object(const UP<object>& obj) {
    return dynamic_cast<const object*>(obj.get()) != nullptr;
}
bool is_object(const UP<serializable>& obj) {
    return dynamic_cast<const object*>(obj.get()) != nullptr;
}

bool is_evaluated(const UP<evaluated>& obj) {
    return dynamic_cast<const evaluated*>(obj.get()) != nullptr;
}
bool is_evaluated(const UP<serializable>& obj) {
    return dynamic_cast<const evaluated*>(obj.get()) != nullptr;
}

bool is_serializable(const UP<evaluated>& obj) {
    return dynamic_cast<const serializable*>(obj.get()) != nullptr;
}
bool is_serializable(const UP<serializable>& obj) {
    return dynamic_cast<const serializable*>(obj.get()) != nullptr;
}





// Debug funcs
std::string call_inspect(const UP<object>& obj)             { return obj->inspect(); }
std::string call_inspect(const UP<evaluated>& obj)          { return obj->inspect(); }
std::string call_inspect(const UP<serializable>& obj)       { return obj->inspect(); }
std::string call_inspect(const avec<UP<serializable>>& objs) { 
    std::stringstream ss;
    bool first = true;
    for (const auto& obj : objs) {
        if (!first) { ss << ", "; }
        if (obj == nullptr) {
            ss << "NULLPTR";
        } else {
            ss << obj->inspect(); }
        first = false;
    }; 
    return ss.str();
}
const UP<object>& index_avec(const avec<UP<object>>& vec, size_t index) {
    return vec[index]; 
}
size_t avec_size(const avec<UP<object>>& vec) {
    return vec.size();
}
