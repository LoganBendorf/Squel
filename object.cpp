#pragma once

// objects are made in the parser and should probably stay there, used to parse and return values from expressions
    // i.e (10 + 10) will return an integer_object with the value 20

#include "token.h"
#include "helpers.h"

#include "object.h"

#include <string>
#include <iostream>


// null_object
std::string null_object::inspect() const {
    return std::string("NULL_OBJECT");
}
object_type null_object::type() const {
    return NULL_OBJ;
}
std::string null_object::data() const {
    return std::string("NULL_OBJECT");
}


static std::span<const char* const> operator_type_span() {
    static constexpr const char* operator_type_to_string[] = {
        "ADD_OP", "SUB_OP", "MUL_OP", "DIV_OP",
        "EQUALS_OP", "NOT_EQUALS_OP", "LESS_THAN_OP", "LESS_THAN_OR_EQUAL_TO_OP", "GREATER_THAN_OP", "GREATER_THAN_OR_EQUAL_TO_OP",
        "OPEN_PAREN_OP", "OPEN_BRACKET_OP",
        "DOT_OP",
    };
    return operator_type_to_string;
}

static std::string operator_type_to_string(operator_type index) {
    return std::string(operator_type_span()[index]);
}


// operator_object
operator_object::operator_object(operator_type type) {
    op_type = type;
}

std::string operator_object::inspect() const {
    return operator_type_to_string(op_type); }
object_type operator_object::type() const {
    return OPERATOR_OBJ;}
std::string operator_object::data() const {
    return operator_type_to_string(op_type);}


// infix_expression_object
infix_expression_object::infix_expression_object(operator_object* set_op, object* set_left) {
    op = set_op;
    left = set_left;
    right = NULL;
}
std::string infix_expression_object::inspect() const {
    std::string str = "Op: " + op->inspect();
    if (left != NULL) {
        str += ". Left: " + left->inspect();}
    if (right != NULL) {
        str += ". Right: " + right->inspect();}
    str += "\n";
    return str;}
object_type infix_expression_object::type() const {
    return INFIX_EXPRESSION_OBJ;}
std::string infix_expression_object::data() const {
    return std::string("Shouldn't get data from infix expression object");}



// integer_object
integer_object::integer_object(std::string val) {
    value = std::stoi(val);}
    integer_object::integer_object(int val) {
    value = val;}
    integer_object::integer_object() {
    value = 0;}

std::string integer_object::inspect() const {
    return std::to_string(value);}

object_type integer_object::type() const {
    return INTEGER_OBJ;}

std::string integer_object::data() const {
    return std::to_string(value);}


// decimal_object
decimal_object::decimal_object(std::string val) {
    value = std::stod(val);}
    decimal_object::decimal_object(double val) {
    value = val;}
    decimal_object::decimal_object() {
    value = 0;}
std::string decimal_object::inspect() const {
    return std::to_string(value);}
object_type decimal_object::type() const {
    return DECIMAL_OBJ;}
std::string decimal_object::data() const {
    return std::to_string(value);}



// string_object
    string_object::string_object(std::string val) {
        value = val;
    }
    std::string string_object::inspect() const {
        return value;
    }
    object_type string_object::type() const {
        return STRING_OBJ;
    }
    std::string string_object::data() const {
        return value;
    }

//zerofill is implicitly unsigned im pretty sure
// SQL_data_type_object
std::string SQL_data_type_object::inspect() const {
    std::string ret = "[Type: ";
    if (prefix != NONE) {
        ret +=  token_type_to_string(prefix);}
        
    ret += token_type_to_string(data_type) + " (" + std::to_string(parameter_value) + ")";
    if (!elements.empty()) {
        ret += ". Elements: ";
        for (int i = 0; i < elements.size(); i++) {
            ret += elements[i];
            ret += ", ";
        }
    }
    ret += "]";
    return ret;
}
object_type SQL_data_type_object::type() const {
    return SQL_DATA_TYPE_OBJ;
}
std::string SQL_data_type_object::data() const {
    return std::to_string(parameter_value);
}

// function_object
std::string function_object::inspect() const {
    std::string ret_str = name + ":\n";
    for (const auto& arg : arguments) {
        ret_str += "[" + arg.first->inspect() + ", " + token_type_to_string(arg.second.type) + "]\n"; }
    ret_str += return_type->inspect();
    for (const auto& expr : expressions) {
        ret_str += expr->inspect(); }
    return ret_str;}
object_type function_object::type() const {
    return FUNCTION_OBJ;}
std::string function_object::data() const {
    return "FUNCTION_OBJECT";}


// column_object
column_object::column_object(std::string name, SQL_data_type_object* type, std::string default_val) {
    column_name = name;
    data_type = type;
    default_value = default_val;
}
std::string column_object::inspect() const {
    std::string str = "[Column name: " + column_name + "], ";
    str += data_type->inspect();
    str += ", [Default: " + default_value  + "]\n";
    return str;
}
object_type column_object::type() const {
    return COLUMN_OBJ;
}
std::string column_object::data() const {
    return column_name + " " + data_type->data() + " " + default_value;
}

// error_object
std::string error_object::inspect() const {
    return "ERROR: " + value;
}
object_type error_object::type() const {
    return ERROR_OBJ;
}
std::string error_object::data() const {
    return value;
}


// Statements
// if_statement
if_statement::if_statement() {
    other = new null_object();
}

std::string if_statement::inspect() const {
    std::string ret_str = condition->inspect();
    for (const auto& element : body) {
        ret_str += element->inspect(); }
    ret_str += other->inspect();
    return ret_str;
}
object_type if_statement::type() const {
    return IF_STATEMENT;
}
std::string if_statement::data() const {
    return "IF_STATEMENT";
}

// block_statement
block_statement::block_statement(std::vector<object*> set_body) {
    body = set_body;
}

std::string block_statement::inspect() const {
    std::string ret_str = "";
    for (const auto& element : body) {
        ret_str += element->inspect() + ", "; }
    if (body.size() > 0) {
        ret_str = ret_str.substr(0, ret_str.size() - 2); }
    return ret_str;
}
object_type block_statement::type() const {
    return BLOCK_STATEMENT; }
std::string block_statement::data() const {
    return "BLOCK_STATEMENT"; }

// end_if_statement
std::string end_if_statement::inspect() const {
    return "END_IF_STATEMENT"; }
object_type end_if_statement::type() const {
    return END_IF_STATEMENT; }
std::string end_if_statement::data() const {
    return "END_IF_STATEMENT"; }

// end_statement

std::string end_statement::inspect() const {
    return "END_STATEMENT"; }
object_type end_statement::type() const {
    return END_STATEMENT; }
std::string end_statement::data() const {
    return "END_STATEMENT"; }

// return_statement
return_statement::return_statement(object* expr) {
    expression = expr;
}

std::string return_statement::inspect() const {
    std::string ret_string = "Return:\n";
    ret_string += expression->inspect();
    return ret_string; 
}
object_type return_statement::type() const {
    return RETURN_STATEMENT; }
std::string return_statement::data() const {
    return "RETURN_STATEMENT"; }

