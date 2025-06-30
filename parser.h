#pragma once

#include "allocator_aliases.h"
#include "node.h"

#include <vector>

class node;
struct token;

void parser_init(std::vector<token> toks);
avec<UP<node>> parse();

