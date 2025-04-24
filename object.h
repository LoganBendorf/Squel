#pragma once

// objects are made in the parser and should probably stay there, used to parse and return values from expressions
    // i.e (10 + 10) will return an integer_object with the value 20

#include "token.h"
#include "helpers.h"

#include <string>
#include <iostream>


enum object_type {
    ERROR_OBJ, NULL_OBJ, INFIX_EXPRESSION_OBJ, INTEGER_OBJ, DECIMAL_OBJ, STRING_OBJ, PREFIX_OPERATOR_OBJ, SQL_DATA_TYPE_OBJ,
    COLUMN_OBJ, FUNCTION_OBJ, OPERATOR_OBJ,
    IF_STATEMENT, BLOCK_STATEMENT, END_IF_STATEMENT, END_STATEMENT, RETURN_STATEMENT
};

class object {
    public:
    virtual std::string inspect() const = 0;  
    virtual object_type type() const = 0;    
    virtual std::string data() const = 0;    
    virtual ~object() = default;            
};

class null_object : public object {
    public:
    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
};

enum operator_type {
    ADD_OP, SUB_OP, MUL_OP, DIV_OP,
    EQUALS_OP, NOT_EQUALS_OP, LESS_THAN_OP, LESS_THAN_OR_EQUAL_TO_OP, GREATER_THAN_OP, GREATER_THAN_OR_EQUAL_TO_OP,
    OPEN_PAREN_OP, OPEN_BRACKET_OP,
    DOT_OP,
};


class operator_object : public object {
    public:
    operator_object(operator_type type);

    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;

    public:
    operator_type op_type;
};

class infix_expression_object : public object {
    public:
    infix_expression_object(operator_object* set_op, object* set_left);
    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;

    public:
    operator_object* op;
    object* left;
    object* right;
};


class integer_object : public object {

    public:
    integer_object(std::string val);
    integer_object(int val);
    integer_object();
    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;

    public:
    int value;
};
class decimal_object : public object {

    public:
    decimal_object(std::string val);
    decimal_object(double val);
    decimal_object();
    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;

    public:
    double value; // value can be cast to float later
};



class string_object : public object {
    public:
    string_object(std::string val);
    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;

    public:
    std::string value;
};

//zerofill is implicitly unsigned im pretty sure
class SQL_data_type_object: public object {
    public:
    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;

    public:
    token_type prefix;
    token_type data_type;
    int parameter_value;
    std::vector<std::string> elements; // for SET
};

class function_object : public object {
    public:
    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;

    public:
    std::string name;
    std::vector<std::pair<object*, token>> arguments;
    SQL_data_type_object* return_type;
    std::vector<object*> expressions;
};


class column_object: public object {
    public:
    column_object(std::string name, SQL_data_type_object* type, std::string default_val);
    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;

    public:
    std::string column_name;
    SQL_data_type_object* data_type;
    std::string default_value;
};

class error_object : public object {
    public:
    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;

    public:
    std::string value;
};


// Statements
class if_statement : public object {

    public:
    if_statement();
    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;

    public:
    object* condition;
    std::vector<object*> body;
    object* other;
};

class block_statement : public object {

    public:
    block_statement(std::vector<object*> set_body);

    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;

    public:
    std::vector<object*> body;
};

class end_if_statement : public object {
    public:
    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
};

class end_statement : public object {
    public:
    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;
};

class return_statement : public object {
    public:
    return_statement(object* expr);
    std::string inspect() const override;
    object_type type() const override;
    std::string data() const override;

    public:
    object* expression;
};