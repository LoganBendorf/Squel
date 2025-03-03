
#include "node.h"

void eval_init(std::vector<node*> nodds);
void eval();
void eval_create_table(const create_table* info);
void eval_select_from(const select_from* info);
void print_table(table tab);
void eval_insert_into(const insert_into* info);