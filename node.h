#pragma once

#include "pch.h"

#include "structs_and_macros.h"
#include "object.h"
#include "arena_aliases.h"



enum node_type {
    NULL_NODE, FUNCTION_NODE, INSERT_INTO_NODE, SELECT_FROM_NODE, ALTER_TABLE_NODE, CREATE_TABLE_NODE, SELECT_NODE,

    // CUSTOM
    ASSERT_NODE,
};

class node {

    protected:
    static arena<node> node_arena_alias; // all arenas are alias for the global one

    public:
    virtual astring inspect() const = 0;
    virtual node_type type() const = 0;
    virtual node* clone(bool use_arena) const = 0;  //!!MAJOR should be use_arena = true??? maybe?
    virtual ~node() = default;

    // static void* operator new(std::size_t size);
    // static void* operator new(std::size_t size, bool use_arena);
    // static void  operator delete([[maybe_unused]] void* ptr, [[maybe_unused]] bool b) noexcept;
    // static void  operator delete(void* ptr) noexcept;
    // static void* operator new[](std::size_t size);
    // static void  operator delete[]([[maybe_unused]] void* p) noexcept;

    static void* operator new(std::size_t size, bool use_arena) {
        node* nd;
        if (use_arena) {
            nd = node_arena_alias.allocate(size, alignof(node));
            if (nd) { 
                nd->in_arena = true; }
        } else {
            auto heap_arena = arena<node>{HEAP};
            nd = heap_arena.allocate(size, alignof(node));
            if (nd) { 
                nd->in_arena = false;}
        }
        return nd;
    }

    static void* operator new(std::size_t size) {
        node* nd = node_arena_alias.allocate(size, alignof(node));
        if (nd) { 
            nd->in_arena = true; }
        return nd;
    }
    
    // Matching delete for placement new above
    static void operator delete([[maybe_unused]] void* ptr, bool) noexcept {
        std::cout << "\n\nDELETE BAD!!!!\n\n";
        exit(1);
    }
    
    // Standard delete
    static void operator delete(void* ptr) noexcept {
        if (!ptr) return;
        
        node* nd = static_cast<node*>(ptr);
        if (nd->in_arena) {
            // Arena nodes - the arena handles cleanup
            // In a real implementation, you might want to call:
            // global_node_arena.deallocate_bytes(ptr);
            return;
        } else {
            ::operator delete(ptr);
        }
    }
    
    // Array versions
    static void* operator new[](std::size_t size, bool use_arena) {
        if (use_arena) {
            return node_arena_alias.allocate(size, alignof(node));
        } else {
            return ::operator new[](size);
        }
    }
    
    static void* operator new[](std::size_t size) {
        return ::operator new[](size);
    }
    
    static void operator delete[](void* ptr) noexcept {
        if (!ptr) return;
        // For arrays, we'd need additional metadata to track arena vs heap
        // This is simplified - in practice you'd want to track this
        ::operator delete[](ptr);
    }


    protected:
    bool in_arena = true;
};

class null_node : public node {

    public:
    astring inspect() const override;
    node_type type() const override;
    null_node* clone(bool use_arena) const override;
};

class function : public node {

    public:
    function(function_object* set_func);
    ~function();

    astring inspect() const override;
    node_type type() const override;
    function* clone(bool use_arena) const override;
    
    public:
    function_object* func;
};

class insert_into : public node {

    public:
    insert_into(object* set_value, bool clone = false);
    ~insert_into();

    astring inspect() const override;
    node_type type() const override;
    insert_into* clone(bool use_arena) const override;

    public:
    object* value;
};

class select_node : public node {
    
    public:
    select_node(object* set_value, bool clone = false);
    ~select_node();

    astring inspect() const override;
    node_type type() const override;
    select_node* clone(bool use_arena) const override;

    public:
    object* value;
};

class select_from : public node {
    
    public:
    select_from(object* set_value, bool clone = false);
    ~select_from();

    astring inspect() const override;
    node_type type() const override;
    select_from* clone(bool use_arena) const override;

    public:
    object* value;
};

class alter_table : public node {

    public:
    alter_table(object* set_table_name, object* set_tab_edit);
    ~alter_table();
    
    astring inspect() const override;
    node_type type() const override;
    alter_table* clone(bool use_arena) const override;

    public:
    object* table_name;
    object* table_edit;
};

class create_table : public node {

    public:
    create_table(object* set_table_name, const avec<table_detail_object*>& set_details);
    ~create_table();

    astring inspect() const override;
    node_type type() const override;
    create_table* clone(bool use_arena) const override;

    public:
    object* table_name;
    avec<table_detail_object*> details;
};


// CUSTOM
class assert_node : public node {
    
    public:
    assert_node(assert_object* set_value, bool clone = false);
    ~assert_node();

    astring inspect() const override;
    node_type type() const override;
    assert_node* clone(bool use_arena) const override;

    public:
    assert_object* value;
};