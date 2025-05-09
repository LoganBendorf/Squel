

#include "node.h"
#include "helpers.h"
#include "object.h"

#include <string>
#include <vector>
#include <sstream>

class arena;
extern arena arena_inst;

void* node::operator new(std::size_t size, bool use_arena) {
    if (use_arena) {
        return arena_inst.allocate(size, alignof(node)); // maybe get rid of second parameter
    }
    else {
        return ::operator new(size);
    }
}
    
void node::operator delete([[maybe_unused]] void* ptr, [[maybe_unused]] bool b) noexcept {
    std::cout << "\n\nDELETE BAD!!!!\n\n";
    exit(1);
}
void node::operator delete(void* ptr) noexcept {
    // if it came from the heap, free it; if arena, do nothing
    ::operator delete(ptr);
}
void* node::operator new[](std::size_t size, bool use_arena) {
    if (use_arena) {
        return arena_inst.allocate(size, alignof(node));
    }
    else {
        return ::operator new(size);
    }
}
void node::operator delete[]([[maybe_unused]] void* p) noexcept {
    std::cout << "\n\nDELETE BAD!!!!\n\n";
    exit(1);
}

// null_node
std::string null_node::inspect() const {
    return std::string("nada"); 
}
node_type null_node::type() const {
    return NULL_NODE; 
}
null_node* null_node::clone(bool use_arena) const {
    return new (use_arena) null_node();
}

// function_node
function::function(function_object* set_func, bool use_arena) {
    in_arena = use_arena;
    if (use_arena) {
        func = set_func;       
    } else {
        func = set_func->clone(use_arena);
    }
}
function::~function() {
    if (!in_arena) {
      delete func;
    }
}
std::string function::inspect() const {
    return std::string("function_node\n"); 
}
node_type function::type() const {
    return FUNCTION_NODE; 
}
function* function::clone(bool use_arena) const {
    return new (use_arena) function(*this);
}

// insert_into_node
insert_into::insert_into(object* set_table_name, std::vector<object*> set_fields, std::vector<object*> set_values, bool use_arena) {
    in_arena = use_arena;
    if (use_arena) {
        table_name = set_table_name;

        void* mem = arena_inst.allocate(sizeof(std::vector<object*>), alignof(std::vector<object*>)); 
        auto vec_ptr = new (mem) std::vector<object*>();                                  
        fields = vec_ptr;                                                             
        arena_inst.register_dtor([vec_ptr]() {                                      
            vec_ptr->std::vector<object*>::~vector();                                              \
        });
        for (const auto& obj : set_fields) {
            fields->push_back(obj);
        }

        mem = arena_inst.allocate(sizeof(std::vector<object*>), alignof(std::vector<object*>)); 
        vec_ptr = new (mem) std::vector<object*>();                                  
        values = vec_ptr;                                                             
        arena_inst.register_dtor([vec_ptr]() {                                      
            vec_ptr->std::vector<object*>::~vector();                                              \
        });
        for (const auto& obj : set_values) {
            values->push_back(obj);
        }

    } else {
        table_name = set_table_name->clone(use_arena);
        fields = new std::vector<object*>;
        for (const auto& obj : set_fields) {
            fields->push_back(obj->clone(use_arena));
        }
        values = new std::vector<object*>;
        for (const auto& obj : set_values) {
            values->push_back(obj->clone(use_arena));
        }
    }
}
insert_into::~insert_into() {
    if (!in_arena) {
      delete table_name;
      for (const auto& field : *fields) {
        delete field;
      }
      delete fields;
      for (const auto& value : *values) {
        delete value;
      }
      delete values;
    }
}
std::string insert_into::inspect() const {
    std::string ret_str = "";
    ret_str += "insert_into: ";
    ret_str += table_name->inspect() + "\n";

    ret_str += "[Fields: ";
    for (const auto& field : *fields) {
        ret_str += field->inspect() + ", "; }
    if (fields->size() > 0) {
        ret_str = ret_str.substr(0, ret_str.size() - 2); }

    ret_str += "], [Values: ";
    for (const auto& value : *values) {
        ret_str  += value->inspect() + ", "; }
    if (values->size() > 0) {
        ret_str = ret_str.substr(0, ret_str.size() - 2); }

    ret_str += "]\n";
    return ret_str;
}
node_type insert_into::type() const {
    return INSERT_INTO_NODE; 
}
insert_into* insert_into::clone(bool use_arena) const {
    return new (use_arena) insert_into(*this);
}

// select_from_node
select_from::select_from(object* set_table_name, std::vector<object*> set_column_names, bool set_asterisk, object* set_condition, bool use_arena) {
    in_arena = use_arena;
    if (use_arena) {
        table_name = set_table_name;

        void* mem = arena_inst.allocate(sizeof(std::vector<object*>), alignof(std::vector<object*>)); 
        auto vec_ptr = new (mem) std::vector<object*>();                                  
        column_names = vec_ptr;                                                             
        arena_inst.register_dtor([vec_ptr]() {                                      
            vec_ptr->std::vector<object*>::~vector();                                              \
        });
        for (const auto& obj : set_column_names) {
            column_names->push_back(obj);
        }

        asterisk = set_asterisk;
        condition = set_condition;
    } else {
        table_name = set_table_name->clone(use_arena);

        column_names = new std::vector<object*>();
        for (const auto& obj : set_column_names) {
            column_names->push_back(obj->clone(use_arena));
        }
        
        asterisk = set_asterisk;
        condition = set_condition->clone(use_arena);
    }
}
select_from::~select_from() {
    if (!in_arena) {
      delete table_name;
      for (const auto& col_name : *column_names) {
        delete col_name;
      }
      delete column_names;
      delete condition;
    }
}
std::string select_from::inspect() const {
    std::string ret_str = "select_from: " + table_name->inspect() + "\n";

    if (condition) {
        ret_str += "condition: " + condition->inspect() + "\n"; }
    
    for (const auto& col_name : *column_names) {
        ret_str += col_name->inspect() + ", ";}
    if (column_names->size() > 0) {
        ret_str = ret_str.substr(0, ret_str.size() - 2); }

    ret_str += "\n";
    return ret_str;

}
node_type select_from::type() const {
    return SELECT_FROM_NODE;
}
select_from* select_from::clone(bool use_arena) const {
    return new (use_arena) select_from(*this);
}

// alter_table_node
alter_table::alter_table(object* set_table_name, object* set_tab_edit, bool use_arena) {
    in_arena = use_arena;
    if (use_arena) {
        table_name = set_table_name;
        table_edit = set_tab_edit;
    } else {
        table_name = set_table_name->clone(use_arena);
        table_edit = set_tab_edit->clone(use_arena);
    }
}
alter_table::~alter_table() {
    if (!in_arena) {
      delete table_name;
      delete table_edit;
    }
}
std::string alter_table::inspect() const {
    std::string ret_str = "";
    ret_str += "alter_table: ";
    ret_str += table_name->inspect() + "\n";
    ret_str += table_edit->inspect();
    return ret_str;
}
node_type alter_table::type() const {
    return ALTER_TABLE_NODE; 
}
alter_table* alter_table::clone(bool use_arena) const {
    return new (use_arena) alter_table(*this);
}

// create_table_node
create_table::create_table(object* set_table_name, std::vector<table_detail_object*> set_details, bool use_arena) {
    in_arena = use_arena;
    if (use_arena) {
        table_name = set_table_name;

        void* mem = arena_inst.allocate(sizeof(std::vector<table_detail_object*>), alignof(std::vector<table_detail_object*>)); 
        auto vec_ptr = new (mem) std::vector<table_detail_object*>();                                  
        details = vec_ptr;                                                             
        arena_inst.register_dtor([vec_ptr]() {                                      
            vec_ptr->std::vector<table_detail_object*>::~vector();                                              \
        });
        for (const auto& detail : set_details) {
            details->push_back(detail);
        }

    } else {
        table_name = set_table_name->clone(use_arena);

        details = new std::vector<table_detail_object*>();
        for (const auto& detail : set_details) {
            details->push_back(detail->clone(use_arena));
        }
    }
}
create_table::~create_table() {
    if (!in_arena) {
      delete table_name;
      for (const auto& detail : *details) {
        delete detail;
      }
      delete details;
    }
}
std::string create_table::inspect() const {
    std::string ret_str = "";
    ret_str += "create_table: ";
    ret_str += table_name->inspect() + "\n[";
    for (const auto& detail : *details) {
        ret_str += detail->inspect() + ", "; }

    if (details->size() > 0) {
        ret_str = ret_str.substr(0, ret_str.size() - 2); }

    ret_str += "]\n";
    return ret_str;
}
node_type create_table::type() const {
    return CREATE_TABLE_NODE; 
}
create_table* create_table::clone(bool use_arena) const {
    return new (use_arena) create_table(*this);
}


