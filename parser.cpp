#include "pch.h"

#include "parser.h"
#include "node.h"
#include "helpers.h"
#include "object.h"


extern std::vector<std::string> errors;
extern std::vector<table> tables;
extern display_table display_tab;

static std::vector<node*> nodes;

static std::vector<token> tokens;
static int token_position = 0;

static token prev_token = {ERROR, std::string("garbage"), 0, 0};

static int loop_count = 0;

struct numeric {
    bool is_decimal;
    std::string value;
};

static void parse_alter();
static void parse_insert();
static void parse_select();
static void parse_create();
static void parse_create_table();
static bool is_numeric_identifier();
// static int count_zeroes(std::string integer_lit_data);
static object* parse_default_value(SQL_data_type_object* data_type);
static object* parse_data_type();


object* prefix_parse_functions_with_obj(object* obj);
object* prefix_parse_functions_with_token(token tok);
object* infix_parse_functions_with_obj(object* obj);
object* infix_parse_functions_with_token(token tok);
object* parse_expression(int precedence);

object* eval_expression(object* expression);
object* eval_infix_expression(token op, object* left, object* right);


static token peek();
static token_type peek_type();
static std::string peek_data();
static token peek_ahead();


enum precedences {
    LOWEST = 1, PREFIX = 6, HIGHEST = 1000
};  



#define push_error_return(x)                    \
        token curTok;                           \
        if (token_position >= tokens.size()) {  \
            if (tokens.size() > 0 && token_position == tokens.size()) { \
                curTok = tokens[token_position - 1]; \
                curTok.position += curTok.data.size(); \
            } else {                            \
                curTok.type = LINE_END; curTok.data = ""; curTok.line = -1; curTok.position = -1; \
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
                curTok.type = LINE_END; curTok.data = ""; curTok.line = -1; curTok.position = -1; \
            }                                   \
        } else {                                \
            curTok = tokens[token_position];}   \
        std::string error = x;                  \
        error = error + ". Line = " + std::to_string(curTok.line) + ", position = " + std::to_string(curTok.position);\
        errors.push_back(error);                \
        return ""   

#define push_error_return_error_object(x)       \
        token curTok;                           \
        if (token_position >= tokens.size()) {  \
            if (tokens.size() > 0 && token_position == tokens.size()) { \
                curTok = tokens[token_position - 1]; \
                curTok.position += curTok.data.size(); \
            } else {                            \
                curTok.type = LINE_END; curTok.data = ""; curTok.line = -1; curTok.position = -1; \
            }                                   \
        } else {                                \
            curTok = tokens[token_position];}   \
        std::string error = x;                  \
        error = error + ". Line = " + std::to_string(curTok.line) + ", position = " + std::to_string(curTok.position);\
        errors.push_back(error);                \
        return new error_object();

#define push_error_return_error_object_prev_tok(x)\
        token tok = prev_token;                 \
        std::string error = x;                  \
        error = error + ". Line = " + std::to_string(tok.line) + ", position = " + std::to_string(tok.position);\
        errors.push_back(error);                \
        return new error_object();


// sussy using macro in macro
#define advance_and_check(x)                \
    prev_token = tokens[token_position];    \
    token_position++;                       \
    if (token_position >= tokens.size()) {  \
        push_error_return(x);}              

// sussy using macro in macro
#define advance_and_check_ret_str(x)        \
    prev_token = tokens[token_position];    \
    token_position++;                       \
    if (token_position >= tokens.size()) {  \
        push_error_return_empty_string(x);} 

// sussy using macro in macro
#define advance_and_check_ret_obj(x)        \
    prev_token = tokens[token_position];    \
    token_position++;                       \
    if (token_position >= tokens.size()) {  \
        push_error_return_error_object(x);} 

void parser_init(std::vector<token> toks) {
    tokens = toks;
    token_position = 0;
    loop_count = 0;
    nodes.clear();

}


// it token has parse function us it, else error
// underscores are allowed in words
std::vector<node*> parse() {
    if (tokens.size() == 0) {
        errors.push_back("parse(): No tokens");
        return nodes;}

    switch (peek_type()) {
    case CREATE:
        parse_create(); break;
    case INSERT:
        parse_insert(); break;
    case SELECT:
        parse_select(); break;
    case ALTER:
        parse_alter(); break;
    default:
        std::string error = "Unknown keyword or inappropriate usage (" + peek_data() +  ") Token type = "
        + token_type_to_string(peek().type) + ". Line = " + std::to_string(peek().line) 
        + ". Position = " + std::to_string(peek().position);
        std::cout << error << std::endl;
        exit(1);
    }

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

static void parse_alter() {
    if (peek_type() != ALTER) {
        std::cout << "parse_alter() called with non-ALTER token";
        exit(1);
    }

    advance_and_check("No tokens after ALTER");

    if (peek_type() != TABLE) {
        push_error_return("ALTER, only works on TABLE for now");}

    advance_and_check("No tokens after ALTER TABLE");

    alter_table* info = new alter_table();

    object* expression = parse_expression(LOWEST);
    expression = eval_expression(expression);

    if (expression->type() != STRING_OBJ) {
        push_error_return("ALTER TABLE, table name must be string");}

    info->table_name = expression->data();

    if (peek_type() != ADD) {
        push_error_return("ALTER TABLE, only supports ADD");}

    advance_and_check("No tokens after ADD");

    if (peek_type() != COLUMN) {
        push_error_return("ALTER TABLE, only supports ADD COLUMN");}

    advance_and_check("No tokens after ADD COLUMN");

    expression = parse_expression(LOWEST);
    expression = eval_expression(expression);

    if (expression->type() != STRING_OBJ) {
        push_error_return("ALTER TABLE, column name must be string");}

    object* obj = parse_data_type();
    if (obj->type() != SQL_DATA_TYPE_OBJ) {
        push_error_return("Data type error");}
    
    SQL_data_type_object* data_type = static_cast<SQL_data_type_object*>(obj);

    column_object* col = new column_object(expression->data(), static_cast<SQL_data_type_object*>(data_type), "");

    if (peek_type() == DEFAULT) {
        int err_count = errors.size();
        std::string default_value = parse_default_value(data_type)->data(); // NOTE: string for now
        if (err_count < errors.size()) {
            return;}
        col->default_value = default_value;
    }
    if (peek_type() != SEMICOLON) {
        push_error_return("Missing ending semicolon");}
    token_position++;

    info->table_edit = col;


    nodes.push_back(info);
}

// inserts rows
static void parse_insert() {
    if (peek_type() != INSERT) {
        std::cout << "parse_insert() called with non-INSERT token";
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
            push_error_return("No open parenthesis after INSERT INTO table");}

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
                    push_error_return("VALUES must be string or numeric, token was " + token_type_to_string(peek_type()));}
                }
            
            object* expression = parse_expression(LOWEST);
            expression = eval_expression(expression);
            info->values.push_back(expression);
            
            // advance_and_check("VALUES missing closing parenthesis");

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
            push_error_return("INSERT INTO, field names and VALUES have non-equal size");}

    } else {
        push_error_return("Unknown INSERT usage");}

    nodes.push_back(info);
}

#define advance_and_check_list(x)            \
    prev_token = tokens[token_position];    \
    token_position++;                       \
    if (token_position >= tokens.size()) {  \
        push_error_return_list(x);}     

#define push_error_return_list(x)                    \
    token curTok;                           \
    if (token_position >= tokens.size()) {  \
        if (tokens.size() > 0 && token_position == tokens.size()) { \
            curTok = tokens[token_position - 1]; \
            curTok.position += curTok.data.size(); \
        } else {                            \
            curTok.type = LINE_END; curTok.data = ""; curTok.line = -1; curTok.position = -1; \
        }                                   \
    } else {                                \
        curTok = tokens[token_position];}   \
    std::string error = x;                  \
    error = error + ". Line = " + std::to_string(curTok.line) + ", position = " + std::to_string(curTok.position);\
    errors.push_back(error);                \
    std::vector<object*> errored_list;      \
    return errored_list;


// Starts on first value, end on end val
static std::vector<object*> parse_comma_seperated_list(token_type end_val) {

    std::vector<object*> list;


    int loop_count = 0;
    while (loop_count++ < 100) {
        object* cur = parse_expression(LOWEST);
        cur = eval_expression(cur);
        if (!is_listable(cur)) {
            push_error_return_list("Object is not listable, sad!");}

        list.push_back(cur);
        
        if (peek_type() == end_val) {
            break;}
        
        if (peek_type() != COMMA) {
            push_error_return_list("Items in list must be comma seperated");}

        advance_and_check_list("parse_comma_seperated_list(): No values after comma");
    }

    if (loop_count >= 100) {
        push_error_return_list("Too many loops during comma seperated list traversal, likely weird error");}

    return list;
}


static void parse_select() {
    if (peek_type() != SELECT) {
        std::cout << "parse_select() called with non-select token";
        exit(1);}

    advance_and_check("No tokens after SELECT")

    int error_start_count = errors.size();
    std::vector<object*> list = parse_comma_seperated_list(FROM);
    if (errors.size() > error_start_count) {
        return;}

    if (list.size() == 0) {
        push_error_return("SELECT, list must contain values, stupid >:)");}

    for (int i = 0; i < list.size(); i++) {
        if (list[i]->type() != STRING_OBJ) {
            push_error_return("SELECT, list items must be strings or *");}
    }
    
    if (peek_type() != FROM) {
        push_error_return("Token not allowed after SELECT list");}

    advance_and_check("No values after FROM");

    object* name_obj = parse_expression(LOWEST);
    name_obj = eval_expression(name_obj);
    if (name_obj->type() != STRING_OBJ) {
        push_error_return("Invalid table name");}
    
    select_from* info = new select_from();
    info->table_name = name_obj->data();

    if (list[0]->data() == "*") {
        if (list.size() > 1) {
            push_error_return("Too many entries in list after *");}
        info->asterisk = true;
    } else {
        for (int i = 0; i < list.size(); i++) {
            info->column_names.push_back(list[i]->data());}
        info->asterisk = false;
    }   

    if (peek_type() == WHERE) {
        advance_and_check("No values after SELECT FROM WHERE");

        object* expression = parse_expression(LOWEST);
        if (expression->type() != INFIX_EXPRESSION_OBJ) {
            push_error_return("Expected condition in SELECT FROM WHERE, got (" + expression->inspect() + ")");}
        infix_expression_object* condition_obj = static_cast<infix_expression_object*>(expression);
        condition_obj->left = eval_expression(condition_obj->left);
        condition_obj->right = eval_expression(condition_obj->right);

        info->condition = condition_obj;
    } else {
        null_object* null_obj = new null_object();
        info->condition = null_obj;
    }
    
    // Advance past semicolon
    if (peek_type() != SEMICOLON) {
        push_error_return("parse_select(): Missing semicolon ending semicolon, instread got " + token_type_to_string(peek_type()));}
        token_position++;
        
    nodes.push_back(info);

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

        object* data_type = parse_data_type();
        if (data_type->type() != SQL_DATA_TYPE_OBJ) {
            push_error_return("Data type error");}

        col.data_type = static_cast<SQL_data_type_object*>(data_type);

        bool recent_error = error_index < errors.size();
        if (recent_error) {
            return;}

        if (peek_type() != COMMA && peek_type() != CLOSE_PAREN) {
            if (peek_data() == "DEFAULT"){
                col.default_value = parse_default_value(col.data_type)->data(); // NOTE: string for now
                bool recent_error = error_index < errors.size();
                if (recent_error) {
                    return;}
                // advance_and_check("parse_create_table(): no values after DEFAULT x");
            } else {
                push_error_return("Invalid something after data type");
            }
        } else {
            col.default_value = "";
        }

        info->column_datas.push_back(col);

        // advance_and_check("parse_create_table(): No values after entry");


        if (peek_type() == CLOSE_PAREN && peek_ahead().type == SEMICOLON) {
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

    // std::cout << "AFTER TABLE CREATED, TOKEN KEYWORD == " << token_type_to_string(peek_type()) << std::endl;
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
            curTok.type = LINE_END; curTok.data = ""; curTok.line = -1; curTok.position = -1; \
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
    
    switch (tok.type) {
        case MINUS: return true; break;
        case DOT: return true; break;
        default: return false;
    }
}


int numeric_precedence(token tok) {
    switch (tok.type) {
        case EQUAL:         return 2; break;
        case NOT_EQUAL:     return 2; break;
        case LESS_THAN:     return 3; break;
        case GREATER_THAN:  return 3; break;
        case PLUS:          return 4; break;
        case MINUS:         return 4; break;
        case SLASH:         return 5; break;
        case ASTERISK:      return 5; break;
        case OPEN_PAREN:    return 7; break;
        case OPEN_BRACKET:  return 8; break;
        default:
            return LOWEST;
    }
}

object* parse_prefix_minus() {

	advance_and_check_ret_obj("No right expression for prefix minus sign");

    if (!is_numeric_token(peek())) {
        push_error_return_error_object("Minus sign can only prefix numerics"); }

	object* right = parse_expression(PREFIX);
    if (!is_numeric_object(right)) {
        push_error_return_error_object("Minus sign can only prefix numerics"); }

    integer_object* obj = new integer_object(- std::stoi(right->data()));

	return obj;
}


object* parse_infix_operator(object* left) {

    infix_expression_object* expression = new infix_expression_object(peek(), left);

    int precedence = numeric_precedence(peek());

    advance_and_check_ret_obj("No tokens after infix operator");
    
    object* right = parse_expression(precedence);
    if (right->type() == ERROR_OBJ) {
        return right;}

    expression->right = right;

    return expression;

}

object* eval_expression(object* expression) {

    switch(expression->type()) {
        case INTEGER_OBJ:
            return expression; break;
        case STRING_OBJ:
            return expression; break;
        case INFIX_EXPRESSION_OBJ: {
             infix_expression_object* infix_expr = static_cast<infix_expression_object*>(expression);
            object* left = eval_expression(infix_expr->left);
            if (left->type() == ERROR_OBJ) {
                return left;}
            object* right = eval_expression(infix_expr->right);
            if (right->type() == ERROR_OBJ) {
                return right;}
            return eval_infix_expression(infix_expr->op, left, right);
        } break;

        default:
            push_error_return_error_object("Cannot evaluate expression of type " + expression->inspect());
    }
}

object* eval_infix_expression(token op, object* left, object* right) {

    switch (op.type) {
        case PLUS:
            if (is_numeric_object(left) && is_numeric_object(right)) {
                return new integer_object(static_cast<integer_object*>(left)->value + static_cast<integer_object*>(right)->value);
            } 
            else if (is_string_object(left) && is_string_object(right)) {
                return new string_object(left->data() + right->data());
            } 
            else {
                std::string error_str = "No infix " + token_type_to_string(op.type) + " operation for " + left->inspect() + " and " + right->inspect();
                push_error_return_error_object(error_str);
            }
            break;
        case MINUS:
            if (!is_numeric_object(left) || !is_numeric_object(right)) {
                std::string error_str = "No infix " + token_type_to_string(op.type) + " operation for " + left->inspect() + " and " + right->inspect();
                push_error_return_error_object(error_str);}
            return new integer_object(static_cast<integer_object*>(left)->value - static_cast<integer_object*>(right)->value);
            break;
        case ASTERISK:
            if (!is_numeric_object(left) || !is_numeric_object(right)) {
                std::string error_str = "No infix " + token_type_to_string(op.type) + " operation for " + left->inspect() + " and " + right->inspect();
                push_error_return_error_object(error_str);}
            return new integer_object(static_cast<integer_object*>(left)->value * static_cast<integer_object*>(right)->value);
            break;
        case SLASH:
            if (!is_numeric_object(left) || !is_numeric_object(right)) {
                std::string error_str = "No infix " + token_type_to_string(op.type) + " operation for " + left->inspect() + " and " + right->inspect();
                push_error_return_error_object(error_str);}
            return new integer_object(static_cast<integer_object*>(left)->value / static_cast<integer_object*>(right)->value);
            break;
        case DOT:
            if (left->type() != INTEGER_OBJ || left->type() != INTEGER_OBJ) {
                std::string error_str = "No infix " + token_type_to_string(op.type) + " operation for " + left->inspect() + " and " + right->inspect();
                push_error_return_error_object(error_str);}
            return new decimal_object(left->data() + "." + right->data());
            break;
        default:
            push_error_return_error_object("No infix " + token_type_to_string(op.type) + " operator known");
    }
}


// token type will index into keywords, which then indexes into parse_prefix_functions returning the appropriate function
std::vector<token_type> prefix_parse_functions_keywords;
object* prefix_parse_functions_with_obj(object* obj) {
    switch (obj->type()) {
    default:
        return NULL;
    }
}
object* prefix_parse_functions_with_token(token tok) {
    switch (tok.type) {
    case MINUS:
        return parse_prefix_minus(); break;
    case INTEGER_LITERAL:
        advance_and_check_ret_obj("No values after integer prefix");
        return new integer_object(tok.data); break;
    case STRING_LITERAL:
        advance_and_check_ret_obj("No values after string prefix");
        return new string_object(tok.data); break;
    case OPEN_PAREN:{
        advance_and_check_ret_obj("No values after open parenthesis in expression");
        object* expression =  parse_expression(LOWEST);
        if (peek_type() != CLOSE_PAREN) {
            push_error_return_error_object("Expected closing parenthesis");}
        advance_and_check_ret_obj("No values after close parenthesis in expression");
        return expression;
    } break;
    case ASTERISK:
        advance_and_check_ret_obj("No values after * prefix");
        return new string_object(tok.data); break;
    default:
        push_error_return_error_object("No prefix function for (" + token_type_to_string(tok.type) + ")");
        return NULL;
    }
}
object* infix_parse_functions_with_obj(object* left) {
    switch (peek_type()) {
    case PLUS:
        return parse_infix_operator(left); break;
    case MINUS:
        return parse_infix_operator(left); break;
    case ASTERISK:
        return parse_infix_operator(left); break;
    case SLASH:
        return parse_infix_operator(left); break;
    case DOT:
        return parse_infix_operator(left); break;
    case EQUAL:
        return parse_infix_operator(left); break;
    case GREATER_THAN:
        return parse_infix_operator(left); break;
    case LESS_THAN:
        return parse_infix_operator(left); break;
    default:
    push_error_return_error_object("No infix function for (" + token_type_to_string(peek_type()) + ") and left (" + left->inspect() + ")");
        return NULL;
    }
}



object* parse_expression(int precedence) {
    
    // std::cout << "parse_expression called with " << token_type_to_string(peek_type()) << std::endl;

    object* left = prefix_parse_functions_with_token(peek());
    if (!left || left->type() == ERROR_OBJ) {
        return new error_object();}

    while (peek_type() != LINE_END &&
           peek_type() != CLOSE_PAREN &&
           peek_type() != SEMICOLON &&
           peek_type() != COMMA && 
           precedence < numeric_precedence(peek()) ) {
            left = infix_parse_functions_with_obj(left);
            if (!left || left->type() == ERROR_OBJ) {
                return new error_object();}

            // advance_and_check_ret_obj("parse_expression(); infix missing right");

            // left = infix_parse_functions_with_obj(left);
            
           }     
        
    return left;
}
        
  
static object* parse_default_value(SQL_data_type_object* data_type) {

    if (peek_data() != "DEFAULT") {
        push_error_return_error_object("parse_default_value(): called with non-DEFAULT value token");
    }

    advance_and_check_ret_obj("No value after DEFAULT");

    object* obj = parse_expression(LOWEST);
    obj = eval_expression(obj);
    std::cout << "default value parsed expression = " << obj->inspect() << std::endl;

    // advance_and_check_ret_obj("Nothing after DEFAULT x");
    
    object* insert_obj = can_insert(obj, data_type);
    if (insert_obj->type() == ERROR_OBJ) {
        errors.push_back("Default value error, " + insert_obj->data());
        return insert_obj;}

    return insert_obj;
}

// likely want to do some expression parsing in here for if someone does something like VARCHAR(2 * 10)
// advances after type
static object* parse_data_type() {

    SQL_data_type_object* data_type = new SQL_data_type_object();
    data_type->prefix = NONE;

    bool unsign = false;
    bool zerofill = false;

    if (token_position >= tokens.size()) {
        push_error_return_error_object("No data type token");}  
    
    if (peek_data() == "UNSIGNED") {
        data_type->prefix == UNSIGNED;
        unsign = true;
        advance_and_check_ret_obj("No data type after UNSIGNED");
    } else if (peek_data() == "ZEROFILL") {
        data_type->prefix == ZEROFILL;
        unsign = true;
        zerofill = true;
        advance_and_check_ret_obj("No data type after ZEROFILL");
    }

    if (peek_type() != STRING_LITERAL) {
        std::string msg = "Data type has bad token type (" + token_type_to_string(peek_type()) + ")\n";
        push_error_return_error_object(msg);}

    std::string type = peek_data();
    advance_and_check_ret_obj("parse_data_type(): Nothing after data type");

    // Basic string
    if (type == "TINYBLOB") {
        data_type->data_type = CHAR;
        data_type->parameter_value = 255;
        return data_type;
    } else if (type == "TINYTEXT") {
        data_type->data_type = CHAR;
        data_type->parameter_value = 255;
        return data_type;
    } else if (type == "MEDIUMTEXT") {
        data_type->data_type = CHAR;
        data_type->parameter_value = 255;
        return data_type;
    } else if (type == "MEDIUMBLOB") {
        data_type->data_type = CHAR;
        data_type->parameter_value = 255;
        return data_type;
    } else if (type == "LONGTEXT") {
        data_type->data_type = CHAR;
        data_type->parameter_value = 255;
        return data_type;
    } else if (type == "LONGBLOB") {
        data_type->data_type = CHAR;
        data_type->parameter_value = 255;
        return data_type;
    } 

    if (type == "VARCHAR") { 
        //advance_and_check_ret_str("parse_data_type(): VARCHAR missing length");

        if (peek_type() != OPEN_PAREN) {
            push_error_return_error_object("parse_data_type(): VARCHAR missing open parenthesis");}

        advance_and_check_ret_obj("parse_data_type(): VARCHAR missing length");

        object* obj = parse_expression(LOWEST);
        if (obj->type() != INTEGER_OBJ) {
            push_error_return_error_object("parse_data_type(): VARCHAR parameter contain non-integer");}
        if (static_cast<integer_object*>(obj)->value <= 0) {
            push_error_return_error_object("parse_data_type(): VARCHAR parameter must be greater than 0");}
        
        // advance_and_check_ret_obj("parse_data_type(): VARCHAR nothing after length");

        if (peek_type() != CLOSE_PAREN) {
            push_error_return_error_object("parse_data_type(): VARCHAR missing closing parenthesis");}

        advance_and_check_ret_obj("parse_data_type(): VARCHAR nothing after closing parenthesis");

        data_type->data_type = VARCHAR;
        data_type->parameter_value = std::stoi(obj->data());
        return data_type;
    }
    
    // Basic numeric
    if (type == "BOOL") {
        data_type->data_type = BOOL;
        return data_type;
    } else if (type == "BOOLEAN") {
        data_type->data_type = BOOL;
        return data_type;
    }

    // Basic time
    if (type == "DATE") {
        data_type->data_type = DATA;
        return data_type;
    } else if (type == "YEAR") {
        data_type->data_type = YEAR;
        return data_type;
    }

    // String
    if (type == "SET") {
        if (peek_type() == OPEN_PAREN) {
            advance_and_check_ret_obj("parse_data_type(): ");
            if (peek_type() == CLOSE_PAREN) {
                advance_and_check_ret_obj("parse_data_type(): ");
                data_type->data_type = SET;
                return data_type;
            }

            std::vector<std::string> elements;
            int count = 1;
            if (peek_type() != STRING_LITERAL) {
                errors.push_back("Can only add strings to SET");}
            elements.push_back(tokens[token_position].data);
            
            advance_and_check_ret_obj("Missing closing parenthesis");

            while (peek_type() != CLOSE_PAREN && peek_type() != LINE_END) {
                if (peek_type() != COMMA) {
                    errors.push_back("Comma must seperate SET elements");
                    break;}
                
                    advance_and_check_ret_obj("SET missing element after comma");
                
                if (peek_type() != STRING_LITERAL) {
                    errors.push_back("Can only add strings to SET");}
                
                elements.push_back(peek_data());
                
                advance_and_check_ret_obj("SET missing closing parenthesis");

                if (count++ == 64) {
                    errors.push_back("SET cannot have more than 64 elements");
                    break;}
            }

            if (peek_type() != CLOSE_PAREN && errors.empty()) {
                errors.push_back("SET missing closing parenthesis");}
            
            data_type->data_type = SET;
            data_type->elements = elements;
            return data_type;
        } else {
            data_type->data_type = SET;
            return data_type;
        }
    }

    // Numeric
    if (type == "BIT") {
        if (peek_type() == OPEN_PAREN) {
            advance_and_check_ret_obj("parse_data_type(): ");
            if (peek_type() == CLOSE_PAREN) {
                advance_and_check_ret_obj("parse_data_type(): ");
                data_type->data_type = BIT;
                data_type->parameter_value = 1;
                return data_type;
            }

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
            advance_and_check_ret_obj("parse_data_type(): ");

            if (peek_type() != CLOSE_PAREN) {
                errors.push_back("BIT missing clossing parenthesis");}
                advance_and_check_ret_obj("parse_data_type(): ");

            data_type->data_type = BIT;
            data_type->parameter_value = num;
            return data_type;
        } else {
            data_type->data_type = BIT;
            data_type->parameter_value = 1;
            return data_type;
        }
        
    } else if (type == "INT") {
        // The number is the display size, can ignore in computations for now. I read the default is 11 for signed and 10 for unsigned but I'm not sure it's standard so...
       if (peek_type() == OPEN_PAREN) {
            advance_and_check_ret_obj("parse_data_type(): ");
            if (peek_type() == CLOSE_PAREN) {
                advance_and_check_ret_obj("parse_data_type(): ");
                if (unsign) {
                    data_type->data_type = INT;
                    data_type->parameter_value = 10;
                    return data_type;
                } else {
                    data_type->data_type = INT;
                    data_type->parameter_value = 11;
                    return data_type;
                }
            }

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
            advance_and_check_ret_obj("parse_data_type(): ");

            if (peek_type() != CLOSE_PAREN) {
                push_error_return_error_object("INT missing clossing parenthesis");}
            advance_and_check_ret_obj("parse_data_type(): ");
            
        } else {
            if (unsign) {
                data_type->data_type = INT;
                data_type->parameter_value = 10;
                return data_type;
            } else {
            data_type->data_type = INT;
            data_type->parameter_value = 11;
            return data_type;
            }
        }

    } else if (type == "DEC" || type == "DECIMAL") {
        push_error_return_error_object_prev_tok("DECIMAL/DEC not supported, too complicated");
    } else if (type == "FLOAT") {
        data_type->data_type = FLOAT;
        data_type->parameter_value = 0;
        return data_type;
    }else if (type == "DOUBLE") {
        data_type->data_type = DOUBLE;
        data_type->parameter_value = 0;
        return data_type;
    }

    push_error_return_error_object("Unknown SQL data type");
}





// Helpers

static token peek() {
    if (token_position >= tokens.size()) {
        token tok = {LINE_END, "", 0, 0};
        return tok;
    } else {
        return tokens[token_position];}
}

static token_type peek_type() {
    if (token_position >= tokens.size()) {
        return LINE_END;
    } else {
        return tokens[token_position].type;}
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
