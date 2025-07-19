#pragma once

#include "allocator_aliases.h"

#include <vector>
#include <string>

class f_table_info_object;
class table_object;

using display_table = struct display_table {
    bool to_display;
    SP<f_table_info_object> table_info;
};

struct cmd_line_arguments {
    bool run_tests;

    bool has_query; 
    std::string query;

    bool has_save_loc;
    std::string save_loc;

    bool debug;
    bool time;
    bool background;
};

struct table_cache {
    bool dirty;
    SP<table_object> table;
};

struct test_container {
    std::string folder_name;
    std::vector<std::string> test_paths;
    int max_tests = 0;
    int current_test_num = 0;
};

// TODO Have expected value for result
struct test {
    std::string text;
    bool except_fail;
    // Something like
    // bool has_expected_result;
    // std::string expected_result;
};



// IDK why these aren't in macros.h or something
#define CUR_LOC std::source_location::current()

#define FILE_NAME_STR                                           \
    std::filesystem::path(CUR_LOC.file_name()).filename().string()