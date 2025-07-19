#pragma once

#include <vector>
#include <string>

struct token;

std::vector<token> lexer(const std::string& input_str);