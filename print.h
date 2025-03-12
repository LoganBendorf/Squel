#pragma once

#include "structs_and_macros.h"
#include "node.h"
#include <vector>


void print_column(table tab, int column_index);
void print_tokens(std::vector<token> tokens);
void print_nodes(std::vector<node*> nodes);
