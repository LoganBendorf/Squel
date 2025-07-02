
#include "../pch.h"

#include "../parser.h"
#include "../arena.h"
#include "../environment.h"
#include "../evaluator.h"
#include "../lexer.h"
#include "../print.h"

#include <iostream>
#include <chrono>

static std::vector<evaluated_function_object*> global_functions;


int main() {
    
    std::string input = "CREATE OR REPLACE FUNCTION category_func(number DOUBLE)"
                        "RETURNS VARCHAR"
                        "AS $$"
                        "BEGIN"
                            "IF number < 90 THEN"
                            "RETURN 'Low';"
                            "ELSIF number < 110 THEN"
                            "RETURN 'Medium';"
                            "ELSE"
                            "RETURN 'High';"
                            "END IF;"
                        "END;"
                        "$$;"
                        "CREATE OR REPLACE FUNCTION returns_20()"
                        "RETURNS INTEGER"
                        "AS $$"
                        "BEGIN"
                            "RETURN 20;"
                        "END;"
                        "$$;"
                        "CREATE TABLE function_where_test ("
                            "num INT,"
                            "str VARCHAR(returns_20())"
"                        );"
                        "INSERT INTO function_where_test (num, str)"
                        "VALUES (returns_20(), category_func(20));"
                        "INSERT INTO function_where_test (num, str)"
                        "VALUES (returns_20(), category_func(170));"
                        "SELECT * FROM function_where_test WHERE str = category_func(170);";

    arena_inst.init();

    const std::vector<token> tokens = lexer(input);
    // print_tokens(tokens);

    auto start = std::chrono::high_resolution_clock::now();

    parser_init(tokens, global_functions);
    const std::vector<node*> nodes = parse();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    std::cout << "Parser time taken: " << duration.count() << " seconds\n";
    // print_nodes(nodes);

    // environment* env = eval_init(nodes);
    // std::vector<evaluated_function_object*> new_funcs = eval(env);
    // for (const auto& new_func : new_funcs) {
    //     global_functions.push_back(static_cast<evaluated_function_object*>(new_func->clone(false))); 
    // }

    arena_inst.destroy();
}