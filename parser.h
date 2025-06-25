#pragma once

#include "pch.h"

#include "allocator_aliases.h"
#include "node.h"

// class evaluated_function_object;
class node;

// class table_object;
struct token;

void parser_init(std::vector<token> toks, avec<SP<evaluated_function_object>>& g_funcs, avec<SP<table_object>> g_tabs);
avec<UP<node>> parse();

