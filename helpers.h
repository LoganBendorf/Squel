#pragma once

#include "token.h"
#include "allocator_aliases.h"

#include <string>
#include <span>

#define SIZE_T_MAX size_t(-1)

class object;
class evaluated;
class SQL_data_type_object;

std::span<const char* const> token_type_span();
std::string token_type_to_string(token_type index);

bool is_integer_data_type(SQL_data_type_object* data_type);
bool is_string_data_type(SQL_data_type_object* data_type);

bool is_sql_data_type_token(token tok);
bool is_numeric_token(token tok);
bool is_numeric_object(const UP<object>& obj);
bool is_numeric_object(const UP<evaluated>& obj);
bool is_conditional_object(const UP<object>& obj);
bool is_string_object(const UP<object>& obj);
bool is_string_object(const UP<evaluated>& obj);
bool is_evaluated(const UP<evaluated>& obj);


[[maybe_unused]] bool is_numeric_data_type(SQL_data_type_object* data_type);

// Debug funcs
std::string call_inspect(const UP<object>& obj);
std::string call_inspect(const UP<evaluated>& obj);
const UP<object>& index_avec(const avec<UP<object>>& vec, size_t index);
size_t avec_size(const avec<UP<object>>& vec);