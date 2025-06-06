#include "pch.h"

#include "parser.h"

#include "node.h"
#include "helpers.h"
#include "object.h"
#include "token.h"
#include "structs_and_macros.h"
#include "environment.h"


extern std::vector<std::string> errors;
extern display_table display_tab;

static std::vector<node*> nodes;

static std::vector<token> tokens;
static size_t token_position = 0;

static token prev_token = {ERROR_TOKEN, std::string("garbage"), 0, 0};

static size_t g_loop_count = 0;

static std::vector<table_object*> global_tables;
static std::vector<std::string> table_names;
static std::vector<std::string> function_names;
static std::vector<evaluated_function_object*> global_functions;

struct numeric {
    bool is_decimal;
    std::string value;
};

static object* parse_assert();
static void parse_alter();
static object* parse_insert();
static object* parse_select();
static void parse_create();
static void parse_create_or_replace_function();
static void parse_create_table();

// static bool is_numeric_identifier();
static object* parse_comma_seperated_list(token_type end_val);
static std::pair<object*, token_type> parse_comma_seperated_list_ADVANCED(const avec<token_type>& end_values); // Can add precedence as well if I feel like it

static object* parse_function();
static object* prefix_parse_functions_with_token(token tok);
static object* infix_parse_functions_with_obj(object* obj);
static object* parse_expression(size_t precedence);

static bool is_function_name(const std_and_astring_variant& name);
static bool is_table_name(const std_and_astring_variant& name);

static token peek();
static token_type peek_type();
static std::string peek_data();
[[maybe_unused]]static token peek_ahead();


enum precedences {
    LOWEST = 0, SQL_STATEMENT = 1, PREFIX = 6, HIGHEST = 1000
};  


// For simple queries, unlikely speedup was 2x. For more complicated ones, speedup was about 4x
#define push_error_return(x)                            \
        token cur_tok;                                  \
        if (token_position >= tokens.size()) [[unlikely]] {\
            if (tokens.size() > 0 && token_position >= tokens.size()) { \
                cur_tok = tokens[tokens.size() - 1];    \
                cur_tok.position += cur_tok.data.size();\
            } else {                                    \
                cur_tok.type = LINE_END; cur_tok.data = ""; cur_tok.line = SIZE_T_MAX; cur_tok.position = SIZE_T_MAX; \
            }                                           \
        } else {                                        \
            cur_tok = tokens[token_position];}          \
        std::string error = x;                          \
        error = error + ". Line = " + std::to_string(cur_tok.line) + ", position = " + std::to_string(cur_tok.position);\
        errors.push_back(std::move(error));             \
        return                                  

#define push_error_return_empty_string(x)               \
        token cur_tok;                                  \
        if (token_position >= tokens.size()) [[unlikely]]{          \
            if (tokens.size() > 0 && token_position >= tokens.size()) { \
                cur_tok = tokens[tokens.size() - 1];    \
                cur_tok.position += cur_tok.data.size();\
            } else {                                    \
                cur_tok.type = LINE_END; cur_tok.data = ""; cur_tok.line = SIZE_T_MAX; cur_tok.position = SIZE_T_MAX; \
            }                                           \
        } else {                                        \
            cur_tok = tokens[token_position];}          \
        std::string error = x;                          \
        error = error + ". Line = " + std::to_string(cur_tok.line) + ", position = " + std::to_string(cur_tok.position);\
        errors.push_back(std::move(error));             \
        return ""   

#define push_error_return_error_object(x)               \
        token cur_tok;                                  \
        if (token_position >= tokens.size()) [[unlikely]]{          \
            if (tokens.size() > 0 && token_position >= tokens.size()) { \
                cur_tok = tokens[tokens.size() - 1];    \
                cur_tok.position += cur_tok.data.size();\
            } else {                                    \
                cur_tok.type = LINE_END; cur_tok.data = ""; cur_tok.line = SIZE_T_MAX; cur_tok.position = SIZE_T_MAX; \
            }                                           \
        } else {                                        \
            cur_tok = tokens[token_position];}          \
        std::stringstream error;                        \
        error << x;                                     \
        error << ". Line = " + std::to_string(cur_tok.line) + ", position = " + std::to_string(cur_tok.position);\
        errors.push_back(error.str());                  \
        return new error_object(error.str());

#define push_error_return_error_object_prev_tok(x)      \
        token cur_tok = prev_token;                     \
        std::string error = x;                          \
        error = error + ". Line = " + std::to_string(cur_tok.line) + ", position = " + std::to_string(cur_tok.position);\
        errors.push_back(error);                        \
        return new error_object(error);


// sussy using macro in macro
#define advance_and_check(x)                \
    prev_token = tokens[token_position];    \
    token_position++;                       \
    if (token_position >= tokens.size()) [[unlikely]]{  \
        push_error_return(x);}              

// sussy using macro in macro
#define advance_and_check_ret_str(x)        \
    prev_token = tokens[token_position];    \
    token_position++;                       \
    if (token_position >= tokens.size()) [[unlikely]]{  \
        push_error_return_empty_string(x);} 

// sussy using macro in macro
#define advance_and_check_ret_obj(x)        \
    prev_token = tokens[token_position];    \
    token_position++;                       \
    if (token_position >= tokens.size()) [[unlikely]]{  \
        push_error_return_error_object(x);} 

static bool is_function_name(const std_and_astring_variant& name) {

    astring astr;
    std::string stdstr;
    VISIT(name, unwrapped,
        astr = unwrapped;
        stdstr = unwrapped;
    );

    for (const auto& func_name : function_names) {
        if (func_name == stdstr) {
            return true; }
    }

    for (const auto& global_func : global_functions) {
        if (global_func->name == astr) {
            return true; }
    }

    return false;
}

static bool is_table_name(const std_and_astring_variant& name) {

    astring astr;
    std::string stdstr;
    VISIT(name, unwrapped,
        astr = unwrapped;
        stdstr = unwrapped;
    );

    for (const auto& tab_name : table_names) {
        if (tab_name == stdstr) {
            return true; }
    }

    for (const auto& tab : global_tables) {
        if (tab->table_name == astr) {
            return true; }
    }

    return false;
}

void parser_init(std::vector<token> toks, std::vector<evaluated_function_object*> global_funcs, std::vector<table_object*> global_tabs) {
    global_functions = global_funcs;
    global_tables = global_tabs;

    tokens = toks;
    token_position = 0;
    prev_token = {ERROR_TOKEN, std::string("garbage"), 0, 0};
    g_loop_count = 0;
    function_names.clear();
    table_names.clear();
    nodes.clear();
}


// it token has parse function us it, else error
// underscores are allowed in words
std::vector<node*> parse() {
    if (tokens.size() == 0) {
        errors.emplace_back("parse(): No tokens");
        return nodes;}

    switch (peek_type()) {
    case CREATE:
        parse_create(); break;
    case INSERT: {

        object* result = parse_insert();
        switch(result->type()) {
        case INSERT_INTO_OBJECT:
            nodes.emplace_back(new insert_into(result)); break;
        case ERROR_OBJ:
            errors.emplace_back("parse_insert(): Returned error object"); break;
        default:
            errors.emplace_back("parse_insert(): Returned unknown object type");
        } 
        
    } break;
    case SELECT: {

        object* result = parse_select();

        if (result->type() == ERROR_OBJ) {
            errors.emplace_back("parse_select(): Returned error object"); break;
        }
        // Advance past semicolon
        if (peek_type() != SEMICOLON) {
            errors.emplace_back("parse_select(): Missing ending semicolon, instead got " + token_type_to_string(peek_type()));
        } else {
            token_position++;
        }

        switch(result->type()) {
        case SELECT_OBJECT:
            nodes.emplace_back(new select_node(result)); break;
        case SELECT_FROM_OBJECT:
            nodes.emplace_back(new select_from(result)); break;
        default:
            errors.emplace_back("parse_select(): Returned unknown object type");
        } 
         
    } break;
    case ALTER:
        parse_alter(); break;
    // Custom
    case ASSERT: {
        object* result = parse_assert();

        if (result->type() != ASSERT_OBJ) {
            errors.emplace_back("parse_assert(): Returned bad object!"); break; }
            
        // Advance past semicolon
        if (peek_type() != SEMICOLON) {
            errors.emplace_back("parse_assert(): Missing ending semicolon, instead got " + token_type_to_string(peek_type()));
        } else {
            token_position++;
        }


        nodes.emplace_back(new assert_node(static_cast<assert_object*>(result)));
    } break;
    default:
        std::string error = "Unknown keyword or inappropriate usage (" + peek_data() +  ") Token type = "
        + token_type_to_string(peek().type) + ". Line = " + std::to_string(peek().line) 
        + ". Position = " + std::to_string(peek().position);
        errors.emplace_back(error);
    }

    if (!errors.empty()) {
        // Look for end of statement ';', if it's there, go to it then continue looking for errors in the next statement
        while (token_position < tokens.size() && peek_type() != SEMICOLON) {
            token_position++;
        }
        token_position++; // If at semicolon, advance past it
    }

    if (g_loop_count++ > 100) {
        std::cout << "le stuck in loop?";}

    if (peek_type() != LINE_END) {
        parse();
    }

    return nodes;
}


static object* parse_assert() {

    if (peek_type() != ASSERT) {
        push_error_return_error_object("parse_assert(): with non-VALUES token"); }

    size_t line = peek().line;

    advance_and_check_ret_obj("Nothing after ASSERT");

    object* expr = parse_expression(LOWEST);
    if (expr->type() == ERROR_OBJ) {
        push_error_return_error_object("Failed to parse ASSERT expression"); }

    return new assert_object(expr, line);
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

    object* table_name = parse_expression(LOWEST);
    if (table_name->type() == ERROR_OBJ) {
        push_error_return("Failed to get table name"); }

    if (peek_type() != ADD) {
        push_error_return("ALTER TABLE, only supports ADD");}

    advance_and_check("No tokens after ADD");

    if (peek_type() != COLUMN) {
        push_error_return("ALTER TABLE, only supports ADD COLUMN");}

    advance_and_check("No tokens after ADD COLUMN");

    object* col_obj = parse_expression(LOWEST);
    if (col_obj->type() == ERROR_OBJ) {
        return; }

    column_object* col = new column_object(col_obj);

    if (peek_type() == DEFAULT) {
        object* default_value = parse_expression(LOWEST);
        if (default_value->type() == ERROR_OBJ) {
            return; }

        col->default_value = default_value;
    }

    if (peek_type() != SEMICOLON) {
        push_error_return("Missing ending semicolon");}

    token_position++;

    alter_table* info = new alter_table(table_name, col);

    nodes.push_back(info);
}

static object* parse_values() {

    if (peek_type() != VALUES) {
        push_error_return_error_object("parse_values(): with non-VALUES TOKEN!!!!!!!!!!!!");
    }

    advance_and_check_ret_obj("Missing open parenthesis after VALUES");

    if (peek_type() != OPEN_PAREN) {
        push_error_return_error_object("Unknown token after VALUES, should be open parenthesis");}

    advance_and_check_ret_obj("Missing values after open parenthesis");

    // Find values
    object* values_obj = parse_comma_seperated_list(CLOSE_PAREN);
    if (values_obj->type() != GROUP_OBJ) {
        push_error_return_error_object("Failed to parse VALUES list"); }
        
    if (peek_type() != CLOSE_PAREN) {
        push_error_return_error_object("VALUES, missing closing parenthesis");}

    advance_and_check_ret_obj("No values after closing parenthesis");

    return values_obj;
}

// inserts rows
static object* parse_insert() {
    if (peek_type() != INSERT) {
        std::cout << "parse_insert() called with non-INSERT token";
        exit(1);}

    advance_and_check_ret_obj("No tokens after INSERT");

    if (peek_type() != INTO) {
        push_error_return_error_object("Unknown INSERT usage"); }

    advance_and_check_ret_obj("No tokens after INSERT INTO");

    if (peek_type() != STRING_LITERAL) {
        push_error_return_error_object("INSERT INTO bad token type for table name");}

    object* table_name = parse_expression(HIGHEST);
    if (table_name->type() != STRING_OBJ) { // For now must be string
        push_error_return_error_object("INSERT INTO: Failed to parse table name"); }

    if (peek_type() != OPEN_PAREN) {
        push_error_return_error_object("No open parenthesis after INSERT INTO table");}

    advance_and_check_ret_obj("No tokens after INSERT INTO table (");

    // Find field names
    object* fields_obj = parse_comma_seperated_list(CLOSE_PAREN);
    if (fields_obj->type() != GROUP_OBJ) {
        push_error_return_error_object("INSERT INTO: Failed to parse field names"); }
        
    if (peek_type() != CLOSE_PAREN) {
        push_error_return_error_object("Field names missing closing parenthesis");}

    avec<object*> fields = static_cast<group_object*>(fields_obj)->elements;

    advance_and_check_ret_obj("INSERT INTO table, missing VALUES");

    object* values = parse_expression(LOWEST);

    if (peek_type() != SEMICOLON) {
        push_error_return_error_object("INSERT INTO missing closing semicolon");}

    token_position++;

    insert_into_object* info = new insert_into_object(table_name, fields, values);

    return info;
}

#define advance_and_check_list(x)            \
    prev_token = tokens[token_position];    \
    token_position++;                       \
    if (token_position >= tokens.size()) {  \
        push_error_return_list(x);}     

#define push_error_return_list(x)                    \
    token cur_tok;                           \
    if (token_position >= tokens.size()) {  \
        if (tokens.size() > 0 && token_position >= tokens.size()) { \
            cur_tok = tokens[tokens.size() - 1]; \
            cur_tok.position += cur_tok.data.size(); \
        } else {                            \
            cur_tok.type = LINE_END; cur_tok.data = ""; cur_tok.line = SIZE_T_MAX; cur_tok.position = SIZE_T_MAX; \
        }                                   \
    } else {                                \
        cur_tok = tokens[token_position];}   \
    std::string error = x;                  \
    error = error + ". Line = " + std::to_string(cur_tok.line) + ", position = " + std::to_string(cur_tok.position);\
    errors.push_back(error);                \
    std::vector<object*> errored_list;      \
    return errored_list;

#define advance_and_check_pair_list(list, x)      \
    prev_token = tokens[token_position];    \
    token_position++;                       \
    if (token_position >= tokens.size()) {  \
        push_error_return_pair_list(list, x);}     

#define push_error_return_pair_list(list, x)        \
    token cur_tok;                                  \
    if (token_position >= tokens.size()) {          \
        if (tokens.size() > 0 && token_position >= tokens.size()) { \
            cur_tok = tokens[tokens.size() - 1];   \
            cur_tok.position += cur_tok.data.size();\
        } else {                                    \
            cur_tok.type = LINE_END; cur_tok.data = ""; cur_tok.line = SIZE_T_MAX; cur_tok.position = SIZE_T_MAX;  \
        }                                           \
    } else {                                        \
        cur_tok = tokens[token_position];}          \
    std::string error = x;                          \
    error = error + ". Line = " + std::to_string(cur_tok.line) + ", position = " + std::to_string(cur_tok.position);\
    errors.push_back(std::move(error));                        \
    std::pair<object*, token_type> errored_list = {new group_object(list), peek_type()}; \
    return errored_list;

// Starts on first value, end on end val, returns which value it snded on
static std::pair<object*, token_type> parse_comma_seperated_list_ADVANCED(const avec<token_type>& end_values) {

    avec<object*> list;

    if (std::ranges::find(end_values, peek_type()) != end_values.end()) {
        return {new group_object(list), peek_type()}; }

    size_t loop_count = 0;
    while (loop_count++ < 100) {
        object* cur = parse_expression(LOWEST);
        if (cur->type() == ERROR_OBJ) {
            return {cur, peek_type()}; }

        list.push_back(cur);
        
        if (std::ranges::find(end_values, peek_type()) != end_values.end()) {
            break;
        }
        
        if (peek_type() != COMMA) {
            push_error_return_pair_list(list, "Items in list must be comma seperated, got (" + token_type_to_string(peek_type()) + ")");
        }

        advance_and_check_pair_list(list, "parse_comma_seperated_list(): No values after comma");
    }

    if (loop_count >= 100) {
        push_error_return_pair_list(list ,"Too many loops during comma seperated list traversal, likely weird error");}

    return {new group_object(list), peek_type()};
}



// Starts on first value, end on end val
static object* parse_comma_seperated_list(token_type end_val) {

    avec<object*> list;

    if (peek_type() == end_val) {
        return new group_object(list); }

    size_t loop_count = 0;
    while (loop_count++ < 100) {
        object* cur = parse_expression(LOWEST);
        if (cur->type() == ERROR_OBJ) {
            return cur; }

        list.push_back(cur);
        
        if (peek_type() == end_val) {
            break; }
        
        if (peek_type() != COMMA) {
            push_error_return_error_object("Items in list must be comma seperated, got (" + token_type_to_string(peek_type()) + ")");}

        advance_and_check_ret_obj("parse_comma_seperated_list(): No values after comma");
    }

    if (loop_count >= 100) {
        push_error_return_error_object("Too many loops during comma seperated list traversal, likely weird error");}

    return new group_object(list);
}

// Starts on first value, end on end val
static object* parse_comma_seperated_list_end_if_not_comma() {

    avec<object*> list;

    size_t loop_count = 0;
    while (loop_count++ < 100) {
        object* cur = parse_expression(LOWEST);
        if (cur->type() == ERROR_OBJ) {
            return cur; }

        list.push_back(cur);
        
        if (peek_type() != COMMA) {
            break; }

        advance_and_check_ret_obj("parse_comma_seperated_list(): No values after comma");
    }

    if (loop_count >= 100) {
        push_error_return_error_object("Too many loops during comma seperated list traversal, likely weird error");}

    return new group_object(list);
}






bool is_data_type_token(token tok) {
    switch (tok.type) {
    case CHAR: case VARCHAR: case BOOL: case BOOLEAN: case DATE: case YEAR: case SET: case BIT: case INT: case INTEGER: case FLOAT: case DOUBLE: case NONE: case UNSIGNED: case ZEROFILL:
    case TINYBLOB: case TINYTEXT: case MEDIUMTEXT: case MEDIUMBLOB: case LONGTEXT: case LONGBLOB: case DEC: case DECIMAL:
        return true;
        break;
    default:
        return false;
    }
}


static object* parse_select() {
    if (peek_type() != SELECT) {
        std::cout << "parse_select() called with non-select token";
        exit(1);}

    advance_and_check_ret_obj("No tokens after SELECT")

    const avec<token_type> end_types = {FROM, SEMICOLON, CLOSE_PAREN};  //!!MAJOR maybe make global open paren count and return error if count is wrong
    const auto& [selector_group, end_type] = parse_comma_seperated_list_ADVANCED(end_types);
    if (selector_group->type() != GROUP_OBJ) {
        push_error_return_error_object("SELECT: Failed to parse SELECT column indexes"); }

    if (auto it = std::ranges::find(end_types, end_type); it == end_types.end()) {
        push_error_return_error_object("SELECT: Failed to parse SELECT column indexes, strange end type (" + token_type_to_string(end_type) + ")"); }
    

    avec<object*> selectors = static_cast<group_object*>(selector_group)->elements;

    if (selectors.size() == 0) {
        push_error_return_error_object("No values after SELECT");}
    
    if (peek_type() != FROM) {

        if (selectors.size() != 1) {
            push_error_return_error_object("SELECT (column index): Can only take in one value"); }

        object* selector = selectors[0];
        
        select_object* info = new select_object(selector);

        return info;
    }

    if (end_type != FROM) {
            push_error_return_error_object("SELECT FROM: Failed to end on FROM"); }

    advance_and_check_ret_obj("No values after FROM");

    if (selectors[0]->data() == "*") {
        if (selectors.size() > 1) {
            push_error_return_error_object("Too many entries in list after *");}
        selectors[0] = new star_object();
    }

    // Get condition chain
    avec<object*> clause_chain;
    while (peek_type() != SEMICOLON && peek_type() != LINE_END && peek_type() != CLOSE_PAREN) { //!!MAJOR don't like this close paren check, would prefer to not even have to checks

        object* clause = parse_expression(LOWEST);
        if (clause->type() != INFIX_EXPRESSION_OBJ && clause->type() != PREFIX_EXPRESSION_OBJ && clause->type() != STRING_OBJ)  {
            push_error_return_error_object("Expected clause in SELECT FROM, got (" + clause->inspect() + ")");}

        clause_chain.push_back(clause);

    }

    select_from_object* info = new select_from_object(selectors, clause_chain);
    
    return info;

}

static void parse_create() {
    if (peek_type() != CREATE) {
        std::cout << "parse_create() called with non-create token";
        exit(1);}

    advance_and_check("No tokens after CREATE");
    
    switch(peek_type()) {
    case TABLE: 
        parse_create_table(); break;
    case OR:
        advance_and_check("No tokens after OR");
        if (peek_type() != REPLACE) {
            push_error_return("No REPLACE after OR");}

        advance_and_check("No tokens after REPLACE");
        if (peek_type() != FUNCTION) {
            push_error_return("No FUNCTION after REPLACE");}

        parse_create_or_replace_function(); break;
    default:
        push_error_return("Unknown keyword (" + peek_data() + ") after CREATE");
    }
}

static void parse_create_or_replace_function() {
    if (peek_type() != FUNCTION) {
        std::cout << "parse_create_or_replace_function() called with non-FUNCTION token";
        exit(1);}

    advance_and_check("No tokens after FUNCTION");

     // Function name must be string literal, otherwise too much nonsense
    if (peek_type() != STRING_LITERAL) {
        push_error_return("Function name must be string literal"); }

    std::string name = peek().data;

    advance_and_check("No tokens after function name");

    if (peek_type() != OPEN_PAREN) {
        push_error_return("No parenthesis in function declaration"); }

    advance_and_check("No tokens after parenthesis");

    object* parameters = parse_comma_seperated_list(CLOSE_PAREN);
    if (parameters->type() != GROUP_OBJ) {
        push_error_return("Failed to parse parameters"); }
    
    advance_and_check("No tokens after parameters");

    if (peek_type() != RETURNS) {
        push_error_return("No RETURNS in function declaration"); }

    advance_and_check("No tokens after RETURNS");

    // For now, no nonsense in function delcarations
    object* return_type = parse_expression(LOWEST);
    if (return_type->type() != SQL_DATA_TYPE_OBJ) {
        push_error_return("Invalid return type");}

    object* func_body = parse_expression(LOWEST);
    if (func_body->type() != BLOCK_STATEMENT) {
        push_error_return("Failed to parse function body");}

    function_object* func = new function_object(name, static_cast<group_object*>(parameters), static_cast<SQL_data_type_object*>(return_type), static_cast<block_statement*>(func_body));

    function* info = new function(func);

    bool found = false;
    for (const auto& func_name : function_names) {
        if (func_name == name) {
            found = true; }
    }

    if (!found) {
        function_names.push_back(name); }
    
    nodes.push_back(info);
}


// tokens should point to TABLE
static void parse_create_table() {
    if (peek_type() != TABLE) {
        std::cout << "parse_create_table() called with non-create_table token";
        exit(1);}
        
    advance_and_check("No tokens after CREATE TABLE");

    object* table_name = parse_expression(HIGHEST);
    if (table_name->type() != STRING_OBJ) {
        push_error_return("Failed to parse table name"); }

    if (peek_type() != OPEN_PAREN) {
        push_error_return("No open paren '(' after CREATE TABLE");}
    
    advance_and_check("No data in CREATE TABLE");

    avec<table_detail_object*> details;

    size_t max_loops = 0;
    while (max_loops++ < 100) {
        object* name = parse_expression(LOWEST);
        if (name->type() == ERROR_OBJ) {
            push_error_return("CREATE TABLE: Failed to parse column name"); }

        object* data_type; // Hmmmmmmmmmm, not the greatest
        if (name->type() == PARAMETER_OBJ) {
            parameter_object* param = static_cast<parameter_object*>(name);
            if (param->data_type->type() == ERROR_OBJ) {
                return; }

            data_type = param->data_type;
            name = new string_object(param->name);
        } else {
            data_type = parse_expression(LOWEST);
            if (data_type->type() == ERROR_OBJ) {
                push_error_return("CREATE TABLE: Failed to parse column data type"); }
        }


        object* default_value = new null_object();
        if (peek_type() == DEFAULT) {
            advance_and_check("CREATE TABLE: No values after DEFAULT");
            default_value = parse_expression(LOWEST);
            if (default_value->type() == ERROR_OBJ) {
                push_error_return("CREATE TABLE: Failed to parse column default value"); } 
        }

        if (name->type() != STRING_OBJ) {
            push_error_return("CREATE TABLE: Name is not string"); }

        if (data_type->type() != SQL_DATA_TYPE_OBJ) {
             push_error_return("CREATE TABLE: Couldn't find data type"); }

        table_detail_object* detail = new table_detail_object(name->data(), static_cast<SQL_data_type_object*>(data_type), default_value);

        details.push_back(detail);

        if (peek_type() == CLOSE_PAREN) {
            break; }

        if (peek_type() != COMMA) {
            push_error_return("CREATE TABLE: Table details not seperated by comma"); }
        
        advance_and_check("CREATE TABLE: Table details missing closing );")
        
    }

    if (max_loops >= 100) {
        push_error_return("parse_create_table(): Too many loops in field search, bug or over 100 fields");}

    advance_and_check("No closing ';' at end of CREATE TABLE");

    if (peek_type() != SEMICOLON) {
        push_error_return("No closing ';' at end of CREATE TABLE");}

    token_position++;

    create_table* info = new create_table(table_name, details);   

    nodes.push_back(info);

    table_names.push_back(std::string(table_name->data()));
}



size_t numeric_precedence(token tok) {
    switch (tok.type) {
        case AS:            return 1; break; // !!MAJOR not sure
        case WHERE:         return 1; break; // !!MAJOR not sure
        case LEFT:          return 1; break; // !!MAJOR not sure
        case EQUAL:         return 2; break;
        case NOT_EQUAL:     return 2; break;
        case LESS_THAN:     return 3; break;
        case GREATER_THAN:  return 3; break;
        case PLUS:          return 4; break;
        case MINUS:         return 4; break;
        case ASTERISK:      return 5; break;
        case SLASH:         return 5; break;
        case OPEN_PAREN:    return 7; break;
        case OPEN_BRACKET:  return 8; break;
        default:
            return LOWEST;
    }
}

size_t numeric_precedence(operator_object* op) {
    switch (op->op_type) {
        case EQUALS_OP:         return 2; break;
        case NOT_EQUALS_OP:     return 2; break;
        case LESS_THAN_OP:      return 3; break;
        case GREATER_THAN_OP:   return 3; break;
        case ADD_OP:            return 4; break;
        case SUB_OP:            return 4; break;
        case MUL_OP:            return 5; break;
        case DIV_OP:            return 5; break;
        case OPEN_PAREN_OP:     return 7; break;
        case OPEN_BRACKET_OP:   return 8; break;
        default:
            return LOWEST;
    }
}

object* parse_prefix_as() {

    if (peek_type() != AS) {
        push_error_return_error_object("parse_prefix_as() called without AS token"); }

    advance_and_check_ret_obj("No right expression for prefix AS");

    if (peek_type() == $$) {
        return parse_function();
    } else {
        push_error_return_error_object("AS can only be prefix for $$ in a function (for now)"); }
}


object* parse_prefix_if() {

    token starter_token = peek();

    if (peek_type() != IF && peek_type() != ELSIF) {
        push_error_return_error_object("parse_prefix_if() called without IF/ELSIF token"); }

    advance_and_check_ret_obj("No tokens after IF");

    // CONDITION
    object* condition = parse_expression(LOWEST); // LOWEST or PREFIX??
    if (condition->type() == ERROR_OBJ) {
        return condition; }

    if (peek_type() != THEN) {
        push_error_return_error_object("IF statement missing THEN"); }

    advance_and_check_ret_obj("No tokens after THEN");

    // BODY
    avec<object*> body;

    if (peek_type() == BEGIN) {
        size_t max_loops = 0;
        while (max_loops++ < 100) {
            object* expression = parse_expression(LOWEST);
            if (expression->type() == ERROR_OBJ) {
                return expression; }
            
            if (peek_type() == END) {
                break; }
        }

        if (max_loops >= 100) {
            push_error_return_error_object("Too many loops during IF statement parsing"); }

    } else {
        object* expression = parse_expression(LOWEST);
        if (expression->type() == ERROR_OBJ) {
            return expression; }

        body.push_back(expression);
    }

    if_statement* if_stmnt = new if_statement(condition, new block_statement(body), new null_object());


    // ELSIF
    if_statement* prev = if_stmnt;
    
    size_t max_loops = 0;
    while (peek_type() == ELSIF && max_loops++ < 100) { 

        object* statement = parse_expression(LOWEST);
        if (statement->type() == ERROR_OBJ) {
            return statement; }

        if (statement->type() != IF_STATEMENT) {
            push_error_return_error_object("ELSIF statement evaluated to non-IF statement"); }

        prev->other = statement;
        prev = static_cast<if_statement*>(statement);
    }

    if (max_loops >= 100) {
        push_error_return_error_object("Too many loops during ELSIF statement parsing"); }

    // ELSE
    if (peek_type() == ELSE) {
        object* statement = parse_expression(LOWEST);
        if (statement->type() == ERROR_OBJ) {
            return statement; }

        if (statement->type() != BLOCK_STATEMENT) {
            push_error_return_error_object("ELSE statement evaluated to non-BLOCK statement"); }

        prev->other = statement;
    }

    if (starter_token.type == IF) {
        object* endif = parse_expression(LOWEST);
        if (endif->type() == ERROR_OBJ) {
            return endif; }
            
        if (endif->type() != END_IF_STATEMENT) {
            push_error_return_error_object("No END IF in " + token_type_to_string(starter_token.type) + " statement, got (" + std::string(endif->inspect()) + ")"); }
        
    }

    return if_stmnt;
}

object* parse_block_statement() {

    if (peek_type() != ELSE) {
        push_error_return_error_object("parse_block_statement() called with non-ELSE token"); }

    advance_and_check_ret_obj("No tokens after THEN");

    
    // BODY
    avec<object*> body;

    if (peek_type() == BEGIN) {
        size_t max_loops = 0;
        while (max_loops++ < 100) {
            object* expression = parse_expression(LOWEST);
            if (expression->type() == ERROR_OBJ) {
                return expression; }
            
            if (peek_type() == END) {
                break; }
        }

        if (max_loops >= 100) {
            push_error_return_error_object("Too many loops during IF statement parsing"); }

    } else {
        object* expression = parse_expression(LOWEST);
        if (expression->type() == ERROR_OBJ) {
            return expression; }

        body.push_back(expression);
    }

    return new block_statement(body);
}

object* parse_function() {
    
    if (peek_type() != $$) {
        push_error_return_error_object("parse_function() called with non-$$ token"); }

    advance_and_check_ret_obj("No tokens after $$");

    if (peek_type() != BEGIN) {
        push_error_return_error_object("No BEGIN in function"); }

    advance_and_check_ret_obj("No tokens after BEGIN");

    avec<object*> statement_list;
    size_t max_loops = 0;
    while (max_loops++ < 100) {
        object* statement = parse_expression(LOWEST);
        if (statement->type() == ERROR_OBJ) {
            return statement; }

        if (statement->type() == END_STATEMENT) {
            advance_and_check_ret_obj("No values after END");
            if (peek_type() != SEMICOLON) {
                push_error_return_error_object("No semicolon after END"); }
            break; 
        } 

        statement_list.push_back(statement);
    }

    if (max_loops >= 100) {
        push_error_return_error_object("Too many loops during function parse"); }

    block_statement* func = new block_statement(statement_list);

    if (peek_type() == $$) {
        advance_and_check_ret_obj("No value after closing $$"); }

    if (peek_type() != SEMICOLON) {
        push_error_return_error_object("Function declaration missing closing semicolon"); }

    token_position++;

    return func;
}


object* parse_infix_operator(object* left) {

    operator_type op;
    switch (peek_type()) {
    case PLUS:
        op = ADD_OP; break;
    case MINUS:
        op = SUB_OP; break;
    case ASTERISK:
        op = MUL_OP; break;
    case SLASH:
        op = DIV_OP; break;
    case DOT:
        op = DOT_OP; break;
    case EQUAL:
        op = EQUALS_OP; break;
    case NOT_EQUAL: // I guess the lexer handles combing ! and =
        op = NOT_EQUALS_OP; break;
    case LESS_THAN:
        // or equal not handled for now cause lazy
        op = LESS_THAN_OP; break;
    case GREATER_THAN:
        // or equal not handled for now cause lazy
        op = GREATER_THAN_OP; break;
    default:
        push_error_return_error_object("Unknown infix operator (" + token_type_to_string(peek_type()) + ")");
    }

    operator_object* op_obj = new operator_object(op);
    
    size_t precedence = numeric_precedence(op_obj);

    advance_and_check_ret_obj("No values after operator (" + op_obj->inspect() + ")");
    
    object* right = parse_expression(precedence);
    if (right->type() == ERROR_OBJ) {
        return right;}
        
    infix_expression_object* expression = new infix_expression_object(op_obj, left, right);

    return expression;

}


static object* parse_data_type(token tok) {

    token_type prefix = NONE;
    token_type data_type = NONE;
    object* parameter = new null_object();

    bool unsign = false;
    bool zerofill = false;
    zerofill = zerofill; // compiler is annoying

    if (token_position >= tokens.size()) {
        push_error_return_error_object("No data type token");}  
    
    if (tok.type == UNSIGNED) {
        prefix = UNSIGNED;
        unsign = true;
        advance_and_check_ret_obj("No data type after UNSIGNED");
        tok = peek();
    } else if (tok.type == ZEROFILL) {
        prefix = ZEROFILL;
        unsign = true;
        zerofill = true;
        advance_and_check_ret_obj("No data type after ZEROFILL");
        tok = peek();
    }

    if (tok.type == STRING_LITERAL) {
        std::string msg = "Data type has bad token type (" + token_type_to_string(peek_type()) + ")\n";
        push_error_return_error_object(msg);}

    token_type type = tok.type;
    advance_and_check_ret_obj("parse_data_type(): Nothing after data type");

    // Basic string
    switch(type) {
    case TINYBLOB: case TINYTEXT: case MEDIUMTEXT: case MEDIUMBLOB: case LONGTEXT: case LONGBLOB:
        data_type = CHAR;
        parameter = new integer_object(255);
        return new SQL_data_type_object(prefix, data_type, parameter);
    default:
        break;
    }

    if (type == VARCHAR) { 
        //advance_and_check_ret_str("parse_data_type(): VARCHAR missing length");

        if (peek_type() != OPEN_PAREN) {
            data_type = VARCHAR;
            parameter = new integer_object(255);
            return new SQL_data_type_object(prefix, data_type, parameter);
            //push_error_return_error_object("parse_data_type(): VARCHAR missing open parenthesis");
        }

        advance_and_check_ret_obj("parse_data_type(): VARCHAR missing length");


        object* obj = parse_expression(LOWEST);
        if (obj->type() == ERROR_OBJ) {
            return obj; }

        if (peek_type() != CLOSE_PAREN) {
            push_error_return_error_object("parse_data_type(): VARCHAR missing closing parenthesis");}

        advance_and_check_ret_obj("parse_data_type(): VARCHAR nothing after closing parenthesis");

        data_type = VARCHAR;
        parameter = obj;
        return new SQL_data_type_object(prefix, data_type, parameter);
    }
    
    // Basic numeric
    if (type == BOOL) {
        data_type = BOOL;
        return new SQL_data_type_object(prefix, data_type, parameter);
    } else if (type == BOOLEAN) {
        data_type = BOOL;
        return new SQL_data_type_object(prefix, data_type, parameter);
    }

    // Basic time
    if (type == DATE) {
        data_type = DATE;
        return new SQL_data_type_object(prefix, data_type, parameter);
    } else if (type == YEAR) {
        data_type = YEAR;
        return new SQL_data_type_object(prefix, data_type, parameter);
    }

    // String
    if (type == SET) {
        if (peek_type() == OPEN_PAREN) {
            advance_and_check_ret_obj("parse_data_type(): ");
            if (peek_type() == CLOSE_PAREN) {
                advance_and_check_ret_obj("parse_data_type(): ");
                data_type = SET;
                return new SQL_data_type_object(prefix, data_type, parameter);
            }

            object* elements = parse_comma_seperated_list(CLOSE_PAREN);
            if (elements->type() != GROUP_OBJ) {
                push_error_return_error_object("Failed to parse SET elements"); }

            if (peek_type() != CLOSE_PAREN) {
                push_error_return_error_object("SET missing closing parenthesis");}
            
            data_type = SET;
            parameter = elements;
            return new SQL_data_type_object(prefix, data_type, parameter);
        } else {
            data_type = SET;
            return new SQL_data_type_object(prefix, data_type, parameter);
        }
    }

    // Numeric
    if (type == BIT) {
        if (peek_type() == OPEN_PAREN) {
            advance_and_check_ret_obj("parse_data_type(): ");
            if (peek_type() == CLOSE_PAREN) {
                advance_and_check_ret_obj("parse_data_type(): ");
                data_type = BIT;
                parameter = new integer_object(1);
                return new SQL_data_type_object(prefix, data_type, parameter);
            }

            int num = 1;
            if (peek_type() != INTEGER_LITERAL) {
                if (peek_type() != LINE_END) {
                    errors.emplace_back("BIT contained non-integer");}
            } else {
                if (tokens[token_position].data.length() > 2) {
                    errors.emplace_back("BIT cannot be greated than 64");
                } else {
                    num = std::stoi(tokens[token_position].data);
                    if (num < 1) {
                        errors.emplace_back("BIT cannot be less than 1");}
                    if (num > 64) {
                        errors.emplace_back("BIT cannot be greated than 64");}
                }
            }
            advance_and_check_ret_obj("parse_data_type(): ");

            if (peek_type() != CLOSE_PAREN) {
                errors.emplace_back("BIT missing clossing parenthesis");}
                advance_and_check_ret_obj("parse_data_type(): ");

            data_type = BIT;
            parameter = new integer_object(num);
            return new SQL_data_type_object(prefix, data_type, parameter);
        } else {
            data_type = BIT;
            parameter = new integer_object(1);
            return new SQL_data_type_object(prefix, data_type, parameter);
        }
        
    } else if (type == INT || type == INTEGER) {
        // The number is the display size, can ignore in computations for now. I read the default is 11 for signed and 10 for unsigned but I'm not sure it's standard so...
       if (peek_type() == OPEN_PAREN) {
            advance_and_check_ret_obj("parse_data_type(): ");
            if (peek_type() == CLOSE_PAREN) {
                advance_and_check_ret_obj("parse_data_type(): ");
                if (unsign) {
                    data_type = INT;
                    parameter = new integer_object(10);
                    return new SQL_data_type_object(prefix, data_type, parameter);
                } else {
                    data_type = INT;
                    parameter = new integer_object(11);
                    return new SQL_data_type_object(prefix, data_type, parameter);
                }
            }

            int num = 11;
            if (peek_type() != INTEGER_LITERAL) {
                errors.emplace_back("INT contained non-integer");
            } else {
                if (tokens[token_position].data.length() > 3) {
                    errors.emplace_back("INT cannot be greated than 255");
                } else {
                    num = std::stoi(tokens[token_position].data);
                    if (num < 1) {
                        errors.emplace_back("INT cannot be less than 1");}
                    if (num > 255) {
                        errors.emplace_back("INT cannot be greated than 255");}
                }
            }
            advance_and_check_ret_obj("parse_data_type(): ");

            if (peek_type() != CLOSE_PAREN) {
                push_error_return_error_object("INT missing clossing parenthesis");}
            advance_and_check_ret_obj("parse_data_type(): ");
            
        } else {
            if (unsign) {
                data_type = INT;
                parameter = new integer_object(10);
                return new SQL_data_type_object(prefix, data_type, parameter);
            } else {
            data_type = INT;
            parameter = new integer_object(11);
            return new SQL_data_type_object(prefix, data_type, parameter);
            }
        }

    } else if (type == DEC || type == DECIMAL) {
        push_error_return_error_object_prev_tok("DECIMAL/DEC not supported, too complicated");
    } else if (type == FLOAT) {
        data_type = FLOAT;
        return new SQL_data_type_object(prefix, data_type, parameter);
    }else if (type == DOUBLE) {
        data_type = DOUBLE;
        return new SQL_data_type_object(prefix, data_type, parameter);
    }

    push_error_return_error_object("Unknown SQL data type");
}

object* parse_string_literal(token tok) {
    advance_and_check_ret_obj("No values after string prefix");

    if (tok.data == "*") {
        return new star_object(); }

    bool is_func = is_function_name(tok.data);
    if (is_func) {
        if (peek_type() != OPEN_PAREN) {
            push_error_return_error_object("Function missing open parenthesis"); }

        advance_and_check_ret_obj("No values after open paren in function call");

        object* arguments = parse_comma_seperated_list(CLOSE_PAREN);
        if (arguments->type() != GROUP_OBJ) {
            push_error_return_error_object("Failed to parse function call arguments"); }

        advance_and_check_ret_obj("No values after closing parenthesis");
        return new function_call_object(tok.data, static_cast<group_object*>(arguments));
    }

    if (peek_type() == DOT) {
        if (!is_table_name(tok.data)) {
            push_error_return_error_object("table.column: Table (" + tok.data + ") does not exist"); }

        advance_and_check_ret_obj("No values after dot");

        object* column_name_obj = parse_expression(HIGHEST);
        if (column_name_obj->type() != STRING_OBJ) {
            push_error_return_error_object("table.column: Failed to parse column"); }

        column_index_object* obj = new column_index_object(new string_object(tok.data), column_name_obj);
        return obj;
    }

    if (is_sql_data_type_token(peek())) {
        object* obj = parse_expression(LOWEST);
        if (obj->type() == ERROR_OBJ) {
            return obj; }
        
        if (obj->type() != SQL_DATA_TYPE_OBJ) {
            push_error_return_error_object("For now parameters must be a string literal followed by an SQL data type, can make generic later"); }
        
        return new parameter_object(tok.data, static_cast<SQL_data_type_object*>(obj));
    }

    return new string_object(tok.data); 
}

object* prefix_parse_functions_with_token(token tok) {
    switch (tok.type) {
    case CHAR: case VARCHAR: case BOOL: case BOOLEAN: case DATE: case YEAR: case SET: case BIT: case INT: case INTEGER: case FLOAT: case DOUBLE: case NONE: 
    case UNSIGNED: case ZEROFILL: case TINYBLOB: case TINYTEXT: case MEDIUMTEXT: case MEDIUMBLOB: case LONGTEXT: case LONGBLOB: case DEC: case DECIMAL:
        return parse_data_type(tok); break;

    // Built-in functions
    case COUNT: {
        advance_and_check_ret_obj("No values after COUNT");

        if (peek_type() != OPEN_PAREN) {
            push_error_return_error_object("Function missing open parenthesis"); }

        advance_and_check_ret_obj("No values after open paren in function call");

        object* arguments = parse_comma_seperated_list(CLOSE_PAREN);
        if (arguments->type() != GROUP_OBJ) {
            push_error_return_error_object("Failed to parse function call arguments"); }

        advance_and_check_ret_obj("No values after closing parenthesis");
        return new function_call_object(tok.data, static_cast<group_object*>(arguments));
    } break;
    // Built-in functions end

    // SELECT clauses
    case LEFT: {
        advance_and_check_ret_obj("No values after LEFT");

        if (peek_type() != JOIN) {
            push_error_return_error_object("LEFT must be followed by JOIN"); }
        
        advance_and_check_ret_obj("No values after JOIN");

        object* left_table_name = parse_expression(LOWEST);
        if (left_table_name->type() != STRING_OBJ) {
            push_error_return_error_object("LEFT JOIN: Failed to parse left table name"); }

        if (peek_type() != ON) {
            push_error_return_error_object("LEFT JOIN: Only supports ON for now"); }

        advance_and_check_ret_obj("No values after ON");

        object* right_expression = parse_expression(SQL_STATEMENT); // SOMETHING SUSSY HERE!!!!!!!!!!!!!!!!!!!!
        if (right_expression->type() != INFIX_EXPRESSION_OBJ) {
            errors.push_back(std::string(right_expression->data()));
            push_error_return_error_object("LEFT JOIN: Failed to parse right expression"); 
        }

        infix_expression_object* inner = new infix_expression_object(new operator_object(ON_OP), left_table_name, right_expression);

        return new prefix_expression_object(new operator_object(LEFT_JOIN_OP), inner);
    }
    case GROUP: {
        advance_and_check_ret_obj("No values after GROUP");

        if (peek_type() != BY) {
            push_error_return_error_object("GROUP must be followed by BY"); }
        
        advance_and_check_ret_obj("No values after BY");

        object* column_indexes_obj = parse_comma_seperated_list_end_if_not_comma();
        if (column_indexes_obj->type() != GROUP_OBJ) {
            push_error_return_error_object("GROUP BY: failed to get column indexes"); }

        // Can't check if they are specifically Column Index Objects cause they can be aliases, bruh

        return new prefix_expression_object(new operator_object(GROUP_BY_OP), column_indexes_obj);
    }
    case ORDER: {
        advance_and_check_ret_obj("No values after ORDER");

        if (peek_type() != BY) {
            push_error_return_error_object("ORDER must be followed by BY"); }
        
        advance_and_check_ret_obj("No values after BY");

        object* column_indexes_obj = parse_comma_seperated_list_end_if_not_comma();
        if (column_indexes_obj->type() != GROUP_OBJ) {
            push_error_return_error_object("ORDER BY: failed to get column indexes"); }

        // Can't check if they are specifically Column Index Objects cause they can be aliases, bruh

        return new prefix_expression_object(new operator_object(ORDER_BY_OP), column_indexes_obj);
    }
    // SELECT clauses end

    case DEFAULT:
        advance_and_check_ret_obj("No values after DEFAULT");
        return parse_expression(LOWEST);
    case SELECT:
        return parse_select();
    case VALUES: 
        return parse_values();
    case SEMICOLON:
        advance_and_check_ret_obj("No values after semicolon prefix");
        return new semicolon_object(); break;
    case MINUS: {
        advance_and_check_ret_obj("No values after - prefix");
        object* right = parse_expression(LOWEST);
        if (right->type() == ERROR_OBJ) {
            return right; }
        return new prefix_expression_object(new operator_object(NEGATE_OP), right);
    } break;
    case INTEGER_LITERAL:
        advance_and_check_ret_obj("No values after integer prefix");
        return new integer_object(tok.data); break;
    case STRING_LITERAL:
        return parse_string_literal(tok);
    case OPEN_PAREN: {
        advance_and_check_ret_obj("No values after open parenthesis in expression");
        object* expression =  parse_expression(LOWEST);
        if (peek_type() != CLOSE_PAREN) {
            push_error_return_error_object("Expected closing parenthesis");}
        advance_and_check_ret_obj("No values after close parenthesis in expression");
        return expression;
    } break;
    case ASTERISK:
        advance_and_check_ret_obj("No values after * prefix");
        return new star_object(); break;
    case AS:
        return parse_prefix_as(); break;
    case IF:
        return parse_prefix_if(); break;
    case ELSIF:
        return parse_prefix_if(); break;
    case LESS_THAN:
        advance_and_check_ret_obj("No values after less than in expression");
        if (token_position < tokens.size()) { // NOT CHECKED !!
            if (peek_type() == EQUAL) {
                advance_and_check_ret_obj("No values after <= in expression");
                return new operator_object(LESS_THAN_OR_EQUAL_TO_OP);
            }
        }
        return new operator_object(LESS_THAN_OP);
    case RETURN: {
        advance_and_check_ret_obj("No values after RETURN in expression");
        object* expression = parse_expression(LOWEST);
        if (peek_type() == SEMICOLON) {
            advance_and_check_ret_obj("No values after SEMICOLON in return statement"); }
        return new return_statement(expression);
    } break;
    case ELSE:
        return parse_block_statement(); break;
    case END:
        advance_and_check_ret_obj("No values after END in expression");
        if (peek_type() == IF) {
            advance_and_check_ret_obj("No values after END IF in expression");
            if (peek_type() == SEMICOLON) {
                advance_and_check_ret_obj("No values after SEMICOLON in END IF statement"); }
            return new end_if_statement(); 
        }

        if (peek_type() == SEMICOLON) {
            advance_and_check_ret_obj("No values after SEMICOLON in END statement"); }

        return new end_statement(); break;
    default:
        push_error_return_error_object("No prefix function for (" + token_type_to_string(tok.type) + ")");
    }
}

object* infix_parse_functions_with_obj(object* left) {
    switch (peek_type()) {
    case PLUS: case MINUS: case ASTERISK: case SLASH: case DOT: case EQUAL: case GREATER_THAN: case LESS_THAN:
        return parse_infix_operator(left); break;
    case AS: {
        if (left->type() == COLUMN_INDEX_OBJECT) {

            advance_and_check_ret_obj("No values after AS");

            object* right = parse_expression(LOWEST);
            if (right->type() == ERROR_OBJ) {
                errors.emplace_back("Failed to parse infix AS");
                return right;
            }

            return new infix_expression_object(new operator_object(AS_OP), left, right);
        }
        return left;
    } break;
    case WHERE: {
        advance_and_check_ret_obj("No values after WHERE");

        object* expression = parse_expression(LOWEST);
        if (expression->type() != INFIX_EXPRESSION_OBJ) {
            push_error_return_error_object("WHERE: Valued to parse expression"); }


        return new infix_expression_object(new operator_object(WHERE_OP), left, expression);  // Might have to make prefix
    }
    case LEFT: {
        advance_and_check_ret_obj("No values after LEFT");

        if (peek_type() != JOIN) {
            push_error_return_error_object("LEFT must be followed by JOIN"); }
        
        advance_and_check_ret_obj("No values after JOIN");

        object* left_table_name = parse_expression(LOWEST);
        if (left_table_name->type() != STRING_OBJ) {
            push_error_return_error_object("LEFT JOIN: Failed to parse left table name"); }

        if (peek_type() != ON) {
            push_error_return_error_object("LEFT JOIN: Only supports ON for now"); }

        advance_and_check_ret_obj("No values after ON");

        object* right_expression = parse_expression(LOWEST);
        if (right_expression->type() != INFIX_EXPRESSION_OBJ) {
            push_error_return_error_object("LEFT JOIN: Valued to evaluate right expression"); }

        infix_expression_object* inner = new infix_expression_object(new operator_object(ON_OP), left_table_name, right_expression);

        return new infix_expression_object(new operator_object(LEFT_JOIN_OP), left, inner);
    }
    default:
        push_error_return_error_object("No infix function for (" + token_type_to_string(peek_type()) + ") and left (" + std::string(left->inspect()) + ")");
    }
}



object* parse_expression(size_t precedence) {
    
    // std::cout << "parse_expression called with " << token_type_to_string(peek_type()) << std::endl;

    token tok = peek(); // Seperate cause GDB is obnoxious
    object* left = prefix_parse_functions_with_token(tok);
    if (!left || left->type() == ERROR_OBJ) {
        return new error_object();}

    while (peek_type() != LINE_END &&
           peek_type() != CLOSE_PAREN &&
           peek_type() != SEMICOLON &&
           peek_type() != COMMA && 
           precedence < numeric_precedence(peek()) ) {
            object* prev_left = left; // Having prev_left allows for no-ops
            left = infix_parse_functions_with_obj(left);
            if (!left || left->type() == ERROR_OBJ) {
                return new error_object();}

            if (left == prev_left) {
                break; }

            // advance_and_check_ret_obj("parse_expression(); infix missing right");

            // left = infix_parse_functions_with_obj(left);
            
           }     
        
    return left;
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
