
#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QGridLayout>

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <sstream>
#include <cstring>
#include <cctype>

#include "structs.h"

std::vector<table> tables;


void parse();
void parse_create();
void parse_create_table();
std::string parse_data_type();

std::vector<std::string> errors;

std::vector<std::string> function_keywords;
std::vector<std::function<void()>> keyword_parse_functions;

std::vector<token> tokens;
int token_position = 0;

std::string input;
int input_position;

#define push_error_return(x)                \
        std::string error = x;              \
        errors.push_back(error);            \
        return                              \

#define push_error_return_empty_string(x)   \
        std::string error = x;              \
        errors.push_back(error);            \
        return ""                           \

//#define enough_token_check(X)



// CURRENTLY WORKING ON:
// CREATE TABLE
// SELECT
// INSERT

// underscores are allowed in words
int read_identifier() {
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

void tokenizer() {
    while (input_position < input.length()) {

    switch (input[input_position]) {
        case ' ':
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
                int start = read_number();
                if (start == -1) {
                    push_error_return("Illegal integer literal");}
                token tok = {INTEGER_LITERAL, "EMPTY_INTEGER_LITERAL"};
                std::string substring = input.substr(start, input_position - start);
                tok.data = substring;
                tokens.push_back(tok);
            } else {
                token tok = {ILLEGAL, "ILLEGAL"};
                tokens.push_back(tok);
                errors.push_back("Illegal something token");
                input_position++;
            }
        }
    }
    }
}


// find space, see if its a keyword, if it is use its parse function else error
void parse() {
    if (tokens.size() <= 0) {
        push_error_return("No tokens");}

    auto it = std::find(function_keywords.begin(), function_keywords.end(), tokens[token_position].data);
    if (it == function_keywords.end()) {
        std::string error = "Unknown keyword or inappropriate usage (" + tokens[token_position].data +  ") Token = " + std::to_string(token_position);
        errors.push_back(error);
        return;
    }

    int keyword_index = std::distance(function_keywords.begin(), it);

    keyword_parse_functions[keyword_index]();

    if (!errors.empty()) {
        return;}
}

keyword_enum peek() {
    if (token_position >= tokens.size()) {
        return LINE_END;
    } else {
        return tokens[token_position].keyword;}
}

std::string peek_data() {
    if (token_position >= tokens.size()) {
        return "";
    } else {
        return tokens[token_position].data;}
}

keyword_enum peek_ahead() {
    if (token_position + 1 >= tokens.size()) {
        return LINE_END;
    } else {
        return tokens[token_position + 1].keyword;}
}

void print_column(column col) {

    if (col.field_name.length() > 16) {
        std::cout << "nah field name is too long to print";
        exit(1);}

    std::vector<std::string> elements;
    std::stringstream element_stream(col.data_type);

    std::string element;    
    while (element_stream >> element) {
        elements.push_back(element);}

    if (elements.empty()) {
        push_error_return("print_column() Empty column data type");}

    std::string primary = elements[0];
    if (primary == "CHAR") {
        if (elements.size() == 1) {
            push_error_return("print_column() CHAR data type called to print but no size");}
        
        if (col.data.length() > 16) {
            std::string truncated = col.data.substr(0, 16);
            std::cout << truncated << " | ";
        } else {
            int padding_count = 16 - col.data.length();
            std::string padding = "";
            for (int i = 0; i < padding_count; i++) {
                padding += " ";}
            std::cout << padding <<  col.data << " | ";
        }
        
    } else if (primary == "INT") {
        if (elements.size() == 1) {
            push_error_return("print_column() INT data type called to print but no display size");}
        
        int padding_count = col.field_name.length();
        std::string padding = "";
        for (int i = 0; i < padding_count; i++) {
            padding += " ";}
        std:: cout << padding << col.data << " | "; 
    } else {
        push_error_return("Unsupported print data type");}
}

// sussy using macro in macro, might have to unmacro
#define advance_and_check(x)                \
    token_position++;                       \
    if (token_position >= tokens.size()) {  \
        push_error_return(x);}              \

void parse_insert() {
    if (peek() != INSERT) {
        std::cout << "parse_insert() called with non-insert token";
        exit(1);}

    advance_and_check("No tokens after INSERT");

    if (peek() == INTO) {
        advance_and_check("No tokens after INSERT INTO");

        if (peek() != STRING_LITERAL) {
            push_error_return("INSERT INTO bad token type for table name");}

        std::string table_name = peek_data();

        table tab;
        bool found = false;
        for (int i = 0; i < tables.size(); i++) {
            if (tables[i].name == table_name) {
                tab = tables[i];
                found = true;
                break;}
        }

        if (!found) {
            push_error_return("SELECT table not found");}

        advance_and_check("No tokens after INSERT INTO table");

        if (peek() != OPEN_PAREN) {
            push_error_return("No open parenthesis after INSET INTO table");}

        advance_and_check("No tokens after INSERT INTO table (");

        if (peek() == CLOSE_PAREN) {
            push_error_return("INSERT INTO table must contain at least 1 field name");}

        // Find field names
        std::vector<std::string> field_names;
        int count = 1;
        if (peek() != STRING_LITERAL) {
            errors.push_back("INSERT INTO table, name must be a string");}
        field_names.push_back(peek_data());

        advance_and_check("Missing closing parenthesis");

        while (peek() != CLOSE_PAREN && peek() != LINE_END) {
            if (peek() != COMMA) {
                errors.push_back("Comma must seperate field names");
                break;}
            
            advance_and_check("Missing field name after comma");
            
            if (peek() != STRING_LITERAL) {
                errors.push_back("Field names must be strings");}
            
            field_names.push_back(peek_data());
            
            advance_and_check("Field names missing closing parenthesis");

            if (count++ == 32) {
                errors.push_back("Tables cannot have more than 32 columns");
                break;}
        }

        if (!errors.empty()) {
            return;}

        if (peek() != CLOSE_PAREN) {
            errors.push_back("Field names missing closing parenthesis");}

        advance_and_check("INSERT INTO table, missing VALUES");

        if (peek() != VALUES) {
            errors.push_back("INSERT INTO table, missing VALUES");}

        advance_and_check("INSERT INTO table, missing open parenthesis after VALUES");

        if (peek() != OPEN_PAREN) {
            errors.push_back("INSERT INTO table, unknown token after VALUES, should be open parenthesis");}

        advance_and_check("INSERT INTO table, missing values after open parenthesis");

        // Find values
        std::vector<std::string> values;
        count = 1;
        if (peek() != STRING_LITERAL && peek() != INTEGER_LITERAL) {
            errors.push_back("VALUES must be string or integer, token was " + std::to_string(peek()));}
        values.push_back(peek_data());

        advance_and_check("Missing closing parenthesis");

        while (peek() != CLOSE_PAREN && peek() != LINE_END) {
            if (peek() != COMMA) {
                errors.push_back("Comma must seperate VALUES");
                break;}
            
            advance_and_check("Missing VALUE after comma");
            
            if (peek() != STRING_LITERAL && peek() != INTEGER_LITERAL) {
                errors.push_back("VALUES must be string or integer, token was " + std::to_string(peek()));}
            
            values.push_back(peek_data());
            
            advance_and_check("VALUES missing closing parenthesis");

            if (count++ == 32) {
                errors.push_back("VALUE, tables cannot have more than 32 columns");
                break;}
        }

        if (!errors.empty()) {
            return;}

        if (peek() != CLOSE_PAREN) {
            errors.push_back("VALUES missing closing parenthesis");}

        advance_and_check("VALUES missing closing semicolon");

        if (peek() != SEMICOLON) {
            errors.push_back("VALUES missing closing semicolon");}

        if (!errors.empty()) {
            return;}

        if (field_names.size() != values.size()) {
            push_error_return("INSET INTO, field names and VALUES have non-equal size");}

        if (field_names.size() > tab.columns.size()) {
            push_error_return("INSERT INTO, more field names than table has columns");}

        row roh;
        // Table's colums should already contain default values because I said so and it shall be so else die
        for (int i = 0; i < tab.columns.size(); i++) {
            roh.columns.push_back(tab.columns[i]);
        }

        struct field_name_column_pair {
            std::string field_name;
            column* col_ptr;
        };

        std::vector<field_name_column_pair> pairs;

        for (int i = 0; i < field_names.size(); i++) {
            bool found = false;
            for (int j = 0; j < roh.columns.size(); j++) {
                if (field_names[i] == tab.columns[j].field_name) {
                    field_name_column_pair pair = {field_names[i], &tab.columns[j]};
                    pairs.push_back(pair);
                    found = true;
                    break;}
            }
            if (!found) {
                push_error_return("INSERT INTO, field name not found");}
        }

        if (pairs.size() != field_names.size()) {
            std::cout << "Pairs wrong size\n";
            exit(1);}

        // Add new values to row
        for (int i = 0; i < pairs.size(); i++) {
            if (std::isdigit(pairs[i].col_ptr->field_name[0])) {
                if (!std::isdigit(pairs[i].field_name[0])) {
                    push_error_return("INSERT INTO, trying to insert non-integer into integer type column");}
            } else if (std::isalpha(pairs[i].col_ptr->field_name[0])) {
                if (!std::isalpha(pairs[i].field_name[0])) {
                    push_error_return("INSERT INTO, trying to insert non-string into string type column");}
            }
            
            pairs[i].col_ptr->data = pairs[i].field_name;
        }

        // Add row to table
        tab.rows.push_back(roh);


    } else {
        push_error_return("Unknown INSERT usage");}
}

void parse_select() {
    if (peek() != SELECT) {
        std::cout << "parse_select() called with non-select token";
        exit(1);}
    advance_and_check("No tokens after SELECT")

    if (peek() == ASTERISK) {
        advance_and_check("No tokens after SELECT *");
        
        if (peek() == FROM){
            advance_and_check("No tokens after SELECT * FROM");

            if (peek() != STRING_LITERAL) {
                push_error_return("SELECT bad token type for table name");}

            std::string table_name = peek_data();

            table tab;
            bool found = false;
            for (int i = 0; i < tables.size(); i++) {
                if (tables[i].name == table_name) {
                    tab = tables[i];
                    found = true;
                    break;}
            }
            
            if (!found) {
                push_error_return("SELECT table not found");}

            std::cout << table_name << ":\n";

            // Print columns
            for (int i = 0; i < tab.columns.size(); i++) {
                std::cout << tab.columns[i].field_name << " | ";}
            std::cout << std::endl;
            for (int i = 0; i < tab.columns.size(); i++) {
                print_column(tab.columns[i]);}
            std::cout << std::endl;

        } else {
            push_error_return("Unknown SELECT usage");}
    } else {
        push_error_return("Unknown SELECT usage");}
}

void parse_create() {
    if (tokens[token_position].keyword != CREATE) {
        std::cout << "parse_create() called with non-create token";
        exit(1);}
    token_position++;

    if (token_position >= tokens.size()) {
        push_error_return("No tokens after CREATE");}

    if (tokens[token_position].keyword == TABLE) {
        parse_create_table();
    } else {
        push_error_return("Unknown keyword (" + tokens[token_position].data + ") after CREATE");}

}

// tokens should point to TABLE
void parse_create_table() {
    if (tokens[token_position].keyword != TABLE) {
        std::cout << "parse_create_table() called with non-create_table token";
        exit(1);}
        
    advance_and_check("No tokens after CREATE TABLE");

    if (peek() != STRING_LITERAL) {
        push_error_return("Non-string_literal token for table name");}

    table tab;
    tab.name = tokens[token_position].data;

    advance_and_check("No open paren '(' after CREATE TABLE");

    if (peek() != OPEN_PAREN) {
        push_error_return("No open paren '(' after CREATE TABLE");}
    
    advance_and_check("No data in CREATE TABLE");

    while (true) {
        column col;

        if (token_position >= tokens.size()) {
            push_error_return("Out of tokens during CREATE TABLE, field name");}

        if (peek() != STRING_LITERAL) {
            push_error_return("Non-string_literal token for field name");}

        col.field_name = peek_data();

        advance_and_check("No comma after CREATE TABLE field_name");

        advance_and_check("No data type after CREATE TABLE comma");

        std::string data_type = parse_data_type();

        if (!errors.empty()) {
            return;}

        col.data_type = data_type;

        tab.columns.push_back(col);

        if (tokens[token_position].keyword == CLOSE_PAREN && tokens[token_position + 1].keyword == SEMICOLON) {
            token_position += 2;
            break;
        }
    }

    if (!errors.empty()) {
        push_error_return("Unknown error during CREATE TABLE");}

    tables.push_back(tab);
}

// Just returns data type for testing
void parse_data() {
    token_position++;
    std::cout << parse_data_type() << std::endl;
}

// sussy using macro in macro, might have to unmacro
#define advance_and_check_ret_str(x)        \
    token_position++;                       \
    if (token_position >= tokens.size()) {  \
        push_error_return_empty_string(x);} \

std::string parse_data_type() {
    bool unsign = false;
    bool zerofill = false;

    if (token_position >= tokens.size()) {
        push_error_return_empty_string("No data type token");}  
    
    if (tokens[token_position].data == "UNSIGNED") {
        unsign = true;
        advance_and_check_ret_str("No data type after UNSIGNED");
    } else if (peek_data() == "ZEROFILL") {
        unsign = true;
        zerofill = true;
        advance_and_check_ret_str("No data type after ZEROFILL");
    }

    if (peek() != STRING_LITERAL) {
        std::string msg = "Data type has bad token type (" + std::to_string(peek()) + ")\n";
        push_error_return_empty_string(msg);}

    std::string type = tokens[token_position].data;
    token_position++;

    // Basic string
    if (type == "TINYBLOB") {
        return "CHAR 255";
    } else if (type == "TINYTEXT") {
        return "CHAR 255";
    } else if (type == "MEDIUMTEXT") {
        return "CHAR 16777215";
    } else if (type == "MEDIUMBLOB") {
        return "CHAR 16777215";
    } else if (type == "LONGTEXT") {
        return "CHAR 4294967295";
    } else if (type == "LONGBLOB") {
        return "CHAR 4294967295";
    }
    
    // Basic numeric
    if (type == "BOOL") {
        return "BOOL";
    } else if (type == "BOOLEAN") {
        return "BOOL";
    }

    // Basic time
    if (type == "DATE") {
        return "DATE";
    } else if (type == "YEAR") {
        return "YEAR";
    }

    // String
    if (type == "SET") {
        if (peek() == OPEN_PAREN) {
            token_position++;
            if (peek() == CLOSE_PAREN) {
                token_position++;
                return "SET";}

            std::vector<std::string> elements;
            int count = 1;
            if (peek() != STRING_LITERAL) {
                errors.push_back("Can only add strings to SET");}
            elements.push_back(tokens[token_position].data);
            
            advance_and_check_ret_str("Missing closing parenthesis");

            while (peek() != CLOSE_PAREN && peek() != LINE_END) {
                if (peek() != COMMA) {
                    errors.push_back("Comma must seperate SET elements");
                    break;}
                
                advance_and_check_ret_str("SET missing element after comma");
                
                if (peek() != STRING_LITERAL) {
                    errors.push_back("Can only add strings to SET");}
                
                elements.push_back(peek_data());
                
                advance_and_check_ret_str("SET missing closing parenthesis");

                if (count++ == 64) {
                    errors.push_back("SET cannot have more than 64 elements");
                    break;}
            }

            if (peek() != CLOSE_PAREN && errors.empty()) {
                errors.push_back("SET missing closing parenthesis");}
            
            std::string return_string = "SET";
            for (int i = 0; i < elements.size(); i++) {
                return_string += " ";
                return_string += elements[i];
            }
            return return_string;
        } else {
            return "SET";}
    }

    // Numeric
    if (type == "BIT") {
        if (peek() == OPEN_PAREN) {
            token_position++;
            if (peek() == CLOSE_PAREN) {
                token_position++;
                return "BIT 1";}

            int num = 1;
            if (peek() != INTEGER_LITERAL) {
                if (peek() != LINE_END) {
                    errors.push_back("BIT contained non-integer");}
            } else {
                if (tokens[token_position].data.length() > 2) {
                    errors.push_back("BIT cannot be greated than 64");
                } else {
                    num = std::stoi(tokens[token_position].data);
                    if (num < 1) {
                        errors.push_back("BIT cannot be less than 1");}
                    if (num > 64) {
                        errors.push_back("BIT cannot be greated than 64");}
                }
            }
            token_position++;

            if (peek() != CLOSE_PAREN) {
                errors.push_back("BIT missing clossing parenthesis");}
            token_position++;
            
            return "BIT " + std::to_string(num);
        } else {
            return "BIT 1";}
        
    } else if (type == "INT") {
        // The number is the display size, can ignore in computations for now. I read the default is 11 for signed and 10 for unsigned but I'm not sure it's standard so...
       if (peek() == OPEN_PAREN) {
            token_position++;
            if (peek() == CLOSE_PAREN) {
                token_position++;
                if (unsign) {
                    return "INT 10";}
                return "INT 11";}

            int num = 11;
            if (peek() != INTEGER_LITERAL) {
                errors.push_back("INT contained non-integer");
            } else {
                if (tokens[token_position].data.length() > 3) {
                    errors.push_back("INT cannot be greated than 255");
                } else {
                    num = std::stoi(tokens[token_position].data);
                    if (num < 1) {
                        errors.push_back("INT cannot be less than 1");}
                    if (num > 255) {
                        errors.push_back("INT cannot be greated than 255");}
                }
            }
            token_position++;

            if (peek() != CLOSE_PAREN) {
                errors.push_back("INT missing clossing parenthesis");}
            token_position++;
            
            if (!errors.empty()) {
                if (unsign) {
                    return "INT 10";}
                return "INT 11";} // Not sure if to return default or ERROR DATA TYPE
            return "INT " + std::to_string(num);
        } else {
            if (unsign) {
                return "INT 10";}
            return "INT 11";}

    } else if (type == "DEC") {
        push_error_return_empty_string("DEC not support to complicated");}

    return "idk what type that is man";
}

int main (int argc, char* argv[]) {

    // init function vector
    function_keywords.push_back("CREATE");
    keyword_parse_functions.push_back(parse_create);

    function_keywords.push_back("DATA");
    keyword_parse_functions.push_back(parse_data);

    function_keywords.push_back("SELECT");
    keyword_parse_functions.push_back(parse_select);

    function_keywords.push_back("INSERT");
    keyword_parse_functions.push_back(parse_insert);

    //qt
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("Squel");

    QVBoxLayout* layout = new QVBoxLayout(&window);

    QLabel* label_enter_command = new QLabel("ENTER COMMAND:");
    QLineEdit* line_edit_input = new QLineEdit();

    QPushButton *submit_button = new QPushButton("Submit");

    QLabel* label_command_results = new QLabel();

    layout->addWidget(label_enter_command);
    layout->addWidget(line_edit_input);
    layout->addWidget(submit_button);
    layout->addWidget(label_command_results);

    // Connecting button click to capture input from QLineEdit
    QObject::connect(submit_button, &QPushButton::clicked, [&]() {
        QString input = line_edit_input->text();
        label_command_results->setText(input);
    });

    window.show();

    // Get user input forever
    while (true) {
        //std::cout << "> ";
        //std::getline(std::cin, input);
        window.show();
        if (input.length() > 600) {
            std::cout << "INPUT TOO LONG >:(";
            continue;
        }
        if (input.length() == 0) {
            continue;}
        tokenizer();
        for (int i = 0; i < tokens.size(); i++) {
            std::cout << "Keyword: " << std::to_string(tokens[i].keyword) << ", Data: " + tokens[i].data << std::endl;
        }
        
        parse();
        if (errors.size() != 0) {
            for (int i = 0; i < errors.size(); i++) {
                std::cout << "ERROR: " + errors[i] + "\n";
            }
        }
        errors.clear();
        tokens.clear();
        token_position = 0;
        input_position = 0;
    }
    return app.exec();
}