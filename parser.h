#ifndef PARSER_HEADER
#define PARSER_HEADER

#include <string>
#include "structs_and_macros.h"

struct insert_into {
    std::string table_name;
    std::vector<std::string> field_names;
    std::vector<std::string> values;
};

// Meat
void parser_init(std::vector<token> toks);
void parse();
void eval_insert_into(struct insert_into info);
void parse_insert();
void parse_select();
void parse_create();
void parse_create_table();
std::string parse_data_type();

// Helpers
keyword_enum peek();
std::string peek_data();
keyword_enum peek_ahead();

void print_table(table tab);



#endif