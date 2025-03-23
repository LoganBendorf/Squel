
#include "print.h"

#include <iostream>
#include <iomanip>


void print_column(table tab, int column_index) {

    column_data col = tab.column_datas[column_index];

    if (col.field_name.length() > 16) {
        std::cout << "nah field name is too long to print";
        exit(1);}

    std::cout << col.field_name << std::endl;

    for (int i = 0; i < tab.rows.size(); i++) {
        std:: cout << tab.rows[column_index].column_values[i] << std::endl;
    }
}


void print_tokens(std::vector<token> tokens) {
    std::cout << "PRINTING TOKENS ----------------\n";
    for (int i = 0; i < tokens.size(); i++) {
        std::cout << " Keyword: " << std::setw(15) << std::left << token_type_to_string(tokens[i].type) + ","
                                       << " Value: " << tokens[i].data << "\n";
    }
    std::cout << "DONE ---------------------------\n\n";
}


void print_nodes(std::vector<node*> nodes) {
    std:: cout << "PRINTING NODES -----------------\n";
    for (int i = 0; i < nodes.size(); i++) {
        std::cout << std::to_string(i + 1) << ":\n" << nodes[i]->inspect();
    }
    std::cout << "DONE ---------------------------\n\n";
}