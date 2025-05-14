
// objects are made in the parser and should probably stay there, used to parse and return values from expressions
// i.e (10 + 10) will return an integer_object with the value 20

#include "object.h"

#include "token.h"
#include "helpers.h"
#include "structs_and_macros.h" // For table

#include <string>
#include <span>
#include <vector>

#include <iostream> //temp

static std::span<const char* const> object_type_span() {
    static constexpr const char* object_type_chars[] = {
        "ERROR_OBJ", "NULL_OBJ", "INFIX_EXPRESSION_OBJ", "PREFIX_EXPRESSION_OBJ", "INTEGER_OBJ", "DECIMAL_OBJ", "STRING_OBJ", "SQL_DATA_TYPE_OBJ",
        "COLUMN_OBJ", "EVALUATED_COLUMN_OBJ", "FUNCTION_OBJ", "OPERATOR_OBJ", "SEMICOLON_OBJ", "RETURN_VALUE_OBJ", "ARGUMENT_OBJ", "BOOLEAN_OBJ", "FUNCTION_CALL_OBJ",
        "GROUP_OBJ", "PARAMETER_OBJ", "EVALUATED_FUNCTION_OBJ", "VARIABLE_OBJ", "TABLE_DETAIL_OBJECT", "COLUMN_INDEX_OBJECT", "TABLE_INFO_OBJECT", "TABLE_OBJECT",
        "STAR_SELECT_OBJECT", "TABLE_AGGREGATE_OBJECT",

        "IF_STATEMENT", "BLOCK_STATEMENT", "END_IF_STATEMENT", "END_STATEMENT", "RETURN_STATEMENT",

        "INSERT_INTO_OBJECT", "SELECT_OBJECT", "SELECT_FROM_OBJECT",
    };
    return object_type_chars;
}

std::string object_type_to_string(object_type index) {
    return std::string(object_type_span()[index]);
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
std::string operator_type_to_string(operator_type index) {
    return std::string(operator_type_span()[index]);
}


class arena;
extern arena arena_inst;

void* object::operator new(std::size_t size, bool use_arena) {
    if (use_arena) {
        return arena_inst.allocate(size, alignof(object)); // maybe get rid of second parameter
    }
    else {
        return ::operator new(size);
    }
}   
void object::operator delete([[maybe_unused]] void* ptr, [[maybe_unused]] bool b) noexcept {
    std::cout << "\n\nDELETE BAD!!!!\n\n";
    exit(1);
}
void object::operator delete(void* ptr) noexcept {
    // if it came from the heap, free it; if arena, do nothing

    // if (!ptr) {
    //     std::cout << "tried to delete null ptr\n";
    //     return;
    // }
    // object* obj = static_cast<object*>(ptr);
    // if (!obj->in_arena) {
        ::operator delete(ptr);
    // }

}
void* object::operator new[](std::size_t size, bool use_arena) {
    if (use_arena) {
        return arena_inst.allocate(size, alignof(object));
    }
    else {
        return ::operator new[](size);
    }
}
void object::operator delete[]([[maybe_unused]] void* ptr) noexcept {
    // if (!ptr) return;
    // object* obj = static_cast<object*>(ptr);
    // if (!obj->in_arena) {
    //     // match the array allocation
        ::operator delete[](ptr);
    // }
}



#define arena_alloc_std_string(name, set_name)                                       \
    void* mem = arena_inst.allocate(sizeof(std::string), alignof(std::string)); \
    auto str_ptr = new (mem) std::string(set_name);                                  \
    name = str_ptr;                                                             \
    using RealString = std::basic_string<char, std::char_traits<char>, std::allocator<char>>; \
    arena_inst.register_dtor([str_ptr]() {                                      \
        str_ptr->~RealString();                                                 \
    });

#define arena_alloc_empty_std_string(name)                                       \
    void* mem = arena_inst.allocate(sizeof(std::string), alignof(std::string)); \
    auto str_ptr = new (mem) std::string();                                  \
    name = str_ptr;                                                             \
    using RealString = std::basic_string<char, std::char_traits<char>, std::allocator<char>>; \
    arena_inst.register_dtor([str_ptr]() {                                      \
        str_ptr->~RealString();                                                 \
    });

    #define arena_alloc_std_vector(member, setter, type)                                                        \
        void* member##_mem    = arena_inst.allocate(sizeof(std::vector<type>), alignof(std::vector<type>)); \
        auto member##_ptr = new (member##_mem) std::vector<type>();                                         \
        member   = member##_ptr;                                                                            \
        arena_inst.register_dtor([member##_ptr]() {                                                         \
            member##_ptr->std::vector<type>::~vector();                                                     \
        });                                                                                                 \
        for (const auto& elem : setter) {                                                                   \
            member->push_back(elem);                                                                        \
        }

#define arena_alloc_std_vector_clone(member, setter, type)                                                   \
        void* member##_mem    = arena_inst.allocate(sizeof(std::vector<type>), alignof(std::vector<type>)); \
        auto member##_ptr = new (member##_mem) std::vector<type>();                                         \
        member   = member##_ptr;                                                                            \
        arena_inst.register_dtor([member##_ptr]() {                                                         \
            member##_ptr->std::vector<type>::~vector();                                                     \
        });                                                                                                 \
        for (const auto& elem : setter) {                                                                   \
            member->push_back(elem->clone(use_arena));                                                      \
        }




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
null_object* null_object::clone(bool use_arena) const {
    if (use_arena) {
        std::cout << "\\n\nnCLONED WITH ARENA!null_object!!\n\n\n";}
    return new (use_arena) null_object();
}




// operator_object
operator_object::operator_object(const operator_type type) {
    op_type = type;
}
std::string operator_object::inspect() const {
    return operator_type_to_string(op_type); 
}
object_type operator_object::type() const {
    return OPERATOR_OBJ;
}
std::string operator_object::data() const {
    return operator_type_to_string(op_type);
}
operator_object* operator_object::clone(bool use_arena) const {
    if (use_arena) {
        std::cout << "\\n\nnCLONED WITH ARENA!operator_object!!\n\n\n";}
    return new (use_arena) operator_object(op_type);
}

// Table Info Object
table_info_object::table_info_object(table_object* set_tab, std::vector<size_t> set_col_ids, std::vector<size_t> set_row_ids, bool use_arena, bool clone) {
    in_arena = use_arena;
    if (use_arena) {
        if (clone) {
            tab = set_tab->clone(use_arena);
        } else {
            tab = set_tab;
        }

        arena_alloc_std_vector(col_ids, set_col_ids, size_t);

        arena_alloc_std_vector(row_ids, set_row_ids, size_t);

    } else {
        tab = set_tab->clone(use_arena);
        col_ids = new std::vector<size_t>;
        for (const auto& col_id : set_col_ids) {
            col_ids->push_back(col_id);
        }
        row_ids = new std::vector<size_t>;
        for (const auto& row_id : set_row_ids) {
            row_ids->push_back(row_id);
        }
    }
}
table_info_object::~table_info_object() {
    if (!in_arena) {
        // delete tab; Will be delete automatically I believe
        delete col_ids;
        delete row_ids;
    }
}
std::string table_info_object::inspect() const {
    std::string ret_str = "Table name: " + *tab->table_name + "\n";

    ret_str += "Row ids: \n";
    for (const auto& row_id : *row_ids) {
        ret_str += std::to_string(row_id) + ", ";
    }
    if (row_ids->size() > 0) {
        ret_str = ret_str.substr(0, ret_str.size() - 2);
    }

    ret_str += "\nColumn ids: \n";
    for (const auto& col_id : *col_ids) {
        ret_str += std::to_string(col_id) + ", ";
    }
    if (col_ids->size() > 0) {
        ret_str = ret_str.substr(0, ret_str.size() - 2);
    }

    return ret_str; 
}
object_type table_info_object::type() const {
    return TABLE_INFO_OBJECT;
}
std::string table_info_object::data() const {
    return "TABLE_INFO_OBJECT";
}
table_info_object* table_info_object::clone(bool use_arena) const {
    return new (use_arena) table_info_object(tab, *col_ids, *row_ids, use_arena, true);
}

// infix_expression_object
infix_expression_object::infix_expression_object(operator_object* set_op, object* set_left, object* set_right, bool use_arena, bool clone) {
    in_arena = use_arena;
    if (use_arena) {
        if (clone) {
            op = set_op->clone(use_arena);
            left = set_left->clone(use_arena);
            right = set_right->clone(use_arena);
        } else {
            op = set_op;
            left  = set_left;
            right = set_right;
        }
    } else {
        op = set_op->clone(use_arena);
        left = set_left->clone(use_arena);
        right = set_right->clone(use_arena);
    }
}
std::string infix_expression_object::inspect() const {
    std::string ret_str = "[Op: " + op->inspect();
    ret_str += ". Left: " + left->inspect();
    ret_str += ". Right: " + right->inspect() + "]";
    return ret_str;
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
std::string infix_expression_object::data() const {
    return std::string("INFIX_EXPRESSION_OBJ");
}
infix_expression_object* infix_expression_object::clone(bool use_arena) const {
    if (use_arena) {
        std::cout << "\\n\nnCLONED WITH ARENA!infix_expression_object!!\n\n\n";}
    return new (use_arena) infix_expression_object(op, left, right, use_arena, true);
}
operator_type infix_expression_object::get_op_type() const {
    return op->op_type;
}

// prefix_expression_object
prefix_expression_object::prefix_expression_object(operator_object* set_op, object* set_right, bool use_arena, bool clone) {
    in_arena = use_arena;
    if (use_arena) {
        if (clone) {
            op = set_op->clone(use_arena);
            right = set_right->clone(use_arena);
        } else {
            op = set_op;
            right = set_right;
        }
    } else {
        op = set_op->clone(use_arena);
        right = set_right->clone(use_arena);
    }
}
prefix_expression_object::~prefix_expression_object() {
    if (!in_arena) {
      delete op;
      delete right;
    }
}
std::string prefix_expression_object::inspect() const {
    std::string ret_str = "[Op: " + op->inspect();
    ret_str += ". Right: " + right->inspect() + "]";
    return ret_str;
}
object_type prefix_expression_object::type() const {
    return PREFIX_EXPRESSION_OBJ;
}
std::string prefix_expression_object::data() const {
    return std::string("PREFIX_EXPRESSION_OBJ");
}
prefix_expression_object* prefix_expression_object::clone(bool use_arena) const {
    if (use_arena) {
        std::cout << "\\n\nnCLONED WITH ARENA!prefix_expression_object!!\n\n\n";}
    return new (use_arena) prefix_expression_object(op, right, use_arena, true);
}
operator_type prefix_expression_object::get_op_type() const {
    return op->op_type;
}

// integer_object
integer_object::integer_object() {
    value = 0;
}
integer_object::integer_object(int val) {
    value = val;
}
integer_object::integer_object(const std::string& val) {
    value = std::stoi(val);
}
std::string integer_object::inspect() const {
    return std::to_string(value);
}
object_type integer_object::type() const {
    return INTEGER_OBJ;
}
std::string integer_object::data() const {
    return std::to_string(value);
}
integer_object* integer_object::clone(bool use_arena) const {
    if (use_arena) {
        std::cout << "\\n\nnCLONED WITH ARENA!integer_object!!\n\n\n";}
    return new (use_arena) integer_object(value);
}

// decimal_object
decimal_object::decimal_object() {
    value = 0;
}
decimal_object::decimal_object(double val) {
    value = val;
}
decimal_object::decimal_object(const std::string& val) {
    value = std::stod(val);
}
std::string decimal_object::inspect() const {
    return std::to_string(value);
}
object_type decimal_object::type() const {
    return DECIMAL_OBJ;
}
std::string decimal_object::data() const {
    return std::to_string(value);
}
decimal_object* decimal_object::clone(bool use_arena) const {
    if (use_arena) {
        std::cout << "\\n\nnCLONED WITH ARENA!decimal_object!!\n\n\n";}
    return new (use_arena) decimal_object(value);
}

// string_object
string_object::string_object(std::string const& val, bool use_arena) {
    in_arena = use_arena;
    if (use_arena) {
        arena_alloc_std_string(value, val);
    } else {
        value = new std::string(val);
    }
}
string_object::~string_object() {
    if (!in_arena) {
      delete value;
    }
}
std::string string_object::inspect() const {
    return *value;
}
object_type string_object::type() const {
    return STRING_OBJ;
}
std::string string_object::data() const {
    return *value;
}
string_object* string_object::clone(bool use_arena) const {
    if (use_arena) {
        std::cout << "\\n\nnCLONED WITH ARENA!string_object!!\n\n\n";}
    return new (use_arena) string_object(*value, use_arena);
}

// return_value_object
return_value_object::return_value_object(object* val, bool use_arena, bool clone) {
    in_arena = use_arena;
    if (use_arena) {
        if (clone) {
            value = val->clone(use_arena);
        } else {
            value = val;
        }
    } else {
        value = val->clone(use_arena);
    }
}
return_value_object::~return_value_object() {
    if (!in_arena) {
      delete value;
    }
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
return_value_object* return_value_object::clone(bool use_arena) const {
    if (use_arena) {
        std::cout << "\\n\nnCLONED WITH ARENA!return_value_object!!\n\n\n";}
    return new (use_arena) return_value_object(value, use_arena, true);
}

// argument_object
argument_object::argument_object(const std::string& set_name, object* val, bool use_arena, bool clone) {
    in_arena = use_arena;
    if (use_arena) {
        arena_alloc_std_string(name, set_name);
        if (clone) {
            value = val->clone(use_arena);
        } else {
            value = val;
        }
    } else {
        name = new std::string(set_name);
        value = val->clone(use_arena);
    }
}
argument_object::~argument_object() {
    if (!in_arena) {
      delete name;
      delete value;
    }
}
std::string argument_object::inspect() const {
    return "Name: " + *name + ", Value: " + value->inspect();
}
object_type argument_object::type() const {
    return ARGUMENT_OBJ;
}
std::string argument_object::data() const {
    return *name;
}
argument_object* argument_object::clone(bool use_arena) const {
    if (use_arena) {
        std::cout << "\\n\nnCLONED WITH ARENA!argument_object!!\n\n\n";}
    return new (use_arena) argument_object(*name, value, use_arena, true);
}

// variable_object
variable_object::variable_object(const std::string& set_name, object* val, bool use_arena, bool clone) {
    in_arena = use_arena;
    if (use_arena) {
        arena_alloc_std_string(name, set_name);
        if (clone) {
            value = val->clone(use_arena);
        } else {
            value = val;
        }
    } else {
        name = new std::string(set_name);
        value = val->clone(use_arena);
    }
}
variable_object::~variable_object() {
    if (!in_arena) {
      delete name;
      delete value;
    }
}
std::string variable_object::inspect() const {
    return "[Type: Variable, Name: " + *name + ", Value: " + value->inspect() + "]";
}
object_type variable_object::type() const {
    return VARIABLE_OBJ;
}
std::string variable_object::data() const {
    return *name;
}
variable_object* variable_object::clone(bool use_arena) const {
    if (use_arena) {
        std::cout << "\\n\nnCLONED WITH ARENA!variable_object!!\n\n\n";}
    return new (use_arena) variable_object(*name, value, use_arena, true);
}

// boolean_object
boolean_object::boolean_object(bool val) {
    value = val;
}
std::string boolean_object::inspect() const {
    if (value) {
        return "TRUE"; }
    return "FALSE";
}
object_type boolean_object::type() const {
    return BOOLEAN_OBJ;
}
std::string boolean_object::data() const {
    if (value) {
        return "TRUE"; }
    return "FALSE";
}
boolean_object* boolean_object::clone(bool use_arena) const {
    if (use_arena) {
        std::cout << "\\n\nnCLONED WITH ARENA!boolean_object!!\n\n\n";}
    return new (use_arena) boolean_object(value);
}

//zerofill is implicitly unsigned im pretty sure
// SQL_data_type_object
SQL_data_type_object::SQL_data_type_object(token_type set_prefix, token_type set_data_type, object* set_parameter, bool use_arena, bool clone) {
    in_arena = use_arena;
    if (set_parameter == nullptr) {
        std::cout << "!!!ERROR!!! SQL data type object constructed with null parameter\n"; }
    if (set_data_type == NONE) {
        std::cout << "!!!ERROR!!! SQL data type object constructed without data type token\n"; }
    prefix = set_prefix;
    data_type = set_data_type;
    if (use_arena) {
        if (clone) {
            parameter = set_parameter->clone(use_arena);
        } else {
            parameter = set_parameter;
        }
    } else {
        parameter = set_parameter->clone(use_arena);
    }
}
SQL_data_type_object::~SQL_data_type_object() {
    if (!in_arena) {
      delete parameter;
    }
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
SQL_data_type_object* SQL_data_type_object::clone(bool use_arena) const {
    if (use_arena) {
        std::cout << "\\n\nnCLONED WITH ARENA!SQL_data_type_object!!\n\n\n";}
    return new (use_arena) SQL_data_type_object(prefix, data_type, parameter, use_arena, true);
}

// parameter_object
parameter_object::parameter_object(const std::string& set_name, SQL_data_type_object* set_data_type, bool use_arena, bool clone) {
    in_arena = use_arena;
    if (use_arena) {
        arena_alloc_std_string(name, set_name);
        if (clone) {
            data_type = set_data_type->clone(use_arena);
        } else {
            data_type = set_data_type;
        }
    } else {
        name = new std::string(set_name);
        data_type = set_data_type->clone(use_arena);
    }
}
parameter_object::~parameter_object() {
    if (!in_arena) {
      delete name;
      delete data_type;
    }
}
std::string parameter_object::inspect() const {
    std::string ret_str = "[Type: Parameter, Name: " + *name + ", " + data_type->inspect() + "]";
    return ret_str;
}
object_type parameter_object::type() const {
    return PARAMETER_OBJ;
}
std::string parameter_object::data() const {
    return data_type->data();
}
parameter_object* parameter_object::clone(bool use_arena) const {
    if (use_arena) {
        std::cout << "\\n\nnCLONED WITH ARENA!parameter_object!!\n\n\n";}
    return new (use_arena) parameter_object(*name, data_type, use_arena, true);
}

// table_detail_object
table_detail_object::table_detail_object(std::string set_name, SQL_data_type_object* set_data_type, object* set_default_value, bool use_arena, bool clone) {
    in_arena = use_arena;
    if (use_arena) {
        arena_alloc_std_string(name, set_name);
        if (clone) {
            data_type = set_data_type->clone(use_arena);
            default_value = set_default_value->clone(use_arena);
        } else {
            data_type = set_data_type;
            default_value = set_default_value;
        }
    } else {
        name = new std::string(set_name);
        data_type = set_data_type->clone(use_arena);
        default_value = set_default_value->clone(use_arena);
    }
}
table_detail_object::~table_detail_object() {
    if (!in_arena) {
      delete name;
      delete data_type;
      delete default_value;
    }
}
std::string table_detail_object::inspect() const {
    std::string ret_str = "[Type: Table detail, Name: " + *name + ", " + data_type->inspect();
    ret_str += ", Default value: ";
    if (default_value->type() != NULL_OBJ) {
        ret_str += default_value->inspect(); }
    ret_str += "]";
    return ret_str;
}
object_type table_detail_object::type() const {
    return TABLE_DETAIL_OBJECT;
}
std::string table_detail_object::data() const {
    return name->data();
}
table_detail_object* table_detail_object::clone(bool use_arena) const {
    return new (use_arena) table_detail_object(*name, data_type, default_value, use_arena, true);
}

// group_object
group_object::group_object(std::vector<object*> objs, bool use_arena, bool clone) { 
    in_arena = use_arena;
    if (use_arena) {

        if (clone) {
            arena_alloc_std_vector_clone(elements, objs, object*);
        } else {
            arena_alloc_std_vector(elements, objs, object*);
        }

    } else {
        elements = new std::vector<object*>();  
        for (const auto& obj : objs) {
            elements->push_back(obj->clone(use_arena));
        }
    }
}
group_object::~group_object() {
    if (!in_arena) {
        for (const auto& element : *elements) {
            delete element;
        }
        delete elements;
    }
}
std::string group_object::inspect() const {
    std::string ret_str = "";
    for (const auto& element : *elements) {
        ret_str += element->inspect() + ", ";}
    if (elements->size() > 0) {
        ret_str = ret_str.substr(0, ret_str.length() - 2); }
    return ret_str;
}
object_type group_object::type() const {
    return GROUP_OBJ;
}
std::string group_object::data() const {
    return "GROUP_OBJ";
}
group_object* group_object::clone(bool use_arena) const {
    if (use_arena) {
        std::cout << "\\n\nnCLONED WITH ARENA!group_object!!\n\n\n";}
    return new (use_arena) group_object(*elements, use_arena, true);
}

// function_object
function_object::function_object(const std::string& set_name, group_object* set_parameters, SQL_data_type_object* set_return_type, block_statement* set_body, bool use_arena, bool clone) {
    in_arena = use_arena;
    if (use_arena) {
        arena_alloc_std_string(name, set_name);
        if (clone) {
            parameters  = set_parameters->clone(use_arena);
            body        = set_body->clone(use_arena);
            return_type = set_return_type->clone(use_arena);
        } else {
            parameters  = set_parameters;
            body        = set_body;
            return_type = set_return_type;
        }
    } else {
        name        = new std::string(set_name);
        parameters  = set_parameters->clone(use_arena);
        body        = set_body->clone(use_arena);
        return_type = set_return_type->clone(use_arena);
    }
}
function_object::~function_object() {
    if (!in_arena) {
        delete name;
        delete parameters;
        delete return_type;
        delete body;
    }
}
std::string function_object::inspect() const {
    std::string ret_str = "Function name: " + *name + "\n";
    ret_str += "Arguments: (";
    ret_str += parameters->inspect();

    ret_str += ") ";
    ret_str += "\nReturn type: " + return_type->inspect() + "\n";
    ret_str += "Body:\n";
    ret_str += body->inspect();

    return ret_str;
}
object_type function_object::type() const {
    return FUNCTION_OBJ;
}
std::string function_object::data() const {
    return "FUNCTION_OBJECT";
}
function_object* function_object::clone(bool use_arena) const {
    return new (use_arena) function_object(*name, parameters, return_type, body, use_arena, true);
}

// evaluted_function_object
evaluated_function_object::evaluated_function_object(std::string* set_name, std::vector<parameter_object*>* set_parameters, SQL_data_type_object* set_return_type, block_statement* set_body, bool use_arena, bool clone) {
    in_arena = use_arena;
    if (use_arena) {
        if (clone) {
            arena_alloc_std_string(name, *set_name);

            mem    = arena_inst.allocate(sizeof(std::vector<parameter_object*>), alignof(std::vector<parameter_object*>)); 
            auto vec_ptr = new (mem) std::vector<parameter_object*>();                                  
            parameters   = vec_ptr;                                                             
            arena_inst.register_dtor([vec_ptr]() {                                      
                vec_ptr->std::vector<parameter_object*>::~vector();                                              \
            });
            for (const auto& param : *set_parameters) {
                parameters->push_back(param->clone(use_arena));
            }

            return_type = set_return_type->clone(use_arena);
            body = set_body->clone(use_arena);
        } else {
            name = set_name;
            parameters = set_parameters;
            return_type = set_return_type;
            body = set_body;
        }
    } else {
        name = new std::string(*set_name);
        parameters = new std::vector<parameter_object*>();
        for (const auto& param : *set_parameters) {
            parameters->push_back(param->clone(use_arena));
        }
        return_type = set_return_type->clone(use_arena);
        body = set_body->clone(use_arena);
    }

}
evaluated_function_object::evaluated_function_object(function_object* func, std::vector<parameter_object*> new_parameters, bool use_arena, bool clone) {
    in_arena = use_arena;
    if (use_arena) {
        if (clone) {
            arena_alloc_std_string(name, *func->name);

            mem    = arena_inst.allocate(sizeof(std::vector<parameter_object*>), alignof(std::vector<parameter_object*>)); 
            auto vec_ptr = new (mem) std::vector<parameter_object*>();                                  
            parameters   = vec_ptr;                                                             
            arena_inst.register_dtor([vec_ptr]() {                                      
                vec_ptr->std::vector<parameter_object*>::~vector();                                              \
            });
            for (const auto& param : new_parameters) {
                parameters->push_back(param->clone(use_arena));
            }

            return_type  = func->return_type->clone(use_arena);
            body = func->body->clone(use_arena);
        } else {
            name = func->name;
    
            void* mem    = arena_inst.allocate(sizeof(std::vector<parameter_object*>), alignof(std::vector<parameter_object*>)); 
            auto vec_ptr = new (mem) std::vector<parameter_object*>();                                  
            parameters   = vec_ptr;                                                             
            arena_inst.register_dtor([vec_ptr]() {                                      
                vec_ptr->std::vector<parameter_object*>::~vector();                                              
            });
            for (const auto& param : new_parameters) {
                parameters->push_back(param);
            }
    
            return_type  = func->return_type;
            body = func->body;
        }
    } else {
        name = new std::string(*(func->name));
        parameters = new std::vector<parameter_object*>();
        for (const auto& param : new_parameters) {
            parameters->push_back(param->clone(use_arena));
        }
        return_type = func->return_type->clone(use_arena);
        body = func->body->clone(use_arena);
    }
}
evaluated_function_object::~evaluated_function_object() {
    if (!in_arena) {
        delete name;
        for (const auto& param : *parameters) {
            delete param;
        }
        delete parameters;
        delete return_type;
        delete body;
    }
}
std::string evaluated_function_object::inspect() const {
    std::string ret_str = "Function name: " + *name + "\n";
    ret_str += "Arguments: (";
    for (const auto& arg : *parameters) {
        ret_str += arg->inspect() + ", "; 
    }

    if (parameters->size() > 0) {
        ret_str = ret_str.substr(0, ret_str.size() - 2); }

    ret_str += ")";
    ret_str += "\nReturn type: " + return_type->inspect() + "\n";
    ret_str += "Body:\n";
    ret_str += body->inspect();

    return ret_str;
}
object_type evaluated_function_object::type() const {
    return EVALUATED_FUNCTION_OBJ;
}
std::string evaluated_function_object::data() const {
    return "EVALUATED_FUNCTION_OBJ";
}
evaluated_function_object* evaluated_function_object::clone(bool use_arena) const {
    if (use_arena) {
        std::cout << "\\n\nnCLONED WITH ARENA!evaluated_function_object!!\n\n\n";}
    return new (use_arena) evaluated_function_object(name, parameters, return_type, body, use_arena, true);
}

// function_call_object
function_call_object::function_call_object(const std::string& set_name, group_object* args, bool use_arena, bool clone) {
    in_arena = use_arena;
    if (use_arena) {
        arena_alloc_std_string(name, set_name);
        if (clone) {
            arguments = args->clone(use_arena);
        } else {
            arguments = args;
        }
    } else {
        name = new std::string(set_name);
        arguments = args->clone(use_arena);
    }
}
function_call_object::~function_call_object() {
    if (!in_arena) {
        delete name;
        delete arguments;
    }
}
std::string function_call_object::inspect() const {
    std::string ret_str = *name + "(";
    ret_str += arguments->inspect();
    ret_str += ")";
    return ret_str;
}
object_type function_call_object::type() const {
    return FUNCTION_CALL_OBJ;
}
std::string function_call_object::data() const {
    return "FUNCTION_CALL_OBJ";
}
function_call_object* function_call_object::clone(bool use_arena) const {
    if (use_arena) {
        std::cout << "\\n\nnCLONED WITH ARENA!function_call_object!!\n\n\n";}
    return new (use_arena) function_call_object(*name, arguments, use_arena, true);
}

// column_object
column_object::column_object(object* set_name_data_type, bool use_arena, bool clone) {
    in_arena = use_arena;
    if (use_arena) {
        if (clone) {
            name_data_type = set_name_data_type->clone(use_arena);
        } else {
            name_data_type = set_name_data_type;
        }
        default_value = new (use_arena) null_object();
    } else {
        name_data_type = set_name_data_type->clone(use_arena);
        default_value = new (use_arena) null_object();
    }
}
column_object::column_object(object* set_name_data_type, object* default_val, bool use_arena, bool clone) {
    in_arena = use_arena;
    if (use_arena) {
        if (clone) {
            name_data_type = set_name_data_type->clone(use_arena);
            default_value = default_val->clone(use_arena);
        } else {
            name_data_type = set_name_data_type;
            default_value = default_val;
        }
    } else {
        name_data_type = set_name_data_type->clone(use_arena);
        default_value = default_val->clone(use_arena);
    }
}
column_object::~column_object() {
    if (!in_arena) {
        delete name_data_type;
        delete default_value;
    }
}
std::string column_object::inspect() const {
    std::string str = "[Column: ";
    str += name_data_type->inspect();
    str += ", Default: " + default_value->inspect()  + "]\n";
    return str;
}
object_type column_object::type() const {
    return COLUMN_OBJ;
}
std::string column_object::data() const {
    return "COLUMN_OBJ";
}
column_object* column_object::clone(bool use_arena) const {
    return new (use_arena) column_object(name_data_type, default_value, use_arena, true);
}

// evaluated_column_object
evaluated_column_object::evaluated_column_object(const std::string& set_name, SQL_data_type_object* type, const std::string& default_val, bool use_arena, bool clone) {
    in_arena = use_arena;
    if (use_arena) {
        { arena_alloc_std_string(name, set_name); }
        if (clone) {
            data_type = type->clone(use_arena);
        } else {
            data_type = type;
        }
        arena_alloc_std_string(default_value, default_val);
    } else {
        name = new std::string(set_name);
        data_type = type->clone(use_arena);
        default_value = new std::string(default_val);
    }
}
evaluated_column_object::~evaluated_column_object() {
    if (!in_arena) {
        delete name;
        delete data_type;
        delete default_value;
    }
}
std::string evaluated_column_object::inspect() const {
    std::string str = "[Column name: " + *name + "], ";
    str += data_type->inspect();
    str += ", [Default: " + *default_value  + "]\n";
    return str;
}
object_type evaluated_column_object::type() const {
    return EVALUATED_COLUMN_OBJ;
}
std::string evaluated_column_object::data() const {
    return "EVALUATED_COLUMN_OBJ";
}
evaluated_column_object* evaluated_column_object::clone(bool use_arena) const {
    if (use_arena) {
        std::cout << "\\n\nnCLONED WITH ARENA!evaluated_column_object!!\n\n\n";}
    return new (use_arena) evaluated_column_object(*name, data_type, *default_value, use_arena, true);
}

// error_object
error_object::error_object(bool use_arena) {
    in_arena = use_arena;
    if (use_arena) {
        arena_alloc_empty_std_string(value);
    } else {
        value = new std::string();
    }
}
error_object::error_object(const std::string& val, bool use_arena) {
    if (use_arena) {
        arena_alloc_std_string(value, val);
    } else {
        value = new std::string(val);
    }
}
error_object::~error_object() {
    if (!in_arena) {
        delete value;
    }
}
std::string error_object::inspect() const {
    return "ERROR: " + *value;
}
object_type error_object::type() const {
    return ERROR_OBJ;
}
std::string error_object::data() const {
    return *value;
}
error_object* error_object::clone(bool use_arena) const {
    return new (use_arena) error_object(*value, use_arena);
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
semicolon_object* semicolon_object::clone(bool use_arena) const {
    if (use_arena) {
        std::cout << "\\n\nnCLONED WITH ARENA!semicolon_object!!\n\n\n";}
    return new (use_arena) semicolon_object();
}

// Start Select Object
std::string star_select_object::inspect() const {
    return "STAR_SELECT_OBJECT";
}
object_type star_select_object::type() const {
    return STAR_SELECT_OBJECT;
}
std::string star_select_object::data() const {
    return "STAR_SELECT_OBJECT";
}
star_select_object* star_select_object::clone(bool use_arena) const {
    return new (use_arena) star_select_object();
}

// Column index object
column_index_object::column_index_object(object* set_table_name, object* set_column_name, bool use_arena, bool clone) {
    in_arena = use_arena;
    if (use_arena) {
        if (clone) {
            table_name = set_table_name->clone(use_arena);
            column_name = set_column_name->clone(use_arena);
        } else {
            table_name = set_table_name;
            column_name = set_column_name;
        }
    } else {
        table_name = set_table_name->clone(use_arena);
        column_name = set_column_name->clone(use_arena);
    }
}
column_index_object::~column_index_object() {
    if (!in_arena) {
        delete table_name;
        delete column_name;
    }
}
std::string column_index_object::inspect() const {
    return "[Table name: " + table_name->inspect() + ", Column name: " + column_name->inspect() + "]";
}
object_type column_index_object::type() const {
    return COLUMN_INDEX_OBJECT;
}
std::string column_index_object::data() const {
    return "COLUMN_INDEX_OBJECT";
}
column_index_object* column_index_object::clone(bool use_arena) const {
    return new (use_arena) column_index_object(table_name, column_name, use_arena, true);
}



// Table object
table_object::table_object(std::string set_table_name, std::vector<table_detail_object*> set_column_datas, std::vector<group_object*> set_rows, bool use_arena, bool clone) {
    in_arena = use_arena;
    if (use_arena) {
        arena_alloc_std_string(table_name, set_table_name);

        if (clone) {
            arena_alloc_std_vector_clone(column_datas, set_column_datas, table_detail_object*);
            arena_alloc_std_vector_clone(rows, set_rows, group_object*);
        } else {
            arena_alloc_std_vector(column_datas, set_column_datas, table_detail_object*);
            arena_alloc_std_vector(rows, set_rows, group_object*);
        }

    } else {
        table_name = new std::string(set_table_name);
        column_datas = new std::vector<table_detail_object*>;
        for (const auto& col_data : set_column_datas) {
            column_datas->push_back(col_data->clone(use_arena));
        }
        rows = new std::vector<group_object*>;
        for (const auto& roh : set_rows) {
            rows->push_back(roh->clone(use_arena));
        }
    }
}
table_object::~table_object() {
    if (!in_arena) {
        delete table_name;
        for (const auto& col_data : *column_datas) {
            delete col_data;
        }
        delete column_datas;
        for (const auto& roh : *rows) {
            delete roh;
        }
        delete rows;
    }
}
std::string table_object::inspect() const {
    std::string ret_str = "Table name: " + *table_name  + "\n";

    ret_str += "Column data";
    for (const auto& col : *column_datas) {
        ret_str += col->inspect() + ", ";
    }
    if (column_datas->size() > 0) {
        ret_str = ret_str.substr(0, ret_str.size() - 2);
    }
    
    ret_str += "Rows: \n";
    for (const auto& roh : *rows) {
        ret_str += roh->inspect() + "\n";
    }

    return ret_str;
}
object_type table_object::type() const {
    return TABLE_OBJECT;
}
std::string table_object::data() const {
    return "TABLE_OBJECT";
}
table_object* table_object::clone(bool use_arena) const {
    return new (use_arena) table_object(*table_name, *column_datas, *rows, use_arena, true);
}
std::pair<std::string, bool> table_object::get_column_name(size_t index) const{
    if (index >= column_datas->size()) {
        return {"Column index out of bounds", false}; }
    return {*(*column_datas)[index]->name, true};
}
std::pair<SQL_data_type_object*, bool> table_object::get_column_data_type(size_t index) const{
    if (index >= column_datas->size()) {
        return {new SQL_data_type_object(NULL_TOKEN, NULL_TOKEN, new error_object("Column index out of bounds")), false}; }
    return {(*column_datas)[index]->data_type, true};
}
std::pair<size_t, bool> table_object::get_column_index(const std::string& name) const {
    for (size_t i = 0; i < column_datas->size(); i++) {
        std::vector<table_detail_object*> col_datas = *column_datas;
        if (*(col_datas[i]->name) == name) {
            return {i, true};
        }
    }

    return {0, false};
}
std::pair<object*, bool> table_object::get_column_default_value(size_t index) const {
    if (index >= column_datas->size()) {
        return {new error_object("Column index out of bounds"), false}; }

    std::vector<table_detail_object*> columns = (*column_datas);
    return {columns[index]->default_value, true};
}
std::pair<object*, bool> table_object::get_cell_value(size_t row_index, size_t col_index) const {
    if (row_index >= rows->size()) {
        return {new error_object("Row index out of bounds"), false};}

    if (col_index >= column_datas->size()) {
        return {new error_object("Column index out of bounds"), false};}

    group_object* roh = (*rows)[row_index];
    return {(*roh->elements)[col_index], true};
}
std::pair<std::vector<object*>, bool> table_object::get_row_vector(size_t index) const {
    if (index >= rows->size()) {
        return {{new error_object("Row index out of bounds")}, false};}

    group_object* row = (*rows)[index];
    return {(*row->elements), true};
}
std::vector<size_t> table_object::get_row_ids() const {
    
    std::vector<size_t> row_ids;
    row_ids.reserve(rows->size());
    for (size_t i = 0; i < rows->size(); i++) {
        row_ids.push_back(i);
    }

    return row_ids;
}
 bool table_object::check_if_field_name_exists(const std::string& name) const {
    for (const auto& col_data : *column_datas) {
        if (*col_data->name == name) {
            return true; }
    }
    return false;
 }


// Table Aggregate Object
// Not sure I should modify the object or use constructor to create a new one using the old one 
                                                            // i.e. tabble_aggregate_object(table_aggregate_object* old, table_object* table, bool use_arena, bool clone)...
table_aggregate_object::table_aggregate_object(bool use_arena) {
    in_arena = use_arena;
    if (use_arena) {

        void* mem = arena_inst.allocate(sizeof(std::vector<table_object*>), alignof(std::vector<table_object*>)); 
        auto vec_ptr = new (mem) std::vector<table_object*>();                                  
        tables = vec_ptr;                                                             
        arena_inst.register_dtor([vec_ptr]() {                                      
            vec_ptr->std::vector<table_object*>::~vector();
        });


    } else {
        tables = new std::vector<table_object*>;
    }
}
table_aggregate_object::table_aggregate_object(std::vector<table_object*> set_tables, bool use_arena, bool clone) {
    in_arena = use_arena;
    if (use_arena) {

        if (clone) {
            arena_alloc_std_vector_clone(tables, set_tables, table_object*);
        } else {
            arena_alloc_std_vector(tables, set_tables, table_object*);
        }


    } else {
        tables = new std::vector<table_object*>;
        for (const auto& table : set_tables) {
            tables->push_back(table->clone(use_arena));
        }
    }
}
table_aggregate_object::~table_aggregate_object() {
    if (!in_arena) {
        for (const auto& table : *tables) {
            delete table;
        }
        delete tables;
    }
}
std::string table_aggregate_object::inspect() const {
    std::string ret_str = "Contained tables:\n";

    for (const auto& table : *tables) {
        ret_str += *table->table_name = ", "; }

    if (tables->size() > 0) {
        ret_str = ret_str.substr(0, ret_str.size() - 2); }

    return ret_str; 
}
object_type table_aggregate_object::type() const {
    return TABLE_AGGREGATE_OBJECT;
}
std::string table_aggregate_object::data() const {
    return "TABLE_AGGREGATE_OBJECT";
}
table_aggregate_object* table_aggregate_object::clone(bool use_arena) const {
    return new (use_arena) table_aggregate_object(*tables, use_arena, true);
}
std::pair<size_t, object*> table_aggregate_object::get_col_id(std::string column_name) const {

    bool found_col = false;
    size_t id = 0;
    for (const auto& table : *tables) {
        for (const auto& col_data : *table->column_datas) {
            if (*col_data->name == column_name) {
                found_col = true;
                break;
            }
            id++;
        }
    }

    if (!found_col) {
        return {id, new error_object("SELECT FROM: Column index failed to find column (" + column_name + ")")}; 
    }

    return {id, new null_object()};
}
std::pair<size_t, object*> table_aggregate_object::get_col_id(std::string table_name, std::string column_name) const {

    bool found_table = false;
    bool found_col = false;
    size_t id = 0;
    for (const auto& table : *tables) {
        if (*table->table_name == table_name) {
            found_table = true;
            for (const auto& col_data : *table->column_datas) {
                if (*col_data->name == column_name) {
                    found_col = true;
                    break;
                }

                id += 1;
            }
            break;
        } else if (table->column_datas->size() > 0) {
            id += table->column_datas->size() - 1;
        }
    }

    if (!found_table) {
        return {id, new error_object("SELECT FROM: Column index failed to find table (" + table_name + ")")}; 
    }

    if (!found_col) {
        return {id, new error_object("SELECT FROM: Column index failed to find column (" + column_name + ")")}; 
    }

    return {id, new null_object()};
}
std::vector<size_t> table_aggregate_object::get_all_col_ids() const {

    std::vector<size_t> col_ids;
    size_t cur_id = 0;
    for (const auto& table : *tables) {

        col_ids.reserve(table->column_datas->size() + col_ids.capacity());
        for (size_t i = 0; i < table->column_datas->size(); i++) {
            col_ids.push_back(cur_id++);
        }
    }

    return col_ids;
}
std::pair<std::string, bool> table_aggregate_object::get_table_name(size_t index) const {
    if (index >= tables->size()) {
        return {"", false}; }
    return {*(*tables)[index]->table_name, true};
}
table_object* table_aggregate_object::combine_tables(const std::string& name) const {

    std::vector<table_detail_object*> column_datas;
    std::vector<group_object*> rows;

    size_t max_rows = 0;
    size_t max_cols = 0;
    for (const auto& table : *tables) {

        if (table->rows->size() > max_rows) {
            max_rows = table->rows->size(); }

        if (table->column_datas->size() > max_cols) {
            max_cols = table->column_datas->size(); }

        column_datas.reserve(table->column_datas->size() + column_datas.capacity());
        for (const auto& col_data : *table->column_datas) {
            column_datas.push_back(col_data);
        }
    }

    rows.reserve(max_rows);
    for (size_t row_index = 0; row_index < max_rows; row_index++) {

        std::vector<object*> new_row;
        new_row.reserve(max_cols);
        for (const auto& table : *tables) {
            
            if (row_index >= table->rows->size()) {
                for (size_t col_index = 0; col_index < table->column_datas->size(); col_index++) {
                    new_row.emplace_back(new null_object()); 
                }
                continue; 
            }
            
            const auto& [row, in_bounds] = table->get_row_vector(row_index);
            if (!in_bounds) {
                return new table_object("Weird index bug", {}, {}); }

            std::vector<object*> table_row = row;

            for (size_t col_index = 0; col_index < table_row.size(); col_index++) {
                new_row.push_back(table_row[col_index]); 
            }

        }   

        rows.emplace_back(new group_object(new_row));
    }

    return new table_object(name, column_datas, rows);
}
void table_aggregate_object::add_table(table_object* table) {
    tables->push_back(table);
}

// Node objects
// Insert into object
insert_into_object::insert_into_object(object* set_table_name, std::vector<object*> set_fields, object* set_values, bool use_arena, bool clone) {
    in_arena = use_arena;
    if (use_arena) {
        if (clone) {
            table_name = set_table_name->clone(use_arena);
            values = set_values->clone(use_arena);
        } else {
            table_name = set_table_name;
            values = set_values;
        }

        void* mem = arena_inst.allocate(sizeof(std::vector<object*>), alignof(std::vector<object*>)); 
        auto vec_ptr = new (mem) std::vector<object*>();                                  
        fields = vec_ptr;                                                             
        arena_inst.register_dtor([vec_ptr]() {                                      
            vec_ptr->std::vector<object*>::~vector();                                              
        });
        for (const auto& obj : set_fields) {
            if (clone) {
                fields->push_back(obj->clone(use_arena));
            } else {
                fields->push_back(obj);
            }
        }

    } else {
        table_name = set_table_name->clone(use_arena);
        fields = new std::vector<object*>;
        for (const auto& obj : set_fields) {
            fields->push_back(obj->clone(use_arena));
        }
        values = set_values->clone(use_arena);
    }
}
insert_into_object::~insert_into_object() {
    if (!in_arena) {
      delete table_name;
      for (const auto& field : *fields) {
        delete field;
      }
      delete fields;
      delete values;
    }
}
std::string insert_into_object::inspect() const {
    std::string ret_str = "";
    ret_str += "insert_into: ";
    ret_str += table_name->inspect() + "\n";

    ret_str += "[Fields: ";
    for (const auto& field : *fields) {
        ret_str += field->inspect() + ", "; }
    if (fields->size() > 0) {
        ret_str = ret_str.substr(0, ret_str.size() - 2); }

    ret_str += "], [Values: ";
    ret_str += values->inspect();

    ret_str += "]\n";
    return ret_str;
}
object_type insert_into_object::type() const {
    return INSERT_INTO_OBJECT; 
}
std::string insert_into_object::data() const {
    return "INSERT_INTO_OBJECT";
}
insert_into_object* insert_into_object::clone(bool use_arena) const {
    return new (use_arena) insert_into_object(table_name, *fields, values, use_arena, true);
}

// Select object
select_object::select_object(object* set_value, bool use_arena, bool clone) {
    in_arena = use_arena;
    if (use_arena) {
        if (clone) {
            value = set_value->clone(use_arena);
        } else {
            value = set_value;
        }
    } else {
        value = set_value->clone(use_arena);
    }
}
select_object::~select_object() {
    if (!in_arena) {
      delete value;
    }
}
std::string select_object::inspect() const {
    std::string ret_str = "select: " + value->inspect();
    return ret_str;
}
object_type select_object::type() const {
    return SELECT_OBJECT;
}
std::string select_object::data() const {
    return "SELECT_OBJECT";
}
select_object* select_object::clone(bool use_arena) const {
    return new (use_arena) select_object(value, use_arena, true);
}

// Select from object
select_from_object::select_from_object(std::vector<object*> set_column_indexes, std::vector<object*> set_clause_chain, bool use_arena, bool clone) {
    in_arena = use_arena;
    if (use_arena) {

        void* mem = arena_inst.allocate(sizeof(std::vector<object*>), alignof(std::vector<object*>)); 
        auto vec_ptr = new (mem) std::vector<object*>();                                  
        column_indexes = vec_ptr;                                                             
        arena_inst.register_dtor([vec_ptr]() {                                      
            vec_ptr->std::vector<object*>::~vector();                                              
        });
        for (const auto& obj : set_column_indexes) {
            if (clone) {
                column_indexes->push_back(obj->clone(use_arena));
            } else {
                column_indexes->push_back(obj);
            }
        }

        mem = arena_inst.allocate(sizeof(std::vector<object*>), alignof(std::vector<object*>)); 
        auto cond_ptr = new (mem) std::vector<object*>();                                  
        clause_chain = cond_ptr;                                                             
        arena_inst.register_dtor([cond_ptr]() {                                      
            cond_ptr->std::vector<object*>::~vector();                                              
        });
        for (const auto& obj : set_clause_chain) {
            if (clone) {
                clause_chain->push_back(obj->clone(use_arena));
            } else {
                clause_chain->push_back(obj);
            }
        }

    } else {

        column_indexes = new std::vector<object*>();
        for (const auto& col_index : set_column_indexes) {
            column_indexes->push_back(col_index->clone(use_arena));
        }

        clause_chain = new std::vector<object*>();
        for (const auto& clause : set_clause_chain) {
            clause_chain->push_back(clause->clone(use_arena));
        }
    }
}
select_from_object::~select_from_object() {
    if (!in_arena) {
      for (const auto& col_index : *column_indexes) {
        delete col_index;
      }
      delete column_indexes;
      for (const auto& cond : *clause_chain) {
        delete cond;
      }
      delete clause_chain;
    }
}
std::string select_from_object::inspect() const {
    std::string ret_str = "select_from: \n";

    ret_str += "Column indexes: \n";
    for (const auto& col_index : *column_indexes) {
        ret_str += col_index->inspect() + ", ";}
    if (column_indexes->size() > 0) {
        ret_str = ret_str.substr(0, ret_str.size() - 2); }


    if (clause_chain->size() == 1) {
        ret_str += "Clause: \n";
    } else if (clause_chain->size() > 1) {
        ret_str += "Clauses: \n"; }
    for (const auto& cond : *clause_chain) {
        ret_str += cond->inspect() + "\n";
    }
    
    ret_str += "\n";
    return ret_str;

}
object_type select_from_object::type() const {
    return SELECT_FROM_OBJECT;
}
std::string select_from_object::data() const {
    return "SELECT_FROM_OBJECT";
}
select_from_object* select_from_object::clone(bool use_arena) const {
    return new (use_arena) select_from_object(*column_indexes, *clause_chain, use_arena, true);
}




// Statements
// block_statement
block_statement::block_statement(std::vector<object*> set_body, bool use_arena, bool clone) {
    in_arena = use_arena;
    if (use_arena) {
        void* mem = arena_inst.allocate(sizeof(std::vector<object*>), alignof(std::vector<object*>)); 
        auto vec_ptr = new (mem) std::vector<object*>();                                  
        body = vec_ptr;                                                             
        arena_inst.register_dtor([vec_ptr]() {                                      
            vec_ptr->std::vector<object*>::~vector();                                              \
        });

        for (const auto& statement : set_body) {
            if (clone) {
                body->push_back(statement->clone(use_arena));
            } else {
                body->push_back(statement);
            }
        }
    } else {
        body = new std::vector<object*>();  
        for (const auto& statement : set_body) {
            body->push_back(statement->clone(use_arena));
        }
    }
}
block_statement::~block_statement() {
    if (!in_arena) {
        for (const auto& statement : *body) {
            delete statement;
        }
        delete body;
    }
}
std::string block_statement::inspect() const {
    std::string ret_str = "";
    for (const auto& statement : *body) {
        ret_str += statement->inspect() + "\n";}
    return ret_str;
}
object_type block_statement::type() const {
    return BLOCK_STATEMENT; 
}
std::string block_statement::data() const {
    return "BLOCK_STATEMENT"; 
}
block_statement* block_statement::clone(bool use_arena) const {
    if (use_arena) {
        std::cout << "\\n\nnCLONED WITH ARENA!block_statement!!\n\n\n";}
    return new (use_arena) block_statement(*body, use_arena, true);
}

// if_statement
if_statement::if_statement(object* set_condition, block_statement* set_body, object* set_other, bool use_arena, bool clone) {
    in_arena = use_arena;
    if (use_arena) {
        if (clone) {
            condition = set_condition->clone(use_arena);
            body = set_body->clone(use_arena);
            other = set_other->clone(use_arena);
        } else {
            condition = set_condition;
            body = set_body;
            other = set_other;
        }
    } else {
        condition = set_condition->clone(use_arena);
        body = set_body->clone(use_arena);
        other = set_other->clone(use_arena);
    }
}
if_statement::~if_statement() {
    if (!in_arena) {
        delete condition;
        delete body;
        delete other;
    }
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
if_statement* if_statement::clone(bool use_arena) const {
    if (use_arena) {
        std::cout << "\\n\nnCLONED WITH ARENA!if_statement!!\n\n\n";}
    return new (use_arena) if_statement(condition, body, other, use_arena, true);
}

// end_if_statement
std::string end_if_statement::inspect() const {
    return "END_IF_STATEMENT"; 
}
object_type end_if_statement::type() const {
    return END_IF_STATEMENT; 
}
std::string end_if_statement::data() const {
    return "END_IF_STATEMENT"; 
}
end_if_statement* end_if_statement::clone(bool use_arena) const {
    return new (use_arena) end_if_statement();
}

// end_statement
std::string end_statement::inspect() const {
    return "END_STATEMENT"; 
}
object_type end_statement::type() const {
    return END_STATEMENT; 
}
std::string end_statement::data() const {
    return "END_STATEMENT"; 
}
end_statement* end_statement::clone(bool use_arena) const {
    if (use_arena) {
        std::cout << "\\n\nnCLONED WITH ARENA!end_statement!!\n\n\n";}
    return new (use_arena) end_statement();
}

// return_statement
return_statement::return_statement(object* expr, bool use_arena, bool clone) {
    in_arena = use_arena;
    if (use_arena) {
        if (clone) {
            expression = expr->clone(use_arena);
        } else {
            expression = expr;
        }
    } else {
        expression = expr->clone(use_arena);
    }
}
return_statement::~return_statement() {
    if (!in_arena) {
        delete expression;
    }
}
std::string return_statement::inspect() const {
    std::string ret_string = "[Type: Return statement, Value: ";
    ret_string += expression->inspect() + "]";
    return ret_string; 
}
object_type return_statement::type() const {
    return RETURN_STATEMENT; 
}
std::string return_statement::data() const {
    return "RETURN_STATEMENT"; 
}
return_statement* return_statement::clone(bool use_arena) const {
    if (use_arena) {
        std::cout << "\\n\nnCLONED WITH ARENA!return_statement!!\n\n\n";}
    return new (use_arena) return_statement(expression, use_arena, true);
}

