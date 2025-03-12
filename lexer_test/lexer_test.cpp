
#include <string>
#include <vector>
#include <iostream>

#include "lexer.h"
#include "structs_and_macros.h"

void test(std::vector<token> expected_tokens, std::string test_input);



std::vector<std::string> errors;

// YOINKED FROM PARSER
std::string keyword_enum_to_string[] =  
    {"CREATE", "TABLE", "SELECT", "FROM", "INSERT", "INTO", "VALUES", 
        "STRING_LITERAL", "INTEGER_LITERAL", "OPEN_PAREN", "CLOSE_PAREN",
         "SEMICOLON", "COMMA", "ASTERISK", "LINE_END", "ILLEGAL", "NEW_LINE", "DATA", "QUOTE", "BOOL"};



int main() {
    {
    std::string input = 
            "CREATE "
            "TABLE "
            "SELECT "
            "FROM "
            "INSERT "
            "INTO "
            "VALUES "
            "STRING_LITERAL "
            "12 "
            "("
            ")"
            ";"
            ","
            "*"
            "@";
    std::vector<token> expected_tokens =  
                       {{CREATE,         "CREATE",  1, 0},
                        {TABLE,          "TABLE",   1, 7},
                        {SELECT,         "SELECT",  1, 13},
                        {FROM,           "FROM",    1, 20},
                        {INSERT,         "INSERT",  1, 25},
                        {INTO,           "INTO",    1, 32},
                        {VALUES,         "VALUES",  1, 37},
                        {STRING_LITERAL, "STRING_LITERAL", 1, 44},
                        {INTEGER_LITERAL,"12",1, 59},
                        {OPEN_PAREN,     "(", 1, 62},
                        {CLOSE_PAREN,    ")", 1, 63},
                        {SEMICOLON,      ";", 1, 64},
                        {COMMA,          ",", 1, 65},
                        {ASTERISK,       "*", 1, 66},
                        {ILLEGAL,        "@", 1, 67},
                       };
    test(expected_tokens, input);
    std::cout << "Test 1 pass\n";
    }

    {
        std::string input = "";
        std::vector<token> expected_tokens = {};
        test(expected_tokens, input);
        std::cout << "Test 2 pass\n";
    }

    {
        std::string input = "CREATE TABLE default_value_test (\n"
                                "hello INT DEFAULT 12,\n"
                                "goodbye INT\n"
                            ");";
                            
        std::vector<token> expected_tokens =  
                       {{CREATE,         "CREATE",  1, 0},
                        {TABLE,          "TABLE",   1, 7},
                        {STRING_LITERAL, "default_value_test",  1, 13},
                        {OPEN_PAREN,     "(",       1, 32},
                        {STRING_LITERAL, "hello",   2, 0},
                        {STRING_LITERAL, "INT",     2, 6},
                        {STRING_LITERAL, "DEFAULT", 2, 10},
                        {INTEGER_LITERAL,"12",      2, 18},
                        {COMMA          ,",",       2, 20},
                        {STRING_LITERAL, "goodbye", 3, 0},
                        {STRING_LITERAL, "INT",     3, 8},
                        {CLOSE_PAREN,    ")",       4, 0},
                        {SEMICOLON,      ";",       4, 1},
                       };   
        test(expected_tokens, input);
        std::cout << "Test 3 pass\n";
    }
    {
        std::string input = "-010.010"; // Use this is supposed to be weird. It's the PARSER's job to detect it
        std::vector<token> expected_tokens = 
                       {{MINUS,           "-",   1, 0},
                        {INTEGER_LITERAL, "010", 1, 1},
                        {DOT,             ".",   1, 4},
                        {INTEGER_LITERAL, "010", 1, 5},
                        };
        test(expected_tokens, input);
        std::cout << "Test 4 pass\n";
    }
 
}

void test(std::vector<token> expected_tokens, std::string test_input) { 

    std::vector<token> tokens = lexer(test_input);

    if (expected_tokens.size() != tokens.size()) {
        printf("bad token size. got=%ld, expected=%ld\n", tokens.size(), expected_tokens.size());
        for (int i = 0; i < tokens.size(); i++) {
            std::cout << "[Type: " << keyword_enum_to_string[tokens[i].keyword] << ", Data: " << tokens[i].data << "],\n";
        }
        std::cout << std::endl;
        exit(1);
    }
    for (int i = 0; i < tokens.size(); i++) {
        if (tokens[i].keyword != expected_tokens[i].keyword) {
            printf("tokens had mismatching type. Actual: %s (%s). Expected: %s (%s)\n", 
                        tokens[i].data.c_str(), keyword_enum_to_string[tokens[i].keyword].c_str(),
                        expected_tokens[i].data.c_str(), keyword_enum_to_string[expected_tokens[i].keyword].c_str());
            exit(1);
        }
        if (tokens[i].data != expected_tokens[i].data) {
            std::cout << "tokens had mismatching data. Actual (" << tokens[i].data << "). Expected (" << expected_tokens[i].data << ")\n";
            exit(1);
        }
        if (tokens[i].line != expected_tokens[i].line) {
            std::cout << "tokens had mismatching line. Actual (" << std::to_string(tokens[i].line) << "). Expected (" << std::to_string(expected_tokens[i].line) << ")\n";
            std::cout << "Actual data (" << tokens[i].data << "). Expected data (" << expected_tokens[i].data << ")\n";
            exit(1);
        }
        if (tokens[i].position != expected_tokens[i].position) {
            std::cout << "tokens had mismatching position. Actual (" << std::to_string(tokens[i].position) << "). Expected (" << std::to_string(expected_tokens[i].position) << ")\n";
            std::cout << "Actual data (" << tokens[i].data << "). Expected data (" << expected_tokens[i].data << ")\n";
            exit(1);
        }
    }
}