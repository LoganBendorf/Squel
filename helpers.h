#pragma once

#include "token.h"

#include <string>
#include <span>

class object;
class SQL_data_type_object;

std::span<const char* const> token_type_span();
std::string token_type_to_string(token_type index);

bool is_integer_data_type(SQL_data_type_object* data_type);
bool is_string_data_type(SQL_data_type_object* data_type);

bool is_sql_data_type_token(token tok);
bool is_numeric_token(token tok);
bool is_numeric_object(object* obj);
bool is_conditional_object(object* obj);
bool is_string_object(object* obj);