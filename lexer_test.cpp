
#include <string>
#include <vector>

#include "lexer.h"
#include "structs_and_macros.h"

void test(std::vector<token> expected_tokens, std::string test_input);



std::vector<std::string> errors;



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
                       {{CREATE, "CREATE"},
                        {TABLE, "TABLE"},
                        {SELECT, "SELECT"},
                        {FROM, "FROM"},
                        {INSERT, "INSERT"},
                        {INTO, "INTO"},
                        {VALUES, "VALUES"},
                        {STRING_LITERAL, "STRING_LITERAL"},
                        {INTEGER_LITERAL, "12"},
                        {OPEN_PAREN, "("},
                        {CLOSE_PAREN, ")"},
                        {SEMICOLON, ";"},
                        {COMMA, ","},
                        {ASTERISK, "*"},
                        {ILLEGAL, "@"},
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
 
}

void test(std::vector<token> expected_tokens, std::string test_input) { 

    std::vector<token> tokens = lexer(test_input);

    if (expected_tokens.size() != tokens.size()) {
        printf("bad token size. got=%ld, expected=%ld\n", tokens.size(), expected_tokens.size());
        for (int i = 0; i < tokens.size(); i++) {
            std::cout << "[Type: " << tokens[i].keyword << ", Data: " << tokens[i].data << "], ";
        }
        std::cout << std::endl;
        exit(1);
    }
    for (int i = 0; i < tokens.size(); i++) {
        if (tokens[i].keyword != expected_tokens[i].keyword) {
            printf("tokens had mismatching type. Actual (%d). Expected (%d)\n", tokens[i].keyword, expected_tokens[i].keyword);
            exit(1);
        }
        if (tokens[i].data != expected_tokens[i].data) {
            std::cout << "tokens had mismatching data. Actual (" << tokens[i].data << "). Expected (" << expected_tokens[i].data << ")\n";
            exit(1);
        }
    }
}