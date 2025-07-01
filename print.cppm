module;

#include "allocator_aliases.h"
#include "token.h"

#include <vector>

import object;
import node;

export module print;

export {

void print_tokens(const std::vector<token>& tokens);
void print_nodes(const avec<UP<node>>& nodes);
void print_global_tables(const avec<SP<table_object>>& g_tables);

}