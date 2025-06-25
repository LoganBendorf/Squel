#pragma once

#include "pch.h"

#include "structs_and_macros.h"
#include "object.h"
#include "allocator_aliases.h"
#include "allocators.h"



enum node_type : std::uint8_t{
    NULL_NODE, FUNCTION_NODE, INSERT_INTO_NODE, SELECT_FROM_NODE, ALTER_TABLE_NODE, CREATE_TABLE_NODE, SELECT_NODE,

    // CUSTOM
    ASSERT_NODE,
};

class node {

    protected:
    static main_alloc<node> node_allocator_alias;

    public:
    [[nodiscard]] virtual astring inspect() const = 0;
    [[nodiscard]] virtual node_type type() const = 0;
    [[nodiscard]] virtual node* clone() const = 0; 
    virtual ~node() = default;

    static void* operator new(size_t size) {
        return node_allocator_alias.allocate_block(size).mem;
    }

    static void operator delete(void* ptr) noexcept {
        if (ptr == nullptr) { 
            return;
        }
        
        node_allocator_alias.deallocate_block({.size=0, .mem=ptr});
    }

    static void* operator new[](size_t size) {
        return node_allocator_alias.allocate_block(size).mem;
    }

    static void operator delete[](void* ptr) noexcept {
        if (ptr == nullptr) { 
            return;
        }
        
        node_allocator_alias.deallocate_block({.size=0, .mem=ptr});
    }

};

class null_node : public node {

    public:
    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] node_type type() const override;
    [[nodiscard]] null_node* clone() const override;
};

class function : public node {

    public:
    function(function_object* set_func);
    function(UP<function_object> set_func);

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] node_type type() const override;
    [[nodiscard]] function* clone() const override;
    
    public:
    UP<function_object> func;
};

class insert_into : public node {

    public:
    insert_into(insert_into_object* set_value);
    insert_into(UP<insert_into_object> set_value);

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] node_type type() const override;
    [[nodiscard]] insert_into* clone() const override;

    public:
    UP<insert_into_object> value;
};

class select_node : public node {
    
    public:
    select_node(object* set_value);
    select_node(UP<object> set_value);

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] node_type type() const override;
    [[nodiscard]] select_node* clone() const override;

    public:
    UP<object> value;
};

class select_from : public node {
    
    public:
    select_from(object* set_value);
    select_from(UP<object> set_value);

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] node_type type() const override;
    [[nodiscard]] select_from* clone() const override;

    public:
    UP<object> value;
};

class alter_table : public node {

    public:
    alter_table(object* set_table_name, object* set_tab_edit);
    alter_table(UP<object> set_table_name, UP<object> set_tab_edit);
    
    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] node_type type() const override;
    [[nodiscard]] alter_table* clone() const override;

    public:
    UP<object> table_name;
    UP<object> table_edit;
};

class create_table : public node {

    public:
    create_table(object* set_table_name, avec<UP<table_detail_object>>&& set_details);
    create_table(UP<object> set_table_name, avec<UP<table_detail_object>>&& set_details);

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] node_type type() const override;
    [[nodiscard]] create_table* clone() const override;

    public:
    UP<object> table_name;
    avec<UP<table_detail_object>> details;
};


// CUSTOM
class assert_node : public node {
    
    public:
    assert_node(assert_object* set_value);
    assert_node(UP<assert_object> set_value);

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] node_type type() const override;
    [[nodiscard]] assert_node* clone() const override;

    public:
    UP<assert_object> value;
};