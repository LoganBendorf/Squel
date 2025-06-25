
#include "pch.h"

#include "print.h"

#include "helpers.h"
#include "structs_and_macros.h"
#include "node.h"

void print_tokens(std::vector<token> tokens) {
    std::cout << "PRINTING TOKENS ----------------\n";
    for (const auto& tok : tokens) {
        std::cout << " Keyword: " << std::setw(15) << std::left << token_type_to_string(tok.type) + ","
                                       << " Value: " << tok.data << "\n";
    }
    std::cout << "DONE ---------------------------\n\n";
}


void print_nodes(std::vector<node*> nodes) {
    std:: cout << "PRINTING NODES -----------------\n";
    size_t count = 1;
    for (const auto& node : nodes) {
        std::cout << std::to_string(count++) << ":\n" << node->inspect();
    }
    std::cout << "DONE ---------------------------\n\n";
}