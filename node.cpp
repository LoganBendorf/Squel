

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
std::string function::inspect() {
    return std::string("function_node\n"); 
}
node_type function::type() {
    return FUNCTION_NODE; 
}

// insert_into_node
std::string insert_into::inspect()  {
    std::string ret_str = "";
    ret_str += "insert_into: ";
    ret_str += table_name + "\n";
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
select_from::select_from() {
    asterisk = false;
}

std::string select_from::inspect() {
    std::string str = "select_from: " + table_name + "\n";

    if (condition) {
        str += "condition: " + condition->inspect() + "\n";
    }
    
    for (int i = 0; i < column_names.size() - 1 && column_names.size() > 0; i++) {
        str += column_names[i] + ", ";}

    if (column_names.size() > 0) {
        str += column_names[column_names.size() - 1];}
    str += "\n";
    return str;

}

node_type select_from::type() {
    return SELECT_FROM_NODE;
}


// alter_table_node
std::string alter_table::inspect() {
    std::string ret_str = "";
    ret_str += "alter_table: ";
    ret_str += table_name + "\n";
    ret_str += table_edit->inspect();
    return ret_str;
}

node_type alter_table::type() {
    return ALTER_TABLE_NODE; 
}


// create_table_node
std::string create_table::inspect() {
    std::string ret_str = "";
    ret_str += "create_table: ";
    ret_str += table_name + "\n";
    for (int i = 0; i < column_datas.size(); i++) {
        ret_str += column_datas[i].field_name + "], " + column_datas[i].data_type->inspect() + ", [Default: " + column_datas[i].default_value + "]";
        if (i != column_datas.size() - 1) {
            ret_str += ",\n";
        }
    }
    ret_str += "\n";
    return ret_str;
}

node_type create_table::type() {
    return CREATE_TABLE_NODE; 
}


