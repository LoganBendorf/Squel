#pragma once

#include "structs_and_macros.h"

#include <string>

std::string token_type_to_string(token_type index);

bool is_integer_data_type(SQL_data_type_object* data_type);
bool is_decimal_data_type(std::string data_type);
bool is_boolean_data_type(std::string data_type);
bool is_string_data_type(SQL_data_type_object* data_type);

bool is_numeric_token(token tok);
bool is_numeric_object(object* obj);

