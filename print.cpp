
#include "print.h"

#include "helpers.h"

#include <iomanip>
#include <iostream>


void print_column(table tab, size_t column_index) {

    column_data col = tab.column_datas[column_index];

    if (col.field_name.length() > 16) {
        std::cout << "nah field name is too long to print";
        exit(1);}

    std::cout << col.field_name << std::endl;

    for (size_t i = 0; i < tab.rows.size(); i++) {
        std:: cout << tab.rows[column_index].column_values[i] << std::endl;
    }
}


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