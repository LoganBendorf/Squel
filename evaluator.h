#pragma once

#include "allocator_aliases.h"

class node;
class e_node;

void eval_init(avec<UP<node>> nds);
avec<UP<e_node>> eval();