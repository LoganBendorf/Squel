
#include "pch.h"

#include "node.h"

// #include "helpers.h"
#include "object.h"

main_alloc<node> node::node_allocator_alias;

// null_node
astring    null_node::inspect() const { return "Null object inspected"; }
node_type  null_node::type()    const { return NULL_NODE; }
null_node* null_node::clone()   const { return new null_node(); }

// function_node
function::function(function_object*    set_func) { func = UP<function_object>(set_func); }
function::function(UP<function_object> set_func) : func(std::move(set_func)) {}
astring   function::inspect() const { return "Function Node\n"; }
node_type function::type()    const { return  FUNCTION_NODE;    }
function* function::clone()   const { return new function(func->clone()); }

// insert_into_node
insert_into::insert_into(insert_into_object*    set_value) { value = UP<insert_into_object>(set_value); }
insert_into::insert_into(UP<insert_into_object> set_value) : value(std::move(set_value)) {}
astring      insert_into::inspect() const { return "Insert Into Node: " + value->inspect(); }
node_type    insert_into::type()    const { return  INSERT_INTO_NODE; }
insert_into* insert_into::clone()   const { return new insert_into(value->clone()); }

// e_insert_into_node
e_insert_into::e_insert_into(e_insert_into_object*    set_value) { value = UP<e_insert_into_object>(set_value); }
e_insert_into::e_insert_into(UP<e_insert_into_object> set_value) : value(std::move(set_value)) {}
astring        e_insert_into::inspect() const { return value->inspect(); }
node_type      e_insert_into::type()    const { return E_INSERT_INTO_NODE; }
e_insert_into* e_insert_into::clone()   const { return new e_insert_into(value->clone()); }

// select_node
select_node::select_node(object*    set_value) { value = UP<object>(set_value); }
select_node::select_node(UP<object> set_value) : value(std::move(set_value)) {}
astring      select_node::inspect() const { return "Select Node:" + value->inspect(); }
node_type    select_node::type()    const { return  SELECT_NODE; }
select_node* select_node::clone()   const { return new select_node(value->clone()); }

// e_select_node
e_select_node::e_select_node(serializable* set_value)    : value(UP<serializable>(set_value)) {}
e_select_node::e_select_node(UP<serializable> set_value) : value(std::move(set_value)) {}
astring        e_select_node::inspect() const { return "E Select Node: " + value->inspect(); }
node_type      e_select_node::type()    const { return  E_SELECT_NODE; }
e_select_node* e_select_node::clone()   const { return new e_select_node(value->clone()); }

// select_from_node
select_from::select_from(object*    set_value) { value = UP<object>(set_value); }
select_from::select_from(UP<object> set_value) : value(std::move(set_value)) {}
astring      select_from::inspect() const { return "Select From Node: " + value->inspect(); }
node_type    select_from::type()    const { return  SELECT_FROM_NODE; }
select_from* select_from::clone()   const { return new select_from(value->clone()); }

// e_select_from_node
e_select_from_node::e_select_from_node(e_select_from_object*    set_value) { value = UP<e_select_from_object>(set_value); }
e_select_from_node::e_select_from_node(UP<e_select_from_object> set_value) : value(std::move(set_value)) {}
astring             e_select_from_node::inspect() const { return "E Select From Node: " + value->inspect(); }
node_type           e_select_from_node::type()    const { return  E_SELECT_FROM_NODE; }
e_select_from_node* e_select_from_node::clone()   const { return new e_select_from_node(value->clone()); }

// alter_table_node
alter_table_node::alter_table_node(object* set_table_name, object* set_tab_edit) {
    table_name = UP<object>(set_table_name);
    table_edit = UP<object>(set_tab_edit);
}
alter_table_node::alter_table_node(UP<object> set_table_name, UP<object> set_tab_edit) : 
    table_name(std::move(set_table_name)), table_edit(std::move(set_tab_edit)) {}
astring alter_table_node::inspect() const {
    astringstream stream;
    stream << "Alter Table Node: ";
    stream << table_name->inspect() << "\n";
    stream << table_edit->inspect();
    return stream.str();
}
node_type alter_table_node::type() const { return ALTER_TABLE_NODE; }
alter_table_node* alter_table_node::clone() const {
    return new alter_table_node(table_name->clone(), table_edit->clone());
}

// e_alter_table_node
e_alter_table_node::e_alter_table_node(astring set_table_name, evaluated* set_tab_edit) 
    : table_name(set_table_name), table_edit(UP<evaluated>(set_tab_edit)) {}
e_alter_table_node::e_alter_table_node(astring set_table_name, UP<evaluated> set_tab_edit)
    : table_name(std::move(set_table_name)), table_edit(std::move(set_tab_edit)) {}
astring e_alter_table_node::inspect() const {
    astringstream stream;
    stream << "E Alter Table Node: ";
    stream << table_name << "\n";
    stream << table_edit->inspect();
    return stream.str();
}
node_type e_alter_table_node::type() const { return E_ALTER_TABLE_NODE; }
e_alter_table_node* e_alter_table_node::clone() const {
    return new e_alter_table_node(table_name, table_edit->clone());
}

// create_table_node
create_table::create_table(astring set_table_name, group_object* set_details) : 
    table_name(set_table_name), 
    details(UP<group_object>(set_details)) 
{}
create_table::create_table(astring set_table_name, UP<group_object> set_details) : 
    table_name(set_table_name), 
    details(std::move(set_details)) 
{}
astring create_table::inspect() const {
    astringstream stream;
    stream << "Create Table: ";
    stream << table_name + "\n[";
    stream << details->inspect();
    stream << "]\n";

    return stream.str();
}
node_type     create_table::type()  const { return CREATE_TABLE_NODE; }
create_table* create_table::clone() const { return new create_table(table_name, details->clone()); }

// e_create_table
e_create_table::e_create_table(astring set_table_name, e_group_object* set_details) : 
    table_name(set_table_name), 
    details(UP<e_group_object>(set_details)) 
{}
e_create_table::e_create_table(astring set_table_name, UP<e_group_object> set_details) : 
    table_name(set_table_name), 
    details(std::move(set_details)) 
{}
astring e_create_table::inspect() const {
    astringstream stream;
    stream << "E Create Table Node: ";
    stream << table_name + "\n[";
    stream << details->inspect();
    stream << "]\n";

    return stream.str();
}
node_type       e_create_table::type()  const { return E_CREATE_TABLE_NODE; }
e_create_table* e_create_table::clone() const { return new e_create_table(table_name, details->clone()); }


// CUSTOM

// assert_node
assert_node::assert_node(assert_object*    set_value) { value = UP<assert_object>(set_value); }
assert_node::assert_node(UP<assert_object> set_value) : value(std::move(set_value)) {}
astring assert_node::inspect()    const { return value->inspect(); }
node_type assert_node::type()     const { return ASSERT_NODE;      }
assert_node* assert_node::clone() const { return new assert_node(value->clone()); }

// e_assert_node
e_assert_node::e_assert_node(e_assert_object*    set_value) { value = UP<e_assert_object>(set_value); }
e_assert_node::e_assert_node(UP<e_assert_object> set_value) : value(std::move(set_value)) {}
astring e_assert_node::inspect()      const { return value->inspect(); }
node_type e_assert_node::type()       const { return E_ASSERT_NODE;    }
e_assert_node* e_assert_node::clone() const { return new e_assert_node(value->clone()); }