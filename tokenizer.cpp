
#include "structs_and_macros.h"
#include <vector>

extern std::vector<std::string> errors;

static std::vector<token> tokens;

static std::string input;
static int input_position;

static int token_line_position = 0;
static int token_line_number = 0;


int read_identifier() {
    int start = input_position;
    while (input_position < input.length() && (std::isalpha(input[input_position]) || input[input_position] == '_')) {
        input_position++;}
    //printf("start [%d], pos after read [%d]. ", start, input_position);
    //std::string word = input.substr(start, input_position - start);
    //std::cout << "word = " + word + "\n";
    return start;
}

int read_number_literal() {
    int start = input_position;
    while (input_position < input.length() && std::isdigit(input[input_position])) {
        input_position++;}
    return start;
}


std::vector<token> tokenizer(std::string input_str) {
    input = input_str;
    input_position = 0;
    while (input_position < input.length()) {

    switch (input[input_position]) {
        case ' ':
            input_position++;
            break;
        case '\n':
            input_position++;
            break;
        case '\r':
            input_position++;
            break;
        case '(': {
            token tok = {OPEN_PAREN, "OPEN_PAREN"};
            tokens.push_back(tok);
            input_position++;
        } break;
        case ')': {
            token tok = {CLOSE_PAREN, "CLOSE_PAREN"};
            tokens.push_back(tok);
            input_position++;
        } break;
        case ';': {
            token tok = {SEMICOLON, "SEMICOLON"};
            tokens.push_back(tok);
            input_position++;
        } break;
        case ',': {
            token tok = {COMMA, "COMMA"};
            tokens.push_back(tok);
            input_position++;
        } break;
        case '*': {
            token tok = {ASTERISK, "ASTERISK"};
            tokens.push_back(tok);
            input_position++;
        } break;
        default: {
            if (std::isalpha(input[input_position])) {
                int start = read_identifier();
                std::string word = input.substr(start, input_position - start);
                if (word == "CREATE") {
                    token tok = {CREATE, "CREATE"};
                    tokens.push_back(tok);
                    continue;
                } else if (word == "TABLE") {
                    token tok = {TABLE, "TABLE"};
                    tokens.push_back(tok);
                    continue;
                } else if (word == "SELECT") {
                    token tok = {SELECT, "SELECT"};
                    tokens.push_back(tok);
                    continue;
                } else if (word == "FROM") {
                    token tok = {FROM, "FROM"};
                    tokens.push_back(tok);
                    continue;
                } else if (word == "INSERT") {
                    token tok = {INSERT, "INSERT"};
                    tokens.push_back(tok);
                    continue;
                } else if (word == "INTO") {
                    token tok = {INTO, "INTO"};
                    tokens.push_back(tok);
                    continue;
                } else if (word == "VALUES") {
                    token tok = {VALUES, "VALUES"};
                    tokens.push_back(tok);
                    continue;
                }
                token tok = {STRING_LITERAL, "EMPTY_STRING_LITERAL"};
                tok.data = word;
                tokens.push_back(tok);
            } else if (std::isdigit(input[input_position])) {
                int start = read_number_literal();
                if (start == -1) {
                    errors.push_back("Illegal integer literal");
                    std::vector<token> garbage;
                    return garbage;
                }
                token tok = {INTEGER_LITERAL, "EMPTY_INTEGER_LITERAL"};
                std::string substring = input.substr(start, input_position - start);
                tok.data = substring;
                tokens.push_back(tok);
            } else {
                token tok = {ILLEGAL, "ILLEGAL"};
                tokens.push_back(tok);
                errors.push_back("Unknown illegal token (" + std::string(1, input[input_position]) + ")");
                input_position++;
            }
        }
    }
    }
    return tokens;
}