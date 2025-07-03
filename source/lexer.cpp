module;

extern std::vector<std::string> errors;

module lexer;

import helpers;
import token;



static std::string input;
static size_t input_position;

static size_t line_count = 1;
static size_t line_position_count = 0;


static constexpr bool is_alpha(char a) {
    return (std::isalpha(a) != 0);
}

static constexpr bool is_digit(char a) {
    return (std::isdigit(a) != 0);
}


static size_t read_string() {
    size_t start = input_position;
    while (input_position < input.length() && ( is_alpha(input[input_position]) || input[input_position] == '_' || is_digit(input[input_position] ))) {
        input_position++;
    }
    //printf("start [%d], pos after read [%d]. ", start, input_position);
    //std::string word = input.substr(start, input_position - start);
    //std::cout << "word = " + word + "\n";
    return start;
}

// Should be dumb, complicated numbers should be constructed in the PARSER (i.e decimals, negatives)
static size_t read_number() {
    bool num_was_read = false;
    size_t start = input_position;
    while (input_position < input.length() && is_digit(input[input_position])) {
        num_was_read = true;
        input_position++;
    }

    if (num_was_read) {
        return start;
    } else {
        return SIZE_T_MAX;
    }
}

static token create_token(token_type type, std::string data, size_t line, size_t line_position) {
    return token{type, data, line, line_position};
}


// Needs MORE testing
static token parse_quoted_string(char type) {
    // start only used for token creation
    size_t start = input_position;
    size_t start_line = line_count;

    std::string out;
    while (input_position++ < input.length() && (line_position_count++ != 0)) {
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

        case '$': {

            if (input_position + 1 >= input.length()) {
                errors.push_back("singular $");
                std::vector<token> garbage;
                return garbage;
            }
            
            if (input[input_position + 1] != '$') {
                errors.push_back("singular $");
                std::vector<token> garbage;
                return garbage;
            }

            token tok = create_token($$, "$$", line_count, line_position_count);
            tokens.push_back(tok);
            input_position += 2;
            line_position_count += 2;
        } break;

        case '<': {
            token tok = create_token(LESS_THAN, "<", line_count, line_position_count);
            tokens.push_back(tok);
            input_position++;
            line_position_count++;
        } break;
        case '>': {
            token tok = create_token(GREATER_THAN, ">", line_count, line_position_count);
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
        case '/': {
            token tok = create_token(SLASH, "/", line_count, line_position_count);
            tokens.push_back(tok);
            input_position++;
            line_position_count++;
        } break;
        case '+': {
            token tok = create_token(PLUS, "+", line_count, line_position_count);
            tokens.push_back(tok);
            input_position++;
            line_position_count++;
        } break;
        case '!': {
            if (input_position + 1 < input.length()  && input[input_position] == '=') {
                token tok = create_token(NOT_EQUAL, "!=", line_count, line_position_count);
                tokens.push_back(tok);
                input_position++;
                line_position_count++;
                break;
            }
            token tok = create_token(BANG, "!", line_count, line_position_count);
            tokens.push_back(tok);
            input_position++;
            line_position_count++;
        } break;
        case '=': {
            token tok = create_token(EQUAL, "=", line_count, line_position_count);
            tokens.push_back(tok);
            input_position++;
            line_position_count++;
        } break;
        case '.': {
            token tok = create_token(DOT, ".", line_count, line_position_count);
            tokens.push_back(tok);
            input_position++;
            line_position_count++;
        } break;
        case '-': {
            if (input_position + 1 < input.length()  && input[input_position + 1] == '-') {
                while (input_position < input.length() && input[input_position] != '\n') {
                    input_position++;
                }
                break;
            }
            token tok = create_token(MINUS, "-", line_count, line_position_count);
            tokens.push_back(tok);
            input_position++;
            line_position_count++;
        } break;
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
        case '\t':
            input_position++;
            line_position_count += 4;
            break;
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
            line_count++;
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
        default: { // Can put keywords in hashmap
            if (is_alpha(input[input_position])) {
                size_t start = read_string();
                std::string word = input.substr(start, input_position - start);

                bool is_keyword = false;
                for (size_t i = 0; i < token_type_span().size(); i++) {
                    if (word == token_type_span()[i]) {
                        token tok = create_token(static_cast<token_type>(i), word, line_count, line_position_count);
                        tokens.push_back(tok);
                        line_position_count += word.size();
                        is_keyword = true;
                        break;
                    }
                }
                
                if (is_keyword) {
                    continue; }

                token tok = create_token(STRING_LITERAL, "EMPTY_STRING_LITERAL?!?", line_count, line_position_count);
                tok.data = word;
                tokens.push_back(tok);
                line_position_count += word.size();
            } else if (is_digit(input[input_position])) {
                size_t start = read_number();
                if (start == SIZE_T_MAX) {
                    errors.push_back("Invalid number. Line = " + std::to_string(line_count) + ", position = " + std::to_string(line_position_count));
                    std::vector<token> garbage;
                    return garbage;
                }
                std::string substring = input.substr(start, input_position - start);
                token tok = create_token(INTEGER_LITERAL, substring, line_count, line_position_count);
                tokens.push_back(tok);
                line_position_count += substring.size();
            } else {
                token tok = create_token(ILLEGAL, std::string(1, input[input_position]), line_count, line_position_count);
                tokens.push_back(tok);
                std::string err = "Unknown illegal token ("; // Split up cause stupid warning
                err += std::string(1, input[input_position]);
                errors.push_back(err + ")");
                input_position++;
                line_position_count++;
            }
        }
    }
    }
    return tokens;
}