
// objects are made in the parser and should probably stay there, used to parse and return values from expressions
    // i.e (10 + 10) will return an integer_object with the value 20

#include "object.h"

#include "token.h"
#include "helpers.h"

#include <string>
#include <span>
#include <vector>


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

// return_value_object
return_value_object::return_value_object(object* val) {
    value = val;
}
std::string return_value_object::inspect() const {
    return "Returning: " + value->inspect();
}
object_type return_value_object::type() const {
    return RETURN_VALUE_OBJ;
}
std::string return_value_object::data() const {
    return value->data();
}

// argument_object
argument_object::argument_object(std::string set_name, object* val) {
    name = set_name;
    value = val;
}
std::string argument_object::inspect() const {
    return "Name: " + name + ", Value: " + value->inspect();
}
object_type argument_object::type() const {
    return ARGUMENT_OBJ;
}
std::string argument_object::data() const {
    return name;
}

// variable_object
variable_object::variable_object(std::string set_name, object* val) {
    name = set_name;
    value = val;
}
std::string variable_object::inspect() const {
    return "[Type: Variable, Name: " + name + ", Value: " + value->inspect() + "]";
}
object_type variable_object::type() const {
    return VARIABLE_OBJ;
}
std::string variable_object::data() const {
    return name;
}

// boolean_object
boolean_object::boolean_object(bool val) {
    value = val;
}
std::string boolean_object::inspect() const {
    if (value) {
        return "TRUE";
    }
    return "FALSE";
}
object_type boolean_object::type() const {
    return BOOLEAN_OBJ;
}
std::string boolean_object::data() const {
    if (value) {
        return "TRUE";
    }
    return "FALSE";
}

//zerofill is implicitly unsigned im pretty sure
// SQL_data_type_object
SQL_data_type_object::SQL_data_type_object() {
    prefix = NULL_TOKEN;
    data_type = NULL_TOKEN;
    parameter = new null_object();
}

std::string SQL_data_type_object::inspect() const {
    std::string ret_str = "[Type: SQL data type, Data type: ";
    if (prefix != NONE) {
        ret_str += token_type_to_string(prefix);}
        
    ret_str += token_type_to_string(data_type);
    if (parameter->type() != NULL_OBJ) {
        ret_str += "(" + parameter->inspect() + ")"; }

    ret_str += "]";
    return ret_str;
}
object_type SQL_data_type_object::type() const {
    return SQL_DATA_TYPE_OBJ;
}
std::string SQL_data_type_object::data() const {
    return parameter->data();
}

// parameter_object
parameter_object::parameter_object(std::string set_name, SQL_data_type_object* set_data_type) {
    name = set_name;
    data_type = set_data_type;
}

std::string parameter_object::inspect() const {
    std::string ret_str = "[Type: Parameter, Name: " + name + ", " + data_type->inspect() + "]";
    return ret_str;
}
object_type parameter_object::type() const {
    return PARAMETER_OBJ;
}
std::string parameter_object::data() const {
    return data_type->data();
}

// function_object
function_object::function_object() {
    name = {};
    parameters = {};
    return_type = new SQL_data_type_object();
    expressions = {};
}

std::string function_object::inspect() const {
    std::string ret_str = "Function name: " + name + "\n";
    ret_str += "Arguments: (";
    for (const auto& arg : parameters) {
        ret_str += arg->inspect() + ", "; 
    }

    if (parameters.size() > 0) {
        ret_str = ret_str.substr(0, ret_str.size() - 2); }

    ret_str += ") ";
    ret_str += "\nReturn type: " + return_type->inspect() + "\n";
    ret_str += "Body:\n";
    for (const auto& expr : expressions) {
        ret_str += expr->inspect() + "\n"; 
    }
    return ret_str;
}
object_type function_object::type() const {
    return FUNCTION_OBJ;
}
std::string function_object::data() const {
    return "FUNCTION_OBJECT";
}

// evaluted_function_object
evaluated_function_object::evaluated_function_object(function_object* func, std::vector<parameter_object*> new_parameters) {
    name = func->name;
    parameters = new_parameters;
    return_type = func->return_type;
    expressions = func->expressions;
}

std::string evaluated_function_object::inspect() const {
    std::string ret_str = "Function name: " + name + "\n";
    ret_str += "Arguments: (";
    for (const auto& arg : parameters) {
        ret_str += arg->inspect() + ", "; 
    }

    if (parameters.size() > 0) {
        ret_str = ret_str.substr(0, ret_str.size() - 2); }

    ret_str += ")";
    ret_str += "\nReturn type: " + return_type->inspect() + "\n";
    ret_str += "Body:\n";
    for (const auto& expr : expressions) {
        ret_str += expr->inspect() + "\n"; 
    }
    return ret_str;
}
object_type evaluated_function_object::type() const {
    return EVALUATED_FUNCTION_OBJ;
}
std::string evaluated_function_object::data() const {
    return "EVALUATED_FUNCTION_OBJ";
}

// function_call_object
function_call_object::function_call_object(std::string set_name, std::vector<object*> args) {
    name = set_name;
    arguments = args;
}

std::string function_call_object::inspect() const {
    std::string ret_str = name + "(";
    for (const auto& arg : arguments) {
        ret_str += arg->inspect() + ", "; }
    if (arguments.size() > 0) {
        ret_str = ret_str.substr(0, ret_str.size() - 2);
    }
    ret_str += ")";
    return ret_str;
}
object_type function_call_object::type() const {
    return FUNCTION_CALL_OBJ;
}
std::string function_call_object::data() const {
    return "FUNCTION_CALL_OBJ";
}


// column_object
column_object::column_object(std::string set_name, object* type, object* default_val) {
    name = set_name;
    data_type = type;
    default_value = default_val;
}

column_object::column_object(std::string set_name, object* type) {
    name = set_name;
    data_type = type;
    default_value = new null_object();
}

std::string column_object::inspect() const {
    std::string str = "[Column name: " + name + "], ";
    str += data_type->inspect();
    str += ", [Default: " + default_value->inspect()  + "]\n";
    return str;
}

object_type column_object::type() const {
    return COLUMN_OBJ;
}

std::string column_object::data() const {
    return "COLUMN_OBJ";
}

// evaluated_column_object
evaluated_column_object::evaluated_column_object(std::string set_name, SQL_data_type_object* type, std::string default_val) {
    name = set_name;
    data_type = type;
    default_value = default_val;
}
std::string evaluated_column_object::inspect() const {
    std::string str = "[Column name: " + name + "], ";
    str += data_type->inspect();
    str += ", [Default: " + default_value  + "]\n";
    return str;
}
object_type evaluated_column_object::type() const {
    return EVALUATED_COLUMN_OBJ;
}
std::string evaluated_column_object::data() const {
    return "EVALUATED_COLUMN_OBJ";
}

// error_object
error_object::error_object() {
    value = {};
}

error_object::error_object(std::string val) {
    value = val;
}

std::string error_object::inspect() const {
    return "ERROR: " + value;
}
object_type error_object::type() const {
    return ERROR_OBJ;
}
std::string error_object::data() const {
    return value;
}

// semicolon_object
std::string semicolon_object::inspect() const {
    return "SEMICOLON_OBJ";
}
object_type semicolon_object::type() const {
    return SEMICOLON_OBJ;
}
std::string semicolon_object::data() const {
    return "SEMICOLON_OBJ";
}

// group_object
group_object::group_object(std::vector<object*> objs) {
    elements = objs;
}
std::string group_object::inspect() const {
    std::string ret_str = "";
    for (const auto& element : elements) {
        ret_str += element->inspect() + ", ";}
    if (elements.size() > 0) {
        ret_str = ret_str.substr(0, ret_str.length() - 2); }
    return ret_str;
}
object_type group_object::type() const {
    return GROUP_OBJ;
}
std::string group_object::data() const {
    return "GROUP_OBJ";
}


// Statements
// if_statement
if_statement::if_statement() {
    other = new null_object();
}

std::string if_statement::inspect() const {
    std::string ret_str = "IF (" + condition->inspect() + ") THEN \n";

    ret_str += "  " + body->inspect(); // Should add a loop that adds a "  " after every newline

    if (other->type() == IF_STATEMENT) {
        ret_str += "ELSE " + other->inspect();
    } else {
        ret_str += "ELSE \n  " + other->inspect();
    }

    return ret_str;
}
object_type if_statement::type() const {
    return IF_STATEMENT;
}
std::string if_statement::data() const {
    return "IF_STATEMENT";
}

// block_statement
block_statement::block_statement() {
    body = {};
}
// block_statement
block_statement::block_statement(std::vector<object*> set_body) {
    body = set_body;
}

std::string block_statement::inspect() const {
    std::string ret_str = "";
    for (const auto& element : body) {
        ret_str += element->inspect() + "\n"; }
    return ret_str;
}

object_type block_statement::type() const {
    return BLOCK_STATEMENT; 

}
std::string block_statement::data() const {
    return "BLOCK_STATEMENT"; 
}

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
return_statement::return_statement() {
    expression = new null_object();
}
// return_statement
return_statement::return_statement(object* expr) {
    expression = expr;
}

std::string return_statement::inspect() const {
    std::string ret_string = "[Type: Return statement, Value: ";
    ret_string += expression->inspect() + "]";
    return ret_string; 
}

object_type return_statement::type() const {
    return RETURN_STATEMENT; }
std::string return_statement::data() const {
    return "RETURN_STATEMENT"; }

