module;
// objects are made in the parser and should probably stay there, used to parse and return values from expressions
// i.e (10 + 10) will return an integer_object with the value 20

#include "pch.h"

#include "allocator_aliases.h"
#include "token.h"
#include "structs_and_macros.h" // For table

#include <array> 
#include <span>

import helpers;

module object; 



main_alloc<object> object::object_allocator_alias;

static std::span<const char* const> object_type_span() {
    static constexpr std::array object_type_chars = {
        "ERROR_OBJ", "NULL_OBJ", "INFIX_EXPRESSION_OBJ", "PREFIX_EXPRESSION_OBJ", "INTEGER_OBJ", "INDEX_OBJ", "DECIMAL_OBJ", "STRING_OBJ", "SQL_DATA_TYPE_OBJ",
        "COLUMN_OBJ", "EVALUATED_COLUMN_OBJ", "FUNCTION_OBJ", "OPERATOR_OBJ", "SEMICOLON_OBJ", "RETURN_VALUE_OBJ", "ARGUMENT_OBJ", "BOOLEAN_OBJ", "FUNCTION_CALL_OBJ",
        "GROUP_OBJ", "PARAMETER_OBJ", "EVALUATED_FUNCTION_OBJ", "VARIABLE_OBJ", "TABLE_DETAIL_OBJECT", "COLUMN_INDEX_OBJECT", "TABLE_INFO_OBJECT", "TABLE_OBJECT",
        "STAR_OBJECT", "TABLE_AGGREGATE_OBJECT", "COLUMN_VALUES_OBJ",

        "IF_STATEMENT", "BLOCK_STATEMENT", "END_IF_STATEMENT", "END_STATEMENT", "RETURN_STATEMENT",

        "INSERT_INTO_OBJECT", "SELECT_OBJECT", "SELECT_FROM_OBJECT",

        "E_COLUMN_INDEX_OBJECT", "EXPRESSION_STATEMENT", "E_SQL_DATA_TYPE_OBJ", "E_GROUP_OBJ", "E_FUNCTION_CALL_OBJ",
        "E_ARGUMENT_OBJ", "E_VARIABLE_OBJ", "E_PARAMETER_OBJ", "E_BLOCK_STATEMENT", "E_TABLE_DETAIL_OBJECT", "E_RETURN_STATEMENT", "E_SELECT_FROM_OBJECT",
        "E_INFIX_EXPRESSION_OBJ", "E_PREFIX_EXPRESSION_OBJ", "E_INSERT_INTO_OBJECT",


        "ASSERT_OBJ",
    };
    return object_type_chars;
}

astring object_type_to_astring(object_type index) {
    return {object_type_span()[index]};
}

static std::span<const char* const> operator_type_span() {
    static constexpr std::array operator_type_to_string = {
        "ADD_OP", "SUB_OP", "MUL_OP", "DIV_OP", "NEGATE_OP",
        "EQUALS_OP", "NOT_EQUALS_OP", "LESS_THAN_OP", "LESS_THAN_OR_EQUAL_TO_OP", "GREATER_THAN_OP", "GREATER_THAN_OR_EQUAL_TO_OP",
        "OPEN_PAREN_OP", "OPEN_BRACKET_OP", "AS_OP", "LEFT_JOIN_OP", "WHERE_OP", "GROUP_BY_OP", "ORDER_BY_OP", "ON_OP",
        "DOT_OP", "NULL_OP"
    };
    return operator_type_to_string;
}
astring operator_type_to_astring(operator_type index) {
    return operator_type_span()[index];
}



// null_object
astring null_object::inspect() const {
    return "NULL_OBJECT";
}
object_type null_object::type() const {
    return NULL_OBJ;
}
astring null_object::data() const {
    return "NULL_OBJECT";
}
null_object* null_object::clone() const {
    return new null_object();
}

// operator_object
operator_object::operator_object(operator_type type) : 
    op_type(type) 
{}
astring operator_object::inspect() const {
    return operator_type_to_astring(op_type); 
}
object_type operator_object::type() const {
    return OPERATOR_OBJ;
}
astring operator_object::data() const {
    return operator_type_to_astring(op_type);
}
operator_object* operator_object::clone() const {
    return new operator_object(op_type);
}

// Table Info Object
table_info_object::table_info_object(table_object* set_tab, avec<size_t> set_col_ids, avec<size_t> set_row_ids) : 
    col_ids(set_col_ids), 
    row_ids(set_row_ids) {
    tab = SP<table_object>(set_tab);
}
table_info_object::table_info_object(SP<table_object> set_tab, avec<size_t> set_col_ids, avec<size_t> set_row_ids) : 
    tab(set_tab), 
    col_ids(set_col_ids), 
    row_ids(set_row_ids) 
{}
astring table_info_object::inspect() const {
    astringstream stream;
    stream << "Table name: " << tab->table_name << "\n";

    stream << "Row ids: \n";
    bool first = true;
    for (const auto& row_id : row_ids) {
        if (!first) { stream << ", "; }
        stream << row_id;
        first = false;
    }

    stream << "\nColumn ids: \n";
    first = true;
    for (const auto& col_id : col_ids) {
        if (!first) { stream << ", "; }
        stream << col_id;
        first = false;
    }

    return stream.str();
}
object_type table_info_object::type() const {
    return TABLE_INFO_OBJECT;
}
astring table_info_object::data() const {
    return "TABLE_INFO_OBJECT";
}
table_info_object* table_info_object::clone() const {
    avec<size_t> cloned_col;
    cloned_col.reserve(col_ids.size());
    
    for (const auto& col_id : col_ids) {
        cloned_col.push_back(col_id); }
        
    avec<size_t> cloned_row;
    cloned_row.reserve(row_ids.size());
    
    for (const auto& row_id : row_ids) {
        cloned_row.push_back(row_id); }

    return new table_info_object(tab->clone(), cloned_col, cloned_row);
}

// infix_expression_object
infix_expression_object::infix_expression_object(operator_object* set_op, object* set_left, object* set_right) {
    op    = UP<operator_object>(set_op);
    left  = UP<object>(set_left);
    right = UP<object>(set_right);
}
infix_expression_object::infix_expression_object(UP<operator_object> set_op, UP<object> set_left, UP<object> set_right) : 
    op(std::move(set_op)), 
    left(std::move(set_left)), 
    right(std::move(set_right)) 
{}
astring infix_expression_object::inspect() const {
    astringstream stream;
    stream << "[Op: " + op->inspect();
    stream << ". Left: " + left->inspect();
    stream << ". Right: " + right->inspect() + "]";
    return stream.str();
}
object_type infix_expression_object::type() const {
    return INFIX_EXPRESSION_OBJ;
}
astring infix_expression_object::data() const {
    return "INFIX_EXPRESSION_OBJ";
}
infix_expression_object* infix_expression_object::clone() const {
    return new infix_expression_object(op->clone(), left->clone(), right->clone());
}
operator_type infix_expression_object::get_op_type() const {
    return op->op_type;
}

// e_infix_expression_object
e_infix_expression_object::e_infix_expression_object(operator_object* set_op, evaluated* set_left, evaluated* set_right) {
    op    = UP<operator_object>(set_op);
    left  = UP<evaluated>(set_left);
    right = UP<evaluated>(set_right);
}
e_infix_expression_object::e_infix_expression_object(UP<operator_object> set_op, UP<evaluated> set_left, UP<evaluated> set_right) : 
    op(std::move(set_op)), 
    left(std::move(set_left)), 
    right(std::move(set_right)) 
{}
astring e_infix_expression_object::inspect() const {
    astringstream stream;
    stream << "[Op: " + op->inspect();
    stream << ". Left: " + left->inspect();
    stream << ". Right: " + right->inspect() + "]";
    return stream.str();
}
object_type e_infix_expression_object::type() const {
    return E_INFIX_EXPRESSION_OBJ;
}
astring e_infix_expression_object::data() const {
    return "E_INFIX_EXPRESSION_OBJ";
}
e_infix_expression_object* e_infix_expression_object::clone() const {
    return new e_infix_expression_object(op->clone(), left->clone(), right->clone());
}
operator_type e_infix_expression_object::get_op_type() const {
    return op->op_type;
}

// prefix_expression_object
prefix_expression_object::prefix_expression_object(operator_object* set_op, object* set_right){
    op    = UP<operator_object>(set_op);
    right = UP<object>(set_right);
}
prefix_expression_object::prefix_expression_object(UP<operator_object> set_op, UP<object> set_right) : 
    op(std::move(set_op)), 
    right(std::move(set_right))
{}
astring prefix_expression_object::inspect() const {
    astringstream stream;
    stream << "[Op: " + op->inspect();
    stream << ". Right: " + right->inspect() + "]";
    return stream.str();
}
object_type prefix_expression_object::type() const {
    return PREFIX_EXPRESSION_OBJ;
}
astring prefix_expression_object::data() const {
    return astring("PREFIX_EXPRESSION_OBJ");
}
prefix_expression_object* prefix_expression_object::clone() const {
    return new prefix_expression_object(op->clone(), right->clone());
}
operator_type prefix_expression_object::get_op_type() const {
    return op->op_type;
}

// e_prefix_expression_object
e_prefix_expression_object::e_prefix_expression_object(operator_object* set_op, evaluated* set_right){
    op    = UP<operator_object>(set_op);
    right = UP<evaluated>(set_right);
}
e_prefix_expression_object::e_prefix_expression_object(UP<operator_object> set_op, UP<evaluated> set_right) : 
    op(std::move(set_op)), 
    right(std::move(set_right))
{}
astring e_prefix_expression_object::inspect() const {
    astringstream stream;
    stream << "[Op: " + op->inspect();
    stream << ". Right: " + right->inspect() + "]";
    return stream.str();
}
object_type e_prefix_expression_object::type() const {
    return E_PREFIX_EXPRESSION_OBJ;
}
astring e_prefix_expression_object::data() const {
    return "E_PREFIX_EXPRESSION_OBJ";
}
e_prefix_expression_object* e_prefix_expression_object::clone() const {
    return new e_prefix_expression_object(op->clone(), right->clone());
}
operator_type e_prefix_expression_object::get_op_type() const {
    return op->op_type;
}

// integer_object
integer_object::integer_object() : value(0) {
}
integer_object::integer_object(int val) : value(val) {
}
integer_object::integer_object(const std::string& val) : value(std::stoi(val)) {
}
integer_object::integer_object(const astring& val) : value(astring_to_numeric<int>(val)) {
}
astring integer_object::inspect() const {
    return numeric_to_astring<int>(value); 
}
object_type integer_object::type() const {
    return INTEGER_OBJ;
}
astring integer_object::data() const {
    return numeric_to_astring<int>(value); 
}
integer_object* integer_object::clone() const {
    return new integer_object(value);
}

// index_object
index_object::index_object() : 
    value(0) {
}
index_object::index_object(size_t val) : 
    value(val) {
}
index_object::index_object(const std::string& val) : 
    value(std::stoull(val)) {
}
index_object::index_object(const astring& val) : 
    value(astring_to_numeric<size_t>(val)) {
}
astring index_object::inspect() const {
    return numeric_to_astring<size_t>(value); 
}
object_type index_object::type() const {
    return INDEX_OBJ;
}
astring index_object::data() const {
    return numeric_to_astring<size_t>(value); 
}
index_object* index_object::clone() const {
    return new index_object(value);
}

// decimal_object
decimal_object::decimal_object() : 
    value(0) {
}
decimal_object::decimal_object(double val) : 
    value(val) {
}
decimal_object::decimal_object(const std::string& val) : 
    value(std::stod(val)) {
}
decimal_object::decimal_object(const astring& val) : 
    value(astring_to_numeric<double>(val)) {
}
astring decimal_object::inspect() const {
    return numeric_to_astring<double>(value); 
}
object_type decimal_object::type() const {
    return DECIMAL_OBJ;
}
astring decimal_object::data() const {
    return numeric_to_astring<double>(value); 
}
decimal_object* decimal_object::clone() const {
    return new decimal_object(value);
}


// string_object
string_object::string_object(const std_and_astring_variant& val) {
    visit(val, [&](const auto& unwrapped) {
        value = unwrapped;
    });
}
astring string_object::inspect() const {
    return value;
}
object_type string_object::type() const {
    return STRING_OBJ;
}
astring string_object::data() const {
    return value;
}
string_object* string_object::clone() const {
    return new string_object(value);
}

// return_value_object
return_value_object::return_value_object(object* val) {
     value = UP<object>(val);
}
return_value_object::return_value_object(UP<object> val) : 
    value(std::move(val)) 
{}
astring return_value_object::inspect() const {
    return "Returning: " + value->inspect();
}
object_type return_value_object::type() const {
    return RETURN_VALUE_OBJ;
}
astring return_value_object::data() const {
    return value->data();
}
return_value_object* return_value_object::clone() const {
    return new return_value_object(value->clone());
}

// argument_object
argument_object::argument_object(const std_and_astring_variant& set_name, object* val) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });
    
    value = UP<object>(val);
}
argument_object::argument_object(const std_and_astring_variant& set_name, UP<object> val) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });

    value = std::move(val);
}
astring argument_object::inspect() const {
    return "Name: " + name + ", Value: " + value->inspect();
}
object_type argument_object::type() const {
    return ARGUMENT_OBJ;
}
astring argument_object::data() const {
    return name;
}
argument_object* argument_object::clone() const {
    return new argument_object(name, value->clone());
}

// e_argument_object
e_argument_object::e_argument_object(const std_and_astring_variant& set_name, evaluated* val) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });
    
    value = UP<evaluated>(val);
}
e_argument_object::e_argument_object(const std_and_astring_variant& set_name, UP<evaluated> val) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });
    
    value = std::move(val);
}
astring e_argument_object::inspect() const {
    return "Name: " + name + ", Value: " + value->inspect();
}
object_type e_argument_object::type() const {
    return E_ARGUMENT_OBJ;
}
astring e_argument_object::data() const {
    return name;
}
e_argument_object* e_argument_object::clone() const {
    return new e_argument_object(name, value->clone());
}

// variable_object
variable_object::variable_object(const std_and_astring_variant& set_name, object* val) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });

    value = UP<object>(val);
}
variable_object::variable_object(const std_and_astring_variant& set_name, UP<object> val) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });

    value = std::move(val);
}
astring variable_object::inspect() const {
    return "[Type: Variable, Name: " + name + ", Value: " + value->inspect() + "]";
}
object_type variable_object::type() const {
    return VARIABLE_OBJ;
}
astring variable_object::data() const {
    return name;
}
variable_object* variable_object::clone() const {
    return new variable_object(name, value->clone());
}

// e_variable_object
e_variable_object::e_variable_object(const std_and_astring_variant& set_name, evaluated* val) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });

    value = UP<evaluated>(val);
}
e_variable_object::e_variable_object(const std_and_astring_variant& set_name, UP<evaluated> val) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });

    value = std::move(val);
}
astring e_variable_object::inspect() const {
    return "[Type: Variable, Name: " + name + ", Value: " + value->inspect() + "]";
}
object_type e_variable_object::type() const {
    return E_VARIABLE_OBJ;
}
astring e_variable_object::data() const {
    return name;
}
e_variable_object* e_variable_object::clone() const {
    return new e_variable_object(name, value->clone());
}

// boolean_object
boolean_object::boolean_object(bool val) : 
    value(val) 
{}
astring boolean_object::inspect() const {
    if (value) {
        return "TRUE"; }
    return "FALSE";
}
object_type boolean_object::type() const {
    return BOOLEAN_OBJ;
}
astring boolean_object::data() const {
    if (value) {
        return "TRUE"; }
    return "FALSE";
}
boolean_object* boolean_object::clone() const {
    return new boolean_object(value);
}

//zerofill is implicitly unsigned im pretty sure
// SQL_data_type_object
SQL_data_type_object::SQL_data_type_object(token_type set_prefix, token_type set_data_type, object* set_parameter) {
    if (set_data_type == NONE) {
        std::cout << "!!!ERROR!!! SQL data type object constructed without data type token\n"; }

    prefix = set_prefix;
    data_type = set_data_type;

    parameter = UP<object>(set_parameter);
}
SQL_data_type_object::SQL_data_type_object(token_type set_prefix, token_type set_data_type, UP<object> set_parameter) {
    if (set_data_type == NONE) {
        std::cout << "!!!ERROR!!! SQL data type object constructed without data type token\n"; }

    prefix = set_prefix;
    data_type = set_data_type;

    parameter = std::move(set_parameter);
}
astring SQL_data_type_object::inspect() const {
    astringstream stream;
    stream << "[Type: SQL data type, Data type: ";

    if (prefix != NONE) {
        stream << token_type_to_string(prefix) << ", "; }
        
    stream << token_type_to_string(data_type);
    
    if (parameter->type() != NULL_OBJ) {
        stream << "(" + parameter->inspect() + ")"; }

    stream << "]";
    return stream.str();
}
object_type SQL_data_type_object::type() const {
    return SQL_DATA_TYPE_OBJ;
}
astring SQL_data_type_object::data() const {
    return parameter->data();
}
SQL_data_type_object* SQL_data_type_object::clone() const {
    return new SQL_data_type_object(prefix, data_type, parameter->clone());
}

//zerofill is implicitly unsigned im pretty sure
// SQL_data_type_object
e_SQL_data_type_object::e_SQL_data_type_object(token_type set_prefix, token_type set_data_type, evaluated* set_parameter) 
    : prefix(set_prefix), parameter(UP<evaluated>(set_parameter)) 
{
    if (set_data_type == NONE) {
        std::cout << "!!!ERROR!!! e SQL data type object constructed without data type token\n"; }

    data_type = set_data_type;
}
e_SQL_data_type_object::e_SQL_data_type_object(token_type set_prefix, token_type set_data_type, UP<evaluated> set_parameter) 
    : prefix(set_prefix), parameter(std::move(set_parameter))  
{
    if (set_data_type == NONE) {
        std::cout << "!!!ERROR!!! e SQL data type object constructed without data type token\n"; }

    data_type = set_data_type;
}
astring e_SQL_data_type_object::inspect() const {
    astringstream stream;
    stream << "[Type: E_SQL data type, Data type: ";

    if (prefix != NONE) {
        stream << token_type_to_string(prefix) << ", "; }
        
    stream << token_type_to_string(data_type);

    if (parameter->type() != NULL_OBJ) {
        stream << "(" + parameter->inspect() + ")"; }

    stream << "]";
    return stream.str();
}
object_type e_SQL_data_type_object::type() const {
    return E_SQL_DATA_TYPE_OBJ;
}
astring e_SQL_data_type_object::data() const {
    return parameter->data();
}
e_SQL_data_type_object* e_SQL_data_type_object::clone() const {
    return new e_SQL_data_type_object(prefix, data_type, parameter->clone());
}
astring e_SQL_data_type_object::serialize() const {
    astringstream stream;
    if (prefix != NONE) {
        stream << token_type_to_string(prefix) << " "; }
    stream << token_type_to_string(data_type) << ", ";
    stream << parameter->inspect(); //!!MAJOR LAZY RN
    return stream.str();
}

// parameter_object
parameter_object::parameter_object(const std_and_astring_variant& set_name, SQL_data_type_object* set_data_type) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });

    data_type = UP<SQL_data_type_object>(set_data_type);
}
parameter_object::parameter_object(const std_and_astring_variant& set_name, UP<SQL_data_type_object> set_data_type) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });

    data_type = std::move(set_data_type);
}
astring parameter_object::inspect() const {
    return "[Type: Parameter, Name: " + name + ", " + data_type->inspect() + "]";
}
object_type parameter_object::type() const {
    return PARAMETER_OBJ;
}
astring parameter_object::data() const {
    return data_type->data();
}
parameter_object* parameter_object::clone() const {
    return new parameter_object(name, data_type->clone());
}

// e_parameter_object
e_parameter_object::e_parameter_object(const std_and_astring_variant& set_name, e_SQL_data_type_object* set_data_type) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });

    data_type = UP<e_SQL_data_type_object>(set_data_type);
}
e_parameter_object::e_parameter_object(const std_and_astring_variant& set_name, UP<e_SQL_data_type_object> set_data_type) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });

    data_type = std::move(set_data_type);
}
astring e_parameter_object::inspect() const {
    return "[Type: Parameter, Name: " + name + ", " + data_type->inspect() + "]";
}
object_type e_parameter_object::type() const {
    return E_PARAMETER_OBJ;
}
astring e_parameter_object::data() const {
    return data_type->data();
}
e_parameter_object* e_parameter_object::clone() const {
    return new e_parameter_object(name, data_type->clone());
}

// table_detail_object
table_detail_object::table_detail_object(const std_and_astring_variant& set_name, SQL_data_type_object* set_data_type, object* set_default_value) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });
    
    data_type     = UP<SQL_data_type_object>(set_data_type);
    default_value = UP<object>(set_default_value);
}
table_detail_object::table_detail_object(const std_and_astring_variant& set_name, UP<SQL_data_type_object> set_data_type, UP<object> set_default_value) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });
    
    data_type     = std::move(set_data_type);
    default_value = std::move(set_default_value);
}
astring table_detail_object::inspect() const {
    astringstream stream;
    stream << "[Type: Table detail, Name: " + name + ", " + data_type->inspect();
    stream << ", Default value: ";
    if (default_value->type() != NULL_OBJ) {
        stream << default_value->inspect(); }
    stream << "]";
    return stream.str();
}
object_type table_detail_object::type() const {
    return TABLE_DETAIL_OBJECT;
}
astring table_detail_object::data() const {
    return name.data();
}
table_detail_object* table_detail_object::clone() const {
    return new table_detail_object(name, data_type->clone(), default_value->clone());
}

// e_table_detail_object
e_table_detail_object::e_table_detail_object(const std_and_astring_variant& set_name, e_SQL_data_type_object* set_data_type, evaluated* set_default_value) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });
    
    data_type     = UP<e_SQL_data_type_object>(set_data_type);
    default_value = UP<evaluated>(set_default_value);
}
e_table_detail_object::e_table_detail_object(const std_and_astring_variant& set_name, UP<e_SQL_data_type_object> set_data_type, UP<evaluated> set_default_value) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });
    
    data_type     = std::move(set_data_type);
    default_value = std::move(set_default_value);
}
astring e_table_detail_object::inspect() const {
    astringstream stream;
    stream << "[Type: E Table detail, Name: " + name + ", " + data_type->inspect();
    stream << ", Default value: ";
    if (default_value->type() != NULL_OBJ) {
        stream << default_value->inspect(); }
    stream << "]";
    return stream.str();
}
object_type e_table_detail_object::type() const {
    return E_TABLE_DETAIL_OBJECT;
}
astring e_table_detail_object::data() const {
    return name.data();
}
e_table_detail_object* e_table_detail_object::clone() const {
    return new e_table_detail_object(name, data_type->clone(), default_value->clone());
}
astring e_table_detail_object::serialize() const {
    astringstream stream;
    stream << name << ", ";
    stream << data_type->serialize() << ", ";
    stream << default_value->inspect(); // !!MAJOR LAZY RN
    return stream.str();
}

// group_object
group_object::group_object(object* obj) { 
    elements.push_back(UP<object>(obj));
}
group_object::group_object(UP<object> obj) { 
    elements.push_back(std::move(obj));
}
group_object::group_object(avec<UP<object>>&& objs) : 
    elements(std::move(objs))  
{}
astring group_object::inspect() const {
    astringstream stream;
    bool first = true;
    for (const auto& element : elements) {
        if (!first) { stream << ", "; }
        stream << element->inspect();
        first = false;
    }
    return stream.str();
}
object_type group_object::type() const {
    return GROUP_OBJ;
}
astring group_object::data() const {
    return "GROUP_OBJ";
}
group_object* group_object::clone() const {
    avec<UP<object>> cloned_elements;
    cloned_elements.reserve(elements.size());
    
    for (const auto& element : elements) {
        cloned_elements.push_back(UP<object>(element->clone())); }
    
    return new group_object(std::move(cloned_elements));
}

// e_group_object
e_group_object::e_group_object(evaluated* obj) { 
    elements.push_back(UP<evaluated>(obj));
}
e_group_object::e_group_object(UP<evaluated> obj) { 
    elements.push_back(std::move(obj));
}
e_group_object::e_group_object(avec<UP<evaluated>>&& objs) : 
    elements(std::move(objs))  
{}
astring e_group_object::inspect() const {
    astringstream stream;
    bool first = true;
    for (const auto& element : elements) {
        if (!first) { stream << ", "; }
        stream << element->inspect();
        first = false;
    }
    return stream.str();
}
object_type e_group_object::type() const {
    return E_GROUP_OBJ;
}
astring e_group_object::data() const {
    return "E_GROUP_OBJ";
}
e_group_object* e_group_object::clone() const {
    avec<UP<evaluated>> cloned_elements;
    cloned_elements.reserve(elements.size());
    
    for (const auto& element : elements) {
        cloned_elements.push_back(UP<evaluated>(element->clone())); }
    
    return new e_group_object(std::move(cloned_elements));
}

// function_object
function_object::function_object(const std_and_astring_variant& set_name, group_object* set_parameters, SQL_data_type_object* set_return_type, block_statement* set_body) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });

    parameters  = UP<group_object>(set_parameters);
    body        = UP<block_statement>(set_body);
    return_type = UP<SQL_data_type_object>(set_return_type);
}
function_object::function_object(const std_and_astring_variant& set_name, UP<group_object> set_parameters, UP<SQL_data_type_object> set_return_type, UP<block_statement> set_body) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });

    parameters  = std::move(set_parameters);
    body        = std::move(set_body);
    return_type = std::move(set_return_type);
}
astring function_object::inspect() const {
    astringstream stream;
    stream << "Function name: " + name + "\n";
    stream << "Parameters: (";
    stream << parameters->inspect() << ") ";

    stream << "\nReturn type: " + return_type->inspect() + "\n";
    stream << "Body:\n";
    stream << body->inspect();

    return stream.str();
}
object_type function_object::type() const {
    return FUNCTION_OBJ;
}
astring function_object::data() const {
    return "FUNCTION_OBJECT";
}
function_object* function_object::clone() const {
    return new function_object(name, parameters->clone(), return_type->clone(), body->clone());
}

// evaluted_function_object
evaluated_function_object::evaluated_function_object(const std_and_astring_variant& set_name, avec<UP<e_parameter_object>>&& set_parameters, 
                                                     e_SQL_data_type_object* set_return_type, e_block_statement* set_body)
    : parameters(std::move(set_parameters))
{
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });

    return_type = UP<e_SQL_data_type_object>(set_return_type);
    body        = UP<e_block_statement>(set_body);
}
evaluated_function_object::evaluated_function_object(const std_and_astring_variant& set_name, avec<UP<e_parameter_object>>&& set_parameters, 
                                                     UP<e_SQL_data_type_object> set_return_type, UP<e_block_statement> set_body) 
    : parameters(std::move(set_parameters)), return_type(std::move(set_return_type)), body(std::move(set_body))
{
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });
}
astring evaluated_function_object::inspect() const {
    astringstream stream;
    stream << "Function name: " << name << "\n";

    stream << "Arguments: (";
    bool first = true;
    for (const auto& param : parameters) {
        if (!first) { stream << ", "; }
        stream << param->inspect();
        first = false;
    }
    stream << ")";

    stream << "\nReturn type: " + return_type->inspect() + "\n";
    stream << "Body:\n";
    stream << body->inspect();

    return stream.str();
}
object_type evaluated_function_object::type() const {
    return EVALUATED_FUNCTION_OBJ;
}
astring evaluated_function_object::data() const {
    return "EVALUATED_FUNCTION_OBJ";
}
evaluated_function_object* evaluated_function_object::clone() const {
    avec<UP<e_parameter_object>> cloned_params;
    cloned_params.reserve(parameters.size());
    
    for (const auto& param : parameters) {
        cloned_params.push_back(UP<e_parameter_object>(param->clone())); }

    return new evaluated_function_object(name, std::move(cloned_params), return_type->clone(), body->clone());
}

// function_call_object
function_call_object::function_call_object(const std_and_astring_variant& set_name, group_object* args) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });

    arguments = UP<group_object>(args);
}
function_call_object::function_call_object(const std_and_astring_variant& set_name, UP<group_object> args) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });

    arguments = std::move(args);
}
astring function_call_object::inspect() const {
    astringstream stream;
    stream << name + "(";
    stream << arguments->inspect();
    stream << ")";
    return stream.str();
}
object_type function_call_object::type() const {
    return FUNCTION_CALL_OBJ;
}
astring function_call_object::data() const {
    return "FUNCTION_CALL_OBJ";
}
function_call_object* function_call_object::clone() const {
    return new function_call_object(name, arguments->clone());
}

// e_function_call_object
e_function_call_object::e_function_call_object(const std_and_astring_variant& set_name, e_group_object* args) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });

    arguments = UP<e_group_object>(args);
}
e_function_call_object::e_function_call_object(const std_and_astring_variant& set_name, UP<e_group_object> args) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });

    arguments = std::move(args);
}
astring e_function_call_object::inspect() const {
    astringstream stream;
    stream << name + "(";
    stream << arguments->inspect();
    stream << ")";
    return stream.str();
}
object_type e_function_call_object::type() const {
    return E_FUNCTION_CALL_OBJ;
}
astring e_function_call_object::data() const {
    return "E_FUNCTION_CALL_OBJ";
}
e_function_call_object* e_function_call_object::clone() const {
    return new e_function_call_object(name, arguments->clone());
}

// column_object
column_object::column_object(object* set_name_data_type) {
    name_data_type = UP<object>(set_name_data_type);
    default_value = UP<object>(new null_object());
}
column_object::column_object(UP<object> set_name_data_type) : 
    name_data_type(std::move(set_name_data_type)) {
    default_value = UP<object>(new null_object());
}
column_object::column_object(object* set_name_data_type, object* default_val) {
    name_data_type = UP<object>(set_name_data_type);
    default_value  = UP<object>(default_val);
}
column_object::column_object(UP<object> set_name_data_type, UP<object> default_val) : 
    name_data_type(std::move(set_name_data_type)), 
    default_value(std::move(default_val)) 
{}
astring column_object::inspect() const {
    astringstream stream;
    stream << "[Column: ";
    stream << name_data_type->inspect();
    stream << ", Default: " + default_value->inspect()  + "]\n";
    return stream.str();
}
object_type column_object::type() const {
    return COLUMN_OBJ;
}
astring column_object::data() const {
    return "COLUMN_OBJ";
}
column_object* column_object::clone() const {
    return new column_object(name_data_type->clone(), default_value->clone());
}

// column_values_object
column_values_object::column_values_object(avec<UP<evaluated>>&& set_values) : values_wrapper_object(std::move(set_values)) {
}
astring column_values_object::inspect() const {
    astringstream stream;
    stream << "[Column values: ";

    bool first = true;
    for (const auto& val : values) {
        if (!first) { stream << ", "; }
        stream << val->inspect(); 
        first = false;
    }
    stream << "]";
    return stream.str();
}
object_type column_values_object::type() const {
    return COLUMN_VALUES_OBJ;
}
astring column_values_object::data() const {
    return "COLUMN_VALUES_OBJ";
}
column_values_object* column_values_object::clone() const {
    avec<UP<evaluated>> cloned_vals;
    cloned_vals.reserve(values.size());
    
    for (const auto& val : values) {
        cloned_vals.push_back(UP<evaluated>(val->clone())); }

    return new column_values_object(std::move(cloned_vals));
}

// evaluated_column_object
evaluated_column_object::evaluated_column_object(const std_and_astring_variant& set_name, e_SQL_data_type_object* type, evaluated* set_default_value) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });
    
    default_value = UP<evaluated>(set_default_value);
    data_type = UP<e_SQL_data_type_object>(type);
}
evaluated_column_object::evaluated_column_object(const std_and_astring_variant& set_name, UP<e_SQL_data_type_object> type, UP<evaluated> set_default_value) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });
    
    default_value = std::move(set_default_value);
    data_type = std::move(type);
}
astring evaluated_column_object::inspect() const {
    astringstream stream;
    stream << "[Column name: " << name << "], ";
    stream << data_type->inspect();
    stream << ", [Default: " << default_value->inspect() << "]\n";
    return stream.str();
}
object_type evaluated_column_object::type() const {
    return EVALUATED_COLUMN_OBJ;
}
astring evaluated_column_object::data() const {
    return "EVALUATED_COLUMN_OBJ";
}
evaluated_column_object* evaluated_column_object::clone() const {
    return new evaluated_column_object(name, data_type->clone(), default_value->clone());
}

// error_object
error_object::error_object() {
    value = astring();
}
error_object::error_object(const std_and_astring_variant& val) {
    visit(val, [&](const auto& unwrapped) {
        value = unwrapped;
    });
}
astring error_object::inspect() const {
    return "ERROR: " + value;
}
object_type error_object::type() const {
    return ERROR_OBJ;
}
astring error_object::data() const {
    return value;
}
error_object* error_object::clone() const {
    return new error_object(value);
}

// semicolon_object
astring semicolon_object::inspect() const {
    return "SEMICOLON_OBJ";
}
object_type semicolon_object::type() const {
    return SEMICOLON_OBJ;
}
astring semicolon_object::data() const {
    return "SEMICOLON_OBJ";
}
semicolon_object* semicolon_object::clone() const {
    return new semicolon_object();
}

// Star Object
astring star_object::inspect() const {
    return "STAR_OBJECT";
}
object_type star_object::type() const {
    return STAR_OBJECT;
}
astring star_object::data() const {
    return "STAR_OBJECT";
}
star_object* star_object::clone() const {
    return new star_object();
}

// Column index object
column_index_object::column_index_object(object* set_table_name, object* set_column_name) {
    table_name  = UP<object>(set_table_name);
    column_name = UP<object>(set_column_name);
}
column_index_object::column_index_object(UP<object> set_table_name, UP<object> set_column_name) : 
    table_name(std::move(set_table_name)), 
    column_name(std::move(set_column_name)) 
{}
astring column_index_object::inspect() const {
    return "[Table alias: " + table_name->inspect() + ", Column alias: " + column_name->inspect() + "]";
}
object_type column_index_object::type() const {
    return COLUMN_INDEX_OBJECT;
}
astring column_index_object::data() const {
    return "COLUMN_INDEX_OBJECT";
}
column_index_object* column_index_object::clone() const {
    return new column_index_object(table_name->clone(), column_name->clone());
}

// Evaluated column index object
e_column_index_object::e_column_index_object(table_object* set_table, index_object* set_column_index) {
    table        = SP<table_object>(set_table);
    index = UP<index_object>(set_column_index);
}
e_column_index_object::e_column_index_object(SP<table_object> set_table, UP<index_object> set_column_index) : 
    table(std::move(set_table)), 
    index(std::move(set_column_index)) 
{}
astring e_column_index_object::inspect() const {
    return "[Table: " + table->inspect() + ", Column index: " + index->inspect() + "]";
}
object_type e_column_index_object::type() const {
    return E_COLUMN_INDEX_OBJECT;
}
astring e_column_index_object::data() const {
    return "E_COLUMN_INDEX_OBJECT";
}
e_column_index_object* e_column_index_object::clone() const {
    return new e_column_index_object(table->clone(), index->clone());
}



// Table object, only use heap
table_object::table_object(const std_and_astring_variant& set_table_name, e_table_detail_object* set_column_datas, e_group_object* set_rows) {
    visit(set_table_name, [&](const auto& unwrapped) {
        table_name = unwrapped;
    });

    column_datas.push_back(UP<e_table_detail_object>(set_column_datas)); 
    rows.push_back(UP<e_group_object>(set_rows)); 
}
table_object::table_object(const std_and_astring_variant& set_table_name, UP<e_table_detail_object> set_column_datas, UP<e_group_object> set_rows) {
    visit(set_table_name, [&](const auto& unwrapped) {
        table_name = unwrapped;
    });

    column_datas.push_back(std::move(set_column_datas)); 
    rows.push_back(std::move(set_rows)); 
}
table_object::table_object(const std_and_astring_variant& set_table_name, avec<UP<e_table_detail_object>>&& set_column_datas) : 
    column_datas(std::move(set_column_datas)) {
    visit(set_table_name, [&](const auto& unwrapped) {
        table_name = unwrapped;
    });
}
table_object::table_object(const std_and_astring_variant& set_table_name, avec<UP<e_table_detail_object>>&& set_column_datas, avec<UP<e_group_object>>&& set_rows) {
    visit(set_table_name, [&](const auto& unwrapped) {
        table_name = unwrapped;
    });

    column_datas = std::move(set_column_datas);
    rows = std::move(set_rows);
}
astring table_object::inspect() const {
    astringstream stream;
    stream << "Table name: " << table_name  << "\n";

    for (const auto& detail : column_datas) {
        if (detail == nullptr) {
            std::cout << "bruh, " << std::source_location::current().line() << std::endl; }
    }



    stream << "Column data (" << column_datas.size() << "): ";
    bool first = true;
    for (const auto& col : column_datas) {
        if (!first) { stream << ", "; }
        stream << col->inspect();
        first = false;
    }
    
    stream << "\nRows (" << rows.size() << "): \n";
    for (const auto& roh : rows) {
        stream << roh->inspect() << "\n"; }

    return stream.str();
}
object_type table_object::type() const {
    return TABLE_OBJECT;
}
astring table_object::data() const {
    return "TABLE_OBJECT";
}
table_object* table_object::clone() const {
    avec<UP<e_group_object>> cloned_rows;
    cloned_rows.reserve(rows.size());
    
    for (const auto& row : rows) {
        cloned_rows.push_back(UP<e_group_object>(row->clone()));
    }

    avec<UP<e_table_detail_object>> cloned_cols;
    cloned_cols.reserve(column_datas.size());
    
    for (const auto& col_data : column_datas) {
        cloned_cols.push_back(UP<e_table_detail_object>(col_data->clone()));
    }
    
    return new table_object(table_name, std::move(cloned_cols), std::move(cloned_rows));
}
std::pair<const avec<evaluated*>&, bool> table_object::get_const_column(size_t index) const { //!!Expensive, only use if have to, else just use alias

    auto column = avec<evaluated*>();

    if (index >= column_datas.size()) {
        return {column, false}; }
    
    column.reserve(rows.size());
    for (const auto& row : rows) {
        column.push_back(row->elements[index].get()); }

    return {column, true};
}
std::pair<const avec<evaluated*>&, bool> table_object::get_const_column(const std_and_astring_variant& col_name) const {
    size_t index = 0;
    bool ok = false;
    visit(col_name, [&](const auto& unwrapped) {
        const auto& [i, o] = get_column_index(unwrapped);
        index = i;
        ok = o;
    });
    
    if (!ok) {
        avec<evaluated*> fail;
        return {fail, false}; 
    }

    return get_const_column(index);
}
std::pair<astring, bool> table_object::get_column_name(size_t index) const{
    if (index >= column_datas.size()) {
        return {"Column index out of bounds", false}; }
    return {column_datas[index]->name, true};
}
std::pair<UP<e_SQL_data_type_object>, bool> table_object::get_column_data_type(size_t index) const{
    if (index >= column_datas.size()) {
        return {nullptr, false}; }
    
    auto* dt = column_datas[index]->data_type->clone(); //!!MAJOR might no_stack value might cause issues
    return {UP<e_SQL_data_type_object>(dt), true};
}
std::pair<size_t, bool> table_object::get_column_index(const std_and_astring_variant& name) const {
    astring unwrapped_name;
    visit(name, [&](const auto& unwrapped) {
        unwrapped_name = unwrapped;
    });

    for (size_t i = 0; i < column_datas.size(); i++) {
        if (column_datas[i]->name == unwrapped_name) {
            return {i, true};
        }
    }

    return {0, false};
}
std::expected<UP<evaluated>, UP<error_object>> table_object::get_cloned_column_default_value(size_t index) const {
    if (index >= column_datas.size()) {
        return std::unexpected(MAKE_UP(error_object, "Column index out of bounds")); }

    return UP<evaluated>(column_datas[index]->default_value->clone()); //!!MAJOR might no_stack value might cause issues
}
std::pair<UP<evaluated>, bool> table_object::get_cell_value(size_t row_index, size_t col_index) const {
    if (row_index >= rows.size()) {
        return {UP<evaluated>(new error_object("Row index out of bounds")), false};}

    if (col_index >= column_datas.size()) {
        return {UP<evaluated>(new error_object("Column index out of bounds")), false};}

    e_group_object* roh = rows[row_index].get();
    
    return {UP<evaluated>(roh->elements[col_index]->clone()), true}; //!!MAJOR might no_stack value might cause issues
}
std::expected<avec<UP<evaluated>>*, UP<error_object>> table_object::get_row_vec_ptr(size_t index) const {
    if (index >= rows.size()) {
        return std::unexpected(MAKE_UP(error_object, "Row index out of bounds"));}

    const auto& row = rows[index];
    return &row->elements;
}
avec<size_t> table_object::get_row_ids() const {
    avec<size_t> row_ids(rows.size());
    std::iota(row_ids.begin(), row_ids.end(), 0); // Don't change
    return row_ids;
}
bool table_object::check_if_field_name_exists(const std_and_astring_variant& name) const {
    astring unwrapped_name;
    visit(name, [&](const auto& unwrapped) {
        unwrapped_name = unwrapped;
    });

    for (const auto& col_data : column_datas) {
        if (col_data->name == unwrapped_name) {
            return true; }
    }
    return false;
}
astring table_object::get_tab_name() const {
    return table_name;
}

// Table Aggregate Object
// Not sure I should modify the object or use constructor to create a new one using the old one 
                                                            // i.e. tabble_aggregate_object(table_aggregate_object* old, table_object* table)...
table_aggregate_object::table_aggregate_object() = default;
table_aggregate_object::table_aggregate_object(avec<SP<table_object>>&& set_tables) : tables(std::move(set_tables)) {}
astring table_aggregate_object::inspect() const {
    astringstream stream;
    stream <<"Contained tables:\n";

    bool first = true;
    for (const auto& table : tables) {
        if (!first) { stream << ", "; }
        stream << table->table_name; 
        first = false;
    }

    return stream.str(); 
}
object_type table_aggregate_object::type() const {
    return TABLE_AGGREGATE_OBJECT;
}
astring table_aggregate_object::data() const {
    return "TABLE_AGGREGATE_OBJECT";
}
table_aggregate_object* table_aggregate_object::clone() const {
    avec<SP<table_object>> cloned_tabs;
    cloned_tabs.reserve(tables.size());
    
    for (const auto& tab : tables) {
        cloned_tabs.push_back(SP<table_object>(tab->clone())); }

    return new table_aggregate_object(std::move(cloned_tabs));
}
std::expected<size_t, UP<error_object>> table_aggregate_object::get_col_id(const std_and_astring_variant& column_name) const {

    astring unwrapped_col_name;
    visit(column_name, [&](const auto& unwrapped) {
        unwrapped_col_name = unwrapped;
    });

    bool found_col = false;
    size_t id = 0;
    for (const auto& table : tables) {
        for (const auto& col_data : table->column_datas) {
            if (col_data->name == unwrapped_col_name) {
                found_col = true;
                break;
            }
            id++;
        }
    }

    if (!found_col) {
        return std::unexpected(MAKE_UP(error_object, "SELECT FROM: Column index failed to find column (" + unwrapped_col_name + ")")); 
    }

    return id;
}
std::expected<size_t, UP<error_object>> table_aggregate_object::get_col_id(const std_and_astring_variant& table_name, size_t index) const {

    astring unwrapped_table_name;
    visit(table_name, [&](const auto& unwrapped) {
        unwrapped_table_name = unwrapped;
    });

    bool found_table = false;
    size_t id = 0;
    for (const auto& table : tables) {
        if (table->table_name == unwrapped_table_name) {
            found_table = true;
            if (index >= table->column_datas.size()) {
                astringstream stream;
                stream << "Index (" << index << ") out of bounds for (" << unwrapped_table_name << ")";
                return std::unexpected(MAKE_UP(error_object, stream.str())); 
            }
            id += index;
            break;
        } else if (table->column_datas.size() > 0) {
            id += table->column_datas.size() - 1;
        }
    }

    if (!found_table) {
        return std::unexpected(MAKE_UP(error_object, "Failed to find table (" + unwrapped_table_name + ")")); }

    return id;
}
std::expected<size_t, UP<error_object>> table_aggregate_object::get_col_id(const std_and_astring_variant& table_name, const std_and_astring_variant& column_name) const {

    astring unwrapped_table_name;
    visit(table_name, [&](const auto& unwrapped) {
        unwrapped_table_name = unwrapped;
    });
    
    astring unwrapped_col_name;
    visit(column_name, [&](const auto& unwrapped) {
        unwrapped_col_name = unwrapped;
    });

    bool found_table = false;
    bool found_col = false;
    size_t id = 0;
    for (const auto& table : tables) {
        if (table->table_name == unwrapped_table_name) {
            found_table = true;
            for (const auto& col_data : table->column_datas) {
                if (col_data->name == unwrapped_col_name) {
                    found_col = true;
                    break;
                }

                id += 1;
            }
            break;
        } else if (table->column_datas.size() > 0) {
            id += table->column_datas.size() - 1;
        }
    }

    if (!found_table) {
        return std::unexpected(MAKE_UP(error_object, "Failed to find table (" + unwrapped_table_name + ")")); 
    }

    if (!found_col) {
        return std::unexpected(MAKE_UP(error_object, "Failed to find column (" + unwrapped_col_name + ")")); 
    }

    return id;
}
avec<size_t> table_aggregate_object::get_all_col_ids() const {

    size_t total_size = 0;
    for (const auto& table : tables) {
        total_size += table->column_datas.size(); 
    }
    avec<size_t> col_ids;
    col_ids.reserve(total_size); 

    size_t cur_id = 0;
    for (const auto& table : tables) {
        for (size_t i = 0; i < table->column_datas.size(); i++) {
            col_ids.push_back(cur_id++);
        }
    }

    return col_ids;
}
std::pair<size_t, bool> table_aggregate_object::get_last_col_id() const {
 
    bool has_added = false;
    bool prev_added = false;
    size_t count = 0;
    for (const auto& table : tables) {
        size_t tab_size = table->column_datas.size();
        if (tab_size > 0) {
            count += tab_size - 1;
            has_added = true;
            if (prev_added) {
                count += 1;
            }
            prev_added = true;
        } else {
            prev_added = false;
        }

    }

    return {count, has_added};
}
std::pair<astring, bool> table_aggregate_object::get_table_name(size_t index) const {
    if (index >= tables.size()) {
        return {"", false}; }
    return {tables[index]->table_name, true};
}
std::pair<SP<table_object>, bool> table_aggregate_object::get_table(size_t index) const {
    if (index >= tables.size()) {
        return {nullptr, false}; }
    return {tables[index], true};
}

// For now just copies elements,
// Should create a tabble_aggregate_view object like
/*

class table_aggregate_view : virtual public evaluated {
    public:
    table_aggregate_view();
    ~table_aggregate_view() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] table_aggregate_view* clone() const override;

    public:
    avec<UP<table_info_object> table_views;
};

And table_views will contain info for each table, and it will be combined by the configure_print function.
Therefore no copies (or null objects I think) needed.

*/
SP<table_object> table_aggregate_object::combine_tables(const std_and_astring_variant& name) const {

    astring final_name;
    visit(name, [&](const auto& unwrapped) {
        final_name = unwrapped;
    });

    size_t total_columns = 0;
    size_t max_rows = 0;
    size_t max_cols = 0;
    for (const auto& table : tables) {
        total_columns += table->column_datas.size();
        max_rows = std::max(max_rows, table->rows.size());
        max_cols = std::max(max_cols, table->column_datas.size());
    }

    avec<UP<e_table_detail_object>> column_datas;
    column_datas.reserve(total_columns);

    for (const auto& table : tables) {
        for (auto& col_data : table->column_datas) {
            column_datas.push_back(UP<e_table_detail_object>(col_data->clone()));
        }
    }

    avec<UP<e_group_object>> rows;
    rows.reserve(max_rows);
    for (size_t row_index = 0; row_index < max_rows; row_index++) {

        avec<UP<evaluated>> new_row;
        new_row.reserve(max_cols);
        for (const auto& table : tables) {
            
            // Fill in empty rows
            if (row_index >= table->rows.size()) {
                for (size_t col_index = 0; col_index < table->column_datas.size(); col_index++) {
                    new_row.emplace_back(new null_object()); 
                }
                continue; 
            }
            
            const auto result = table->get_row_vec_ptr(row_index);
            if (!result.has_value()) {
                return MAKE_SP(table_object, "Weird index bug", nullptr, nullptr); }

            const auto& row = **result;
                
            for (const auto& col_index : row) {
                new_row.push_back(UP<evaluated>(col_index->clone())); 
            }
        }   

        rows.emplace_back(MAKE_UP(e_group_object, std::move(new_row)));
    }

    return MAKE_SP(table_object, final_name, std::move(column_datas), std::move(rows));
}
void table_aggregate_object::add_table(table_object* table) {
    tables.push_back(UP<table_object>(table));
}
void table_aggregate_object::add_table(const SP<table_object>& table) {
    tables.push_back(table);
}

// Node objects
// Insert into object
insert_into_object::insert_into_object(const std_and_astring_variant& set_table_name, avec<UP<object>>&& set_fields, object* set_values) : 
    fields(std::move(set_fields)) {
    visit(set_table_name, [&](const auto& unwrapped) {
        table_name = unwrapped;
    });
    values = UP<object>(set_values);
}
insert_into_object::insert_into_object(const std_and_astring_variant& set_table_name, avec<UP<object>>&& set_fields, UP<object> set_values) : 
    fields(std::move(set_fields)), 
    values(std::move(set_values)) {
    visit(set_table_name, [&](const auto& unwrapped) {
        table_name = unwrapped;
    });
}
astring insert_into_object::inspect() const {
    astringstream stream;

    stream << "Insert Into: ";
    stream << table_name + "\n";

    stream << "[Fields: ";
    bool first = true;
    for (const auto& field : fields) {
        if (!first) { stream << ", "; }
        stream << field->inspect(); 
        first = false;
    }

    stream << "], [Values: ";
    stream << values->inspect();

    stream << "]\n";
    return stream.str();
}
object_type insert_into_object::type() const {
    return INSERT_INTO_OBJECT; 
}
astring insert_into_object::data() const {
    return "INSERT_INTO_OBJECT";
}
insert_into_object* insert_into_object::clone() const {
    avec<UP<object>> cloned_fields;
    cloned_fields.reserve(fields.size());
    
    for (const auto& field : fields) {
        cloned_fields.push_back(UP<object>(field->clone())); }

    return new insert_into_object(table_name, std::move(cloned_fields), values->clone());
}

// E Insert into object
e_insert_into_object::e_insert_into_object(astring set_table_name, avec<UP<evaluated>>&& set_fields, avec<UP<evaluated>>&& set_values) : 
    table_name(set_table_name), 
    fields(std::move(set_fields)), 
    values(std::move(set_values)) {
    
}
astring e_insert_into_object::inspect() const {
    astringstream stream;

    stream << "E Insert Into: ";
    stream << table_name + "\n";

    stream << "[Fields: ";
    bool first = true;
    for (const auto& field : fields) {
        if (!first) { stream << ", "; }
        stream << field->inspect(); 
        first = false;
    }

    stream << "], [Values: ";
    first = true;
    for (const auto& value : values) {
        if (!first) { stream << ", "; }
        stream << value->inspect(); 
        first = false;
    }

    stream << "]\n";
    return stream.str();
}
object_type e_insert_into_object::type() const {
    return E_INSERT_INTO_OBJECT; 
}
astring e_insert_into_object::data() const {
    return "E_INSERT_INTO_OBJECT";
}
e_insert_into_object* e_insert_into_object::clone() const {
    avec<UP<evaluated>> cloned_fields;
    cloned_fields.reserve(fields.size());
    for (const auto& field : fields) {
        cloned_fields.push_back(UP<evaluated>(field->clone())); }

    avec<UP<evaluated>> cloned_values;
    cloned_values.reserve(values.size());
    for (const auto& value : values) {
        cloned_values.push_back(UP<evaluated>(value->clone())); }

    return new e_insert_into_object(table_name, std::move(cloned_fields), std::move(cloned_values));
}

// Select object
select_object::select_object(object* set_value) {
    value = UP<object>(set_value);
}
select_object::select_object(UP<object> set_value) : 
    value(std::move(set_value)) 
{}
astring select_object::inspect() const {
    return "select: " + value->inspect();
}
object_type select_object::type() const {
    return SELECT_OBJECT;
}
astring select_object::data() const {
    return "SELECT_OBJECT";
}
select_object* select_object::clone() const {
    return new select_object(value->clone());
}

// Select from object
select_from_object::select_from_object(avec<UP<object>>&& set_column_indexes, avec<UP<object>>&& set_clause_chain) : 
    column_indexes(std::move(set_column_indexes)), 
    clause_chain(std::move(set_clause_chain)) 
{}
astring select_from_object::inspect() const {
    astringstream stream;
    stream << "Select From: \n";

    stream << "Column indexes: \n";
    bool first = true;
    for (const auto& col_index : column_indexes) {
        if (!first) { stream << ", ";}
        stream << col_index->inspect();
        first = false;
    }


    if (clause_chain.size() == 1) {
        stream << "\nClause: ";
    } else if (clause_chain.size() > 1) {
        stream << "\nClauses: \n"; }
    for (const auto& cond : clause_chain) {
        stream << cond->inspect() << "\n";
    }
    
    return stream.str();
}
object_type select_from_object::type() const {
    return SELECT_FROM_OBJECT;
}
astring select_from_object::data() const {
    return "SELECT_FROM_OBJECT";
}
select_from_object* select_from_object::clone() const {
    avec<UP<object>> cloned_indexes;
    cloned_indexes.reserve(column_indexes.size());
    
    for (const auto& col_index : column_indexes) {
        cloned_indexes.push_back(UP<object>(col_index->clone())); }
    
    avec<UP<object>> cloned_clauses;
    cloned_clauses.reserve(clause_chain.size());
    
    for (const auto& clause : clause_chain) {
        cloned_clauses.push_back(UP<object>(clause->clone())); }

    return new select_from_object(std::move(cloned_indexes), std::move(cloned_clauses));
}

// e_Select from object
e_select_from_object::e_select_from_object(avec<UP<evaluated>>&& set_column_indexes, avec<UP<evaluated>>&& set_clause_chain) : 
    column_indexes(std::move(set_column_indexes)), 
    clause_chain(std::move(set_clause_chain)) 
{}
astring e_select_from_object::inspect() const {
    astringstream stream;
    stream << "E Select From: \n";

    stream << "Column indexes: \n";
    bool first = true;
    for (const auto& col_index : column_indexes) {
        if (!first) { stream << ", ";}
        stream << col_index->inspect();
        first = false;
    }


    if (clause_chain.size() == 1) {
        stream << "\nClause: ";
    } else if (clause_chain.size() > 1) {
        stream << "\nClauses: \n"; }
    for (const auto& cond : clause_chain) {
        stream << cond->inspect() << "\n";
    }
    
    return stream.str();
}
object_type e_select_from_object::type() const {
    return E_SELECT_FROM_OBJECT;
}
astring e_select_from_object::data() const {
    return "E_SELECT_FROM_OBJECT";
}
e_select_from_object* e_select_from_object::clone() const {

    avec<UP<evaluated>> cloned_indexes;
    cloned_indexes.reserve(column_indexes.size());
    for (const auto& col_index : column_indexes) {
        cloned_indexes.push_back(UP<evaluated>(col_index->clone())); }
    
    avec<UP<evaluated>> cloned_clauses;
    cloned_clauses.reserve(clause_chain.size());
    for (const auto& clause : clause_chain) {
        cloned_clauses.push_back(UP<evaluated>(clause->clone())); }

    return new e_select_from_object(std::move(cloned_indexes), std::move(cloned_clauses));
}



// Statements
// block_statement
block_statement::block_statement(avec<UP<object>>&& set_body) : 
    body(std::move(set_body)) 
{}
astring block_statement::inspect() const {
    astringstream stream;
    for (const auto& statement : body) {
        stream << statement->inspect() << "\n";}
    return stream.str();
}
object_type block_statement::type() const {
    return BLOCK_STATEMENT; 
}
astring block_statement::data() const {
    return "BLOCK_STATEMENT"; 
}
block_statement* block_statement::clone() const {
    avec<UP<object>> cloned_body;
    cloned_body.reserve(body.size());
    
    for (const auto& statement : body) {
        cloned_body.push_back(UP<object>(statement->clone())); }

    return new block_statement(std::move(cloned_body));
}

// e_block_statement
e_block_statement::e_block_statement(avec<UP<evaluated>>&& set_body) : 
    body(std::move(set_body)) 
{}
astring e_block_statement::inspect() const {
    astringstream stream;
    for (const auto& statement : body) {
        stream << statement->inspect() << "\n";}
    return stream.str();
}
object_type e_block_statement::type() const {
    return E_BLOCK_STATEMENT; 
}
astring e_block_statement::data() const {
    return "E_BLOCK_STATEMENT"; 
}
e_block_statement* e_block_statement::clone() const {
    avec<UP<evaluated>> cloned_body;
    cloned_body.reserve(body.size());
    
    for (const auto& statement : body) {
        cloned_body.push_back(UP<evaluated>(statement->clone())); }

    return new e_block_statement(std::move(cloned_body));
}

// Expression Statement
expression_statement::expression_statement(avec<UP<evaluated>>&& set_body, e_return_statement* set_ret_val) : 
    body(std::move(set_body)) {
    ret_val = UP<e_return_statement>(set_ret_val);
}
expression_statement::expression_statement(avec<UP<evaluated>>&& set_body, UP<e_return_statement> set_ret_val) : 
    body(std::move(set_body)), 
    ret_val(std::move(set_ret_val)) 
{}
astring expression_statement::inspect() const {
    astringstream stream;
    stream << "[Body:\n";
    for (const auto& statement : body) {
        stream << statement->inspect() << "\n";}
    stream << ", Return value: " << ret_val->inspect() << "]\n";
    return stream.str();
}
object_type expression_statement::type() const {
    return EXPRESSION_STATEMENT; 
}
astring expression_statement::data() const {
    return "EXPRESSION_STATEMENT"; 
}
expression_statement* expression_statement::clone() const {
    avec<UP<evaluated>> cloned_body;
    cloned_body.reserve(body.size());
    
    for (const auto& statement : body) {
        cloned_body.push_back(UP<evaluated>(statement->clone())); }

    return new expression_statement(std::move(cloned_body), ret_val->clone());
}

// if_statement
if_statement::if_statement(object* set_condition, block_statement* set_body, object* set_other) {
    condition = UP<object>(set_condition);
    body      = UP<block_statement>(set_body);
    other     = UP<object>(set_other);
}
if_statement::if_statement(UP<object> set_condition, UP<block_statement> set_body, UP<object> set_other) : 
    condition(std::move(set_condition)), 
    body(std::move(set_body)), 
    other(std::move(set_other)) 
{}
astring if_statement::inspect() const {
    astringstream stream;
    stream << "IF (" + condition->inspect() + ") THEN \n";

    stream << "  " + body->inspect(); // Should add a loop that adds a "  " after every newline

    if (other->type() == IF_STATEMENT) {
        stream << "ELSE " + other->inspect();
    } else {
        stream << "ELSE \n  " + other->inspect();
    }

    return stream.str();
}
object_type if_statement::type() const {
    return IF_STATEMENT;
}
astring if_statement::data() const {
    return "IF_STATEMENT";
}
if_statement* if_statement::clone() const {
    return new if_statement(condition->clone(), body->clone(), other->clone());
}

// end_if_statement
astring end_if_statement::inspect() const {
    return "END_IF_STATEMENT"; 
}
object_type end_if_statement::type() const {
    return END_IF_STATEMENT; 
}
astring end_if_statement::data() const {
    return "END_IF_STATEMENT"; 
}
end_if_statement* end_if_statement::clone() const {
    return new end_if_statement();
}

// end_statement
astring end_statement::inspect() const {
    return "END_STATEMENT"; 
}
object_type end_statement::type() const {
    return END_STATEMENT; 
}
astring end_statement::data() const {
    return "END_STATEMENT"; 
}
end_statement* end_statement::clone() const {
    return new end_statement();
}

// return_statement
return_statement::return_statement(object* expr) {
    expression = UP<object>(expr);
}
return_statement::return_statement(UP<object> expr) : 
    expression(std::move(expr)) 
{}
astring return_statement::inspect() const {
    astringstream stream;
    stream << "[Type: Return statement, Value: ";
    stream << expression->inspect() + "]";
    return stream.str(); 
}
object_type return_statement::type() const {
    return RETURN_STATEMENT; 
}
astring return_statement::data() const {
    return "RETURN_STATEMENT"; 
}
return_statement* return_statement::clone() const {
    return new return_statement(expression->clone());
}

// e_return_statement
e_return_statement::e_return_statement(evaluated* expr) {
    expression = UP<evaluated>(expr);
}
e_return_statement::e_return_statement(UP<evaluated> expr) : 
    expression(std::move(expr)) 
{}
astring e_return_statement::inspect() const {
    astringstream stream;
    stream << "[Type: E Return statement, Value: ";
    stream << expression->inspect() + "]";
    return stream.str(); 
}
object_type e_return_statement::type() const {
    return E_RETURN_STATEMENT; 
}
astring e_return_statement::data() const {
    return "E_RETURN_STATEMENT"; 
}
e_return_statement* e_return_statement::clone() const {
    return new e_return_statement(expression->clone());
}



// CUSTOM

// assert_object
assert_object::assert_object(object* expr, size_t set_line) : 
    line(set_line) {
    expression = UP<object>(expr);
}
assert_object::assert_object(UP<object> expr, size_t set_line) : 
    line(set_line), 
    expression(std::move(expr)) 
{}
astring assert_object::inspect() const {
    return expression->inspect(); 
}
object_type assert_object::type() const {
    return ASSERT_OBJ; 
}
astring assert_object::data() const {
    return "ASSERT_OBJ"; 
}
assert_object* assert_object::clone() const {
    return new assert_object(expression->clone(), line);
}

