#pragma once


#include <vector>

class environment;
class node;
class evaluated_function_object;

struct table;

environment* eval_init(std::vector<node*> nds, std::vector<evaluated_function_object*> g_functions, std::vector<table> g_tables);
std::pair<std::vector<evaluated_function_object*>, std::vector<table>> eval(environment* env);