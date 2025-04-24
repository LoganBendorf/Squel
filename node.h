#pragma once

#include "structs_and_macros.h"
#include "helpers.h"
#include "object.h"

#include <string>
#include <vector>
#include <iostream>
#include <sstream>


enum node_type {
    NULL_NODE, FUNCTION_NODE, INSERT_INTO_NODE, SELECT_FROM_NODE, ALTER_TABLE_NODE, CREATE_TABLE_NODE,
};



class node {
    public:
    virtual std::string inspect() = 0;
    virtual node_type type() = 0;
    virtual ~node() {};
};

class null_node : public node {
    public:
    std::string inspect() override;
    node_type type() override;
};

class function : public node {
    public:
    std::string inspect() override;
    node_type type() override;
    
    public:
    function_object* func;
};

class insert_into : public node {
    public:
    std::string inspect() override;
    node_type type() override;

    public:
    std::string table_name;
    std::vector<std::string> field_names;
    std::vector<object*> values;
};

class select_from : public node {
    
    public:
    select_from();
    std::string inspect() override;
    node_type type() override;

    public:
    std::string table_name;
    std::vector<std::string> column_names;
    bool asterisk;
    object* condition;
};

class alter_table : public node {
    public:
    std::string inspect() override;
    node_type type() override;

    public:
    std::string table_name;
    object* table_edit;
};

class create_table : public node {

    public:
    std::string inspect() override;
    node_type type() override;

    public:
    std::string table_name;
    std::vector<column_data> column_datas;

};

