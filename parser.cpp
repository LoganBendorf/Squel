#include "pch.h"

#include "parser.h"
#include "node.h"
#include "helpers.h"




std::vector<keyword_enum> function_keywords;
std::vector<std::function<void()>> keyword_parse_functions;

extern std::vector<std::string> errors;
extern std::vector<table> tables;
extern display_table display_tab;

static std::vector<node*> nodes;

static std::vector<token> tokens;
static int token_position = 0;

static int loop_count = 0;

struct numeric {
    bool is_decimal;
    std::string value;
};

static void parse_insert();
static void parse_select();
static void parse_create();
static void parse_create_table();
static bool is_numeric_identifier();
static struct numeric parse_numeric_expression();
static int count_zeroes(std::string integer_lit_data);
static std::string parse_default_value(std::string data_type);
static std::string parse_data_type();

static token peek();
static keyword_enum peek_type();
static std::string peek_data();
static token peek_ahead();



#define push_error_return(x)                    \
        token curTok;                           \
        if (token_position >= tokens.size()) {  \
            if (tokens.size() > 0 && token_position == tokens.size()) { \
                curTok = tokens[token_position - 1]; \
                curTok.position += curTok.data.size(); \
            } else {                            \
                curTok.keyword = LINE_END; curTok.data = ""; curTok.line = -1; curTok.position = -1; \
            }                                   \
        } else {                                \
            curTok = tokens[token_position];}   \
        std::string error = x;                  \
        error = error + ". Line = " + std::to_string(curTok.line) + ", position = " + std::to_string(curTok.position);\
        errors.push_back(error);                \
        return                                  

#define push_error_return_empty_string(x)       \
        token curTok;                           \
        if (token_position >= tokens.size()) {  \
            if (tokens.size() > 0 && token_position == tokens.size()) { \
                curTok = tokens[token_position - 1]; \
                curTok.position += curTok.data.size(); \
            } else {                            \
                curTok.keyword = LINE_END; curTok.data = ""; curTok.line = -1; curTok.position = -1; \
            }                                   \
        } else {                                \
            curTok = tokens[token_position];}   \
        std::string error = x;                  \
        error = error + ". Line = " + std::to_string(curTok.line) + ", position = " + std::to_string(curTok.position);\
        errors.push_back(error);                \
        return ""   

#define push_error_return_empty_object(x)       \
        token curTok;                           \
        if (token_position >= tokens.size()) {  \
            if (tokens.size() > 0 && token_position == tokens.size()) { \
                curTok = tokens[token_position - 1]; \
                curTok.position += curTok.data.size(); \
            } else {                            \
                curTok.keyword = LINE_END; curTok.data = ""; curTok.line = -1; curTok.position = -1; \
            }                                   \
        } else {                                \
            curTok = tokens[token_position];}   \
        std::string error = x;                  \
        error = error + ". Line = " + std::to_string(curTok.line) + ", position = " + std::to_string(curTok.position);\
        errors.push_back(error);                \
        object empty_obj = {ERROR, ""};             \
        return empty_obj


// sussy using macro in macro
#define advance_and_check(x)                \
    token_position++;                       \
    if (token_position >= tokens.size()) {  \
        push_error_return(x);}              

// sussy using macro in macro
#define advance_and_check_ret_str(x)        \
    token_position++;                       \
    if (token_position >= tokens.size()) {  \
        push_error_return_empty_string(x);} 

// sussy using macro in macro
#define advance_and_check_ret_obj(x)        \
    token_position++;                       \
    if (token_position >= tokens.size()) {  \
        push_error_return_empty_object(x);} 

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


// it token has parse function us it, else error
// underscores are allowed in words
std::vector<node*> parse() {
    if (tokens.size() == 0) {
        errors.push_back("parse(): No tokens");
        return nodes;}

    auto it = std::find(function_keywords.begin(), function_keywords.end(), tokens[token_position].keyword);
    if (it == function_keywords.end()) {
        token tok = tokens[token_position];
        std::string error = "Unknown keyword or inappropriate usage (" + peek_data() +  ") Token type = "
                          + keyword_enum_to_string(tok.keyword) + ". Line = " + std::to_string(tok.line) 
                          + ". Position = " + std::to_string(tok.position);
        errors.push_back(error);
        return nodes;
    }

    int keyword_index = std::distance(function_keywords.begin(), it);

    keyword_parse_functions[keyword_index]();

    if (!errors.empty()) {
        // Look for end of statement ';', if it's there, go to it then continue looking for errors in the next statement
        while (token_position < tokens.size() && peek_type() != SEMICOLON) {
            token_position++;
        }
        token_position++; // If at semicolon, advance past it
    }

    if (loop_count++ > 100) {
        std::cout << "le stuck in loop?";}

    if (peek_type() != LINE_END) {
        parse();
    }

    return nodes;
}

// doesnt work, should be inserting ROWS
static void parse_insert() {
    if (peek_type() != INSERT) {
        std::cout << "parse_insert() called with non-insert token";
        exit(1);}

    advance_and_check("No tokens after INSERT");

    insert_into* info = new insert_into();

    if (peek_type() == INTO) {

        advance_and_check("No tokens after INSERT INTO");

        if (peek_type() != STRING_LITERAL) {
            push_error_return("INSERT INTO bad token type for table name");}

        info->table_name = peek_data();

        advance_and_check("No tokens after INSERT INTO table");

        if (peek_type() != OPEN_PAREN) {
            push_error_return("No open parenthesis after INSET INTO table");}

        advance_and_check("No tokens after INSERT INTO table (");

        // Find field names
        int count = 1;

        while (peek_type() != CLOSE_PAREN && peek_type() != LINE_END) {
            
            if (peek_type() != STRING_LITERAL) {
                if (count > 1) {
                    push_error_return("parse_insert(): Comma must serpate field names");
                } else {
                    push_error_return("Field names must be strings");
                }
            }
            
            info->field_names.push_back(peek_data());
            
            advance_and_check("Field names missing closing parenthesis");
            
            if (peek_type() != COMMA) {
                break;}
            
            advance_and_check("Missing field name after comma");

            if (count++ == 32) {
                push_error_return("Tables cannot have more than 32 columns >:(");}
        }

        if (!errors.empty()) {
            return;}

        if (peek_type() != CLOSE_PAREN) {
            push_error_return("Field names missing closing parenthesis");}

        advance_and_check("INSERT INTO table, missing VALUES");

        if (peek_type() != VALUES) {
            push_error_return("INSERT INTO table, missing VALUES");}

        advance_and_check("INSERT INTO table, missing open parenthesis after VALUES");

        if (peek_type() != OPEN_PAREN) {
            push_error_return("INSERT INTO table, unknown token after VALUES, should be open parenthesis");}

        advance_and_check("INSERT INTO table, missing values after open parenthesis");

        // Find values

        count = 1;
        while (count++ < 32) {
        
            if (peek_type() != STRING_LITERAL && !is_numeric_identifier()) {
                if (count > 1) {
                    push_error_return("parse_insert(): Comma must serpate VALUES's values");
                } else {
                    push_error_return("VALUES must be string or numeric, token was " + keyword_enum_to_string(peek_type()));}
                }
            
            struct numeric numeric_expression;
            if (is_numeric_identifier()) {
                numeric_expression = parse_numeric_expression();
                keyword_enum type = numeric_expression.is_decimal ? DECIMAL_LITERAL : INTEGER_LITERAL;
                info->values.push_back(data_type_pair{type, numeric_expression.value});
            } else {
                info->values.push_back(data_type_pair{peek_type(), peek_data()});
            }
            
            advance_and_check("VALUES missing closing parenthesis");

            if (peek_type() == CLOSE_PAREN) {
                break;}

            if (peek_type() != COMMA) {
                push_error_return("VALUES must be seperated by a comma");}

            advance_and_check("Missing values after comma in VALUES");

        }

        
        if (count >= 32) {
            push_error_return("VALUES, tables cannot have more than 32 columns >:(");}
            
        if (peek_type() != CLOSE_PAREN) {
            push_error_return("VALUES, missing closing parenthesis");}

        if (!errors.empty()) {
            return;}

        advance_and_check("VALUES missing closing semicolon");

        if (peek_type() != SEMICOLON) {
            push_error_return("VALUES missing closing semicolon");}
        token_position++;

        if (info->field_names.size() != info->values.size()) {
            push_error_return("INSET INTO, field names and VALUES have non-equal size");}

    } else {
        push_error_return("Unknown INSERT usage");}

    nodes.push_back(info);
}


static void parse_select() {
    if (peek_type() != SELECT) {
        std::cout << "parse_select() called with non-select token";
        exit(1);}
    advance_and_check("No tokens after SELECT")

    if (peek_type() == ASTERISK) {
        advance_and_check("No tokens after SELECT *");
        
        if (peek_type() == FROM){
            advance_and_check("No tokens after SELECT * FROM");

            if (peek_type() != STRING_LITERAL) {
                push_error_return("SELECT bad token type for table name");}

            std::string table_name = peek_data();

            // Advance past string literal
            advance_and_check("parse_select(): Missing semicolon after table name");
            // Advance past semicolon
            if (peek_type() != SEMICOLON) {
                push_error_return("parse_select(): Missing semicolon after table name");}
            token_position++;

            select_from* info = new select_from();
            info->table_name = table_name;
            nodes.push_back(info);

        } else {
            push_error_return("Unknown SELECT usage");}
    } else {
        push_error_return("Unknown SELECT usage");}
}

static void parse_create() {
    if (peek_type() != CREATE) {
        std::cout << "parse_create() called with non-create token";
        exit(1);}

    advance_and_check("No tokens after CREATE");

    if (peek_type() == TABLE) {
        parse_create_table();
    } else {
        push_error_return("Unknown keyword (" + peek_data() + ") after CREATE");}

}


// tokens should point to TABLE
static void parse_create_table() {
    if (peek_type() != TABLE) {
        std::cout << "parse_create_table() called with non-create_table token";
        exit(1);}
        
    advance_and_check("No tokens after CREATE TABLE");

    if (peek_type() != STRING_LITERAL) {
        push_error_return("Non-string literal token for table name. Token data (" + peek_data() + ")");}

    create_table* info = new create_table();
    info->table_name = peek_data();

    advance_and_check("No open paren '(' after CREATE TABLE");

    if (peek_type() != OPEN_PAREN) {
        push_error_return("No open paren '(' after CREATE TABLE");}
    
    advance_and_check("No data in CREATE TABLE");


    int error_index = errors.size();
    int loop_count = 0;
    while (loop_count++ < 100) {

        if (peek_type() != STRING_LITERAL) {
            push_error_return("Non-string literal token for field name. Token data (" + peek_data() + ")");}

        column_data col;
        col.field_name = peek_data();

        advance_and_check("No data type after CREATE TABLE field name");

        col.data_type = parse_data_type();
        bool recent_error = error_index < errors.size();
        if (recent_error) {
            return;}

        // Need to check in gdb if it sshould advance first
        if (peek_type() != COMMA && peek_type() != CLOSE_PAREN) {
            if (peek_data() == "DEFAULT"){
                col.default_value = parse_default_value(col.data_type);
                bool recent_error = error_index < errors.size();
                if (recent_error) {
                    return;}
                advance_and_check("parse_create_table(): no values after DEFAULT x");
            } else {
                push_error_return("Invalid something after data type");
            }
        } else {
            col.default_value = "";
        }

        info->column_datas.push_back(col);

        if (peek_type() == CLOSE_PAREN && peek_ahead().keyword == SEMICOLON) {
            break;
        } else if (peek_type() != COMMA) {
            push_error_return("Non-comma or termination after create table entry");
        }

        advance_and_check("No data after CREATE TABLE comma");
    }
    
    if (loop_count >= 100) {
        push_error_return("parse_create_table(): Too many loops in field search, bug or over 100 fields");}

    if (peek_type() == CLOSE_PAREN) {
        advance_and_check("No closing ';' after table entry");
        if (peek_type() != SEMICOLON) {
            push_error_return("No closing ';' after table entry");}
        token_position++;
    } else {
        push_error_return("No closing ');' after table entry");}

    nodes.push_back(info);

    std::cout << "AFTER TABLE CREATED, TOKEN KEYWORD == " << keyword_enum_to_string(peek_type()) << std::endl;
}

static bool is_numeric_identifier() {
    if (peek_type() == INTEGER_LITERAL ||
        peek_type() == DOT ||
        peek_type() == MINUS
    ) {
        return true;
    }
    return false;
}

#define push_error_return_empty_numeric(x)  \
    token curTok;                           \
    if (token_position >= tokens.size()) {  \
        if (tokens.size() > 0 && token_position == tokens.size()) { \
            curTok = tokens[token_position - 1]; \
            curTok.position += curTok.data.size(); \
        } else {                            \
            curTok.keyword = LINE_END; curTok.data = ""; curTok.line = -1; curTok.position = -1; \
        }                                   \
    } else {                                \
        curTok = tokens[token_position];}   \
    std::string error = x;                  \
    error = error + ". Line = " + std::to_string(curTok.line) + ", position = " + std::to_string(curTok.position);\
    errors.push_back(error);                \
    struct numeric empty_numeric = {false, ""};\
    return empty_numeric

#define advance_and_check_return_numeric(x)    \
    token_position++;                       \
    if (token_position >= tokens.size()) {  \
        push_error_return_empty_numeric(x);}

static bool is_infix_operator(token tok) {
    
    switch (tok.keyword) {
        case MINUS: return true; break;
        case DOT: return true; break;
        default: return false;
    }
}


// need object class which INFIX and stuff INHERIT from ARR ARR ARR ARR ARR
static object parse_numeric_infix(token tok) {
    if (!is_infix_operator(tok)) {
        push_error_return_empty_object("parse_numeric_infix called with non-infix operator");
    }
    
}


static object parse_integer_literal(token tok) {
    if (tok.keyword != INTEGER_LITERAL) {
        push_error_return_empty_object("parse_integer_literal called with non-integer");
    }

    advance_and_check_ret_obj("No values after integer literal");

    std::string value = tok.data;

    if (is_infix_operator(peek())) {
        object right = parse_numeric_infix(peek());
        if (right.type != INTEGER_EXPRESSION || !errors.empty()) {
            push_error_return_empty_object("parse_integer_literal fail");}
        value += right.value; 
    }

    object obj = {INTEGER_EXPRESSION, value};
    return obj;
}

        
static object parse_numeric_prefix(token prefix) {
    
    advance_and_check_ret_obj("Missing values after numeric prefix");

    switch (prefix.keyword) {
        case MINUS: {
            std::string numeric_portion = "-";
            if (peek_data() == ".") {
                numeric_portion += ".";
                advance_and_check_ret_obj("Missing values after numeric prefix");
            }
            numeric_portion += parse_integer_literal(peek());
        } break; 
        case DOT: {

        } break;
        default:
            advance_and_check_ret_obj("Unknown numeric prefix (" + prefix.data + ")");

    }

}
        
        
        // Needs to be reworked, minus sign can also mean subtraction, need seperate function for prefix minus
static struct numeric parse_numeric_expression() {
    struct numeric return_num = {false, ""};
    bool decimal = false;
    bool negative = false;
    bool integer_literal_found = false;

    if (peek_type() == MINUS) {
        negative = true;
        return_num.value += "-";
    }

    keyword_enum last = peek_type();

    if (last == MINUS) {
        advance_and_check_return_numeric("No integer portion after minus sign in integer expression");
    } else {
        advance_and_check_return_numeric("Empty integer expression");
    }

    int loop_count = 0;
    while (loop_count++ < 32) {
        if (peek_type() == DOT) {
            if (decimal == true) {
                push_error_return_empty_numeric("Too many decimals in integer expression");}
            decimal = true;
            return_num.value += ".";
        } else if (peek_type() == INTEGER_LITERAL) {
            integer_literal_found = true;
            return_num.value += peek_data();
            if (!decimal && count_zeroes(peek_data()) > 1) {
                push_error_return_empty_numeric("Too many zeroes in integer expression");}
        } else if (peek_type() == MINUS) {
            push_error_return_empty_numeric("Too many minus signs in integer expression");
        } else {
            break;
        }

        last = peek_type();

        advance_and_check_return_numeric("Early termination during integer expression parsing");
    }

    if (loop_count == 32) {
        push_error_return_empty_numeric("Integer either weird or too big");}

    if (!integer_literal_found) {
        push_error_return_empty_numeric("No integer literal found in integer expression");}
    
    if (last == DOT) {
        push_error_return_empty_numeric("Trailing dot in integer expression");}

    if (last == MINUS) {
        push_error_return_empty_numeric("No integer portion after minus sign in integer expression");}

    return return_num;
}

static int count_zeroes(std::string integer_lit_data) {
    int count = 0;
    for (int i = 0; i < integer_lit_data.size(); i++) {
        if (integer_lit_data.at(i) == '0') {count++;}
    }
    return count;
}

static std::string parse_default_value(std::string data_type) {

    if (peek_data() != "DEFAULT") {
        push_error_return_empty_string("parse_default_value(): called with non-DEFAULT value token");
    }

    advance_and_check_ret_str("No value after DEFAULT");

    if (is_numeric_identifier()) {
        struct numeric numeric_expression = parse_numeric_expression();
        if (!errors.empty()) {
            return "";}

        if (numeric_expression.is_decimal) {
            if (!is_decimal_data_type(data_type)) {
                push_error_return_empty_string("Attemping to set decimal default value to non-decimal column");}   
        } else if (!(is_integer_data_type(data_type))) {
            push_error_return_empty_string("Attemping to set integer default value to non-integer column");}   

        return (peek_data());
    }
    if (peek_type() == BOOL) {
        if (!(is_boolean_data_type(data_type))) {
            push_error_return_empty_string("Attemping to set bool default value to non-bool column");
        }
        return (peek_data());
    }
    
    return "";
}

// advances after type
static std::string parse_data_type() {
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

    if (peek_type() != STRING_LITERAL) {
        std::string msg = "Data type has bad token type (" + std::to_string(peek_type()) + ")\n";
        push_error_return_empty_string(msg);}

    std::string type = peek_data();
    advance_and_check_ret_str("parse_data_type(): Nothing after data type");

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

    if (type == "VARCHAR") {
        //advance_and_check_ret_str("parse_data_type(): VARCHAR missing length");

        if (peek_type() != OPEN_PAREN) {
            push_error_return_empty_string("parse_data_type(): VARCHAR missing open parenthesis");
        }

        advance_and_check_ret_str("parse_data_type(): VARCHAR missing length");

        if (peek_type() != INTEGER_LITERAL) {
            push_error_return_empty_string("parse_data_type(): VARCHAR parameters contain non-integer");
        }
        std::string length = peek_data();
        
        advance_and_check_ret_str("parse_data_type(): VARCHAR nothing after length");

        if (peek_type() != CLOSE_PAREN) {
            push_error_return_empty_string("parse_data_type(): VARCHAR missing closing parenthesis");
        }

        advance_and_check_ret_str("parse_data_type(): VARCHAR nothing after closing parenthesis");

        return "VARCHAR " + length;
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
        if (peek_type() == OPEN_PAREN) {
            advance_and_check_ret_str("parse_data_type(): ");
            if (peek_type() == CLOSE_PAREN) {
                advance_and_check_ret_str("parse_data_type(): ");
                return "SET";}

            std::vector<std::string> elements;
            int count = 1;
            if (peek_type() != STRING_LITERAL) {
                errors.push_back("Can only add strings to SET");}
            elements.push_back(tokens[token_position].data);
            
            advance_and_check_ret_str("Missing closing parenthesis");

            while (peek_type() != CLOSE_PAREN && peek_type() != LINE_END) {
                if (peek_type() != COMMA) {
                    errors.push_back("Comma must seperate SET elements");
                    break;}
                
                advance_and_check_ret_str("SET missing element after comma");
                
                if (peek_type() != STRING_LITERAL) {
                    errors.push_back("Can only add strings to SET");}
                
                elements.push_back(peek_data());
                
                advance_and_check_ret_str("SET missing closing parenthesis");

                if (count++ == 64) {
                    errors.push_back("SET cannot have more than 64 elements");
                    break;}
            }

            if (peek_type() != CLOSE_PAREN && errors.empty()) {
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
        if (peek_type() == OPEN_PAREN) {
            advance_and_check_ret_str("parse_data_type(): ");
            if (peek_type() == CLOSE_PAREN) {
                advance_and_check_ret_str("parse_data_type(): ");
                return "BIT 1";}

            int num = 1;
            if (peek_type() != INTEGER_LITERAL) {
                if (peek_type() != LINE_END) {
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
            advance_and_check_ret_str("parse_data_type(): ");

            if (peek_type() != CLOSE_PAREN) {
                errors.push_back("BIT missing clossing parenthesis");}
                advance_and_check_ret_str("parse_data_type(): ");
            
            return "BIT " + std::to_string(num);
        } else {
            return "BIT 1";}
        
    } else if (type == "INT") {
        // The number is the display size, can ignore in computations for now. I read the default is 11 for signed and 10 for unsigned but I'm not sure it's standard so...
       if (peek_type() == OPEN_PAREN) {
            advance_and_check_ret_str("parse_data_type(): ");
            if (peek_type() == CLOSE_PAREN) {
                advance_and_check_ret_str("parse_data_type(): ");
                if (unsign) {
                    return "INT 10";}
                return "INT 11";}

            int num = 11;
            if (peek_type() != INTEGER_LITERAL) {
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
            advance_and_check_ret_str("parse_data_type(): ");

            if (peek_type() != CLOSE_PAREN) {
                errors.push_back("INT missing clossing parenthesis");}
            advance_and_check_ret_str("parse_data_type(): ");
            
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

static token peek() {
    if (token_position >= tokens.size()) {
        token tok = {LINE_END, "", 0, 0};
        return tok;
    } else {
        return tokens[token_position];}
}

static keyword_enum peek_type() {
    if (token_position >= tokens.size()) {
        return LINE_END;
    } else {
        return tokens[token_position].keyword;}
}

static std::string peek_data() {
    if (token_position >= tokens.size()) {
        return "";
    } else {
        return tokens[token_position].data;}
}

static token peek_ahead() {
    if (token_position + 1 >= tokens.size()) {
        token tok = {LINE_END, "", 0, 0};
        return tok;
    } else {
        return tokens[token_position + 1];}
}
