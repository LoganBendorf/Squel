
#pragma once

#include "pch.h"

enum heap_or_arena {
    HEAP = false, ARENA = true
};

class SQL_data_type_object;

class table_info_object;
typedef struct display_table {
    bool to_display;
    table_info_object* table_info;
} display_table;

struct test {
    std::string folder_name;
    std::vector<std::string> test_paths;
    int max_tests = 0;
    int current_test_num = 0;
};
