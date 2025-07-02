module;

#include "token.h"

#include <vector>
#include <string>

export module lexer;

export {

std::vector<token> lexer(std::string input_str);

}

