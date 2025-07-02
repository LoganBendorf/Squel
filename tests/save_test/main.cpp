#include "../pch.h"

#include "allocators.h"
#include "allocator_aliases.h"
#include "structs_and_macros.h"
#include "token.h"

#include <fstream>  // std::ofstream
#include <iostream>

import object;

extern main_alloc<bool> arena_inst;

std::vector<std::string> errors;
std::vector<std::string> warnings;

avec<SP<table_object>> g_tables;
std::vector<UP<evaluated_function_object>> g_functions;

std::string input = "";

display_table display_tab = {false, nullptr};

std::vector<struct test_container> tests;


bool save_tables();

int main() {

    constexpr size_t size = 1 << 20;
    main_alloc<void>::allocate_stack_memory(size);

    auto type   = MAKE_UP(e_SQL_data_type_object, NONE, INTEGER, UP<evaluated>(new integer_object(11)));
    auto row    = MAKE_UP(e_group_object, UP<evaluated>(new integer_object(420)));
    auto detail = MAKE_UP(e_table_detail_object, "column_name", std::move(type), UP<evaluated>(new null_object()));
    auto table  = MAKE_SP(table_object, "table_name", std::move(detail), std::move(row));

    g_tables.push_back(table);

    save_tables();    
}

bool save_tables() {
    std::ofstream out_file("output.txt");
    if (!out_file) {
        std::cerr << "Error: could not open file for writing\n";
        return false;
    }

    
    for (const auto& table : g_tables) {
        out_file << table->table_name << "\n";

        for (const auto& col_data : table->column_datas) {
            out_file << "\t(" << col_data->serialize() << ")\n";
        }

        for (const auto& row : table->rows) {
            out_file << "\t" << row->inspect() << "\n";
        }
    }

    out_file.close();

    return true;
}
