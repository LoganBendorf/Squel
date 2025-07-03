module;

#include <vector>
#include <string>

export module lexer;

import token;

export {

std::vector<token> lexer(std::string input_str);

}

