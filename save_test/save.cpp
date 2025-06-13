

#include "allocators.h"
#include "allocator_aliases.h"
#include "structs_and_macros.h"
#include "object.h"

#include "token.h"

#include <fstream>  // std::ofstream
#include <iostream>

extern main_alloc<bool> arena_inst;

std::vector<std::string> errors;
std::vector<std::string> warnings;

static avec<table_object*> g_tables;
static std::vector<evaluated_function_object*> g_functions;

std::string input = "";

display_table display_tab = {false, nullptr};

std::vector<struct test_container> tests;


[[maybe_unused]] bool save_tables();

[[maybe_unused]] static void clear_g_tables() {
    for (const auto& tab : g_tables) {
        delete tab;
    }
    g_tables.clear();
}

int main() {

    constexpr size_t size = 1 << 18;
    std::byte stack_buffer[size];
    main_alloc<void>::allocate_stack_memory(stack_buffer, size);


    // SQL_data_type_object* type = new SQL_data_type_object(NONE, INT, new integer_object(11)); // some nonsense here

    // [[maybe_unused]] table_detail_object* detail = new table_detail_object("column_name", type, new null_object());

    // // group_object* content = {new group_object({new integer_object(420)})};
    // [[maybe_unused]] group_object* content = {new group_object(new null_object())};

    // table_object* tab = new table_object("table_name", detail, content);

    // g_tables.push_back(tab);

    // save_tables();

    // clear_g_tables();
    // arena_inst.destroy();

    // auto type = UP<SQL_data_type_object>(new SQL_data_type_object(NONE, INT, new integer_object(11)));
    // auto detail = UP<table_detail_object>(new table_detail_object("column_name", type.get(), new null_object()));
    auto type = new SQL_data_type_object(NONE, INT, new integer_object(11));
    auto detail = new table_detail_object("column_name", type, new null_object());
    
    // Test cloning without table_object
    std::cout << "About to clone detail..." << std::endl;
    table_detail_object* cloned_detail = detail->clone(); // Clone to heap
    // std::cout << "Cloned detail, in_arena=" << cloned_detail->in_arena << std::endl;
    
    // Clean up the clone
    delete cloned_detail;
    
    main_alloc<void>::deallocate_stack_memory();
    
}

bool save_tables() {
    std::ofstream out_file("output.txt");
    if (!out_file) {
        std::cerr << "Error: could not open file for writing\n";
        return false;
    }

    for (const auto& table : g_tables) {
        for ([[maybe_unused]] const auto& column_data : table->column_datas) {
            out_file << "Column!";
        }
    }


    // 3. Close the file (happens automatically when outFile goes out of scope,
    //    but it's good practice to close explicitly if you’re done early).
    out_file.close();

    return true;
}
