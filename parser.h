#pragma once

#include "token.h"

#include <vector>

class evaluated_function_object;
class node;

void parser_init(std::vector<token> toks, std::vector<evaluated_function_object*> global_funcs);
std::vector<node*> parse();

