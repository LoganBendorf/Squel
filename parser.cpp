
#include "parser.h"
#include "node.h"


std::vector<keyword_enum> function_keywords;
std::vector<std::function<void()>> keyword_parse_functions;

extern std::vector<std::string> errors;
extern std::vector<table> tables;
extern display_table display_tab;

static std::vector<node*> nodes;

static std::vector<token> tokens;
static int token_position = 0;

static int loop_count = 0;

#define push_error_return(x)                    \
        token curTok;                           \
        if (token_position >= tokens.size()) {  \
            curTok.keyword = LINE_END; curTok.data = ""; curTok.line = -1; curTok.position = -1; \
        } else {                                \
            curTok = tokens[token_position];}   \
        std::string error = x;                  \
        error = error + ". Line = " + std::to_string(curTok.line) + ", position = " + std::to_string(curTok.position);\
        errors.push_back(error);                \
        return                                  \

#define push_error_return_empty_string(x)       \
        token curTok;                           \
        if (token_position >= tokens.size()) {  \
            curTok.keyword = LINE_END; curTok.data = ""; curTok.line = -1; curTok.position = -1; \
        } else {                                \
            curTok = tokens[token_position];}   \
        std::string error = x;                  \
        error = error + ". Line = " + std::to_string(curTok.line) + ", position = " + std::to_string(curTok.position);\
        errors.push_back(error);                \
        return ""    

#define push_error_return_empty_string(x)       \
        token curTok;                           \
        if (token_position >= tokens.size()) {  \
            curTok.keyword = LINE_END; curTok.data = ""; curTok.line = -1; curTok.position = -1; \
        } else {                                \
            curTok = tokens[token_position];}   \
        std::string error = x;                  \
        error = error + ". Line = " + std::to_string(curTok.line) + ", position = " + std::to_string(curTok.position);\
        errors.push_back(error);                \
        return ""                               \


// sussy using macro in macro
#define advance_and_check(x)                \
    token_position++;                       \
    if (token_position >= tokens.size()) {  \
        push_error_return(x);}              \

// sussy using macro in macro
#define advance_and_check_ret_str(x)        \
    token_position++;                       \
    if (token_position >= tokens.size()) {  \
        push_error_return_empty_string(x);} \

// Here for now I guess
std::string keyword_enum_to_string[] =  
    {"CREATE", "TABLE", "SELECT", "FROM", "INSERT", "INTO", "VALUES", 
        "STRING_LITERAL", "INTEGER_LITERAL", "OPEN_PAREN", "CLOSE_PAREN",
         "SEMICOLON", "COMMA", "ASTERISK", "LINE_END", "ILLEGAL", "NEW_LINE", "DATA", "QUOTE", "BOOL"};


// Meat

void parser_init(std::vector<token> toks) {
    tokens = toks;
    token_position = 0;
    loop_count = 0;
    nodes.clear();

    // init function vector
    function_keywords.push_back(CREATE);
    keyword_parse_functions.push_back(parse_create);

    // function_keywords.push_back(DATA);
    // keyword_parse_functions.push_back(parse_data);

    function_keywords.push_back(SELECT);
    keyword_parse_functions.push_back(parse_select);

    function_keywords.push_back(INSERT);
    keyword_parse_functions.push_back(parse_insert);

}


// find space, see if its a keyword, if it is use its parse function else error
// underscores are allowed in words
std::vector<node*> parse() {
    if (tokens.size() == 0) {
        errors.push_back("parse(): No tokens");
        return nodes;}

    auto it = std::find(function_keywords.begin(), function_keywords.end(), tokens[token_position].keyword);
    if (it == function_keywords.end()) {
        token tok = tokens[token_position];
        std::string error = "Unknown keyword or inappropriate usage (" + peek_data() +  ") Token type = "
                          + std::to_string(tok.keyword) + ". Line = " + std::to_string(tok.line) 
                          + ". Word = " + std::to_string(tok.position);
        errors.push_back(error);
        return nodes;
    }

    int keyword_index = std::distance(function_keywords.begin(), it);

    keyword_parse_functions[keyword_index]();

    if (!errors.empty()) {
        return nodes;}

    if (loop_count++ > 100) {
        std::cout << "le stuck in loop?";}

    if (peek()!= LINE_END) {
        parse();}

    return nodes;
}

// doesnt work, should be inserting ROWS
void parse_insert() {
    if (peek() != INSERT) {
        std::cout << "parse_insert() called with non-insert token";
        exit(1);}

    advance_and_check("No tokens after INSERT");

    insert_into* info = new insert_into();

    if (peek() == INTO) {

        advance_and_check("No tokens after INSERT INTO");

        if (peek() != STRING_LITERAL) {
            push_error_return("INSERT INTO bad token type for table name");}

        info->table_name = peek_data();

        advance_and_check("No tokens after INSERT INTO table");

        if (peek() != OPEN_PAREN) {
            push_error_return("No open parenthesis after INSET INTO table");}

        advance_and_check("No tokens after INSERT INTO table (");

        if (peek() == CLOSE_PAREN) {
            push_error_return("INSERT INTO table must contain at least 1 field name");}

        // Find field names
        int count = 1;
        if (peek() != STRING_LITERAL) {
            errors.push_back("INSERT INTO table, name must be a string");}
        info->field_names.push_back(peek_data());

        advance_and_check("Missing closing parenthesis");

        while (peek() != CLOSE_PAREN && peek() != LINE_END) {
            if (peek() != COMMA) {
                errors.push_back("Comma must seperate field names");
                break;}
            
            advance_and_check("Missing field name after comma");
            
            if (peek() != STRING_LITERAL) {
                errors.push_back("Field names must be strings");}
            
            info->field_names.push_back(peek_data());
            
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
        count = 1;
        if (peek() != STRING_LITERAL && peek() != INTEGER_LITERAL) {
            errors.push_back("VALUES must be string or integer, token was " + std::to_string(peek()));}
        info->values.push_back(peek_data());

        advance_and_check("Missing closing parenthesis");

        while (peek() != CLOSE_PAREN && peek() != LINE_END) {
            if (peek() != COMMA) {
                errors.push_back("Comma must seperate VALUES");
                break;}
            
            advance_and_check("Missing VALUE after comma");
            
            if (peek() != STRING_LITERAL && peek() != INTEGER_LITERAL) {
                errors.push_back("VALUES must be string or integer, token was " + std::to_string(peek()));}
            
            info->values.push_back(peek_data());
            
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

        if (info->field_names.size() != info->values.size()) {
            push_error_return("INSET INTO, field names and VALUES have non-equal size");}

    } else {
        push_error_return("Unknown INSERT usage");}
    
    if (peek() == SEMICOLON) {
        token_position++;}

    nodes.push_back(info);
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

            // Advance past string literal
            advance_and_check("");
            // Advance past semicolon
            token_position++;

            select_from* info = new select_from();
            info->table_name = table_name;
            nodes.push_back(info);

        } else {
            push_error_return("Unknown SELECT usage");}
    } else {
        push_error_return("Unknown SELECT usage");}
}

void parse_create() {
    if (peek() != CREATE) {
        std::cout << "parse_create() called with non-create token";
        exit(1);}

    advance_and_check("No tokens after CREATE");

    if (peek() == TABLE) {
        parse_create_table();
    } else {
        push_error_return("Unknown keyword (" + tokens[token_position].data + ") after CREATE");}

}


// NEED TO ADD NEW LINE CHECKS WAH
// tokens should point to TABLE
void parse_create_table() {
    if (peek() != TABLE) {
        std::cout << "parse_create_table() called with non-create_table token";
        exit(1);}
        
    advance_and_check("No tokens after CREATE TABLE");

    if (peek() != STRING_LITERAL) {
        push_error_return("Non-string literal token for table name. Token data (" + peek_data() + ")");}

    create_table* info = new create_table();
    info->table_name = peek_data();

    advance_and_check("No open paren '(' after CREATE TABLE");

    if (peek() != OPEN_PAREN) {
        push_error_return("No open paren '(' after CREATE TABLE");}
    
    advance_and_check("No data in CREATE TABLE");

    while (true) {

        if (peek() != STRING_LITERAL) {
            push_error_return("Non-string literal token for field name. Token data (" + peek_data() + ")");}

        column_data col;
        col.field_name = peek_data();

        advance_and_check("No data type after CREATE TABLE field name");

        col.data_type = parse_data_type();
        if (!errors.empty()) {
            return;}

        // Need to check in gdb if it sshould advance first
        if (peek() != COMMA && peek() != CLOSE_PAREN) {
            col.default_value = parse_default_value(col.data_type);
            if (!errors.empty()) {
                return;}
            advance_and_check("");
        } else {
            col.default_value = "";
        }

        info->column_datas.push_back(col);

        if (peek() == CLOSE_PAREN && peek_ahead() == SEMICOLON) {
            break;
        } else if (peek() != COMMA) {
            push_error_return("Non-comma or termination after create table entry");
        }

        advance_and_check("No data after CREATE TABLE comma");
    }

    if (peek() == CLOSE_PAREN) {
        advance_and_check("No closing ';' after table entry");
        if (peek() != SEMICOLON) {
            push_error_return("No closing ';' after table entry");}
        token_position++;
    } else {
        push_error_return("No closing ');' after table entry");}

    if (!errors.empty()) {
        push_error_return("Unknown error during CREATE TABLE");}

    nodes.push_back(info);

    std::cout << "AFTER TABLE CREATED, TOKEN KEYWORD == " << keyword_enum_to_string[peek()] << std::endl;
}

bool is_integer_data_type(std::string data_type) {
    if (data_type.find("CHAR") == std::string::npos &&
        data_type.find("INT") == std::string::npos &&
        data_type.find("BOOL") == std::string::npos
    ) {
        return false;
    }
    return true;
}

bool is_boolean_data_type(std::string data_type) {
    if (data_type.find("BOOL") == std::string::npos) {
        return false;
    }
    return true;
}

std::string parse_default_value(std::string data_type) {
    if (peek_data() == "DEFAULT") {
        advance_and_check_ret_str("No value after DEFAULT");
        if (peek() == INTEGER_LITERAL) {
            if (!(is_integer_data_type(data_type))) {
                push_error_return_empty_string("Attemping to set integer default value to non-integer column");
            }   
            return (peek_data());
        }
        if (peek() == BOOL) {
            if (!(is_boolean_data_type(data_type))) {
                push_error_return_empty_string("Attemping to set bool default value to non-bool column");
            }
            return (peek_data());
        }
    }
    return "";
}


std::string parse_data_type() {
    bool unsign = false;
    bool zerofill = false;

    if (token_position >= tokens.size()) {
        push_error_return_empty_string("No data type token");}  
    
    if (peek_data() == "UNSIGNED") {
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

    std::string type = peek_data();
    advance_and_check_ret_str("");

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
            advance_and_check_ret_str("");
            if (peek() == CLOSE_PAREN) {
                advance_and_check_ret_str("");
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
            advance_and_check_ret_str("");
            if (peek() == CLOSE_PAREN) {
                advance_and_check_ret_str("");
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
            advance_and_check_ret_str("");

            if (peek() != CLOSE_PAREN) {
                errors.push_back("BIT missing clossing parenthesis");}
                advance_and_check_ret_str("");
            
            return "BIT " + std::to_string(num);
        } else {
            return "BIT 1";}
        
    } else if (type == "INT") {
        // The number is the display size, can ignore in computations for now. I read the default is 11 for signed and 10 for unsigned but I'm not sure it's standard so...
       if (peek() == OPEN_PAREN) {
            advance_and_check_ret_str("");
            if (peek() == CLOSE_PAREN) {
                advance_and_check_ret_str("");
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
            advance_and_check_ret_str("");

            if (peek() != CLOSE_PAREN) {
                errors.push_back("INT missing clossing parenthesis");}
            advance_and_check_ret_str("");
            
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
        push_error_return_empty_string("DEC not support t0o complicated");}

    return "idk what type that is man";
}





// Helpers

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
