#pragma once

#include "pch.h"
#include "arena_aliases.h"

class evaluated_function_object;
class node;

class table_object;
struct token;

template<typename T>
class arena;

void parser_init(std::vector<token> toks, std::vector<evaluated_function_object*> global_funcs, avec<table_object*> global_tabs);
std::vector<node*> parse();

