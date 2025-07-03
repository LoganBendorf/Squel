module;

#include "allocator_aliases.h"

#include <vector>

export module parser;

import node;
import token;

export {

void parser_init(std::vector<token> toks);
avec<UP<node>> parse();

}
