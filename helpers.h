#pragma once

#include "pch.h"

#include "token.h"
#include "allocator_aliases.h"

#define SIZE_T_MAX size_t(-1)

class object;
class SQL_data_type_object;

std::span<const char* const> token_type_span();
std::string token_type_to_string(token_type index);

bool is_integer_data_type(SQL_data_type_object* data_type);
bool is_string_data_type(SQL_data_type_object* data_type);

bool is_sql_data_type_token(token tok);
bool is_numeric_token(token tok);
bool is_numeric_object(const UP<object>& obj);
bool is_conditional_object(const UP<object>& obj);
bool is_string_object(const UP<object>& obj);
bool is_evaluated(const UP<object>& obj);


[[maybe_unused]] bool is_numeric_data_type(SQL_data_type_object* data_type);