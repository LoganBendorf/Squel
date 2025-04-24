#pragma once


#include "node.h"
#include "environment.h"

#include <vector>

environment* eval_init(std::vector<node*> nodes);
void eval(environment* env);