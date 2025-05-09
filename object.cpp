
// objects are made in the parser and should probably stay there, used to parse and return values from expressions
// i.e (10 + 10) will return an integer_object with the value 20

#include "object.h"

#include "token.h"
#include "helpers.h"

#include <string>
#include <span>
#include <vector>

#include <iostream> //temp

static std::span<const char* const> object_type_span() {
    static constexpr const char* object_type_chars[] = {
        "ERROR_OBJ", "NULL_OBJ", "INFIX_EXPRESSION_OBJ", "PREFIX_EXPRESSION_OBJ", "INTEGER_OBJ", "DECIMAL_OBJ", "STRING_OBJ", "SQL_DATA_TYPE_OBJ",
        "COLUMN_OBJ", "EVALUATED_COLUMN_OBJ", "FUNCTION_OBJ", "OPERATOR_OBJ", "SEMICOLON_OBJ", "RETURN_VALUE_OBJ", "ARGUMENT_OBJ", "BOOLEAN_OBJ", "FUNCTION_CALL_OBJ",
        "GROUP_OBJ", "PARAMETER_OBJ", "EVALUATED_FUNCTION_OBJ", "VARIABLE_OBJ", "TABLE_DETAIL_OBJECT",

        "IF_STATEMENT", "BLOCK_STATEMENT", "END_IF_STATEMENT", "END_STATEMENT", "RETURN_STATEMENT"
    };
    return object_type_chars;
}

std::string object_type_to_string(object_type index) {
    return std::string(object_type_span()[index]);
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


static std::span<const char* const> operator_type_span() {
    static constexpr const char* operator_type_to_string[] = {
        "ADD_OP", "SUB_OP", "MUL_OP", "DIV_OP", "NEGATE_OP",
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
table_detail_object::table_detail_object(object* set_name, object* set_data_type, object* set_default_value, bool use_arena, bool clone) {
    in_arena = use_arena;
    if (use_arena) {
        if (clone) {
            name = set_name->clone(use_arena);
            data_type = set_data_type->clone(use_arena);
            default_value = set_default_value->clone(use_arena);
        } else {
            name = set_name;
            data_type = set_data_type;
            default_value = set_default_value;
        }
    } else {
        name = set_name->clone(use_arena);
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
    std::string ret_str = "[Type: Table detail, Name: " + name->inspect() + ", " + data_type->inspect();
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
    return new (use_arena) table_detail_object(name, data_type, default_value, use_arena, true);
}

// group_object
group_object::group_object(std::vector<object*> objs, bool use_arena, bool clone) { 
    in_arena = use_arena;
    if (use_arena) {
        void* mem = arena_inst.allocate(sizeof(std::vector<object*>), alignof(std::vector<object*>)); 
        auto vec_ptr = new (mem) std::vector<object*>();                                  
        elements = vec_ptr;                                                             
        arena_inst.register_dtor([vec_ptr]() {                                      
            vec_ptr->std::vector<object*>::~vector();
        });

        for (const auto& obj : objs) {
            if (clone) {
                elements->push_back(obj->clone(use_arena));
            } else{
                elements->push_back(obj);
            }
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
                vec_ptr->std::vector<parameter_object*>::~vector();                                              \
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

