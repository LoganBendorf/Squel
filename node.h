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
    virtual std::string inspect() const = 0;
    virtual node_type type() const = 0;
    virtual node* clone(bool use_arena) const = 0;  //!!MAJOR should be use_arena = true??? maybe?
    virtual ~node() = default;

    static void* operator new(std::size_t size, bool use_arena = true);
    static void  operator delete([[maybe_unused]] void* ptr, [[maybe_unused]] bool b) noexcept;
    static void  operator delete(void* ptr) noexcept;
    static void* operator new[](std::size_t size, bool use_arena = true);
    static void  operator delete[]([[maybe_unused]] void* p) noexcept;

    protected:
    bool in_arena = true;
};

class null_node : public node {

    public:
    std::string inspect() const override;
    node_type type() const override;
    null_node* clone(bool use_arena) const override;
};

class function : public node {

    public:
    function(function_object* set_func, bool use_arena = true);
    ~function();

    std::string inspect() const override;
    node_type type() const override;
    function* clone(bool use_arena) const override;
    
    public:
    function_object* func;
};

class insert_into : public node {

    public:
    insert_into(object* set_table_name, std::vector<object*> set_fields, std::vector<object*> set_values, bool use_arena = true);
    ~insert_into();

    std::string inspect() const override;
    node_type type() const override;
    insert_into* clone(bool use_arena) const override;

    public:
    object* table_name;
    std::vector<object*>* fields;
    std::vector<object*>* values;
};

class select_from : public node {
    
    public:
    select_from(object* set_table_name, std::vector<object*> set_column_names, bool set_asterisk, object* set_condition, bool use_arena = true);
    ~select_from();

    std::string inspect() const override;
    node_type type() const override;
    select_from* clone(bool use_arena) const override;

    public:
    object* table_name;
    std::vector<object*>* column_names;
    bool asterisk;
    object* condition;
};

class alter_table : public node {

    public:
    alter_table(object* set_table_name, object* set_tab_edit, bool use_arena = true);
    ~alter_table();
    
    std::string inspect() const override;
    node_type type() const override;
    alter_table* clone(bool use_arena) const override;

    public:
    object* table_name;
    object* table_edit;
};

class create_table : public node {

    public:
    create_table(object* set_table_name, std::vector<table_detail_object*> set_details, bool use_arena = true);
    ~create_table();

    std::string inspect() const override;
    node_type type() const override;
    create_table* clone(bool use_arena) const override;

    public:
    object* table_name;
    std::vector<table_detail_object*>* details;
};
