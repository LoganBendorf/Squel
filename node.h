#pragma once

#include "structs_and_macros.h"
#include "object.h"

#include <string>
#include <vector>


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
    function(function_object* set_func);
    std::string inspect() override;
    node_type type() override;
    
    public:
    function_object* func;
};

class insert_into : public node {
    public:
    insert_into(object* set_table_name, std::vector<object*> set_fields, std::vector<object*> set_values);
    std::string inspect() override;
    node_type type() override;

    public:
    object* table_name;
    std::vector<object*> fields;
    std::vector<object*> values;
};

class select_from : public node {
    
    public:
    select_from(object* set_table_name, std::vector<object*> set_column_names, bool set_asterisk, object* set_condition);
    std::string inspect() override;
    node_type type() override;

    public:
    object* table_name;
    std::vector<object*> column_names;
    bool asterisk;
    object* condition;
};

class alter_table : public node {
    public:
    alter_table(object* tab_name, object* tab_edit);
    std::string inspect() override;
    node_type type() override;

    public:
    object* table_name;
    object* table_edit;
};

class create_table : public node {

    public:
    create_table(object* set_table_name, std::vector<table_detail_object*> set_details);
    std::string inspect() override;
    node_type type() override;

    public:
    object* table_name;
    std::vector<table_detail_object*> details;

};

