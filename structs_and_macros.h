
#pragma once

#include "object.h"

#include <string>
#include <vector>


typedef struct column_data {
    std::string field_name;
    SQL_data_type_object* data_type;
    std::string default_value;
} column_data;

typedef struct row {
    std::vector<std::string> column_values;
} row;

typedef struct table {
    std::string name;
    std::vector<column_data> column_datas;
    std::vector<row> rows;
} table;

typedef struct display_table {
    bool to_display;
    table tab;
    std::vector<std::string> column_names;
    std::vector<int> row_ids;
} display_table;

struct test {
    std::string folder_name;
    std::vector<std::string> test_paths;
    int max_tests = 0;
    int current_test_num = 0;
};
