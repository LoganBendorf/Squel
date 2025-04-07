#pragma once

#include "structs_and_macros.h"
#include "helpers.h"
#include "object.h"

#include <string>
#include <vector>
#include <iostream>
#include <sstream>



class node {
    public:
    virtual std::string inspect() = 0;
    virtual std::string type() = 0;
    virtual ~node() {};
};

class null_node : public node {
    public:
    std::string inspect() override {
        return std::string("nada");
    }
    std::string type() override {
        return std::string("null");
    }
};

class insert_into : public node {
    public:
    std::string inspect() override {
        std::stringstream output;
        output << "insert_into: ";
        output << table_name << "\n";
        output << "[Field names: ";
        for (int i = 0; i < field_names.size() - 1; i++) {
            output << field_names[i] << ", ";
        }
        if (field_names.size() > 0) {
            output << field_names[field_names.size() - 1];
        }

        output << "], [Values: ";
        for (int i = 0; i < values.size() - 1; i++) {
            output  << values[i]->inspect() << ", ";
        }
        if (values.size() > 0) {
            output << values[values.size() - 1]->inspect();
        }
        output << "]\n";
        return output.str();
    }

    std::string type() override {
        return "insert_into";
    }

    public:
    std::string table_name;
    std::vector<std::string> field_names;
    std::vector<object*> values;
};

class select_from : public node {
    
    public:
    select_from() {
        asterisk = false;}
    std::string inspect() override {
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
    std::string type() override {
        return std::string("select_from");}

    public:
    std::string table_name;
    std::vector<std::string> column_names;
    bool asterisk;
    object* condition;
};

class alter_table : public node {
    public:
    std::string inspect() override {
        std::stringstream output;
        output << "alter_table: ";
        output << table_name << "\n";
        output << table_edit->inspect();
        return output.str();
    }
    std::string type() override {
        return std::string("alter_table");
    }
    public:
    std::string table_name;
    object* table_edit;
};

class create_table : public node {
    public:
    
    std::string inspect() override {
        std::stringstream output;
        output << "create_table: ";
        output << table_name << "\n";
        for (int i = 0; i < column_datas.size(); i++) {
            output << "[Column name: " << column_datas[i].field_name << "], " << column_datas[i].data_type->inspect() << ", [Default: " << column_datas[i].default_value << "]";
            if (i != column_datas.size() - 1) {
                output << ",\n";
            }
        }
        output << "\n";
        return output.str();
    }
    std::string type() override {
        return std::string("create_table");
    }
    public:
    std::string table_name;
    std::vector<column_data> column_datas;

};

