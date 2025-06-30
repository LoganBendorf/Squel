
#include "pch.h"

#include "print.h"

#include "helpers.h"
#include "structs_and_macros.h"
#include "node.h"

void print_tokens(const std::vector<token>& tokens) {
    std::cout << "PRINTING TOKENS ----------------\n";
    for (const auto& tok : tokens) {
        std::cout << " Keyword: " << std::setw(15) << std::left << token_type_to_string(tok.type) + ","
                                       << " Value: " << tok.data << "\n";
    }
    std::cout << "DONE ---------------------------\n\n";
}


void print_nodes(const avec<UP<node>>& nodes) {
    std::string num_tables = std::to_string(nodes.size());
    size_t dash_length = 14 - num_tables.length();
    std::cout << "PRINTING NODES (" << num_tables << ") " << std::string(dash_length, '-') << "\n";
    size_t count = 1;
    for (const auto& node : nodes) {
        std::cout << std::to_string(count++) << ":\n" << node->inspect();
    }
    std::cout << "DONE ---------------------------\n\n";
}

void print_global_tables(const avec<SP<table_object>>& g_tables) {
    std::string num_tables = std::to_string(g_tables.size());
    size_t dash_length = 13 - num_tables.length();
    std::cout << "PRINTING TABLES (" << num_tables << ") " << std::string(dash_length, '-') << "\n";
    size_t count = 1;
    for (const auto& table : g_tables) {
        std::cout << std::to_string(count++) << ":\n" << table->inspect();
    }
    std::cout << "DONE ---------------------------\n\n";
}