#pragma once

#include "node.h"
#include <vector>

// Meat
void parser_init(std::vector<token> toks);
std::vector<node*> parse();

