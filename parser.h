#ifndef PARSER_HEADER
#define PARSER_HEADER

#include <string>
#include "structs_and_macros.h"
#include "node.h"

// Meat
void parser_init(std::vector<token> toks);
std::vector<node*> parse();
void parse_insert();
void parse_select();
void parse_create();
void parse_create_table();
std::string parse_data_type();

// Helpers
keyword_enum peek();
std::string peek_data();
keyword_enum peek_ahead();



#endif