#pragma once

#include "pch.h"
#include "allocator_aliases.h"

class environment;
class node;
class evaluated_function_object;
class table_object;

SP<environment> eval_init(avec<UP<node>> nds, avec<SP<evaluated_function_object>>& g_functions, avec<SP<table_object>>& g_tables);
std::pair<avec<SP<evaluated_function_object>>, avec<SP<table_object>>> eval(SP<environment> env);