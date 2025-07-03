module;

#include "allocator_aliases.h"

#include <vector>

export module print;

import object;
import node;
import token;

export {

void print_tokens(const std::vector<token>& tokens);
void print_nodes(const avec<UP<node>>& nodes);
void print_global_tables(const avec<SP<table_object>>& g_tables);

}