#pragma once

#include "allocator_aliases.h"

#include <source_location>

class node;
class e_node;
class evaluated;
class object;
class environment;

void eval_init(avec<UP<node>> nds);
avec<UP<e_node>> eval();

// For reading files
[[maybe_unused]] UP<evaluated> eval_expression(object*    expression, SP<environment> env, const std::source_location& loc = std::source_location::current());
                 UP<evaluated> eval_expression(UP<object> expression, SP<environment> env, const std::source_location& loc = std::source_location::current());