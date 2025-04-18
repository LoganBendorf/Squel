#pragma once

// objects are made in the parser and should probably stay there, used to parse and return values from expressions
    // i.e (10 + 10) will return an integer_object with the value 20

#include "token.h"
#include "helpers.h"

#include <string>
#include <iostream>


enum object_type {
    ERROR_OBJ, NULL_OBJ, INFIX_EXPRESSION_OBJ, INTEGER_OBJ, DECIMAL_OBJ, STRING_OBJ, PREFIX_OPERATOR_OBJ, SQL_DATA_TYPE_OBJ,
    COLUMN_OBJ
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
    std::string inspect() const override {
        return std::string("NULL_OBJECT");}
    object_type type() const override {
        return NULL_OBJ;}
    std::string data() const override {
        return std::string("NULL_OBJECT");}
};

class infix_expression_object : public object {
    public:
    infix_expression_object(token set_op, object* set_left) {
        op = set_op;
        left = set_left;
        right = NULL;
    }
    std::string inspect() const override {
        std::string str = "Op: " + token_type_to_string(op.type);
        if (left != NULL) {
            str += ". Left: " + left->inspect();}
        if (right != NULL) {
            str += ". Right: " + right->inspect();}
        str += "\n";
        return str;}
    object_type type() const override {
        return INFIX_EXPRESSION_OBJ;}
    std::string data() const override {
        return std::string("Shouldn't get data from infix expression object");}

    public:
    token op;
    object* left;
    object* right;
};


class integer_object : public object {

    public:
    integer_object(std::string val) {
        value = std::stoi(val);}
    integer_object(int val) {
        value = val;}
    integer_object() {
        value = 0;}
    std::string inspect() const override {
        return std::to_string(value);}
    object_type type() const override {
        return INTEGER_OBJ;}
    std::string data() const override {
        return std::to_string(value);}
    public:
    int value;
};
class decimal_object : public object {

    public:
    decimal_object(std::string val) {
        value = std::stod(val);}
    decimal_object(double val) {
        value = val;}
    decimal_object() {
        value = 0;}
    std::string inspect() const override {
        return std::to_string(value);}
    object_type type() const override {
        return DECIMAL_OBJ;}
    std::string data() const override {
        return std::to_string(value);}
    public:
    double value; // value can be cast to float later
};



class string_object : public object {
    public:
    string_object(std::string val) {
        value = val;
    }
    std::string inspect() const override {
        return value;
    }
    object_type type() const override {
        return STRING_OBJ;
    }
    std::string data() const override {
        return value;
    }
    public:
    std::string value;
};


enum SQL_data_type_prefix {
    NONE, UNSIGNED, ZEROFILL //zerofill is implicitly unsigned im pretty sure
};

static std::string SQL_data_type_prefix_to_string(SQL_data_type_prefix index) {
    constexpr const char* type_to_string[] =  
    {"NONE", "UNSIGNED", "ZEROFILL"};
    if (index > sizeof(type_to_string)) {
        std::cout << "invalid index\n"; exit(1);}
    return std::string(type_to_string[index]);
}

enum SQL_data_type {
    CHAR, VARCHAR, BOOL, DATA, YEAR, SET, BIT, INT, FLOAT, DOUBLE
};

static std::string SQL_data_type_to_string(SQL_data_type index) {
    constexpr const char* type_to_string[] =  
    {"CHAR", "VARCHAR", "BOOL", "DATA", "YEAR", "SET", "BIT", "INT", "FLOAT", "DOUBLE"};
    if (index > sizeof(type_to_string)) {
        std::cout << "invalid index\n"; exit(1);}
    return std::string(type_to_string[index]);
}


class SQL_data_type_object: public object {
    public:
    std::string inspect() const override {
        std::string ret = "[Type: ";
        if (prefix != NONE) {
            ret +=  SQL_data_type_prefix_to_string(prefix);}
            
        ret += SQL_data_type_to_string(data_type) + " (" + std::to_string(parameter_value) + ")";
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
    object_type type() const override {
        return SQL_DATA_TYPE_OBJ;
    }
    std::string data() const override {
        return std::to_string(parameter_value);
    }
    public:
    SQL_data_type_prefix prefix;
    SQL_data_type data_type;
    int parameter_value;
    std::vector<std::string> elements; // for SET
};

class column_object: public object {
    public:
    column_object(std::string name, SQL_data_type_object* type, std::string default_val) {
        column_name = name;
        data_type = type;
        default_value = default_val;
    }
    std::string inspect() const override {
        std::string str = "[Column name: " + column_name + "], ";
        str += data_type->inspect();
        str += ", [Default: " + default_value  + "]\n";
        return str;
    }
    object_type type() const override {
        return COLUMN_OBJ;
    }
    std::string data() const override {
        return column_name + " " + data_type->data() + " " + default_value;
    }
    public:
    std::string column_name;
    SQL_data_type_object* data_type;
    std::string default_value;
};

class error_object : public object {
    public:
    std::string inspect() const override {
        return "ERROR: " + value;
    }
    object_type type() const override {
        return ERROR_OBJ;
    }
    std::string data() const override {
        return value;
    }
    public:
    std::string value;
};