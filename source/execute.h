#pragma once

#include "allocator_aliases.h"
#include "environment.h"

class e_node;
class serializable;
class evaluated;
class s_table_detail_object;
class error_object;
class s_parameter_object;
class s_table_expr;
class s_environment;

void execute_init(avec<UP<e_node>> nds);
void execute();

// For reading files
UP<serializable> make_serializable(UP<evaluated> object, SP<s_environment> env);
[[nodiscard]] std::expected<UP<s_table_detail_object>, UP<error_object>> parameter_to_table_detail(UP<s_parameter_object> param_obj);