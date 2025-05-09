#pragma once

// objects are made in the parser and should probably stay there, used to parse and return values from expressions
    // i.e (10 + 10) will return an integer_object with the value 20

#include "token.h"
#include "arena.h"

#include <string>
#include <vector>

enum object_type {
    ERROR_OBJ, NULL_OBJ, INFIX_EXPRESSION_OBJ, PREFIX_EXPRESSION_OBJ, INTEGER_OBJ, DECIMAL_OBJ, STRING_OBJ, SQL_DATA_TYPE_OBJ,
    COLUMN_OBJ, EVALUATED_COLUMN_OBJ, FUNCTION_OBJ, OPERATOR_OBJ, SEMICOLON_OBJ, RETURN_VALUE_OBJ, ARGUMENT_OBJ, BOOLEAN_OBJ, FUNCTION_CALL_OBJ,
    GROUP_OBJ, PARAMETER_OBJ, EVALUATED_FUNCTION_OBJ, VARIABLE_OBJ, TABLE_DETAIL_OBJECT,

    IF_STATEMENT, BLOCK_STATEMENT, END_IF_STATEMENT, END_STATEMENT, RETURN_STATEMENT,
};

std::string object_type_to_string(object_type index);

class object {

    public:
    virtual std::string inspect() const = 0;  
    virtual object_type type() const = 0;    
    virtual std::string data() const = 0;    
    virtual object* clone(bool use_arena) const = 0;    
    virtual ~object() noexcept = default; 
    
    
    static void* operator new(std::size_t size, bool use_arena = true);
    static void  operator delete([[maybe_unused]] void* ptr, [[maybe_unused]] bool b) noexcept;
    static void  operator delete(void* ptr) noexcept;
    static void* operator new[](std::size_t size, bool use_arena = true);
    static void  operator delete[]([[maybe_unused]] void* p) noexcept;

    public:
    bool in_arena = true;
    
};

class null_object : public object {

    public:
    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
    null_object* clone(bool use_arena) const override;
};

enum operator_type {
    ADD_OP, SUB_OP, MUL_OP, DIV_OP, NEGATE_OP,
    EQUALS_OP, NOT_EQUALS_OP, LESS_THAN_OP, LESS_THAN_OR_EQUAL_TO_OP, GREATER_THAN_OP, GREATER_THAN_OR_EQUAL_TO_OP,
    OPEN_PAREN_OP, OPEN_BRACKET_OP,
    DOT_OP,
};


class operator_object : public object {
    public:
    operator_object(const operator_type type);

    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
    operator_object* clone(bool use_arena) const override;

    public:
    operator_type op_type;
};

class infix_expression_object : public object {

    public:
    infix_expression_object(operator_object* set_op, object* set_left, object* set_right, bool use_arena = true, bool clone = false);
    ~infix_expression_object();
    
    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
    infix_expression_object* clone(bool use_arena) const override;

    public:
    operator_object* op;
    object* left;
    object* right;
};

class prefix_expression_object : public object {

    public:
    prefix_expression_object(operator_object* set_op, object* set_right, bool use_arena = true, bool clone = false);
    ~prefix_expression_object();

    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
    prefix_expression_object* clone(bool use_arena) const override;

    public:
    operator_object* op;
    object* right;
};

class integer_object : public object {

    public:
    integer_object();
    integer_object(int val);
    integer_object(const std::string& val);

    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
    integer_object* clone(bool use_arena) const override;

    public:
    int value;
};

class decimal_object : public object {

    public:
    decimal_object();
    decimal_object(double val);
    decimal_object(const std::string& val);

    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
    decimal_object* clone(bool use_arena) const override;

    public:
    double value; // value can be cast to float later
};

class string_object : public object {

    public:
    string_object(const std::string& val, bool use_arena = true);
    ~string_object();

    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
    string_object* clone(bool use_arena) const override;

    public:
    std::string* value;
};

class return_value_object : public object {

    public:
    return_value_object(object* val, bool use_arena = true, bool clone = false);
    ~return_value_object();

    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
    return_value_object* clone(bool use_arena) const override;

    public:
    object* value;
};

class argument_object : public object {

    public:
    argument_object(const std::string& set_name, object* val, bool use_arena = true, bool clone = false);
    ~argument_object();

    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
    argument_object* clone(bool use_arena) const override;

    public:
    std::string* name;
    object* value;
};

class variable_object : public object {

    public:
    variable_object(const std::string& set_name, object* val, bool use_arena = true, bool clone = false);
    ~variable_object();

    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
    variable_object* clone(bool use_arena) const override;

    public:
    std::string* name;
    object* value;
};

class boolean_object : public object {

    public:
    boolean_object(bool val);

    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
    boolean_object* clone(bool use_arena) const override;

    public:
    bool value;
};

//zerofill is implicitly unsigned im pretty sure
class SQL_data_type_object: public object {

    public:
    SQL_data_type_object(token_type set_prefi, token_type set_data_type, object* set_parameter, bool use_arena = true, bool clone = false);
    ~SQL_data_type_object();

    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
    SQL_data_type_object* clone(bool use_arena) const override;

    public:
    token_type prefix;
    token_type data_type;
    object* parameter;
};

class parameter_object : public object {

    public:
    parameter_object(const std::string& set_name, SQL_data_type_object* set_data_type, bool use_arena = true, bool clone = false);
    ~parameter_object();

    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
    parameter_object* clone(bool use_arena) const override;

    public:
    std::string* name;
    SQL_data_type_object* data_type;
};

class table_detail_object : public object {

    public:
    table_detail_object(object* set_name, object* set_data_type, object* set_default_value, bool use_arena = true, bool clone = false);
    ~table_detail_object();

    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
    table_detail_object* clone(bool use_arena) const override;

    public:
    object* name;
    object* data_type;
    object* default_value;
};

class group_object : public object {

    public:
    group_object(std::vector<object*> objs, bool use_arena = true, bool clone = false);
    ~group_object();

    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
    group_object* clone(bool use_arena) const override;

    public:
    std::vector<object*>* elements;
};

class block_statement;

class function_object : public object {

    public:
    function_object(const std::string& set_name, group_object* set_parameters, SQL_data_type_object* set_return_type, block_statement* set_body, bool use_arena = true, bool clone = false);
    ~function_object();

    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
    function_object* clone(bool use_arena) const override;

    public:
    std::string* name;
    group_object* parameters;
    SQL_data_type_object* return_type;
    block_statement* body;
};

class evaluated_function_object : public object {

    public:
    evaluated_function_object(std::string* set_name, std::vector<parameter_object*>* set_parameters, SQL_data_type_object* set_return_type, block_statement* set_body, bool use_arena = true, bool clone = false);
    evaluated_function_object(function_object* func, std::vector<parameter_object*> new_parameters, bool use_arena = true, bool clone = false);
    ~evaluated_function_object();

    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
    evaluated_function_object* clone(bool use_arena) const override;

    public:
    std::string* name;
    std::vector<parameter_object*>* parameters;
    SQL_data_type_object* return_type;
    block_statement* body;
};

class function_call_object : public object {

    public:
    function_call_object(const std::string& set_name, group_object* args, bool use_arena = true, bool clone = false);
    ~function_call_object();

    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
    function_call_object* clone(bool use_arena) const override;

    public:
    std::string* name;
    group_object* arguments;
};


class column_object: public object {

    public:
    column_object(object* name_data_type, bool use_arena = true, bool clone = false);
    column_object(object* name_data_type, object* default_val, bool use_arena = true, bool clone = false);
    ~column_object();

    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
    column_object* clone(bool use_arena) const override;

    public:
    object* name_data_type;
    object* default_value;
};

class evaluated_column_object: public object {

    public:
    evaluated_column_object(const std::string& name, SQL_data_type_object* type, const std::string& default_val, bool use_arena = true, bool clone = false);
    ~evaluated_column_object();

    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
    evaluated_column_object* clone(bool use_arena) const override;

    public:
    std::string* name;
    SQL_data_type_object* data_type;
    std::string* default_value;
};

class error_object : public object {

    public:
    error_object(bool use_arena = true);
    error_object(const std::string& val, bool use_arena = true);
    ~error_object();

    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
    error_object* clone(bool use_arena) const override;

    public:
    std::string* value;
};

class semicolon_object : public object {

    public:
    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
    semicolon_object* clone(bool use_arena) const override;
};

// Statements
class block_statement : public object {

    public:
    block_statement(std::vector<object*> set_body, bool use_arena = true, bool clone = false);
    ~block_statement();

    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
    block_statement* clone(bool use_arena) const override;

    public:
    std::vector<object*>* body;
};

class if_statement : public object {

    public:
    if_statement(object* set_condition, block_statement* set_body, object* set_other, bool use_arena = true, bool clone = false);
    ~if_statement();

    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
    if_statement* clone(bool use_arena) const override;

    public:
    object* condition;
    block_statement* body;
    object* other;
};

class end_if_statement : public object {

    public:
    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
    end_if_statement* clone(bool use_arena) const override;
};

class end_statement : public object {

    public:
    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
    end_statement* clone(bool use_arena) const override;
};

class return_statement : public object {

    public:
    return_statement(object* expr, bool use_arena = true, bool clone = false);
    ~return_statement();
    
    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
    return_statement* clone(bool use_arena) const override;

    public:
    object* expression;
};