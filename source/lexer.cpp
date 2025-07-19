

#include "pch.h"

#include "lexer.h"

#include "token.h"
#include "helpers.h"
#include "logger.h"

extern logger<error_msg> sql_errors;

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

// '|' character is reserved for serialization only >:( 


static size_t read_string() {
    size_t start = input_position;
    while (input_position < input.length() && ( is_alpha(input[input_position]) || input[input_position] == '_' || is_digit(input[input_position] ))) {
        if (input[input_position] == '|') {
            sql_errors.add_msg("Failed to read string literal, contained '|'", CUR_LOC); 
            return 0;
        }
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

// Tiny little "" are allowed in here then checked for 0 length later
static token parse_quoted_string(char type) {
    // start only used for token creation
    size_t start = input_position;
    size_t start_line = line_count;

    std::stringstream out;
    while (input_position++ < input.length()) {
        line_position_count++;

        if (input_position == input.length() - 1 && input[input_position] != type) {
            sql_errors.add_msg("Lexer: Couldn't find end of quotes. In (" + input.substr(start, input_position - start - 1) + ")", CUR_LOC);
            return token{};
        }

        if (input[input_position] == '|') {
            sql_errors.add_msg("Failed to read string literal, contained '|'", CUR_LOC); 
            return token{};
        }

        if (input[input_position] == '\n') {
            line_count++;
            line_position_count = 0;
        }

        if (input[input_position] == '\\') {
            if (input_position + 1 >= input.length()) {
                sql_errors.add_msg("Lexer: \\ raw backslash has no partner (i.e. the n in \\n is missing)", CUR_LOC);
                return token{};
            }

            input_position++;
            line_position_count++;
            if (input[input_position] == '\\') {
                out << '\\';
            } else if (input[input_position] == 'n') {
                out << '\n';
            } else if (input[input_position] == type) {
                out << type;
            }

            continue;
        }

        if (input[input_position] == type) {
            // if (input_position + 1 < input.length() && input[input_position + 1] == type) {
            //     // Quote is escaped, add it then skip over the next
            //     out << input[input_position];
            //     input_position++;
            //     line_position_count++;
            //     continue;
            // } else {
            //     break;
            // }
            break;
        }

        out << input[input_position];
    }

    input_position++;
    line_position_count++;

    // Concat contiguous quoted strings
    while (input_position < input.length() && (input[input_position] == '\'' || input[input_position] == '\"')) {
        token str_tok = parse_quoted_string(input[input_position]);
        out << str_tok.data;
    }

    return create_token(STRING_LITERAL, out.str(), start_line, start);
}

std::vector<token> lexer(const std::string& input_str) {
    std::vector<token> tokens;
    input = input_str;
    input_position = 0;
    line_count = 1;
    line_position_count = 0;
    while (input_position < input.length()) {

    switch (input[input_position]) {

        case '|' :
            sql_errors.add_msg("Input contained '|'", CUR_LOC); 
            return {};
        // Ignore \n
        case '\\': {
            if (input_position + 1 >= input.length()) {
                token tok = create_token(ILLEGAL, std::string(1, input[input_position]), line_count, line_position_count);
                tokens.push_back(tok);
                std::stringstream err;
                err << "Unknown illegal token (" << input[input_position] << ")";
                sql_errors.add_msg(err.str(), CUR_LOC);
                input_position++;
                break;
            }

            input_position++;

            if (input[input_position] != 'n') {
                token tok = create_token(ILLEGAL, std::string(1, input[input_position]), line_count, line_position_count);
                tokens.push_back(tok);
                sql_errors.add_msg("Unknown usage of (\\)", CUR_LOC);
                input_position++;
                break;
            }

            input_position++;
        } break;

        case '$': {

            if (input_position + 1 >= input.length()) {
                sql_errors.add_msg("singular $", CUR_LOC);
                return {};
            }
            
            if (input[input_position + 1] != '$') {
                sql_errors.add_msg("singular $", CUR_LOC);
                return {};
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
                line_position_count++; // FIXME Shouldn't this be += 2???
                break;
            }
            token tok = create_token(BANG, "!", line_count, line_position_count);
            tokens.push_back(tok);
            input_position++;
            line_position_count++;
        } break;
        case '=': {
            if (input_position + 1 < input.length()  && input[input_position] == '=') {
                token tok = create_token(EQUAL, "==", line_count, line_position_count);
                tokens.push_back(tok);
                input_position += 2; 
                line_position_count += 2;
                break;
            }
            token tok = create_token(ASSIGNMENT, "=", line_count, line_position_count);
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
            if (sql_errors.has_msgs()) {
                return {}; }

            tokens.push_back(tok);
            // input_position++;
            // line_position_count++;
        } break;
        case '\"': {
            token tok = parse_quoted_string('\"');
            if (sql_errors.has_msgs()) {
                return {}; }

            tokens.push_back(tok);
            // input_position++;
            // line_position_count++;
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
                if (sql_errors.has_msgs()) {
                    return {}; }

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

                token tok = create_token(STRING_LITERAL, "!ERR_STR_LIT!", line_count, line_position_count);
                tok.data = word;
                tokens.push_back(tok);
                line_position_count += word.size();
            } else if (is_digit(input[input_position])) {
                size_t start = read_number();
                if (start == SIZE_T_MAX) {
                    sql_errors.add_msg("Invalid number. Line = " + std::to_string(line_count) + ", position = " + std::to_string(line_position_count), CUR_LOC);
                    return {};
                }

                std::string substring = input.substr(start, input_position - start);
                token tok = create_token(INTEGER_LITERAL, substring, line_count, line_position_count);
                tokens.push_back(tok);
                line_position_count += substring.size();
            } else {
                token tok = create_token(ILLEGAL, std::string(1, input[input_position]), line_count, line_position_count);
                tokens.push_back(tok);
                std::stringstream err;
                err << "Unknown illegal token (" << input[input_position] << ")";
                sql_errors.add_msg(err.str(), CUR_LOC);
                input_position++;
                line_position_count++;
            }
        }
    }
    }
    return tokens;
}