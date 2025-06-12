
// objects are made in the parser and should probably stay there, used to parse and return values from expressions
// i.e (10 + 10) will return an integer_object with the value 20

#include "object.h"

#include "token.h"
#include "helpers.h"
#include "structs_and_macros.h" // For table


arena<object> object::object_arena_alias; 

static std::span<const char* const> object_type_span() {
    static constexpr const char* object_type_chars[] = {
        "ERROR_OBJ", "NULL_OBJ", "INFIX_EXPRESSION_OBJ", "PREFIX_EXPRESSION_OBJ", "INTEGER_OBJ", "INDEX_OBJ", "DECIMAL_OBJ", "STRING_OBJ", "SQL_DATA_TYPE_OBJ",
        "COLUMN_OBJ", "EVALUATED_COLUMN_OBJ", "FUNCTION_OBJ", "OPERATOR_OBJ", "SEMICOLON_OBJ", "RETURN_VALUE_OBJ", "ARGUMENT_OBJ", "BOOLEAN_OBJ", "FUNCTION_CALL_OBJ",
        "GROUP_OBJ", "PARAMETER_OBJ", "EVALUATED_FUNCTION_OBJ", "VARIABLE_OBJ", "TABLE_DETAIL_OBJECT", "COLUMN_INDEX_OBJECT", "TABLE_INFO_OBJECT", "TABLE_OBJECT",
        "STAR_OBJECT", "TABLE_AGGREGATE_OBJECT", "COLUMN_VALUES_OBJ",

        "IF_STATEMENT", "BLOCK_STATEMENT", "END_IF_STATEMENT", "END_STATEMENT", "RETURN_STATEMENT",

        "INSERT_INTO_OBJECT", "SELECT_OBJECT", "SELECT_FROM_OBJECT",

        "ASSERT_OBJ",
    };
    return object_type_chars;
}

astring object_type_to_astring(object_type index) {
    return astring(object_type_span()[index]);
}

static std::span<const char* const> operator_type_span() {
    static constexpr const char* operator_type_to_string[] = {
        "ADD_OP", "SUB_OP", "MUL_OP", "DIV_OP", "NEGATE_OP",
        "EQUALS_OP", "NOT_EQUALS_OP", "LESS_THAN_OP", "LESS_THAN_OR_EQUAL_TO_OP", "GREATER_THAN_OP", "GREATER_THAN_OR_EQUAL_TO_OP",
        "OPEN_PAREN_OP", "OPEN_BRACKET_OP", "AS_OP", "LEFT_JOIN_OP", "WHERE_OP", "GROUP_BY_OP", "ORDER_BY_OP", "ON_OP",
        "DOT_OP",
    };
    return operator_type_to_string;
}
astring operator_type_to_astring(operator_type index) {
    return astring(operator_type_span()[index]);
}



// null_object
astring null_object::inspect() const {
    return astring("NULL_OBJECT");
}
object_type null_object::type() const {
    return NULL_OBJ;
}
astring null_object::data() const {
    return astring("NULL_OBJECT");
}
null_object* null_object::clone(bool use_arena) const {
    return new (use_arena) null_object(use_arena);
}




// operator_object
operator_object::operator_object(const operator_type type, bool use_arena) : object(use_arena) {
    op_type = type;
}
astring operator_object::inspect() const {
    return operator_type_to_astring(op_type); 
}
object_type operator_object::type() const {
    return OPERATOR_OBJ;
}
astring operator_object::data() const {
    return operator_type_to_astring(op_type);
}
operator_object* operator_object::clone(bool use_arena) const {
    return new (use_arena) operator_object(op_type, use_arena);
}




// Table Info Object
table_info_object::table_info_object(table_object* set_tab, const avec<size_t>& set_col_ids, const avec<size_t>& set_row_ids, bool use_arena, bool clone) : object(use_arena) {
    if (in_arena) {
        if (clone) {
            tab = set_tab->clone(in_arena);
        } else {
            tab = set_tab;
        }

        col_ids = avec<size_t>();
        for (const auto& col_id : set_col_ids) {
            col_ids.push_back(col_id);
        }

        row_ids = avec<size_t>();
        for (const auto& row_id : set_row_ids) {
            row_ids.push_back(row_id);
        }
    } else {
        tab = set_tab->clone(in_arena);

        col_ids = hvec_copy(size_t);
        for (const auto& col_id : set_col_ids) {
            col_ids.push_back(col_id);
        }

        row_ids = hvec_copy(size_t);
        for (const auto& row_id : set_row_ids) {
            row_ids.push_back(row_id);
        }
    }
}
table_info_object::~table_info_object() {
    if (!in_arena) {
        delete tab; // Is cloned so it's ok to delete the table
    }
}
astring table_info_object::inspect() const {
    astringstream stream;
    stream << "Table name: " << tab->table_name << "\n";

    stream << "Row ids: \n";
    bool first = true;
    for (const auto& row_id : row_ids) {
        if (!first) stream << ", ";
        stream << row_id;
        first = false;
    }

    stream << "\nColumn ids: \n";
    first = true;
    for (const auto& col_id : col_ids) {
        if (!first) stream << ", ";
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
table_info_object* table_info_object::clone(bool use_arena) const {
    return new (use_arena) table_info_object(tab, col_ids, row_ids, use_arena, true);
}

// infix_expression_object
infix_expression_object::infix_expression_object(operator_object* set_op, object* set_left, object* set_right, bool use_arena, bool clone)  : expression_object(use_arena) {
    
    if (in_arena) {
        if (clone) {
            op = set_op->clone(in_arena);
            left = set_left->clone(in_arena);
            right = set_right->clone(in_arena);
        } else {
            op = set_op;
            left  = set_left;
            right = set_right;
        }
    } else {
        op = set_op->clone(in_arena);
        left = set_left->clone(in_arena);
        right = set_right->clone(in_arena);
    }
}
astring infix_expression_object::inspect() const {
    astringstream stream;
    stream << "[Op: " + op->inspect();
    stream << ". Left: " + left->inspect();
    stream << ". Right: " + right->inspect() + "]";
    return stream.str();
}
infix_expression_object::~infix_expression_object() {
    if (!in_arena) {
      delete op;
      delete left;
      delete right;
    }
}
object_type infix_expression_object::type() const {
    return INFIX_EXPRESSION_OBJ;
}
astring infix_expression_object::data() const {
    return astring("INFIX_EXPRESSION_OBJ");
}
infix_expression_object* infix_expression_object::clone(bool use_arena) const {
    return new (use_arena) infix_expression_object(op, left, right, use_arena, true);
}
operator_type infix_expression_object::get_op_type() const {
    return op->op_type;
}

// prefix_expression_object
prefix_expression_object::prefix_expression_object(operator_object* set_op, object* set_right, bool use_arena, bool clone)  : expression_object(use_arena) {
    if (in_arena) {
        if (clone) {
            op = set_op->clone(in_arena);
            right = set_right->clone(in_arena);
        } else {
            op = set_op;
            right = set_right;
        }
    } else {
        op = set_op->clone(in_arena);
        right = set_right->clone(in_arena);
    }
}
prefix_expression_object::~prefix_expression_object() {
    if (!in_arena) {
      delete op;
      delete right;
    }
}
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
prefix_expression_object* prefix_expression_object::clone(bool use_arena) const {
    return new (use_arena) prefix_expression_object(op, right, use_arena, true);
}
operator_type prefix_expression_object::get_op_type() const {
    return op->op_type;
}

// integer_object
integer_object::integer_object(bool use_arena) : object(use_arena) {
    value = 0;
}
integer_object::integer_object(int val, bool use_arena) : object(use_arena) {
    value = val;
}
integer_object::integer_object(const std::string& val, bool use_arena) : object(use_arena) {
    value = std::stoi(val);
}
integer_object::integer_object(const astring& val, bool use_arena) : object(use_arena) {
    value = astring_to_numeric<int>(val);
}
astring integer_object::inspect() const {
    return numeric_to_astring<int>(value, in_arena); 
}
object_type integer_object::type() const {
    return INTEGER_OBJ;
}
astring integer_object::data() const {
    return numeric_to_astring<int>(value, in_arena); 
}
integer_object* integer_object::clone(bool use_arena) const {
    return new (use_arena) integer_object(value, use_arena);
}

// index_object
index_object::index_object(bool use_arena) : object(use_arena) {
    value = 0;
}
index_object::index_object(size_t val, bool use_arena) : object(use_arena) {
    value = val;
}
index_object::index_object(const std::string& val, bool use_arena) : object(use_arena) {
    value = std::stoull(val);
}
index_object::index_object(const astring& val, bool use_arena) : object(use_arena) {
    value = astring_to_numeric<size_t>(val);
}
astring index_object::inspect() const {
    return numeric_to_astring<size_t>(value, in_arena); 
}
object_type index_object::type() const {
    return INDEX_OBJ;
}
astring index_object::data() const {
    return numeric_to_astring<size_t>(value, in_arena); 
}
index_object* index_object::clone(bool use_arena) const {
    return new (use_arena) index_object(value, use_arena);
}

// decimal_object
decimal_object::decimal_object(bool use_arena) : object(use_arena) {
    value = 0;
}
decimal_object::decimal_object(double val, bool use_arena) : object(use_arena) {
    value = val;
}
decimal_object::decimal_object(const std::string& val, bool use_arena) : object(use_arena) {
    value = std::stod(val);
}
decimal_object::decimal_object(const astring& val, bool use_arena) : object(use_arena) {
    value = astring_to_numeric<double>(val);
}
astring decimal_object::inspect() const {
    return numeric_to_astring<double>(value, in_arena); 
}
object_type decimal_object::type() const {
    return DECIMAL_OBJ;
}
astring decimal_object::data() const {
    return numeric_to_astring<double>(value, in_arena); 
}
decimal_object* decimal_object::clone(bool use_arena) const {
    return new (use_arena) decimal_object(value, use_arena);
}


// string_object
string_object::string_object(const std_and_astring_variant& val, bool use_arena) : object(use_arena)  {

    VISIT(val, unwrapped,
        if (in_arena) {
            value = astring(unwrapped);
        } else {
            value = hstring(unwrapped);
        }
    );

}
string_object::~string_object() {
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
string_object* string_object::clone(bool use_arena) const {
    return new (use_arena) string_object(value, use_arena);
}

// return_value_object
return_value_object::return_value_object(object* val, bool use_arena, bool clone) : object(use_arena) {
    
    if (in_arena) {
        if (clone) {
            value = val->clone(in_arena);
        } else {
            value = val;
        }
    } else {
        value = val->clone(in_arena);
    }
}
return_value_object::~return_value_object() {
    if (!in_arena) {
      delete value;
    }
}
astring return_value_object::inspect() const {
    return "Returning: " + value->inspect();
}
object_type return_value_object::type() const {
    return RETURN_VALUE_OBJ;
}
astring return_value_object::data() const {
    return value->data();
}
return_value_object* return_value_object::clone(bool use_arena) const {
    return new (use_arena) return_value_object(value, use_arena, true);
}

// argument_object
argument_object::argument_object(const std_and_astring_variant& set_name, object* val, bool use_arena, bool clone) : object(use_arena) {


    VISIT(set_name, unwrapped,
        if (in_arena) {
            name = astring(unwrapped);
        } else {
            name = hstring(unwrapped);
        }
    );
    
    if (in_arena) {
        if (clone) {
            value = val->clone(in_arena);
        } else {
            value = val;
        }
    } else {
        value = val->clone(in_arena);
    }
}
argument_object::~argument_object() {
    if (!in_arena) {
      delete value;
    }
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
argument_object* argument_object::clone(bool use_arena) const {
    return new (use_arena) argument_object(name, value, use_arena, true);
}

// variable_object
variable_object::variable_object(const std_and_astring_variant& set_name, object* val, bool use_arena, bool clone) : object(use_arena) {
    
    VISIT(set_name, unwrapped,
        if (in_arena) {
            name = astring(unwrapped);
            if (clone) {
                value = val->clone(in_arena);
            } else {
                value = val;
            }
        } else {
            name = hstring(unwrapped);
            value = val->clone(in_arena);
        }
    );
}
variable_object::~variable_object() {
    if (!in_arena) {
      delete value;
    }
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
variable_object* variable_object::clone(bool use_arena) const {
    return new (use_arena) variable_object(name, value, use_arena, true);
}

// boolean_object
boolean_object::boolean_object(bool val, bool use_arena) : object(use_arena) {
    value = val;
}
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
boolean_object* boolean_object::clone(bool use_arena) const {
    return new (use_arena) boolean_object(value, use_arena);
}

//zerofill is implicitly unsigned im pretty sure
// SQL_data_type_object
SQL_data_type_object::SQL_data_type_object(token_type set_prefix, token_type set_data_type, object* set_parameter, bool use_arena, bool clone) : object(use_arena) {
    
    if (set_parameter == nullptr) {
        std::cout << "!!!ERROR!!! SQL data type object constructed with null parameter\n"; }
    if (set_data_type == NONE) {
        std::cout << "!!!ERROR!!! SQL data type object constructed without data type token\n"; }

    prefix = set_prefix;
    data_type = set_data_type;

    if (in_arena) {
        if (clone) {
            parameter = set_parameter->clone(in_arena);
        } else {
            parameter = set_parameter;
        }
    } else {
        parameter = set_parameter->clone(in_arena);
    }
}
SQL_data_type_object::~SQL_data_type_object() {
    if (!in_arena) {
      delete parameter;
    }
}
astring SQL_data_type_object::inspect() const {
    astringstream stream;
    stream << "[Type: SQL data type, Data type: ";
    if (prefix != NONE) {
        stream << token_type_to_string(prefix);}
        
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
SQL_data_type_object* SQL_data_type_object::clone(bool use_arena) const {
    return new (use_arena) SQL_data_type_object(prefix, data_type, parameter, use_arena, true);
}

// parameter_object
parameter_object::parameter_object(const std_and_astring_variant& set_name, SQL_data_type_object* set_data_type, bool use_arena, bool clone) : object(use_arena) {
    
    VISIT(set_name, unwrapped,
        if (in_arena) {
            name = astring(unwrapped);
            if (clone) {
                data_type = set_data_type->clone(in_arena);
            } else {
                data_type = set_data_type;
            }
        } else {
            name = hstring(unwrapped);
            data_type = set_data_type->clone(in_arena);
        }
    );
}
parameter_object::~parameter_object() {
    if (!in_arena) {
      delete data_type;
    }
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
parameter_object* parameter_object::clone(bool use_arena) const {
    return new (use_arena) parameter_object(name, data_type, use_arena, true);
}

// table_detail_object
table_detail_object::table_detail_object(const std_and_astring_variant& set_name, SQL_data_type_object* set_data_type, object* set_default_value, bool use_arena, bool clone) : object(use_arena) {

    astring astr;
    std::string stdstr;
    VISIT(set_name, unwrapped,
        astr = astring(unwrapped);
        stdstr = hstring(unwrapped);
    );
    
    if (in_arena) {
        name = astr;
        if (clone) {
            data_type = set_data_type->clone(in_arena);
            default_value = set_default_value->clone(in_arena);
        } else {
            data_type = set_data_type;
            default_value = set_default_value;
        }
    } else {
        name = stdstr;

        bool set_dt_in_heap = !set_data_type->in_arena;
        bool set_dv_in_heap = !set_default_value->in_arena;

        if (set_dt_in_heap) {
            data_type = set_data_type;
        } else {
            data_type = set_data_type->clone(HEAP);
        }

        if (set_dv_in_heap) {
            default_value = set_default_value;
        } else {
            default_value = set_default_value->clone(HEAP);
        }
    }
}
table_detail_object::~table_detail_object() {
    if (!in_arena) {
      delete data_type;
      delete default_value;
    }
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
table_detail_object* table_detail_object::clone(bool use_arena) const {
    return new (use_arena) table_detail_object(name, data_type, default_value, use_arena, true);
}

// group_object
group_object::group_object(object* objs, bool use_arena, bool clone) : object(use_arena) { 
    
    if (in_arena) {
        elements = avec<object*>(); 
        if (clone) {
            elements.push_back(objs->clone(in_arena));
        } else {
            elements.push_back(objs);
        }
    } else {
        bool obj_in_heap = !objs->in_arena;

        elements = hvec_copy(object*);
        if (obj_in_heap) {
            elements.push_back(objs);
        } else {
            elements.push_back(objs->clone(in_arena));
        }
    }
}
group_object::group_object(const avec<object*>& objs, bool use_arena, bool clone) : object(use_arena)  { 
    
    if (in_arena) {
        elements = avec<object*>();
        if (clone) {
            for (const auto& obj : objs) {
                elements.push_back(obj->clone(in_arena));
            }
        } else {
            for (const auto& obj : objs) {
                elements.push_back(obj);
            }
        }

    } else {
        bool vec_in_heap = objs.get_allocator().use_std_alloc;

        elements = hvec_copy(object*);  
        if (vec_in_heap) {
            for (const auto& obj : objs) {
                elements.push_back(obj);
            }
        } else {
            for (const auto& obj : objs) {
                elements.push_back(obj->clone(in_arena));
            }
        }
    }
}
group_object::~group_object() {
    if (!in_arena) {
        for (const auto& element : elements) {
            delete element;
        }
    }
}
astring group_object::inspect() const {
    astringstream stream;
    bool first = true;
    for (const auto& element : elements) {
        if (!first) stream << ", ";
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
group_object* group_object::clone(bool use_arena) const {
    return new (use_arena) group_object(elements, use_arena, true);
}

// function_object
function_object::function_object(const std_and_astring_variant& set_name, group_object* set_parameters, SQL_data_type_object* set_return_type, block_statement* set_body, bool use_arena, bool clone) : object(use_arena) {
    
    VISIT(set_name, unwrapped,
        if (in_arena) {
            name = astring(unwrapped);
            if (clone) {
                parameters  = set_parameters->clone(in_arena);
                body        = set_body->clone(in_arena);
                return_type = set_return_type->clone(in_arena);
            } else {
                parameters  = set_parameters;
                body        = set_body;
                return_type = set_return_type;
            }
        } else {
            name        = hstring(unwrapped);
            parameters  = set_parameters->clone(in_arena);
            body        = set_body->clone(in_arena);
            return_type = set_return_type->clone(in_arena);
        }
    );
}
function_object::~function_object() {
    if (!in_arena) {
        delete parameters;
        delete return_type;
        delete body;
    }
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
function_object* function_object::clone(bool use_arena) const {
    return new (use_arena) function_object(name, parameters, return_type, body, use_arena, true);
}

// evaluted_function_object
evaluated_function_object::evaluated_function_object(const std_and_astring_variant& set_name, 
    const avec<parameter_object*>& set_parameters, SQL_data_type_object* set_return_type, 
    block_statement* set_body, bool use_arena, bool clone) : object(use_arena)
{
    
    VISIT(set_name, unwrapped,
        if (in_arena) {
            name = astring(unwrapped);
            if (clone) {
                parameters = avec<parameter_object*>();
                for (const auto& param : set_parameters) {
                    parameters.push_back(param->clone(in_arena)); }

                return_type = set_return_type->clone(in_arena);
                body = set_body->clone(in_arena);
            } else {
                parameters = set_parameters;
                return_type = set_return_type;
                body = set_body;
            }
        } else {
            name = hstring(unwrapped);

            parameters = hvec_copy(parameter_object*);
            for (const auto& param : set_parameters) {
                parameters.push_back(param->clone(in_arena));
            }

            return_type = set_return_type->clone(in_arena);
            body = set_body->clone(in_arena);
        }
    );

}
evaluated_function_object::evaluated_function_object(function_object* func, const avec<parameter_object*>& new_parameters, bool use_arena, bool clone) : object(use_arena) {
    
    if (in_arena) {
        name = astring(func->name);
        if (clone) {
            
            parameters = avec<parameter_object*>();         
            for (const auto& param : new_parameters) {
                parameters.push_back(param->clone(in_arena));
            }

            return_type  = func->return_type->clone(in_arena);
            body = func->body->clone(in_arena);
        } else {
            
    
            parameters = avec<parameter_object*>(); 
            for (const auto& param : new_parameters) {
                parameters.push_back(param);
            }
    
            return_type = func->return_type;
            body = func->body;
        }
    } else {
        name = hstring(func->name);
        parameters = hvec_copy(parameter_object*);
        for (const auto& param : new_parameters) {
            parameters.push_back(param->clone(in_arena));
        }
        return_type = func->return_type->clone(in_arena);
        body = func->body->clone(in_arena);
    }
}
evaluated_function_object::~evaluated_function_object() {
    if (!in_arena) {
        for (const auto& param : parameters) {
            delete param;
        }
        delete return_type;
        delete body;
    }
}
astring evaluated_function_object::inspect() const {
    astringstream stream;
    stream << "Function name: " << name << "\n";

    stream << "Arguments: (";
    bool first = true;
    for (const auto& param : parameters) {
        if (!first) stream << ", ";
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
evaluated_function_object* evaluated_function_object::clone(bool use_arena) const {
    return new (use_arena) evaluated_function_object(name, parameters, return_type, body, use_arena, true);
}

// function_call_object
function_call_object::function_call_object(const std_and_astring_variant& set_name, group_object* args, bool use_arena, bool clone) : object(use_arena) {
    
    VISIT(set_name, unwrapped,
        if (in_arena) {
            name = astring(unwrapped);
            if (clone) {
                arguments = args->clone(in_arena);
            } else {
                arguments = args;
            }
        } else {
            name = hstring(unwrapped);
            arguments = args->clone(in_arena);
        }
    );
}
function_call_object::~function_call_object() {
    if (!in_arena) {
        delete arguments;
    }
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
function_call_object* function_call_object::clone(bool use_arena) const {
    return new (use_arena) function_call_object(name, arguments, use_arena, true);
}

// column_object
column_object::column_object(object* set_name_data_type, bool use_arena, bool clone) : object(use_arena) {
    
    if (in_arena) {
        if (clone) {
            name_data_type = set_name_data_type->clone(in_arena);
        } else {
            name_data_type = set_name_data_type;
        }
        default_value = new (in_arena) null_object();
    } else {
        name_data_type = set_name_data_type->clone(in_arena);
        default_value = new (in_arena) null_object();
    }
}
column_object::column_object(object* set_name_data_type, object* default_val, bool use_arena, bool clone) : object(use_arena) {
    
    if (in_arena) {
        if (clone) {
            name_data_type = set_name_data_type->clone(in_arena);
            default_value = default_val->clone(in_arena);
        } else {
            name_data_type = set_name_data_type;
            default_value = default_val;
        }
    } else {
        name_data_type = set_name_data_type->clone(in_arena);
        default_value = default_val->clone(in_arena);
    }
}
column_object::~column_object() {
    if (!in_arena) {
        delete name_data_type;
        delete default_value;
    }
}
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
column_object* column_object::clone(bool use_arena) const {
    return new (use_arena) column_object(name_data_type, default_value, use_arena, true);
}

// column_values_object
column_values_object::column_values_object(const avec<object*>& set_values, bool use_arena, bool clone) : values_wrapper_object(use_arena) {
    
    if (in_arena) {
        values = avec<object*>();
        if (clone) {
            for (const auto val : set_values) {
                values.push_back(val->clone(in_arena)); 
            }
        } else {
            for (const auto val : set_values) {
                values.push_back(val); 
            }
            
        }
    } else {
        values = hvec_copy(object*);
        for (const auto& obj : set_values) {
            values.push_back(obj->clone(in_arena)); }
    }
}

column_values_object::~column_values_object() {
    if (!in_arena) {
        for (const auto& val : values) {
            delete val; }
    }
}
astring column_values_object::inspect() const {
    astringstream stream;
    stream << "[Column values: ";

    bool first = true;
    for (const auto& val : values) {
        if (!first) stream << ", ";
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
column_values_object* column_values_object::clone(bool use_arena) const {
    return new (use_arena) column_values_object(values, use_arena, true);
}

// evaluated_column_object
evaluated_column_object::evaluated_column_object(const std_and_astring_variant& set_name, SQL_data_type_object* type, 
                                                 const std_and_astring_variant& set_default_value, bool use_arena, bool clone) : values_wrapper_object(use_arena)
{
    
    VISIT(set_name, unwrapped,
        if (in_arena) {
            name = astring(unwrapped);
        } else {
            name = hstring(unwrapped);
        }
    );
    
    VISIT(set_default_value, unwrapped,
        if (in_arena) {
            default_value = astring(unwrapped);
        } else {
            default_value = hstring(unwrapped);
        }
    );

    if (in_arena) {
        if (clone) {
            data_type = type->clone(in_arena);
        } else {
            data_type = type;
        }
    } else {
        data_type = type->clone(in_arena);
    }
}
evaluated_column_object::~evaluated_column_object() {
    if (!in_arena) {
        delete data_type;
    }
}
astring evaluated_column_object::inspect() const {
    astringstream stream;
    stream << "[Column name: " << name << "], ";
    stream << data_type->inspect();
    stream << ", [Default: " << default_value  << "]\n";
    return stream.str();
}
object_type evaluated_column_object::type() const {
    return EVALUATED_COLUMN_OBJ;
}
astring evaluated_column_object::data() const {
    return "EVALUATED_COLUMN_OBJ";
}
evaluated_column_object* evaluated_column_object::clone(bool use_arena) const {
    return new (use_arena) evaluated_column_object(name, data_type, default_value, use_arena, true);
}

// error_object
error_object::error_object(bool use_arena) : object(use_arena) {
    if (in_arena) {
        value = astring();
    } else {
        value = hstring("");
    }
}
error_object::error_object(const std_and_astring_variant& val, bool use_arena) : object(use_arena) {
    VISIT(val, unwrapped,
        if (in_arena) {
            value = astring(unwrapped);
        } else {
            value = hstring(unwrapped);
        }
    );
}
error_object::~error_object() {
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
error_object* error_object::clone(bool use_arena) const {
    return new (use_arena) error_object(value, use_arena);
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
semicolon_object* semicolon_object::clone(bool use_arena) const {
    return new (use_arena) semicolon_object(use_arena);
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
star_object* star_object::clone(bool use_arena) const {
    return new (use_arena) star_object(use_arena);
}

// Column index object
column_index_object::column_index_object(object* set_table_name, object* set_column_name, bool use_arena, bool clone) : object(use_arena) {
    
    if (in_arena) {
        if (clone) {
            table_name = set_table_name->clone(in_arena);
            column_name = set_column_name->clone(in_arena);
        } else {
            table_name = set_table_name;
            column_name = set_column_name;
        }
    } else {
        table_name = set_table_name->clone(in_arena);
        column_name = set_column_name->clone(in_arena);
    }
}
column_index_object::~column_index_object() {
    if (!in_arena) {
        delete table_name;
        delete column_name;
    }
}
astring column_index_object::inspect() const {
    return "[Table alias: " + table_name->inspect() + ", Column alias: " + column_name->inspect() + "]";
}
object_type column_index_object::type() const {
    return COLUMN_INDEX_OBJECT;
}
astring column_index_object::data() const {
    return "COLUMN_INDEX_OBJECT";
}
column_index_object* column_index_object::clone(bool use_arena) const {
    return new (use_arena) column_index_object(table_name, column_name, use_arena, true);
}



// Table object, only use heap
table_object::table_object(const std_and_astring_variant& set_table_name, table_detail_object* set_column_datas, group_object* set_rows, [[maybe_unused]] bool clone) : object(HEAP) {

    VISIT(set_table_name, unwrapped,
        table_name = hstring(unwrapped);
    );

    column_datas = hvec_copy(table_detail_object*);
    if (set_column_datas->in_arena) {
        column_datas.push_back(set_column_datas->clone(HEAP));
    } else {
        column_datas.push_back(set_column_datas);
    }
    
    rows = hvec_copy(group_object*);
    if (set_rows->in_arena) {
        rows.push_back(set_rows->clone(HEAP));
    } else {
        rows.push_back(set_rows);
    }

}
table_object::table_object(const std_and_astring_variant& set_table_name, const avec<table_detail_object*>& set_column_datas, const avec<group_object*>& set_rows, [[maybe_unused]] bool clone) : object(HEAP) {
    
    VISIT(set_table_name, unwrapped,
        table_name = hstring(unwrapped);
    );

    column_datas = hvec_copy(table_detail_object*);
    for (const auto& col_data : set_column_datas) {
        if (col_data->in_arena) {
            column_datas.push_back(col_data->clone(HEAP));
        } else {
            column_datas.push_back(col_data);
        }
            
    }

    rows = hvec_copy(group_object*);
    for (const auto& row : set_rows) {
        if (row->in_arena) {
            rows.push_back(row->clone(HEAP));
        } else {
            rows.push_back(row);
        }
    }
}
table_object::~table_object() {
    if (!in_arena) {
        for (const auto& col_data : column_datas) {
            delete col_data;
        }
        for (const auto& roh : rows) {
            delete roh;
        }
    }
}
astring table_object::inspect() const {
    astringstream stream = hstringstream();
    stream << "Table name: " << table_name  << "\n";

    stream << "Column data (" << std::to_string(column_datas.size()) << "): ";
    bool first = true;
    for (const auto& col : column_datas) {
        if (!first) stream << ", ";
        stream << col->inspect();
        first = false;
    }
    
    stream << "\nRows (" << std::to_string(rows.size()) << "): \n";
    for (const auto& roh : rows) {
        stream << roh->inspect() << "\n"; }

    return stream.str();
}
object_type table_object::type() const {
    return TABLE_OBJECT;
}
astring table_object::data() const {
    return hstring("TABLE_OBJECT");
}
table_object* table_object::clone([[maybe_unused]] bool use_arena) const {
    return new (HEAP) table_object(table_name, column_datas, rows, true);
}
std::pair<avec<object*>, bool> table_object::get_column(size_t index) const { //!!Expensive, only use if have to, else just use alias

    hvec(object*, column);

    if (index >= column_datas.size()) {
        return {column, false}; }
    
    column.reserve(rows.size());
    for (const auto& row : rows) {
        column.push_back(row->elements[index]); }

    return {column, true};
}
std::pair<avec<object*>, bool> table_object::get_column(const std_and_astring_variant& col_name) const {

    size_t index;
    bool ok;
    VISIT(col_name, unwrapped,
        const auto& [i, o] = get_column_index(unwrapped);
        index = i;
        ok = o;
    );
    if (!ok) {
        avec<object*> fail;
        return {fail, false}; 
    }

    return get_column(index);
}
std::pair<astring, bool> table_object::get_column_name(size_t index) const{
    if (index >= column_datas.size()) {
        return {"Column index out of bounds", false}; }
    return {column_datas[index]->name, true};
}
std::pair<SQL_data_type_object*, bool> table_object::get_column_data_type(size_t index) const{
    if (index >= column_datas.size()) {
        return {new SQL_data_type_object(NULL_TOKEN, NULL_TOKEN, new error_object("Column index out of bounds")), false}; }
    return {column_datas[index]->data_type, true};
}
std::pair<size_t, bool> table_object::get_column_index(const std_and_astring_variant& name) const {
    astring unwraped_name;
    VISIT(name, unwrapped,
        unwraped_name = unwrapped;
    );

    for (size_t i = 0; i < column_datas.size(); i++) {
        avec<table_detail_object*> col_datas = column_datas;
        if (col_datas[i]->name == unwraped_name) {
            return {i, true};
        }
    }

    return {0, false};
}
std::pair<object*, bool> table_object::get_column_default_value(size_t index) const {
    if (index >= column_datas.size()) {
        return {new error_object("Column index out of bounds"), false}; }

    return {column_datas[index]->default_value, true};
}
std::pair<object*, bool> table_object::get_cell_value(size_t row_index, size_t col_index) const {
    if (row_index >= rows.size()) {
        return {new error_object("Row index out of bounds"), false};}

    if (col_index >= column_datas.size()) {
        return {new error_object("Column index out of bounds"), false};}

    group_object* roh = rows[row_index];
    return {roh->elements[col_index], true};
}
std::pair<const avec<object*>&, bool> table_object::get_row_vector(size_t index) const {
    if (index >= rows.size()) {
        return {{new error_object("Row index out of bounds")}, false};}

    group_object* row = rows[index];
    return {row->elements, true};
}
avec<size_t> table_object::get_row_ids() const {
    
    avec<size_t> row_ids(rows.size());
    std::iota(row_ids.begin(), row_ids.end(), 0);
    return row_ids;
}
 bool table_object::check_if_field_name_exists(const std_and_astring_variant& name) const {
    astring unwrapped_name;
    VISIT(name, unwrapped,
        unwrapped_name = unwrapped;
    );
    for (const auto& col_data : column_datas) {
        if (col_data->name == unwrapped_name) {
            return true; }
    }
    return false;
 }


// Table Aggregate Object
// Not sure I should modify the object or use constructor to create a new one using the old one 
                                                            // i.e. tabble_aggregate_object(table_aggregate_object* old, table_object* table, bool clone)...
table_aggregate_object::table_aggregate_object(bool use_arena) : object(use_arena) {

    if (in_arena) {
        tables = avec<table_object*>();
    } else {
        tables = hvec_copy(table_object*);
    }
}
table_aggregate_object::table_aggregate_object(const avec<table_object*>& set_tables, bool use_arena, bool clone) : object(use_arena) {
    
    if (in_arena) {
        if (clone) {
            tables = avec<table_object*>();
            for (const auto& tab : set_tables) {
                tables.push_back(tab->clone(in_arena));
            }
        } else {
            tables = avec<table_object*>();
            for (const auto& tab : set_tables) {
                tables.push_back(tab);
            }
        }
    } else {
        tables = hvec_copy(table_object*);
        for (const auto& table : set_tables) {
            tables.push_back(table->clone(in_arena));
        }
    }
}
table_aggregate_object::~table_aggregate_object() {
    if (!in_arena) {
        for (const auto& table : tables) {
            delete table;
        }
    }
}
astring table_aggregate_object::inspect() const {
    
    astringstream stream;
    stream <<"Contained tables:\n";

    bool first = true;
    for (const auto& table : tables) {
        if (!first) stream << ", ";
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
table_aggregate_object* table_aggregate_object::clone(bool use_arena) const {
    return new (use_arena) table_aggregate_object(tables, use_arena, true);
}
std::pair<size_t, object*> table_aggregate_object::get_col_id(const std_and_astring_variant& column_name) const {

    astring unwrapped_col_name;
    VISIT(column_name, unwrapped,
        unwrapped_col_name = unwrapped;
    );

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
        return {id, new error_object("SELECT FROM: Column index failed to find column (" + unwrapped_col_name + ")")}; 
    }

    return {id, new null_object()};
}
std::pair<size_t, object*> table_aggregate_object::get_col_id(const std_and_astring_variant& table_name, size_t index) const {

    astring unwrapped_table_name;
    VISIT(table_name, unwrapped,
        unwrapped_table_name = unwrapped;
    );  

    bool found_table = false;
    size_t id = 0;
    for (const auto& table : tables) {
        if (table->table_name == unwrapped_table_name) {
            found_table = true;
            if (index >= table->column_datas.size()) {
                astringstream stream;
                stream << "Index (" + index << ") out of bounds for (" << unwrapped_table_name << ")";
                return {id, new error_object(stream.str())}; }
            id += index;
            break;
        } else if (table->column_datas.size() > 0) {
            id += table->column_datas.size() - 1;
        }
    }

    if (!found_table) {
        return {id, new error_object("Failed to find table (" + unwrapped_table_name + ")")}; }

    return {id, new null_object()};
}
std::pair<size_t, object*> table_aggregate_object::get_col_id(const std_and_astring_variant& table_name, const std_and_astring_variant& column_name) const {

    astring unwrapped_table_name;
    VISIT(table_name, unwrapped,
        unwrapped_table_name = unwrapped;
    );
    
    astring unwrapped_col_name;
    VISIT(column_name, unwrapped,
        unwrapped_col_name = unwrapped;
    );

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
        return {id, new error_object("SELECT FROM: Column index failed to find table (" + unwrapped_table_name + ")")}; 
    }

    if (!found_col) {
        return {id, new error_object("SELECT FROM: Column index failed to find column (" + unwrapped_col_name + ")")}; 
    }

    return {id, new null_object()};
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
table_object* table_aggregate_object::combine_tables(const std_and_astring_variant& name) const {

    astring unwrapped_name;
    VISIT(name, unwrapped,
        unwrapped_name = unwrapped;
    );

    size_t total_columns = 0;
    size_t max_rows = 0;
    size_t max_cols = 0;
    for (const auto& table : tables) {
        total_columns += table->column_datas.size();
        max_rows = std::max(max_rows, table->rows.size());
        max_cols = std::max(max_cols, table->column_datas.size());
    }

    hvec(table_detail_object*, column_datas);
    column_datas.reserve(total_columns);

    for (const auto& table : tables) {
        for (const auto& col_data : table->column_datas) {
            column_datas.push_back(col_data);
        }
    }

    hvec(group_object*, rows);
    rows.reserve(max_rows);
    for (size_t row_index = 0; row_index < max_rows; row_index++) {

        hvec(object*, new_row);
        new_row.reserve(max_cols);
        for (const auto& table : tables) {
            
            // Fill in empty rows
            if (row_index >= table->rows.size()) {
                for (size_t col_index = 0; col_index < table->column_datas.size(); col_index++) {
                    new_row.emplace_back(new null_object()); 
                }
                continue; 
            }
            
            const auto& [row, in_bounds] = table->get_row_vector(row_index);
            if (!in_bounds) {
                return new table_object("Weird index bug", {}, {}); }

            for (size_t col_index = 0; col_index < row.size(); col_index++) {
                new_row.push_back(row[col_index]); 
            }

        }   

        rows.emplace_back(new group_object(new_row));
    }

    return new table_object(unwrapped_name, column_datas, rows);
}
void table_aggregate_object::add_table(table_object* table) {
    tables.push_back(table);
}

// Node objects
// Insert into object
insert_into_object::insert_into_object(object* set_table_name, const avec<object*>& set_fields, object* set_values, bool use_arena, bool clone) : object(use_arena) {
    
    if (in_arena) {
        if (clone) {
            table_name = set_table_name->clone(in_arena);

            fields = avec<object*>();
            for (const auto& field : set_fields) {
                fields.push_back(field->clone(in_arena));
            }
            values = set_values->clone(in_arena);
        } else {
            table_name = set_table_name;
            values = set_values;
        }

        fields = avec<object*>();
        for (const auto& field : set_fields) {
            fields.push_back(field);
        }

    } else {
        table_name = set_table_name->clone(in_arena);
        fields = hvec_copy(object*);
        for (const auto& obj : set_fields) {
            fields.push_back(obj->clone(in_arena));
        }
        values = set_values->clone(in_arena);
    }
}
insert_into_object::~insert_into_object() {
    if (!in_arena) {
      delete table_name;
      for (const auto& field : fields) {
        delete field;
      }
      delete values;
    }
}
astring insert_into_object::inspect() const {
    astringstream stream;

    stream << "insert_into: ";
    stream << table_name->inspect() + "\n";

    stream << "[Fields: ";
    bool first = true;
    for (const auto& field : fields) {
        if (!first) stream << ", ";
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
insert_into_object* insert_into_object::clone(bool use_arena) const {
    return new (use_arena) insert_into_object(table_name, fields, values, use_arena, true);
}

// Select object
select_object::select_object(object* set_value, bool use_arena, bool clone) : object(use_arena) {
    
    if (in_arena) {
        if (clone) {
            value = set_value->clone(in_arena);
        } else {
            value = set_value;
        }
    } else {
        value = set_value->clone(in_arena);
    }
}
select_object::~select_object() {
    if (!in_arena) {
      delete value;
    }
}
astring select_object::inspect() const {
    return "select: " + value->inspect();
}
object_type select_object::type() const {
    return SELECT_OBJECT;
}
astring select_object::data() const {
    return "SELECT_OBJECT";
}
select_object* select_object::clone(bool use_arena) const {
    return new (use_arena) select_object(value, true);
}

// Select from object
select_from_object::select_from_object(const avec<object*>& set_column_indexes, const avec<object*>& set_clause_chain, bool use_arena, bool clone) : object(use_arena) {
    
    if (in_arena) {
        if (clone) {
            column_indexes = avec<object*>();
            for (const auto& col_index : set_column_indexes) {
                column_indexes.push_back(col_index->clone(col_index));
            }
    
            clause_chain = avec<object*>();
            for (const auto& clause : set_clause_chain) {
                clause_chain.push_back(clause->clone(in_arena));
            }
        } else {
            column_indexes = avec<object*>();
            for (const auto& col_index : set_column_indexes) {
                column_indexes.push_back(col_index);
            }
    
            clause_chain = avec<object*>();
            for (const auto& clause : set_clause_chain) {
                clause_chain.push_back(clause);
            }
        }
    } else {

        column_indexes = hvec_copy(object*);
        for (const auto& col_index : set_column_indexes) {
            column_indexes.push_back(col_index->clone(in_arena));
        }

        clause_chain = hvec_copy(object*);
        for (const auto& clause : set_clause_chain) {
            clause_chain.push_back(clause->clone(in_arena));
        }
    }
}
select_from_object::~select_from_object() {
    if (!in_arena) {
      for (const auto& col_index : column_indexes) {
        delete col_index;
      }
      for (const auto& cond : clause_chain) {
        delete cond;
      }
    }
}
astring select_from_object::inspect() const {
    astringstream stream;
    stream << "select_from: \n";

    stream << "Column indexes: \n";
    bool first = true;
    for (const auto& col_index : column_indexes) {
        if (!first) stream << ", ";
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
select_from_object* select_from_object::clone(bool use_arena) const {
    return new (use_arena) select_from_object(column_indexes, clause_chain, use_arena, true);
}




// Statements
// block_statement
block_statement::block_statement(const avec<object*>& set_body, bool use_arena, bool clone) : object(use_arena) {
    
    if (in_arena) {
        body = avec<object*>();
        if (clone) {
            for (const auto& statement : set_body) {
                body.push_back(statement->clone(in_arena));
            }
        } else {
            for (const auto& statement : set_body) {
                body.push_back(statement);
            }
        }
    } else {
        body = hvec_copy(object*);  
        for (const auto& statement : set_body) {
            body.push_back(statement->clone(in_arena));
        }
    }
}
block_statement::~block_statement() {
    if (!in_arena) {
        for (const auto& statement : body) {
            delete statement;
        }
    }
}
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
block_statement* block_statement::clone(bool use_arena) const {
    return new (use_arena) block_statement(body, use_arena, true);
}

// if_statement
if_statement::if_statement(object* set_condition, block_statement* set_body, object* set_other, bool use_arena, bool clone) : object(use_arena) {
    
    if (in_arena) {
        if (clone) {
            condition = set_condition->clone(in_arena);
            body = set_body->clone(in_arena);
            other = set_other->clone(in_arena);
        } else {
            condition = set_condition;
            body = set_body;
            other = set_other;
        }
    } else {
        condition = set_condition->clone(in_arena);
        body = set_body->clone(in_arena);
        other = set_other->clone(in_arena);
    }
}
if_statement::~if_statement() {
    if (!in_arena) {
        delete condition;
        delete body;
        delete other;
    }
}
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
if_statement* if_statement::clone(bool use_arena) const {
    return new (use_arena) if_statement(condition, body, other, true);
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
end_if_statement* end_if_statement::clone(bool use_arena) const {
    return new (use_arena) end_if_statement(use_arena);
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
end_statement* end_statement::clone(bool use_arena) const {
    return new (use_arena) end_statement();
}

// return_statement
return_statement::return_statement(object* expr, bool use_arena, bool clone) : object(use_arena) {
    
    if (in_arena) {
        if (clone) {
            expression = expr->clone(in_arena);
        } else {
            expression = expr;
        }
    } else {
        expression = expr->clone(in_arena);
    }
}
return_statement::~return_statement() {
    if (!in_arena) {
        delete expression;
    }
}
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
return_statement* return_statement::clone(bool use_arena) const {
    return new (use_arena) return_statement(expression, use_arena, true);
}



// CUSTOM

// assert_object
assert_object::assert_object(object* expr, size_t set_line, bool use_arena, bool clone) : object(use_arena) {
    
    line = set_line;
    if (in_arena) {
        if (clone) {
            expression = expr->clone(in_arena);
        } else {
            expression = expr;
        }
    } else {
        expression = expr->clone(in_arena);
    }
}
assert_object::~assert_object() {
    if (!in_arena) {
        delete expression;
    }
}
astring assert_object::inspect() const {
    return expression->inspect(); 
}
object_type assert_object::type() const {
    return ASSERT_OBJ; 
}
astring assert_object::data() const {
    return "ASSERT_OBJ"; 
}
assert_object* assert_object::clone(bool use_arena) const {
    return new (use_arena) assert_object(expression, line, true);
}

