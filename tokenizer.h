#ifndef TOKENIZER
#define TOKENIZER

#include "structs_and_macros.h"
#include <vector>
#include <string>

int read_identifier();

int read_number_literal();


std::vector<token> tokenizer(std::string input_str);

#endif