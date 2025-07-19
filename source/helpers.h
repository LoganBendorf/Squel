#pragma once

#include "allocator_aliases.h"

#include "token.h"

#include <string>
#include <span>

class object;
class evaluated;
class serializable;
class SQL_data_type_object;

constexpr size_t SIZE_T_MAX = size_t(-1);

bool is_integer_data_type(SQL_data_type_object* data_type);

bool is_string_data_type(token_type data_type);
bool is_string_data_type(SQL_data_type_object* data_type);

bool is_sql_data_type_token(token tok);

bool is_numeric_token (token tok);
bool is_numeric_object(const UP<object>&    obj);
bool is_numeric_object(const UP<evaluated>& obj);
bool is_numeric_object(const UP<serializable>& obj);
bool is_conditional_object(const UP<object>&obj);
bool is_string_object (const UP<object>&    obj);
bool is_string_object (const UP<evaluated>& obj);
bool is_string_object (const UP<serializable>& obj);

bool is_object        (const UP<object>&    obj);
bool is_object        (const UP<serializable>&    obj);

bool is_evaluated     (const UP<evaluated>& obj);
bool is_evaluated     (const UP<serializable>& obj);

bool is_serializable  (const UP<evaluated>& obj);
bool is_serializable  (const UP<serializable>& obj);


[[maybe_unused]] bool is_numeric_data_type(SQL_data_type_object* data_type);

std::string get_stack_trace();

// Debug funcs
std::string call_inspect(const UP<object>& obj);
std::string call_inspect(const UP<evaluated>& obj);
std::string call_inspect(const UP<serializable>& obj);
std::string call_inspect(const avec<UP<serializable>>& objs);
const UP<object>& index_avec(const avec<UP<object>>& vec, size_t index);
size_t avec_size(const avec<UP<object>>& vec);