#ifndef LEXER
#define LEXER

#include "structs_and_macros.h"
#include <vector>
#include <string>

int read_string();

int read_number();


std::vector<token> lexer(std::string input_str);
token create_token(keyword_enum keyword, std::string data, int line, int line_position);
token parse_quoted_string(char type);


#endif