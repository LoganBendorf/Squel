

#include "node.h"
#include "helpers.h"
#include "object.h"

#include <string>
#include <vector>
#include <sstream>


// null_node
std::string null_node::inspect() {
    return std::string("nada"); 
}
node_type null_node::type() {
    return NULL_NODE; 
}

// function_node
function::function(function_object* set_func) {
    func = set_func;
}

std::string function::inspect() {
    return std::string("function_node\n"); 
}
node_type function::type() {
    return FUNCTION_NODE; 
}

// insert_into_node
insert_into::insert_into(object* set_table_name, std::vector<object*> set_fields, std::vector<object*> set_values) {
    table_name = set_table_name;
    fields = set_fields;
    values = set_values;
}

std::string insert_into::inspect()  {
    std::string ret_str = "";
    ret_str += "insert_into: ";
    ret_str += table_name->inspect() + "\n";
    ret_str += "[Fields: ";
    for (int i = 0; i < fields.size(); i++) {
        ret_str += fields[i]->inspect() + ", "; }

    if (fields.size() > 0) {
        ret_str = ret_str.substr(0, ret_str.size() - 2); }

    ret_str += "], [Values: ";
    for (int i = 0; i < values.size() - 1; i++) {
        ret_str  += values[i]->inspect() + ", "; }

    if (values.size() > 0) {
        ret_str += values[values.size() - 1]->inspect(); }
    ret_str += "]\n";
    return ret_str;
}

node_type insert_into::type() {
    return INSERT_INTO_NODE; 
}


// select_from_node
select_from::select_from(object* set_table_name, std::vector<object*> set_column_names, bool set_asterisk, object* set_condition) {
    table_name = set_table_name;
    column_names = set_column_names;
    asterisk = set_asterisk;
    condition = set_condition;
}

std::string select_from::inspect() {
    std::string ret_str = "select_from: " + table_name->inspect() + "\n";

    if (condition) {
        ret_str += "condition: " + condition->inspect() + "\n"; }
    
    for (int i = 0; i < column_names.size(); i++) {
        ret_str += column_names[i]->inspect() + ", ";}

    if (column_names.size() > 0) {
        ret_str = ret_str.substr(0, ret_str.size() - 2); }

    ret_str += "\n";
    return ret_str;

}

node_type select_from::type() {
    return SELECT_FROM_NODE;
}


// alter_table_node
alter_table::alter_table(object* tab_name, object* tab_edit) {
    table_name = tab_name;
    table_edit = tab_edit;
}

std::string alter_table::inspect() {
    std::string ret_str = "";
    ret_str += "alter_table: ";
    ret_str += table_name->inspect() + "\n";
    ret_str += table_edit->inspect();
    return ret_str;
}

node_type alter_table::type() {
    return ALTER_TABLE_NODE; 
}


// create_table_node
create_table::create_table(object* set_table_name, std::vector<table_detail_object*> set_details){
    table_name = set_table_name;
    details = set_details;
}

std::string create_table::inspect() {
    std::string ret_str = "";
    ret_str += "create_table: ";
    ret_str += table_name->inspect() + "\n[";
    for (const auto& detail : details) {
        ret_str += detail->inspect() + ", "; }

    if (details.size() > 0) {
        ret_str = ret_str.substr(0, ret_str.size() - 2); }

    ret_str += "]\n";
    return ret_str;
}

node_type create_table::type() {
    return CREATE_TABLE_NODE; 
}


