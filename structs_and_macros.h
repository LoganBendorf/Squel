
#pragma once

#include "allocator_aliases.h"

#include <vector>
#include <string>

class SQL_data_type_object;

class table_info_object;
using display_table = struct display_table {
    bool to_display;
    SP<table_info_object> table_info;
};

struct test_container {
    std::string folder_name;
    std::vector<std::string> test_paths;
    int max_tests = 0;
    int current_test_num = 0;
};

struct test {
    std::string text;
    bool except_fail;
};


#define SIZE_T_MIN_FUNC(a, b) (a) < (b) ? (a) : (b)