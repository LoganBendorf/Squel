#pragma once

#include "allocator_aliases.h"
#include "token.h"

#include <vector>
// #include <cstddef>

class node;
class object;
class group_object;
class error_object;

void parser_init(std::vector<token> toks);
avec<UP<node>> parse();

// For reading files
token peek();
[[nodiscard]] UP<object> parse_expression(size_t precedence, const token& tok = peek());
[[nodiscard]] std::expected<UP<group_object>, UP<error_object>> parse_list_until_line_end();