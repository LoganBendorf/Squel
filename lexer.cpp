
#include "structs_and_macros.h"
#include <vector>

extern std::vector<std::string> errors;

static std::string input;
static int input_position;

static int line_count = 1;
static int line_position_count = 0;


int read_string() {
    int start = input_position;
    while (input_position < input.length() && (std::isalpha(input[input_position]) || input[input_position] == '_')) {
        input_position++;}
    //printf("start [%d], pos after read [%d]. ", start, input_position);
    //std::string word = input.substr(start, input_position - start);
    //std::cout << "word = " + word + "\n";
    return start;
}

int read_number() {
    int start = input_position;
    while (input_position < input.length() && std::isdigit(input[input_position])) {
        input_position++;}
    return start;
}

token create_token(keyword_enum keyword, std::string data, int line, int line_position) {
    return token{keyword, data, line, line_position};
}

// Needs testing
token parse_quoted_string(char type) {
    // start only used for token creation
    int start = input_position;
    int start_line = line_count;

    std::string out;
    while (input_position++ < input.length() && (line_position_count++)) {
        if (input[input_position] == '\n') {
            line_count++;
            line_position_count = 0;
        }

        if (input[input_position] == '\\') {
            if (!(input_position + 1 < input.length())) {
                errors.push_back("Lexer: \\ raw backslash has no partner (i.e. the n in \\n is missing) ");
                return token{};
            }
            input_position++;
            line_position_count++;
            if (input[input_position] == '\\') {
                out.push_back('\\');
            } else if (input[input_position] == 'n') {
                out.push_back('\n');
            }
            continue;
        }

        if (input[input_position] == type) {
            if (input_position + 1 < input.length() && input[input_position + 1] == type) {
                // Quote is escaped, add it then skip over the next
                out.push_back(input[input_position]);
                input_position++;
                line_position_count++;
                continue;
            } else {
                break;
            }
        }
        out.push_back(input[input_position]);
    }
    if (input_position >= input.length()) {
        errors.push_back("Lexer: Couldn't find end of quotes");
        return token{};
    }
    return create_token(STRING_LITERAL, out, start_line, start);
}

std::vector<token> lexer(std::string input_str) {
    std::vector<token> tokens;
    input = input_str;
    input_position = 0;
    line_count = 1;
    line_position_count = 0;
    while (input_position < input.length()) {

    switch (input[input_position]) {
        case '\'': {
            token tok = parse_quoted_string('\'');
            if (!errors.empty()) {
                std::vector<token> garbage;
                return garbage;
            }
            tokens.push_back(tok);
            input_position++;
            line_position_count++;
        } break;
        case '\"': {
            token tok = parse_quoted_string('\"');
            if (!errors.empty()) {
                std::vector<token> garbage;
                return garbage;
            }
            tokens.push_back(tok);
            input_position++;
            line_position_count++;
        } break;
        case ' ':
            input_position++;
            line_position_count++;
            break;
        case '\n':
            input_position++;
            line_count++;
            line_position_count = 0;
            break;
        case '\r':
            input_position++;
            line_position_count++;
            line_position_count = 0;
            break;
        case '(': {
            token tok = create_token(OPEN_PAREN, "(", line_count, line_position_count);
            tokens.push_back(tok);
            input_position++;
            line_position_count++;
        } break;
        case ')': {
            token tok = create_token(CLOSE_PAREN, ")", line_count, line_position_count);
            tokens.push_back(tok);
            input_position++;
            line_position_count++;
        } break;
        case ';': {
            token tok = create_token(SEMICOLON, ";", line_count, line_position_count);
            tokens.push_back(tok);
            input_position++;
            line_position_count++;
        } break;
        case ',': {
            token tok = create_token(COMMA, ",", line_count, line_position_count);
            tokens.push_back(tok);
            input_position++;
            line_position_count++;
        } break;
        case '*': {
            token tok = create_token(ASTERISK, "*", line_count, line_position_count);
            tokens.push_back(tok);
            input_position++;
            line_position_count++;
        } break;
        default: {
            if (std::isalpha(input[input_position])) {
                int start = read_string();
                std::string word = input.substr(start, input_position - start);
                if (word == "CREATE") {
                    token tok = create_token(CREATE, "CREATE", line_count, line_position_count);
                    tokens.push_back(tok);
                    line_position_count += word.size();
                    continue;
                } else if (word == "TABLE") {
                    token tok = create_token(TABLE, "TABLE", line_count, line_position_count);
                    tokens.push_back(tok);
                    line_position_count += word.size();
                    continue;
                } else if (word == "SELECT") {
                    token tok = create_token(SELECT, "SELECT", line_count, line_position_count);
                    tokens.push_back(tok);
                    line_position_count += word.size();
                    continue;
                } else if (word == "FROM") {
                    token tok = create_token(FROM, "FROM", line_count, line_position_count);
                    tokens.push_back(tok);
                    line_position_count += word.size();
                    continue;
                } else if (word == "INSERT") {
                    token tok = create_token(INSERT, "INSERT", line_count, line_position_count);
                    tokens.push_back(tok);
                    line_position_count += word.size();
                    continue;
                } else if (word == "INTO") {
                    token tok = create_token(INTO, "INTO", line_count, line_position_count);
                    tokens.push_back(tok);
                    line_position_count += word.size();
                    continue;
                } else if (word == "VALUES") {
                    token tok = create_token(VALUES, "VALUES", line_count, line_position_count);
                    tokens.push_back(tok);
                    line_position_count += word.size();
                    continue;
                } else if (word == "true") {
                    token tok = create_token(BOOL, "true", line_count, line_position_count);
                    tokens.push_back(tok);
                    line_position_count += word.size();
                    continue;
                } else if (word == "false") {
                    token tok = create_token(BOOL, "false", line_count, line_position_count);
                    tokens.push_back(tok);
                    line_position_count += word.size();
                    continue;
                }
                token tok = create_token(STRING_LITERAL, "EMPTY_STRING_LITERAL?!?", line_count, line_position_count);
                tok.data = word;
                tokens.push_back(tok);
                line_position_count += word.size();
            } else if (std::isdigit(input[input_position])) {
                int start = read_number();
                if (start == -1) {
                    errors.push_back("Illegal integer literal. Line = " + std::to_string(line_count) + ", position = " + std::to_string(line_position_count));
                    std::vector<token> garbage;
                    return garbage;
                }
                token tok = create_token(INTEGER_LITERAL, "EMPTY_INTEGER_LITERAL?!?", line_count, line_position_count);
                std::string substring = input.substr(start, input_position - start);
                tok.data = substring;
                tokens.push_back(tok);
                line_position_count += substring.size();
            } else {
                token tok = create_token(ILLEGAL, std::string(1, input[input_position]), line_count, line_position_count);
                tokens.push_back(tok);
                errors.push_back("Unknown illegal token (" + std::string(1, input[input_position]) + ")");
                input_position++;
                line_position_count++;
            }
        }
    }
    }
    return tokens;
}