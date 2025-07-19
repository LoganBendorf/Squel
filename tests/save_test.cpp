
#include "pch.h"

#include "allocators.h"
#include "allocator_aliases.h"
#include "object.h"
#include "structs.h"

#include "files.h"
#include "logger.h"



logger<error_msg> errors;
logger<error_msg> sql_errors;
logger<warning_msg> sql_warnings;
avec<table_cache> table_caches;
std::vector<UP<e_function_object>> g_functions;
std::string input;
display_table display_tab = {false, nullptr};
bool DEBUG = true;
cmd_line_arguments g_args;

std::vector<test_container> tests;

int main() {

    constexpr size_t size = 1 << 20;
    main_alloc<void>::allocate_stack_memory(size);
  
    auto type  = MAKE_UP(s_SQL_data_type_object, NONE, INTEGER, UP<serializable>(new integer_object(11)));
    auto row           = MAKE_UP(s_group_object, UP<serializable>(new integer_object(420)));
    auto detail = MAKE_UP( s_table_detail_object, "column_name", std::move(type));
    avec<UP<s_table_expr>> empty_exprs;
    auto table           = MAKE_SP(table_object, "table_name", std::move(detail), std::move(empty_exprs), std::move(row));

    table_caches.emplace_back(false, table);

    // save_tables(g_tables, "test_saved/output.txt");    
}
