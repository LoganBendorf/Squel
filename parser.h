#pragma once

#include "pch.h"

class evaluated_function_object;
class node;

class table_object;
struct token;

void parser_init(std::vector<token> toks, std::vector<evaluated_function_object*> global_funcs, std::vector<table_object*> global_tabs);
std::vector<node*> parse();

