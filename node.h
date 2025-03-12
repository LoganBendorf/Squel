#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <sstream>

#include "structs_and_macros.h"
#include "helpers.h"


class node {
    public:
    virtual std::string inspect() = 0;
    virtual std::string type() = 0;
    virtual ~node() {};
};

class null : public node {
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
        for (int i = 0; i < field_names.size(); i++) {
            output << field_names[i] << " ";
        }
        output << "\n";
        for (int i = 0; i < values.size(); i++) {
            output << "Type: " << keyword_enum_to_string(values[i].type) << ", data: " << values[i].data << " ";
        }
        output << "\n";
        return output.str();
    }

    std::string type() override {
        return "insert_into";
    }

    public:
    std::string table_name;
    std::vector<std::string> field_names;
    std::vector<struct data_type_pair> values;
};

class select_from : public node {
    public:
    std::string inspect() override {
        return "select_from: " + table_name + "\n";
    }
    std::string type() override {
        return std::string("select_from");
    }

    public:
    std::string table_name;
};

class create_table : public node {
    public:
    std::string inspect() override {
        std::stringstream output;
        output << "create_table: ";
        output << table_name << "\n";
        for (int i = 0; i < column_datas.size(); i++) {
            output << column_datas[i].field_name << " " << column_datas[i].data_type << " "<< column_datas[i].default_value << " ";
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

