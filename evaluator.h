#pragma once

#include "pch.h"

class environment;
class node;
class evaluated_function_object;
class table_object;

environment* eval_init(std::vector<node*> nds, std::vector<evaluated_function_object*> g_functions, std::vector<table_object*> g_tables);
std::pair<std::vector<evaluated_function_object*>, std::vector<table_object*>> eval(environment* env);