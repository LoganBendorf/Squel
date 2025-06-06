
#include "pch.h"

#include "node.h"
#include "helpers.h"
#include "object.h"

arena<node> node::node_arena_alias; 

// null_node
astring null_node::inspect() const {
    return "nada"; 
}
node_type null_node::type() const {
    return NULL_NODE; 
}
null_node* null_node::clone(bool use_arena) const {
    return new (use_arena) null_node();
}

// function_node
function::function(function_object* set_func) {
    if (in_arena) {
        func = set_func;       
    } else {
        func = set_func->clone(in_arena);
    }
}
function::~function() {
    if (!in_arena) {
      delete func;
    }
}
astring function::inspect() const {
    return "function_node\n"; 
}
node_type function::type() const {
    return FUNCTION_NODE; 
}
function* function::clone(bool use_arena) const {
    return new (use_arena) function(func);
}

// insert_into_node
insert_into::insert_into(object* set_value, bool clone) {
    if (in_arena) {
        if (clone) {
            value = set_value->clone(in_arena);
        } else {
            value = set_value;
        }

    } else {
        value = set_value->clone(in_arena);
    }
}
insert_into::~insert_into() {
    if (!in_arena) {
      delete value;
    }
}
astring insert_into::inspect() const {
    return value->inspect();
}
node_type insert_into::type() const {
    return INSERT_INTO_NODE; 
}
insert_into* insert_into::clone(bool use_arena) const {
    return new (use_arena) insert_into(value, true);
}

// select_node
select_node::select_node(object* set_value, bool clone) {
    if (in_arena) {
        if (clone) {
            value = set_value->clone(in_arena);
        } else {
            value = set_value;
        }
    } else {
        value = set_value->clone(in_arena);
    }
}
select_node::~select_node() {
    if (!in_arena) {
      delete value;
    }
}
astring select_node::inspect() const {
    return value->inspect();
}
node_type select_node::type() const {
    return SELECT_NODE;
}
select_node* select_node::clone(bool use_arena) const {
    return new (use_arena) select_node(value, true);
}


// select_from_node
select_from::select_from(object* set_value, bool clone) {
    if (in_arena) {
        if (clone) {
            value = set_value->clone(in_arena);
        } else {
            value = set_value;
        }
    } else {
        value = set_value->clone(in_arena);
    }
}
select_from::~select_from() {
    if (!in_arena) {
      delete value;
    }
}
astring select_from::inspect() const {
    return value->inspect();
}
node_type select_from::type() const {
    return SELECT_FROM_NODE;
}
select_from* select_from::clone(bool use_arena) const {
    return new (use_arena) select_from(value, true);
}

// alter_table_node
alter_table::alter_table(object* set_table_name, object* set_tab_edit) {
    if (in_arena) {
        table_name = set_table_name;
        table_edit = set_tab_edit;
    } else {
        table_name = set_table_name->clone(in_arena);
        table_edit = set_tab_edit->clone(in_arena);
    }
}
alter_table::~alter_table() {
    if (!in_arena) {
      delete table_name;
      delete table_edit;
    }
}
astring alter_table::inspect() const {
    astringstream stream;
    stream << "alter_table: ";
    stream << table_name->inspect() + "\n";
    stream << table_edit->inspect();
    return stream.str();
}
node_type alter_table::type() const {
    return ALTER_TABLE_NODE; 
}
alter_table* alter_table::clone(bool use_arena) const {
    return new (use_arena) alter_table(table_name, table_edit);
}

// create_table_node
create_table::create_table(object* set_table_name, const avec<table_detail_object*>& set_details) {
    if (in_arena) {
        table_name = set_table_name;

        details = avec<table_detail_object*>();
        for (const auto& detail : set_details) {
            details.push_back(detail);
        }

    } else {
        table_name = set_table_name->clone(in_arena);

        details = hvec(table_detail_object*);
        for (const auto& detail : set_details) {
            details.push_back(detail->clone(in_arena));
        }
    }
}
create_table::~create_table() {
    if (!in_arena) {
      delete table_name;
      for (const auto& detail : details) {
        delete detail;
      }
    }
}
astring create_table::inspect() const {
    astringstream stream;
    stream << "create_table: ";
    stream << table_name->inspect() + "\n[";

    bool first = true;
    for (const auto& detail : details) {
        if (!first) stream << ", ";
        stream << detail->inspect(); 
        first = false;
    }
    stream << "]\n";

    return stream.str();
}
node_type create_table::type() const {
    return CREATE_TABLE_NODE; 
}
create_table* create_table::clone(bool use_arena) const {
    return new (use_arena) create_table(*this);
}


// CUSTOM


// assert_node
assert_node::assert_node(assert_object* set_value, bool clone) {
    if (in_arena) {
        if (clone) {
            value = set_value->clone(in_arena);
        } else {
            value = set_value;
        }
    } else {
        value = set_value->clone(in_arena);
    }
}
assert_node::~assert_node() {
    if (!in_arena) {
      delete value;
    }
}
astring assert_node::inspect() const {
    return value->inspect();
}
node_type assert_node::type() const {
    return ASSERT_NODE;
}
assert_node* assert_node::clone(bool use_arena) const {
    return new (use_arena) assert_node(value, true);
}