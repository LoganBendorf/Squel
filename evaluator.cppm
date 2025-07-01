module;

#include "allocator_aliases.h"

import node;

export module evaluator;

export {

void eval_init(avec<UP<node>> nds);
avec<UP<e_node>> eval();

}