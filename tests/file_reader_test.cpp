#include "pch.h"

#include "allocator_aliases.h"
#include "object.h"
#include "lexer.h"
#include "helpers.h"
#include "environment.h"
#include "parser.h"
#include "evaluator.h"
#include "execute.h"
#include "logger.h"


#include "structs.h"
#include "print.h"
#include "files.h"

avec<table_cache> table_caches;
std::vector<SP<e_function_object>> g_functions;
logger<error_msg> errors;
logger<error_msg> sql_errors;
logger<warning_msg> sql_warnings;
display_table display_tab = {false, nullptr};
bool DEBUG = true;
cmd_line_arguments g_args;


int main() {
    // bool rc = read_saved_files(g_tables, "test_saved/output.txt");
    // if (rc) {
    //     std::cout << "No errors during table reading, printing tables..." << std::endl;
    //     print_global_tables(g_tables);
    // } else {
    //     std::cout << "Failed to read tables" << std::endl;
    //     return 1;
    // }
}
