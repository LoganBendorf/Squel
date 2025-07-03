module;

#include "allocator_aliases.h"

export module evaluator;

import node;

export {

void eval_init(avec<UP<node>> nds);
avec<UP<e_node>> eval();

}