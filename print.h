#pragma once

#include "pch.h"

class node;
struct token;
struct table;


void print_tokens(std::vector<token> tokens);
void print_nodes(std::vector<node*> nodes);
