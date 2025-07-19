
// objects are made in the parser and should probably stay there, used to parse and return values from expressions
// i.e (10 + 10) will return an integer_object with the value 20

#include "pch.h"

#include "object.h"

#include "allocator_aliases.h"
#include "token.h"
#include "macros.h"



main_alloc<object> object::object_allocator_alias;

static std::span<const char* const> object_type_span() {
    static constexpr std::array object_type_chars = {
        "ERROR_OBJ", "NULL_OBJ", "INFIX_EXPRESSION_OBJ", "PREFIX_EXPRESSION_OBJ", "INTEGER_OBJ", "TIMESTAMP_OBJ", "INDEX_OBJ", "DECIMAL_OBJ", "STRING_OBJ",
        "OPERATOR_OBJ", "SEMICOLON_OBJ", "RETURN_VALUE_OBJ", "BOOLEAN_OBJ", "AUTO_INCREMENT_OBJ", "CURRENT_TIMESTAMP_OBJ", "PRIMARY_KEY_OBJ", "DELIMITER_OBJ",
        "TABLE_OBJ",
        "STAR_OBJ", "TABLE_AGGREGATE_OBJ",

        "IF_STATEMENT", "END_IF_STATEMENT", "END_STATEMENT", "RETURN_STATEMENT",

        "INSERT_INTO_OBJ", "SELECT_OBJ", "SELECT_FROM_OBJ",


        
        "COLUMN_OBJ",        "E_COLUMN_OBJ",        "S_COLUMN_OBJ",
        "COLUMN_INDEX_OBJ",  "E_COLUMN_INDEX_OBJ",  "S_COLUMN_INDEX_OBJ", 
        "TABLE_INFO_OBJ",                                               "F_TABLE_INFO_OBJ",
        "TABLE_DETAIL_OBJ",  "E_TABLE_DETAIL_OBJ",  "S_TABLE_DETAIL_OBJ",
        "PARAMETER_OBJ",     "E_PARAMETER_OBJ",     "S_PARAMETER_OBJ", 
        "GROUP_OBJ",         "E_GROUP_OBJ",         "S_GROUP_OBJ",
        "SQL_DATA_TYPE_OBJ", "E_SQL_DATA_TYPE_OBJ", "S_SQL_DATA_TYPE_OBJ",
        "FUNCTION_OBJ",      "E_FUNCTION_OBJ",      "S_FUNCTION_OBJ",
        "FUNCTION_CALL_OBJ",                        "S_FUNCTION_CALL_OBJ",

        "TABLE_EXPR_OBJ",     "E_TABLE_EXPR_OBJ",       "S_TABLE_EXPR_OBJ",
        "TABLE_COLUMN_EXPR_OBJ", "E_TABLE_COLUMN_EXPR_OBJ", "S_TABLE_COLUMN_EXPR_OBJ",
        "CONSTRAINT_OBJ",    "E_CONSTRAINT_OBJ",    "S_CONSTRAINT_OBJ",
        "UNIQUE_OBJ",        "E_UNIQUE_OBJ",        "S_UNIQUE_OBJ",
        "DEFAULT_VALUE_OBJ", "E_DEFAULT_VALUE_OBJ", "S_DEFAULT_VALUE_OBJ",
        "DEFAULT_VALUE_FUNC_OBJ", "E_DEFAULT_VALUE_FUNC_OBJ", "S_DEFAULT_VALUE_FUNC_OBJ",
        "HASH_OBJ",          "E_HASH_OBJ",          "S_HASH_OBJ",
        "FOREIGN_KEY_OBJ",    "E_FOREIGN_KEY_OBJ",      "S_FOREIGN_KEY_OBJ",
        "VARIABLE_OBJ",       "E_VARIABLE_OBJ",         "S_VARIABLE_OBJ",
        "ARGUMENT_OBJ",       "E_ARGUMENT_OBJ",         "S_ARGUMENT_OBJ",

        "BLOCK_STATEMENT",   "E_BLOCK_STATEMENT",   "S_BLOCK_STATEMENT",

        
        "EXPRESSION_STATEMENT",
        "E_RETURN_STATEMENT", "E_SELECT_FROM_OBJ",
        "E_INFIX_EXPRESSION_OBJ", "E_PREFIX_EXPRESSION_OBJ", "E_INSERT_INTO_OBJ",
        
        // CUSTOM
        "ASSERT_OBJ", "E_ASSERT_OBJ"
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
astring      null_object::inspect()   const { return "NULL_OBJ"; }
object_type  null_object::type()      const { return  NULL_OBJ;  }
astring      null_object::data()      const { return "NULL_OBJ"; }
null_object* null_object::clone()     const { return  new null_object(); }
astring      null_object::serialize() const { return  ""; }

// timestamp_object
timestamp_object::timestamp_object() {
    value = static_cast<size_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}
astring           timestamp_object::inspect()   const { return "TIMESTAMP_OBJ"; }
object_type       timestamp_object::type()      const { return  TIMESTAMP_OBJ;  }
astring           timestamp_object::data()      const { return "TIMESTAMP_OBJ"; }
timestamp_object* timestamp_object::clone()     const { return  new timestamp_object(); }
astring           timestamp_object::serialize() const { return  numeric_to_astring<size_t>(value); }

// auto_increment_object
astring                auto_increment_object::inspect()   const { return "AUTO_INCREMENT"; }
object_type            auto_increment_object::type()      const { return  AUTO_INCREMENT_OBJ;  }
astring                auto_increment_object::data()      const { return "AUTO_INCREMENT"; }
auto_increment_object* auto_increment_object::clone()     const { return  new auto_increment_object(); }
astring                auto_increment_object::serialize() const { return "AUTO_INCREMENT"; }

// current_timestamp_object
astring                   current_timestamp_object::inspect()   const { return "CURRENT_TIMESTAMP"; }
object_type               current_timestamp_object::type()      const { return  CURRENT_TIMESTAMP_OBJ;  }
astring                   current_timestamp_object::data()      const { return "CURRENT_TIMESTAMP"; }
current_timestamp_object* current_timestamp_object::clone()     const { return  new current_timestamp_object(); }
astring                   current_timestamp_object::serialize() const { return "CURRENT_TIMESTAMP"; }

// primary_key_object
primary_key_object::primary_key_object(astring set_col_name) : column_name(set_col_name) {}
astring             primary_key_object::inspect()   const { return "PRIMARY_KEY(" + column_name + ")"; }
object_type         primary_key_object::type()      const { return  PRIMARY_KEY_OBJ; }
astring             primary_key_object::data()      const { return "PRIMARY_KEY"; }
primary_key_object* primary_key_object::clone()     const { return  new primary_key_object(column_name); }
astring             primary_key_object::serialize() const { return "PRIMARY_KEY(" + column_name + ")"; }

// delimiter_object
delimiter_object::delimiter_object(astring set_value) : value(set_value) {}
astring           delimiter_object::inspect()   const { return "DELIMITER(" + value + ")"; }
object_type       delimiter_object::type()      const { return  DELIMITER_OBJ; }
astring           delimiter_object::data()      const { return "DELIMITER";    }
delimiter_object* delimiter_object::clone()     const { return  new delimiter_object(value); }
astring           delimiter_object::serialize() const { return "DELIMITER(\"" + value + "\")"; }

// operator_object
operator_object::operator_object(operator_type type) : op_type(type) {}
astring          operator_object::inspect()   const { return operator_type_to_astring(op_type); }
object_type      operator_object::type()      const { return OPERATOR_OBJ; }
astring          operator_object::data()      const { return operator_type_to_astring(op_type); }
operator_object* operator_object::clone()     const { return new operator_object(op_type); }
astring          operator_object::serialize() const { return data(); }


// Table Info Object
table_info_object::table_info_object(const std_and_astring_variant& set_tab_name, avec<size_t> set_col_ids, avec<size_t> set_row_ids)
    : col_ids(set_col_ids), row_ids(set_row_ids) {
    visit(set_tab_name, [&](const auto& unwrapped) {
        table_name = unwrapped;
    });
}
astring table_info_object::inspect() const {
    astringstream stream;
    stream << "Table name: " << table_name << "\n";

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
object_type table_info_object::type() const { return  TABLE_INFO_OBJ;  }
astring     table_info_object::data() const { return "TABLE_INFO_OBJ"; }
table_info_object* table_info_object::clone() const {
    avec<size_t> cloned_col;
    cloned_col.reserve(col_ids.size());
    
    for (const auto& col_id : col_ids) {
        cloned_col.push_back(col_id); }
        
    avec<size_t> cloned_row;
    cloned_row.reserve(row_ids.size());
    
    for (const auto& row_id : row_ids) {
        cloned_row.push_back(row_id); }

    return new table_info_object(table_name, cloned_col, cloned_row);
}

// S Table Info Object
f_table_info_object::f_table_info_object(SP<table_object> set_table, avec<size_t> set_col_ids, avec<size_t> set_row_ids)
    : table(set_table), col_ids(set_col_ids), row_ids(set_row_ids) {}
astring f_table_info_object::inspect() const {
    astringstream stream;
    stream << "Table name: " << table->table_name << "\n";

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
object_type f_table_info_object::type() const { return  F_TABLE_INFO_OBJ;  }
astring f_table_info_object::data()     const { return "F_TABLE_INFO_OBJ"; }
f_table_info_object* f_table_info_object::clone() const {
    avec<size_t> cloned_col;
    cloned_col.reserve(col_ids.size());
    for (const auto& col_id : col_ids) {
        cloned_col.push_back(col_id); }
        
    avec<size_t> cloned_row;
    cloned_row.reserve(row_ids.size());
    for (const auto& row_id : row_ids) {
        cloned_row.push_back(row_id); }

    return new f_table_info_object(table, cloned_col, cloned_row);
}
astring f_table_info_object::serialize() const {
    FATAL_ERROR_STACK_TRACE_THROW("Can not serialize F Table Info Object", CUR_LOC);
    return "";
}

// infix_expr_object
infix_expr_object::infix_expr_object(operator_object* set_op, object* set_left, object* set_right) {
    op    = UP<operator_object>(set_op);
    left  = UP<object>(set_left);
    right = UP<object>(set_right);
}
infix_expr_object::infix_expr_object(UP<operator_object> set_op, UP<object> set_left, UP<object> set_right) : 
    op(std::move(set_op)), 
    left(std::move(set_left)), 
    right(std::move(set_right)) 
{}
astring infix_expr_object::inspect() const {
    astringstream stream;
    stream << "[Op: " + op->inspect();
    stream << ". Left: " + left->inspect();
    stream << ". Right: " + right->inspect() + "]";
    return stream.str();
}
object_type        infix_expr_object::type()        const { return  INFIX_EXPRESSION_OBJ;  }
astring            infix_expr_object::data()        const { return "INFIX_EXPRESSION_OBJ"; }
infix_expr_object* infix_expr_object::clone()       const { return new infix_expr_object(op->clone(), left->clone(), right->clone()); }
operator_type      infix_expr_object::get_op_type() const { return op->op_type; }

// e_infix_expr_object
e_infix_expr_object::e_infix_expr_object(operator_object* set_op, evaluated* set_left, evaluated* set_right) {
    op    = UP<operator_object>(set_op);
    left  = UP<evaluated>(set_left);
    right = UP<evaluated>(set_right);
}
e_infix_expr_object::e_infix_expr_object(UP<operator_object> set_op, UP<evaluated> set_left, UP<evaluated> set_right) : 
    op(std::move(set_op)), 
    left(std::move(set_left)), 
    right(std::move(set_right)) 
{}
astring e_infix_expr_object::inspect() const {
    astringstream stream;
    stream << "[Op: " + op->inspect();
    stream << ". Left: " + left->inspect();
    stream << ". Right: " + right->inspect() + "]";
    return stream.str();
}
object_type          e_infix_expr_object::type()        const { return  E_INFIX_EXPRESSION_OBJ;  }
astring              e_infix_expr_object::data()        const { return "E_INFIX_EXPRESSION_OBJ"; }
e_infix_expr_object* e_infix_expr_object::clone()       const { return new e_infix_expr_object(op->clone(), left->clone(), right->clone()); }
operator_type        e_infix_expr_object::get_op_type() const { return op->op_type; }

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
object_type               prefix_expression_object::type()        const { return  PREFIX_EXPRESSION_OBJ;  }
astring                   prefix_expression_object::data()        const { return "PREFIX_EXPRESSION_OBJ"; }
prefix_expression_object* prefix_expression_object::clone()       const { return new prefix_expression_object(op->clone(), right->clone()); }
operator_type             prefix_expression_object::get_op_type() const { return op->op_type; }

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
object_type                 e_prefix_expression_object::type()        const { return  E_PREFIX_EXPRESSION_OBJ;  }
astring                     e_prefix_expression_object::data()        const { return "E_PREFIX_EXPRESSION_OBJ"; }
e_prefix_expression_object* e_prefix_expression_object::clone()       const { return new e_prefix_expression_object(op->clone(), right->clone()); }
operator_type               e_prefix_expression_object::get_op_type() const { return op->op_type; }

// integer_object
integer_object::integer_object()                       : value(0)   {}
integer_object::integer_object(int val)                : value(val) {}
integer_object::integer_object(const std::string& val) : value(std::stoi(val)) {}
integer_object::integer_object(const astring& val)     : value(astring_to_numeric<int>(val)) {}
astring         integer_object::inspect()   const { return numeric_to_astring<int>(value); }
object_type     integer_object::type()      const { return INTEGER_OBJ;                    }
astring         integer_object::data()      const { return numeric_to_astring<int>(value); }
integer_object* integer_object::clone()     const { return new integer_object(value);  }
astring         integer_object::serialize() const { return data();                         }

// index_object
index_object::index_object() : value(0) {}
index_object::index_object(size_t val) : value(val) {}
index_object::index_object(const std::string& val) : 
    value(std::stoull(val)) {
}
index_object::index_object(const astring& val) : 
    value(astring_to_numeric<size_t>(val)) {
}
astring       index_object::inspect() const { return numeric_to_astring<size_t>(value); }
object_type   index_object::type()    const { return INDEX_OBJ; }
astring       index_object::data()    const { return numeric_to_astring<size_t>(value); }
index_object* index_object::clone()   const { return new index_object(value);       }

// decimal_object
decimal_object::decimal_object()                       : value(0)   {}
decimal_object::decimal_object(double val)             : value(val) {}
decimal_object::decimal_object(const std::string& val) : value(std::stod(val)) {}
decimal_object::decimal_object(const astring& val)     : value(astring_to_numeric<double>(val)) {}
astring         decimal_object::inspect()   const { return numeric_to_astring<double>(value); }
object_type     decimal_object::type()      const { return DECIMAL_OBJ; }
astring         decimal_object::data()      const { return numeric_to_astring<double>(value); }
decimal_object* decimal_object::clone()     const { return new decimal_object(value);     }
astring         decimal_object::serialize() const { return data();      }


// string_object
string_object::string_object(const std_and_astring_variant& val) {
    visit(val, [&](const auto& unwrapped) {
        value = unwrapped;
    });
}
astring        string_object::inspect()   const { return value;      }
object_type    string_object::type()      const { return STRING_OBJ; }
astring        string_object::data()      const { return value;      }
string_object* string_object::clone()     const { return new string_object(value); }
astring        string_object::serialize() const { return value;      }

// return_value_object
return_value_object::return_value_object(object* val)    { value = UP<object>(val); }
return_value_object::return_value_object(UP<object> val) : value(std::move(val))   {}
astring              return_value_object::inspect() const { return "Returning: " + value->inspect(); }
object_type          return_value_object::type()    const { return RETURN_VALUE_OBJ; }
astring              return_value_object::data()    const { return value->data();    }
return_value_object* return_value_object::clone()   const { return new return_value_object(value->clone()); }

// argument_object
argument_object::argument_object(const std_and_astring_variant& set_name, object* val)
    : value(UP<object>(val)) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });
}
argument_object::argument_object(const std_and_astring_variant& set_name, UP<object> val) 
    : value(std::move(val)) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    }); 
}
astring          argument_object::inspect() const { return "Name: " + name + ", Value: " + value->inspect(); }
object_type      argument_object::type()    const { return ARGUMENT_OBJ; }
astring          argument_object::data()    const { return name; }
argument_object* argument_object::clone()   const { return new argument_object(name, value->clone()); }

// e_argument_object
e_argument_object::e_argument_object(const std_and_astring_variant& set_name, evaluated* val) 
    : value(UP<evaluated>(val)) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });
}
e_argument_object::e_argument_object(const std_and_astring_variant& set_name, UP<evaluated> val) 
    : value(std::move(val)) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });
}
astring            e_argument_object::inspect() const { return "Name: " + name + ", Value: " + value->inspect(); }
object_type        e_argument_object::type()    const { return E_ARGUMENT_OBJ; }
astring            e_argument_object::data()    const { return name; }
e_argument_object* e_argument_object::clone()   const { return new e_argument_object(name, value->clone()); }

// s_argument_object
s_argument_object::s_argument_object(const std_and_astring_variant& set_name, serializable* val) 
    : value(UP<serializable>(val)) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });
}
s_argument_object::s_argument_object(const std_and_astring_variant& set_name, UP<serializable> val) 
    : value(std::move(val)) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });
}
astring            s_argument_object::inspect()   const { return "Name: " + name + ", Value: " + value->inspect(); }
object_type        s_argument_object::type()      const { return S_ARGUMENT_OBJ; }
astring            s_argument_object::data()      const { return name; }
s_argument_object* s_argument_object::clone()     const { return new s_argument_object(name, value->clone()); }
astring            s_argument_object::serialize() const { return name + ", " + value->serialize(); }

// variable_object
variable_object::variable_object(const std_and_astring_variant& set_name, object* val) 
    : value(UP<object>(val)) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });
}
variable_object::variable_object(const std_and_astring_variant& set_name, UP<object> val) 
    : value(std::move(val)) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });
}
astring          variable_object::inspect() const { return "[Type: Variable, Name: " + name + ", Value: " + value->inspect() + "]"; }
object_type      variable_object::type()    const { return VARIABLE_OBJ; }
astring          variable_object::data()    const { return name; }
variable_object* variable_object::clone()   const { return new variable_object(name, value->clone()); }

// e_variable_object
e_variable_object::e_variable_object(const std_and_astring_variant& set_name, evaluated* val) 
    : value(UP<evaluated>(val)){
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });
}
e_variable_object::e_variable_object(const std_and_astring_variant& set_name, UP<evaluated> val) 
    : value(std::move(val)) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });
}
astring e_variable_object::inspect() const {
    return "[Type: E Variable, Name: " + name + ", Value: " + value->inspect() + "]";
}
object_type        e_variable_object::type()  const { return E_VARIABLE_OBJ; }
astring            e_variable_object::data()  const { return name; }
e_variable_object* e_variable_object::clone() const { return new e_variable_object(name, value->clone()); }

// s_variable_object
s_variable_object::s_variable_object(const std_and_astring_variant& set_name, serializable* val) 
    : value(UP<serializable>(val)){
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });
}
s_variable_object::s_variable_object(const std_and_astring_variant& set_name, UP<serializable> val) 
    : value(std::move(val)) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });
}
astring s_variable_object::inspect() const {
    return "[Type: S Variable, Name: " + name + ", Value: " + value->inspect() + "]";
}
object_type        s_variable_object::type()      const { return S_VARIABLE_OBJ; }
astring            s_variable_object::data()      const { return name; }
s_variable_object* s_variable_object::clone()     const { return new s_variable_object(name, value->clone()); }
astring            s_variable_object::serialize() const { return name + ", " + value->serialize(); }

// boolean_object
boolean_object::boolean_object(bool val)  : value(val) {}
astring         boolean_object::inspect()   const { return value ? "TRUE" : "FALSE";  }
object_type     boolean_object::type()      const { return BOOLEAN_OBJ; }
astring         boolean_object::data()      const { return value ? "TRUE" : "FALSE";  }
boolean_object* boolean_object::clone()     const { return new boolean_object(value); }
astring         boolean_object::serialize() const { return data(); }

// function_call_object
function_call_object::function_call_object(const std_and_astring_variant& set_name, group_object* args) 
    : arguments(UP<group_object>(args)) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });
}
function_call_object::function_call_object(const std_and_astring_variant& set_name, UP<group_object> args) 
    : arguments(std::move(args)) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });
}
astring function_call_object::inspect() const {
    astringstream stream;
    stream << name;
    stream << "(" << arguments->inspect() << ")";
    return stream.str();
}
object_type           function_call_object::type()  const { return  FUNCTION_CALL_OBJ;  }
astring               function_call_object::data()  const { return "FUNCTION_CALL_OBJ"; }
function_call_object* function_call_object::clone() const { return new function_call_object(name, arguments->clone()); }

// s_function_call_object
s_function_call_object::s_function_call_object(const std_and_astring_variant& set_name, e_group_object* args) 
    : arguments(UP<e_group_object>(args)) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });
}
s_function_call_object::s_function_call_object(const std_and_astring_variant& set_name, UP<e_group_object> args) 
    : arguments(std::move(args)) {
    visit(set_name, [&](const auto& unwrapped) {
        name = unwrapped;
    });
}
astring                 s_function_call_object::inspect() const { return name + "(" + arguments->inspect() + ")"; }
object_type             s_function_call_object::type()    const { return  S_FUNCTION_CALL_OBJ;  }
astring                 s_function_call_object::data()    const { return "S_FUNCTION_CALL_OBJ"; }
s_function_call_object* s_function_call_object::clone()   const { return new s_function_call_object(name, arguments->clone()); }



// error_object
error_object::error_object() { value = astring(); }
error_object::error_object(const std_and_astring_variant& val) {
    visit(val, [&](const auto& unwrapped) {
        value = unwrapped;
    });
}
astring       error_object::inspect() const { return "ERROR: " + value; }
object_type   error_object::type()    const { return ERROR_OBJ; }
astring       error_object::data()    const { return value;     }
error_object* error_object::clone()   const { return new error_object(value); }

// semicolon_object
astring           semicolon_object::inspect() const { return "SEMICOLON_OBJ"; }
object_type       semicolon_object::type()    const { return  SEMICOLON_OBJ;  }
astring           semicolon_object::data()    const { return "SEMICOLON_OBJ"; }
semicolon_object* semicolon_object::clone()   const { return new semicolon_object(); }

// Star Object
astring      star_object::inspect() const { return "STAR_OBJ"; }
object_type  star_object::type()    const { return  STAR_OBJ;  }
astring      star_object::data()    const { return "STAR_OBJ"; }
star_object* star_object::clone()   const { return new star_object(); }



// Table object
table_object::table_object(const std_and_astring_variant& set_table_name, avec<UP<s_table_detail_object>>&& set_column_data, 
                           avec<UP<s_table_expr>>&& set_exprs, avec<UP<s_group_object>>&& set_rows) : 
    column_data(std::move(set_column_data)),
    exprs(std::move(set_exprs)),
    rows(std::move(set_rows)) {
    visit(set_table_name, [&](const auto& unwrapped) {
        table_name = unwrapped;
    });
}
table_object::table_object(const std_and_astring_variant& set_table_name, avec<UP<s_table_detail_object>>&& set_column_data, 
                           avec<UP<s_table_expr>>&& set_exprs, UP<s_group_object> set_rows) : 
    column_data(std::move(set_column_data)),
    exprs(std::move(set_exprs)) {
    visit(set_table_name, [&](const auto& unwrapped) {
        table_name = unwrapped;
    });

    rows.push_back(std::move(set_rows));
}
table_object::table_object(const std_and_astring_variant& set_table_name, UP<s_table_detail_object> set_column_data, 
                           avec<UP<s_table_expr>>&& set_exprs, UP<s_group_object> set_rows) : 
    exprs(std::move(set_exprs)) {
    visit(set_table_name, [&](const auto& unwrapped) {
        table_name = unwrapped;
    });

    column_data.push_back(std::move(set_column_data));
    rows.push_back(std::move(set_rows));
}
table_object::table_object(const std_and_astring_variant& set_table_name, UP<s_table_detail_object> set_column_data, 
                           UP<s_table_expr> set_exprs, UP<s_group_object> set_rows) 
{
    visit(set_table_name, [&](const auto& unwrapped) {
        table_name = unwrapped;
    });

    column_data.push_back(std::move(set_column_data));
    exprs.push_back(std::move(set_exprs));
    rows.push_back(std::move(set_rows));
}
astring table_object::inspect() const {
    astringstream stream;
    stream << "Table name: " << table_name  << "\n";

    for (const auto& detail : column_data) {
        if (detail == nullptr) {
            std::cout << FILE_NAME_STR << ": " << CUR_LOC.function_name() << ". ";
            std::cout << "bruh, " << std::source_location::current().line() << std::endl; }
    }

    stream << "Column data (" << column_data.size() << "): ";
    bool first = true;
    for (const auto& col : column_data) {
        if (!first) { stream << ", "; }
        stream << col->inspect();
        first = false;
    }

    stream << "Table expressions (" << column_data.size() << "): ";
    first = true;
    for (const auto& expr : exprs) {
        if (!first) { stream << ", "; }
        stream << expr->inspect();
        first = false;
    }
    
    stream << "\nRows (" << rows.size() << "): \n";
    for (const auto& roh : rows) {
        stream << roh->inspect() << "\n"; }

    return stream.str();
}
object_type   table_object::type()  const { return  TABLE_OBJ;  }
astring       table_object::data()  const { return "TABLE_OBJ"; }
table_object* table_object::clone() const {

    avec<UP<s_table_detail_object>> cloned_cols;
    cloned_cols.reserve(column_data.size());
    for (const auto& col_data : column_data) {
        cloned_cols.push_back(UP<s_table_detail_object>(col_data->clone()));
    }

    avec<UP<s_table_expr>> cloned_exprs;
    cloned_exprs.reserve(exprs.size());
    for (const auto& expr : exprs) {
        cloned_exprs.push_back(UP<s_table_expr>(expr->clone()));
    }

    avec<UP<s_group_object>> cloned_rows;
    cloned_rows.reserve(rows.size());
    for (const auto& row : rows) {
        cloned_rows.push_back(UP<s_group_object>(row->clone()));
    }
    
    return new table_object(table_name, std::move(cloned_cols), std::move(cloned_exprs), std::move(cloned_rows));
}
astring table_object::serialize() const {
    astringstream stream;
    stream << "\n" << table_name << "\n";

    for (const auto& detail : column_data) {
        if (detail == nullptr) {
            std::cout << FILE_NAME_STR << ": " << CUR_LOC.function_name() << ". ";
            std::cout << "bruh, " << std::source_location::current().line() << std::endl; }
    }

    stream << "Columns " << column_data.size() << ":\n\t";
    bool first = true;
    for (const auto& col : column_data) {
        if (!first) { stream << "|"; }
        stream << col->serialize();
        first = false;
    }

    stream << "\nExpressions " << exprs.size() << ":\n\t";
    first = true;
    for (const auto& expr : exprs) {
        if (!first) { stream << "|"; }
        stream << expr->serialize();
        first = false; 
    }
    
    stream << "\nRows " << rows.size() << ":";
    for (const auto& row : rows) {
        stream << "\n\t";
        first = true;
        for (const auto& element : row->elements) {
            if (!first) { stream << "|"; }
            stream << element->serialize();
            first = false; 
        }
    }
    stream << "\n";

    return stream.str();
}
std::pair<const avec<serializable*>&, bool> table_object::get_const_column(size_t index) const { //!!Expensive, only use if have to, else just use alias

    auto column = avec<serializable*>();

    if (index >= column_data.size()) {
        return {column, false}; }
    
    column.reserve(rows.size());
    for (const auto& row : rows) {
        column.push_back(row->elements[index].get()); }

    return {column, true};
}
std::pair<const avec<serializable*>&, bool> table_object::get_const_column(const std_and_astring_variant& col_name) const {
    size_t index = 0;
    bool ok = false;
    visit(col_name, [&](const auto& unwrapped) {
        const auto& [i, o] = get_column_index(unwrapped);
        index = i;
        ok = o;
    });
    
    if (!ok) {
        avec<serializable*> fail;
        return {fail, false}; 
    }

    return get_const_column(index);
}
std::pair<astring, bool> table_object::get_column_name(size_t index) const{
    if (index >= column_data.size()) {
        return {"Column index out of bounds", false}; }
    return {column_data[index]->name, true};
}
std::pair<UP<s_SQL_data_type_object>, bool> table_object::get_column_data_type(size_t index) const{
    if (index >= column_data.size()) {
        return {nullptr, false}; }
    
    auto* dt = column_data[index]->data_type->clone();
    return {UP<s_SQL_data_type_object>(dt), true};
}
std::pair<size_t, bool> table_object::get_column_index(const std_and_astring_variant& name) const {
    astring unwrapped_name;
    visit(name, [&](const auto& unwrapped) {
        unwrapped_name = unwrapped;
    });

    for (size_t i = 0; i < column_data.size(); i++) {
        if (column_data[i]->name == unwrapped_name) {
            return {i, true};
        }
    }

    return {0, false};
}
std::expected<std::optional<UP<s_default_value_object>>, UP<error_object>> table_object::get_cloned_column_default_value(size_t index) const {
    if (index >= column_data.size()) {
        return std::unexpected(MAKE_UP(error_object, "Column index out of bounds")); }

    if (column_data[index]->default_value.has_value()) {
        return UP<s_default_value_object>(column_data[index]->default_value.value()->clone());
    } else {
        return std::nullopt;
    }
}
std::pair<UP<serializable>, bool> table_object::get_cell_value(size_t row_index, size_t col_index) const {
    if (row_index >= rows.size()) {
        return {UP<serializable>(new error_object("Row index out of bounds")), false};}

    if (col_index >= column_data.size()) {
        return {UP<serializable>(new error_object("Column index out of bounds")), false};}

    s_group_object* roh = rows[row_index].get();
    
    return {UP<serializable>(roh->elements[col_index]->clone()), true};
}
std::expected<avec<UP<serializable>>*, UP<error_object>> table_object::get_row_vec_ptr(size_t index) const {
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

    for (const auto& col_data : column_data) {
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
object_type table_aggregate_object::type() const { return  TABLE_AGGREGATE_OBJ;  }
astring     table_aggregate_object::data() const { return "TABLE_AGGREGATE_OBJ"; }
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
        for (const auto& col_data : table->column_data) {
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
            if (index >= table->column_data.size()) {
                astringstream stream;
                stream << "Index (" << index << ") out of bounds for (" << unwrapped_table_name << ")";
                return std::unexpected(MAKE_UP(error_object, stream.str())); 
            }
            id += index;
            break;
        } else if (table->column_data.size() > 0) {
            id += table->column_data.size() - 1;
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
            for (const auto& col_data : table->column_data) {
                if (col_data->name == unwrapped_col_name) {
                    found_col = true;
                    break;
                }

                id += 1;
            }
            break;
        } else if (table->column_data.size() > 0) {
            id += table->column_data.size() - 1;
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
        total_size += table->column_data.size(); 
    }
    avec<size_t> col_ids;
    col_ids.reserve(total_size); 

    size_t cur_id = 0;
    for (const auto& table : tables) {
        for (size_t i = 0; i < table->column_data.size(); i++) {
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
        size_t tab_size = table->column_data.size();
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

class table_aggregate_view : virtual public serializable {
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
    size_t total_exprs = 0;
    size_t max_cols = 0;
    size_t max_rows = 0;
    for (const auto& table : tables) {
        total_columns += table->column_data.size();
        total_exprs = table->exprs.size();
        max_cols = std::max(max_cols, table->column_data.size());
        max_rows = std::max(max_rows, table->rows.size());
    }

    avec<UP<s_table_detail_object>> column_data;
    column_data.reserve(total_columns);
    for (const auto& table : tables) {
        for (auto& col_data : table->column_data) {
            column_data.push_back(UP<s_table_detail_object>(col_data->clone()));
        }
    }

    // TODO Expressions might be violated during merge, might have to check, or not idc.
    avec<UP<s_table_expr>> exprs;
    exprs.reserve(total_exprs);
    for (const auto& table : tables) {
        for (auto& expr : table->exprs) {
            exprs.push_back(UP<s_table_expr>(expr->clone()));
        }
    }

    avec<UP<s_group_object>> rows;
    rows.reserve(max_rows);
    for (size_t row_index = 0; row_index < max_rows; row_index++) {

        avec<UP<serializable>> new_row;
        new_row.reserve(max_cols);
        for (const auto& table : tables) {
            
            // Fill in empty rows
            if (row_index >= table->rows.size()) {
                for (size_t col_index = 0; col_index < table->column_data.size(); col_index++) {
                    new_row.emplace_back(new null_object()); 
                }
                continue; 
            }
            
            const auto result = table->get_row_vec_ptr(row_index);
            if (!result.has_value()) {
                return MAKE_SP(table_object, "Weird index bug", nullptr, nullptr, nullptr); }

            const auto& row = **result;
                
            for (const auto& col_index : row) {
                new_row.push_back(UP<serializable>(col_index->clone())); 
            }
        }   

        rows.emplace_back(MAKE_UP(s_group_object, std::move(new_row)));
    }

    return MAKE_SP(table_object, final_name, std::move(column_data), std::move(exprs), std::move(rows));
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
object_type insert_into_object::type() const { return  INSERT_INTO_OBJ;  }
astring     insert_into_object::data() const { return "INSERT_INTO_OBJ"; }
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
    values(std::move(set_values)) 
{}
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
object_type e_insert_into_object::type() const { return  E_INSERT_INTO_OBJ;  }
astring     e_insert_into_object::data() const { return "E_INSERT_INTO_OBJ"; }
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
select_object::select_object(object*    set_value) { value = UP<object>(set_value);   }
select_object::select_object(UP<object> set_value) : value(std::move(set_value))     {}
astring        select_object::inspect() const { return "select: " + value->inspect(); }
object_type    select_object::type()    const { return  SELECT_OBJ;  }
astring        select_object::data()    const { return "SELECT_OBJ"; }
select_object* select_object::clone()   const { return new select_object(value->clone()); }

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
object_type select_from_object::type() const { return  SELECT_FROM_OBJ;  }
astring     select_from_object::data() const { return "SELECT_FROM_OBJ"; }
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
object_type e_select_from_object::type() const { return  E_SELECT_FROM_OBJ;  }
astring     e_select_from_object::data() const { return "E_SELECT_FROM_OBJ"; }
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

// Expression Statement
expression_statement::expression_statement(avec<UP<evaluated>>&& set_body, e_return_statement* set_ret_val) : 
    body(std::move(set_body)),
    ret_val(UP<e_return_statement>(set_ret_val)) 
{}
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
object_type expression_statement::type() const { return  EXPRESSION_STATEMENT;  }
astring     expression_statement::data() const { return "EXPRESSION_STATEMENT"; }
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
object_type   if_statement::type()  const { return  IF_STATEMENT;  }
astring       if_statement::data()  const { return "IF_STATEMENT"; }
if_statement* if_statement::clone() const {return new if_statement(condition->clone(), body->clone(), other->clone()); }

// end_if_statement
astring           end_if_statement::inspect() const { return "END_IF_STATEMENT";     }
object_type       end_if_statement::type()    const { return  END_IF_STATEMENT;      }
astring           end_if_statement::data()    const { return "END_IF_STATEMENT";     }
end_if_statement* end_if_statement::clone()   const { return new end_if_statement(); }

// end_statement
astring        end_statement::inspect() const { return "END_STATEMENT";     }
object_type    end_statement::type()    const { return  END_STATEMENT;      }
astring        end_statement::data()    const { return "END_STATEMENT";     }
end_statement* end_statement::clone()   const { return new end_statement(); }

// return_statement
return_statement::return_statement(object*    expr) { expression = UP<object>(expr); }
return_statement::return_statement(UP<object> expr) : expression(std::move(expr))     {}
astring return_statement::inspect() const {
    astringstream stream;
    stream << "[Type: Return statement, Value: ";
    stream << expression->inspect() + "]";
    return stream.str(); 
}
object_type       return_statement::type()  const { return  RETURN_STATEMENT;  }
astring           return_statement::data()  const { return "RETURN_STATEMENT"; }
return_statement* return_statement::clone() const { return new return_statement(expression->clone()); }

// e_return_statement
e_return_statement::e_return_statement(evaluated*    expr) { expression = UP<evaluated>(expr); }
e_return_statement::e_return_statement(UP<evaluated> expr) : expression(std::move(expr))        {}
astring e_return_statement::inspect() const {
    astringstream stream;
    stream << "[Type: E Return statement, Value: ";
    stream << expression->inspect() + "]";
    return stream.str(); 
}
object_type         e_return_statement::type()  const { return  E_RETURN_STATEMENT;  }
astring             e_return_statement::data()  const { return "E_RETURN_STATEMENT"; }
e_return_statement* e_return_statement::clone() const { return new e_return_statement(expression->clone()); }



// CUSTOM

// assert_object
assert_object::assert_object(size_t set_line, object* expr) : 
    line(set_line) {
    expression = UP<object>(expr);
}
assert_object::assert_object(size_t set_line, UP<object> expr) : 
    line(set_line), 
    expression(std::move(expr)) 
{}
astring        assert_object::inspect()  const { return  expression->inspect(); }
object_type    assert_object::type()     const { return  ASSERT_OBJ;  }
astring        assert_object::data()     const { return "ASSERT_OBJ"; }
assert_object* assert_object::clone()    const { return new assert_object(line, expression->clone()); }

// e_assert_object
e_assert_object::e_assert_object(size_t set_line, evaluated* expr) : 
    line(set_line) {
    expression = UP<evaluated>(expr);
}
e_assert_object::e_assert_object(size_t set_line, UP<evaluated> expr) : 
    line(set_line), 
    expression(std::move(expr)) 
{}
astring          e_assert_object::inspect() const { return  expression->inspect(); }
object_type      e_assert_object::type()    const { return  E_ASSERT_OBJ;  }
astring          e_assert_object::data()    const { return "E_ASSERT_OBJ"; }
e_assert_object* e_assert_object::clone()   const { return new e_assert_object(line, expression->clone()); }

