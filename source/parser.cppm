module;

#include "allocator_aliases.h"

#include <vector>

import node;
struct token;

export module parser;


export {

void parser_init(std::vector<token> toks);
avec<UP<node>> parse();

}
