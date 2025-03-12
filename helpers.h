#pragma once

#include "structs_and_macros.h"

#include <string>

std::string keyword_enum_to_string(keyword_enum index);

bool is_integer_data_type(std::string data_type);
bool is_decimal_data_type(std::string data_type);
bool is_boolean_data_type(std::string data_type);
bool is_string_data_type(std::string data_type);

