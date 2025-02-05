
#include <string>
#include <vector>

#include "tokenizer.h"
#include "structs_and_macros.h"

void test(std::vector<token> expected_tokens, std::string test_input);



std::vector<std::string> errors;

std::string input;
std::vector<token> expected_tokens;

int main() {
    {
    input = "CREATE";
    expected_tokens = {{CREATE, "CREATE"}};
    test(expected_tokens, input);
    }

    printf("no errors found yay\n");    
}

void test(std::vector<token> expected_tokens, std::string test_input) { 

    std::vector<token> tokens = tokenizer(test_input);

    if (expected_tokens.size() != tokens.size()) {
        printf("bad token size\n");
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