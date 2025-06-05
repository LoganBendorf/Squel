
#include "structs_and_macros.h"
#include "arena.h"
#include "object.h"

#include "token.h"

#include <fstream>  // std::ofstream
#include <iostream>

template<typename T>
using avec = std::vector<T, MyAllocator<T>>;

int main() {

    SQL_data_type_object* type = new SQL_data_type_object(NONE, INT, new integer_object(11)); // some nonsense here

    table_detail_object* detail = new table_detail_object("column_name", type, new null_object());

    table_object* tab = new table_object("table_name", {detail}, {new group_object({new integer_object(420)})});


}

bool save_table(table_object* table) {
    std::ofstream outFile("output.txt");
    if (!outFile) {
        std::cerr << "Error: could not open file for writing\n";
        return false;
    }

    for (const auto& column_data : *table->column_datas) {

    }

    // 2. Write whatever you need; here, a simple string.
    outFile << "Hello, world!\n";
    outFile << "Saving this line to output.txt\n";

    // 3. Close the file (happens automatically when outFile goes out of scope,
    //    but it's good practice to close explicitly if you’re done early).
    outFile.close();

    return true;
}
