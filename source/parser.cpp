#include "pch.h"

#include "parser.h"

#include "node.h"
#include "helpers.h"
#include "object.h"
#include "token.h"
#include "environment.h"
#include "allocator_aliases.h"
#include "macros.h"
#include "structs.h"
#include "logger.h"

extern logger<error_msg> sql_errors;

extern cmd_line_arguments g_args;

static avec<UP<node>> nodes;

static std::vector<token> tokens;
static size_t token_position = 0;

extern bool DEBUG;



static token prev_token = {ERROR_TOKEN, "garbage", 0, 0};



static std::expected<UP<assert_object>, UP<error_object>> parse_assert();
static void parse_alter();
static std::expected<UP<insert_into_object>, UP<error_object>> parse_insert();
static UP<object> parse_select();
static void parse_create();
static void parse_create_or_replace_function();
static void parse_create_table();

static std::expected<UP<group_object>, UP<error_object>> parse_comma_seperated_list(token_type end_val);
static std::expected<std::pair<UP<group_object>, token_type>, UP<error_object>> parse_comma_seperated_list_ADVANCED(const avec<token_type>& end_values); // Can add precedence as well if I feel like it
static std::expected<UP<group_object>, UP<error_object>> parse_list_until_comma(std::vector<token_type> end_vals);

static std::expected<UP<block_statement>, UP<error_object>> parse_function();
static UP<object> prefix_parse(token tok);
static std::expected<UP<infix_expr_object>, UP<error_object>> infix_parse(UP<object> left);


static token_type peek_type();
static std::string peek_data();
[[maybe_unused]]static token peek_ahead();


enum precedences : std::uint8_t {
    LOWEST = 0, SQL_STATEMENT = 1, PREFIX = 6, HIGHEST = 255
};  


#define log_sql_error(x)                               \
    do {                                               \
        std::stringstream err;                         \
        err << x;                                      \
        sql_errors.add_msg(err.str(), CUR_LOC);        \
    } while(0)                

    

// For simple queries, unlikely speedup was 2x. For more complicated ones, speedup was about 4x
#define push_err_ret(x)                                                                 \
        token cur_tok;                                                                  \
        if (token_position >= tokens.size()) [[unlikely]] {                             \
            if (tokens.size() > 0 && token_position >= tokens.size()) {                 \
                cur_tok = tokens[tokens.size() - 1];                                    \
                cur_tok.position += cur_tok.data.size();                                \
            } else {                                                                    \
                cur_tok.type = LINE_END; cur_tok.data = ""; cur_tok.line = SIZE_T_MAX; cur_tok.position = SIZE_T_MAX; \
            }                                                                           \
        } else {                                                                        \
            cur_tok = tokens[token_position];}                                          \
        std::stringstream err;                                                          \
        err << x;                                                                       \
        sql_errors.add_msg<parser_error_msg>(err.str(), cur_tok, CUR_LOC);              \
        return;


#define push_err_ret_err_obj(x)                                                         \
        token cur_tok;                                                                  \
        if (token_position >= tokens.size()) [[unlikely]]{                              \
            if (tokens.size() > 0 && token_position >= tokens.size()) {                 \
                cur_tok = tokens[tokens.size() - 1];                                    \
                cur_tok.position += cur_tok.data.size();                                \
            } else {                                                                    \
                cur_tok.type = LINE_END; cur_tok.data = ""; cur_tok.line = SIZE_T_MAX; cur_tok.position = SIZE_T_MAX; \
            }                                                                           \
        } else {                                                                        \
            cur_tok = tokens[token_position];}                                          \
        std::stringstream err;                                                          \
        err << x;                                                                       \
        sql_errors.add_msg<parser_error_msg>(err.str(), cur_tok, CUR_LOC);              \
        return UP<object>(new error_object(err.str()))

#define push_err_ret_unx_err_obj(x)                                                     \
        token cur_tok;                                                                  \
        if (token_position >= tokens.size()) [[unlikely]] {                             \
            if (tokens.size() > 0 && token_position >= tokens.size()) {                 \
                cur_tok = tokens[tokens.size() - 1];                                    \
                cur_tok.position += cur_tok.data.size();                                \
            } else {                                                                    \
                cur_tok.type = LINE_END; cur_tok.data = ""; cur_tok.line = SIZE_T_MAX; cur_tok.position = SIZE_T_MAX; \
            }                                                                           \
        } else {                                                                        \
            cur_tok = tokens[token_position];}                                          \
        std::stringstream err;                                                          \
        err << x;                                                                       \
        sql_errors.add_msg<parser_error_msg>(err.str(), cur_tok, CUR_LOC);              \
        return std::unexpected(MAKE_UP(error_object, err.str()));

#define push_err_ret_err_obj_prev_tok(x)                                                \
        token cur_tok = prev_token;                                                     \
        std::stringstream err;                                                          \
        err << x;                                                                       \
        sql_errors.add_msg<parser_error_msg>(err.str(), cur_tok, CUR_LOC);              \
        return UP<object>(new error_object(err.str()));


// sussy using macro in macro
// push_err_ret
#define advance_and_check(x)                            \
    if (token_position < tokens.size()) [[likely]] {    \
        prev_token = tokens[token_position]; }          \
    token_position++;                                   \
    if (token_position >= tokens.size() && (!g_args.has_query && !g_args.run_tests && !g_args.background)) [[unlikely]] {  \
        push_err_ret(x); }              

// sussy using macro in macro
// push_err_ret_err_obj
#define advance_and_check_ret_obj(x)                    \
    if (token_position < tokens.size()) [[likely]] {    \
        prev_token = tokens[token_position]; }          \
    token_position++;                                   \
    if (token_position >= tokens.size() && (!g_args.has_query && !g_args.run_tests && !g_args.background)) [[unlikely]] {  \
        push_err_ret_err_obj(x); }     

// push_err_ret_unx_err_obj
#define advance_and_check_ret_unx_obj(x)                \
    if (token_position < tokens.size()) [[likely]] {    \
        prev_token = tokens[token_position]; }          \
    token_position++;                                   \
    if (token_position >= tokens.size() && (!g_args.has_query && !g_args.run_tests && !g_args.background)) [[unlikely]] {  \
        push_err_ret_unx_err_obj(x); }     


void parser_init(std::vector<token> toks) {
    tokens = toks;
    token_position = 0;
    prev_token = {.type=ERROR_TOKEN, .data="garbage", .line=0, .position=0};

    nodes = avec<UP<node>>();
}

avec<UP<node>> parse() {

    if (tokens.size() == 0) {
        log_sql_error("No tokens");
        return std::move(nodes);
    }

    size_t loop_count = 0;

    while (loop_count++ < 100) {

        switch (peek_type()) {
        case CREATE:
            parse_create(); break;
        case INSERT: {
            auto result = parse_insert();
            if (result.has_value()) {
                nodes.emplace_back(UP<node>(new insert_into(std::move(*result))));
            } else {
                log_sql_error("parse_insert(): Returned error object"); }
        } break;
        case SELECT: {
            UP<object> result = parse_select();
            if (result->type() == ERROR_OBJ) {
                log_sql_error("parse_select(): Returned error object"); break; }
            // Advance past semicolon
            if (peek_type() != SEMICOLON) {
                log_sql_error("parse_select(): Missing ending semicolon, instead got " + token_type_to_string(peek_type()));
            } else {
                token_position++; }

            switch(result->type()) {
            case SELECT_OBJ:
                nodes.emplace_back(UP<node>(new select_node(std::move(result)))); break;
            case SELECT_FROM_OBJ:
                nodes.emplace_back(UP<node>(new select_from(std::move(result)))); break;
            default:
                log_sql_error("parse_select(): Returned unknown object type"); } 
            
        } break;
        case ALTER:
            parse_alter(); break;
        // Custom
        case ASSERT: {
            auto result = parse_assert();
            if (!result.has_value()) {
                log_sql_error("parse_assert(): Returned bad object!"); break; }
                
            if (peek_type() != SEMICOLON) {
                log_sql_error("parse_assert(): Missing ending semicolon, instead got " + token_type_to_string(peek_type()));
            } else {
                token_position++; }

            nodes.emplace_back(UP<node>(new assert_node(std::move(*result))));
        } break;
        default:
            std::string error = "Unknown keyword or inappropriate usage (" + peek_data() +  ") Token type = " + token_type_to_string(peek().type);
            sql_errors.add_msg<parser_error_msg>(error, peek(), CUR_LOC);
        }

        if (sql_errors.has_msgs()) {
            // Look for end of statement ';', if it's there, go to it then continue looking for errors in the next statement
            while (token_position < tokens.size() && peek_type() != SEMICOLON) {
                token_position++; }
            token_position++; // If at semicolon, advance past it
        }

        if (peek_type() == LINE_END) {
            break; }
    }

    if (loop_count == 100) {
        std::cout << "le got stuck in loop?";}

    return std::move(nodes);
}


static std::expected<UP<assert_object>, UP<error_object>> parse_assert() {

    if (peek_type() != ASSERT) {
        std::cout << "parse_assert(): with non-ASSERT token" << std::endl; exit(1); }

    size_t line = peek().line;

    advance_and_check_ret_unx_obj("Nothing after ASSERT");

    UP<object> expr = parse_expression(LOWEST);
    if (expr->type() == ERROR_OBJ) {
        push_err_ret_unx_err_obj("Failed to parse ASSERT expression"); }

    return MAKE_UP(assert_object, line, std::move(expr));
}

// FIXME Should just use table_detail_object
static void parse_alter() {
    
    if (peek_type() != ALTER) { FATAL_ERROR_THROW("parse_alter() called with non-ALTER token", CUR_LOC); }

    advance_and_check("No tokens after ALTER");

    if (peek_type() != TABLE) {
        push_err_ret("ALTER, only works on TABLE for now");}

    advance_and_check("No tokens after ALTER TABLE");

    UP<object> table_name = parse_expression(LOWEST);
    if (table_name->type() == ERROR_OBJ) {
        push_err_ret("Failed to get table name"); }

    if (peek_type() != ADD) {
        push_err_ret("ALTER TABLE, only supports ADD");}

    advance_and_check("No tokens after ADD");

    if (peek_type() != COLUMN) {
        push_err_ret("ALTER TABLE, only supports ADD COLUMN");}

    advance_and_check("No tokens after ADD COLUMN");

    UP<object> col_obj = parse_expression(LOWEST);
    if (col_obj->type() == ERROR_OBJ) {
        return; }
    
    // Needs to check type aswell

    UP<table_detail_object> col; // idk yet

    // Fix aswell
    if (peek_type() == DEFAULT) {
        UP<object> default_value = parse_expression(LOWEST);
        if (default_value->type() == ERROR_OBJ) {
            return; }
        if (default_value->type() != DEFAULT_VALUE_OBJ) {
            return; }
    }

    if (peek_type() != SEMICOLON) {
        push_err_ret("Missing ending semicolon");}

    token_position++;

    nodes.push_back(UP<node>(new alter_table_node(std::move(table_name), CAST_UP(object, col))));
}

static std::expected<UP<group_object>, UP<error_object>> parse_values() {

    if (peek_type() != VALUES) {
        FATAL_ERROR_THROW("parse_values() called with non-VALUES token", CUR_LOC); }

    advance_and_check_ret_unx_obj("Missing open parenthesis after VALUES");

    if (peek_type() != OPEN_PAREN) {
        push_err_ret_unx_err_obj("Unknown token after VALUES, should be open parenthesis");}

    advance_and_check_ret_unx_obj("Missing values after open parenthesis");

    // Find values
    auto result = parse_comma_seperated_list(CLOSE_PAREN);
    if (!result.has_value()) {
        push_err_ret_unx_err_obj("Failed to parse VALUES list"); }

    UP<group_object>& values_obj = *result;
        
    if (peek_type() != CLOSE_PAREN) {
        push_err_ret_unx_err_obj("VALUES, missing closing parenthesis");}

    advance_and_check_ret_unx_obj("No values after closing parenthesis");

    return std::move(values_obj);
}

// inserts rows
static std::expected<UP<insert_into_object>, UP<error_object>> parse_insert() {

    if (peek_type() != INSERT) { FATAL_ERROR_THROW("parse_insert() called with non-INSERT token", CUR_LOC); }

    advance_and_check_ret_unx_obj("No tokens after INSERT");

    if (peek_type() != INTO) {
        push_err_ret_unx_err_obj("Unknown INSERT usage"); }

    advance_and_check_ret_unx_obj("No tokens after INSERT INTO");

    if (peek_type() != STRING_LITERAL) {
        push_err_ret_unx_err_obj("INSERT INTO bad token type for table name");}

    astring table_name = peek_data();

    advance_and_check_ret_unx_obj("No open parenthesis after INSERT INTO table");

    if (peek_type() != OPEN_PAREN) {
        push_err_ret_unx_err_obj("No open parenthesis after INSERT INTO table");}

    advance_and_check_ret_unx_obj("No tokens after INSERT INTO table (");

    // Find field names
    auto result = parse_comma_seperated_list(CLOSE_PAREN);
    if (!result.has_value()) {
        push_err_ret_unx_err_obj("INSERT INTO: Failed to parse field names"); }

    UP<group_object>& fields_obj = *result;
        
    if (peek_type() != CLOSE_PAREN) {
        push_err_ret_unx_err_obj("Field names missing closing parenthesis");}

    avec<UP<object>> fields = std::move(fields_obj->elements);

    advance_and_check_ret_unx_obj("INSERT INTO table, missing VALUES");

    UP<object> values = parse_expression(LOWEST);

    if (peek_type() != SEMICOLON) {
        push_err_ret_unx_err_obj("INSERT INTO missing closing semicolon");}

    token_position++;

    UP<insert_into_object> info = MAKE_UP(insert_into_object, table_name, std::move(fields), std::move(values));

    return std::move(info);
}

#define advance_and_check_list(x)            \
    prev_token = tokens[token_position];    \
    token_position++;                       \
    if (token_position >= tokens.size()) {  \
        push_err_ret_list(x);}     

#define push_err_ret_list(x)                    \
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
    std::vector<UP<object>> errored_list;      \
    return errored_list;

#define advance_and_check_pair_list(list, x)      \
    prev_token = tokens[token_position];    \
    token_position++;                       \
    if (token_position >= tokens.size()) {  \
        push_err_ret_pair_list(list, x);}     

#define push_err_ret_pair_list(list, x)        \
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
    errors.emplace_back(std::move(error));          \
    std::pair<UP<object>, token_type> errored_list = {new group_object(list), peek_type()}; \
    return errored_list;

// Starts on first value, end on end val, returns which value it snded on
static std::expected<std::pair<UP<group_object>, token_type>, UP<error_object>> parse_comma_seperated_list_ADVANCED(const avec<token_type>& end_values) {

    avec<UP<object>> list;

    if (std::ranges::find(end_values, peek_type()) != end_values.end()) {
        auto grp = MAKE_UP(group_object, std::move(list));
        return std::make_pair(std::move(grp), peek_type()); }

    size_t loop_count = 0;
    while (loop_count++ < 100) {
        UP<object> cur = parse_expression(LOWEST);
        if (cur->type() == ERROR_OBJ) {
            return std::unexpected(CAST_UP(error_object, cur)); }

        list.emplace_back(std::move(cur));
        
        if (std::ranges::find(end_values, peek_type()) != end_values.end()) {
            break;
        }
        
        if (peek_type() != COMMA) {
            push_err_ret_unx_err_obj("Items in list must be comma seperated, got (" + token_type_to_string(peek_type()) + ")");
        }

        advance_and_check_ret_unx_obj("parse_comma_seperated_list_ADVANCED(): No values after comma");
    }

    if (loop_count >= 100) {
        push_err_ret_unx_err_obj("Too many loops during comma seperated list traversal, likely weird error");}

    return std::make_pair(MAKE_UP(group_object, std::move(list)), peek_type());
}



// Starts on first value, end on end val
static std::expected<UP<group_object>, UP<error_object>> parse_comma_seperated_list(token_type end_val) {

    avec<UP<object>> list;

    if (peek_type() == end_val) {
        return MAKE_UP(group_object, std::move(list)); }

    size_t loop_count = 0;
    while (loop_count++ < 100) {
        UP<object> cur = parse_expression(LOWEST);
        if (cur->type() == ERROR_OBJ) {
            return std::unexpected(CAST_UP(error_object, cur)); }

        list.push_back(std::move(cur));
        
        if (peek_type() == end_val) {
            break; }
        
        if (peek_type() != COMMA) {
            push_err_ret_unx_err_obj("Items in list must be comma seperated, got (" + token_type_to_string(peek_type()) + ")");}

        advance_and_check_ret_unx_obj("parse_comma_seperated_list(): No values after comma");
    }

    if (loop_count >= 100) {
        push_err_ret_unx_err_obj("Too many loops during comma seperated list traversal, likely weird error");}

    return MAKE_UP(group_object, std::move(list));
}

// Starts on first value, end on end val
static std::expected<UP<group_object>, UP<error_object>> parse_comma_seperated_list_end_if_not_comma() {

    avec<UP<object>> list;

    size_t loop_count = 0;
    while (loop_count++ < 100) {
        UP<object> cur = parse_expression(LOWEST);
        if (cur->type() == ERROR_OBJ) {
            return std::unexpected(CAST_UP(error_object, cur)); }

        list.push_back(std::move(cur));
        
        if (peek_type() != COMMA) {
            break; }

        advance_and_check_ret_unx_obj("parse_comma_seperated_list_end_if_not_comma(): No values after comma");
    }

    if (loop_count >= 100) {
        push_err_ret_unx_err_obj("Too many loops during comma seperated list traversal, likely weird error");}

    return MAKE_UP(group_object, std::move(list));
}

// Starts on first value, end on end val
static std::expected<UP<group_object>, UP<error_object>> parse_list_until_comma(std::vector<token_type> end_vals) {

    avec<UP<object>> list;

    size_t loop_count = 0;
    while (loop_count++ < 100) {
        UP<object> cur = parse_expression(LOWEST);
        if (cur->type() == ERROR_OBJ) {
            return std::unexpected(CAST_UP(error_object, cur)); }

        list.push_back(std::move(cur));
        
        if (peek_type() == COMMA) { break; }

        bool should_break = false;
        for (const auto& end_val : end_vals) {
            if (peek_type() == end_val) {
                should_break = true;
                break; 
            }
        }
        if (should_break) { break; }
    }

    if (loop_count >= 100) {
        push_err_ret_unx_err_obj("Too many loops during list traversal, likely weird error");}

    return MAKE_UP(group_object, std::move(list));
}


[[maybe_unused]] static bool is_data_type_token(token tok) {
    switch (tok.type) {
    case CHAR: case VARCHAR: case BOOL: case BOOLEAN: case DATE: case YEAR: case SET: case BIT: case INT: case INTEGER: case FLOAT: case DOUBLE: case NONE: case UNSIGNED: case ZEROFILL:
    case TINYBLOB: case TINYTEXT: case MEDIUMTEXT: case MEDIUMBLOB: case LONGTEXT: case LONGBLOB: case DEC: case DECIMAL: case TIMESTAMP:
        return true;
        break;
    default:
        return false;
    }
}


static UP<object> parse_select() {

    if (peek_type() != SELECT) { FATAL_ERROR_THROW("parse_select() called with non-SELECT token", CUR_LOC); }

    advance_and_check_ret_obj("No tokens after SELECT")

    const avec<token_type> end_types = {FROM, SEMICOLON, CLOSE_PAREN};  // TODO Maybe make global open paren count and return error if count is wrong
    auto result = parse_comma_seperated_list_ADVANCED(end_types);
    if (!result.has_value()) {
        push_err_ret_err_obj("SELECT: Failed to parse SELECT column indexes"); }

    auto& selector_group = (*result).first;
    const auto& end_type = (*result).second;

    if (auto it = std::ranges::find(end_types, end_type); it == end_types.end()) {
        push_err_ret_err_obj("SELECT: Failed to parse SELECT column indexes, strange end type (" + token_type_to_string(end_type) + ")"); }
    

    avec<UP<object>> selectors = std::move(CAST_UP(group_object, selector_group)->elements);

    if (selectors.size() == 0) {
        push_err_ret_err_obj("No values after SELECT");}
    
    if (peek_type() != FROM) {

        if (selectors.size() != 1) {
            push_err_ret_err_obj("SELECT (column index): Can only take in one value"); }

        UP<object> selector = std::move(selectors[0]);
        
        UP<select_object> info = MAKE_UP(select_object, std::move(selector));

        return CAST_UP(object, info);
    }

    if (end_type != FROM) {
            push_err_ret_err_obj("SELECT FROM: Failed to end on FROM"); }

    advance_and_check_ret_obj("No values after FROM");

    if (selectors[0]->data() == "*") {
        if (selectors.size() > 1) {
            push_err_ret_err_obj("Too many entries in list after *");}
        selectors[0] = UP<object>(new star_object());
    }

    // Get condition chain
    avec<UP<object>> clause_chain;
    while (peek_type() != SEMICOLON && peek_type() != LINE_END && peek_type() != CLOSE_PAREN) { // FIXME Don't like this close paren check, would prefer to not even have to check
                                                                                                // Maybe use a global paren count
        UP<object> clause = parse_expression(LOWEST);
        if (clause->type() != INFIX_EXPRESSION_OBJ && clause->type() != PREFIX_EXPRESSION_OBJ && clause->type() != STRING_OBJ)  {
            push_err_ret_err_obj("Expected clause in SELECT FROM, got (" + clause->inspect() + ")");}

        clause_chain.push_back(std::move(clause));

    }

    UP<select_from_object> info = MAKE_UP(select_from_object, std::move(selectors), std::move(clause_chain));
    
    return CAST_UP(object, info);
}

static void parse_create() {
    if (peek_type() != CREATE) { FATAL_ERROR_THROW("parse_create() called with non-CREATE token", CUR_LOC); }

    advance_and_check("No tokens after CREATE");
    
    switch(peek_type()) {
    case TABLE: 
        parse_create_table(); break;
    case OR:
        advance_and_check("No tokens after OR");
        if (peek_type() != REPLACE) {
            push_err_ret("No REPLACE after OR");}

        advance_and_check("No tokens after REPLACE");
        if (peek_type() != FUNCTION) {
            push_err_ret("No FUNCTION after REPLACE");}

        parse_create_or_replace_function(); break;
    default:
        push_err_ret("Unknown keyword (" + peek_data() + ") after CREATE");
    }
}

static void parse_create_or_replace_function() {
    if (peek_type() != FUNCTION) { FATAL_ERROR_THROW("parse_create_or_replace_function() called with non-FUNCTION token", CUR_LOC); }

    advance_and_check("No tokens after FUNCTION");

     // Function name must be string literal, otherwise too much nonsense
    if (peek_type() != STRING_LITERAL) {
        push_err_ret("Function name must be string literal"); }

    astring name = peek_data();

    advance_and_check("No tokens after function name");

    if (peek_type() != OPEN_PAREN) {
        push_err_ret("No parenthesis in function declaration"); }

    advance_and_check("No tokens after parenthesis");

    auto result = parse_comma_seperated_list(CLOSE_PAREN);
    if (!result.has_value()) {
        push_err_ret("Failed to parse parameters"); }

    UP<group_object>& parameters = *result; // FIXME Get rid of these references and use std::move(*result)
    
    advance_and_check("No tokens after parameters");

    if (peek_type() != RETURNS) {
        push_err_ret("No RETURNS in function declaration"); }

    advance_and_check("No tokens after RETURNS");

    // For now, no nonsense in function delcarations
    UP<object> return_type = parse_expression(LOWEST);
    if (return_type->type() != SQL_DATA_TYPE_OBJ) {
        push_err_ret("Invalid return type");}

    UP<object> func_body = parse_expression(LOWEST);
    if (func_body->type() != BLOCK_STATEMENT) {
        push_err_ret("Failed to parse function body");}

    UP<function_object> func = MAKE_UP(function_object, name, std::move(parameters->elements), CAST_UP(SQL_data_type_object, return_type), CAST_UP(block_statement, func_body));

    UP<function> info = MAKE_UP(function, std::move(func));
    
    nodes.push_back(CAST_UP(node, info));
}


// tokens should point to TABLE
static void parse_create_table() {

    if (peek_type() != TABLE) { FATAL_ERROR_THROW("parse_create_table() called with non-TABLE token", CUR_LOC); }
        
    advance_and_check("No tokens after CREATE TABLE");

    if (peek_type() != STRING_LITERAL) {
        push_err_ret("CREATE TABLE bad token type for table name");}

    astring table_name = peek_data();

    advance_and_check("No values after table name");

    if (peek_type() != OPEN_PAREN) {
        push_err_ret("No open paren '(' after CREATE TABLE");}
    
    advance_and_check("No data in CREATE TABLE");

    auto result = parse_comma_seperated_list(CLOSE_PAREN);
    if (!result.has_value()) {
        push_err_ret("Failed to parse CREATE TABLE columns"); }

    UP<group_object> details = CAST_UP(group_object, std::move(*result));;

    // Verify types
    if (DEBUG) [[unlikely]] { std::cout << "Parse Create Table, types:\n"; }
    for (const auto& obj : details->elements) {
        if (DEBUG) [[unlikely]] { std::cout << "\t" << object_type_to_astring(obj->type()) << std::endl; }
        switch (obj->type()) {
            case PARAMETER_OBJ: case TABLE_COLUMN_EXPR_OBJ: case TABLE_EXPR_OBJ: break;
            default: push_err_ret("CREATE TABLE: Invalid object in table details. Type: " << object_type_to_astring(obj->type()) << ", (" << obj->inspect() << ")"); break;
        }
    }

    if (DEBUG) [[unlikely]] {
        std::cout << "\nParse Create Table, inspected:\n";
        for (const auto& obj : details->elements) {
            std::cout << "\t" << obj->inspect() << std::endl;
        }
    }
        
    advance_and_check("No closing ';' at end of CREATE TABLE");

    if (peek_type() != SEMICOLON) {
        push_err_ret("No closing ';' at end of CREATE TABLE");}

    token_position++;

    UP<create_table> info = MAKE_UP(create_table, table_name, std::move(details));   

    nodes.push_back(CAST_UP(node, info));
}



static size_t numeric_precedence(const token& tok) {
    switch (tok.type) {
        case AS:            return 1; break; // Precedence seems to work
        case WHERE:         return 1; break; // Precedence seems to work
        case LEFT:          return 1; break; // Precedence seems to work
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

static size_t numeric_precedence(const UP<operator_object>& op) {
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

static UP<object> parse_prefix_as() {

    if (peek_type() != AS) {
        push_err_ret_err_obj("parse_prefix_as() called without AS token"); }

    advance_and_check_ret_obj("No right expression for prefix AS");

    if (peek_type() == $$) {
        auto result = parse_function();
        if (result.has_value()) {
            return CAST_UP(object, std::move(*result));
        } else {
            return CAST_UP(object, std::move(result).error());
        }
    } else {
        push_err_ret_err_obj("AS can only be prefix for $$ in a function (for now)"); 
    }
}


static std::expected<UP<if_statement>, UP<error_object>> parse_prefix_if() {

    token starter_token = peek();

    if (peek_type() != IF && peek_type() != ELSIF) {
        push_err_ret_unx_err_obj("parse_prefix_if() called without IF/ELSIF token"); }

    advance_and_check_ret_unx_obj("No tokens after IF");

    // CONDITION
    UP<object> condition = parse_expression(LOWEST); // LOWEST or PREFIX??
    if (condition->type() == ERROR_OBJ) {
        return std::unexpected(CAST_UP(error_object, condition)); }

    if (peek_type() != THEN) {
        push_err_ret_unx_err_obj("IF statement missing THEN"); }

    advance_and_check_ret_unx_obj("No tokens after THEN");

    // BODY
    avec<UP<object>> body;

    if (peek_type() == BEGIN) {
        size_t max_loops = 0;
        while (max_loops++ < 100) {
            UP<object> expression = parse_expression(LOWEST);
            if (expression->type() == ERROR_OBJ) {
                return std::unexpected(CAST_UP(error_object, expression)); }
            
            if (peek_type() == END) {
                break; }
        }

        if (max_loops >= 100) {
            push_err_ret_unx_err_obj("Too many loops during IF statement parsing"); }

    } else {
        UP<object> expression = parse_expression(LOWEST);
        if (expression->type() == ERROR_OBJ) {
            return std::unexpected(CAST_UP(error_object, expression)); }

        body.push_back(std::move(expression));
    }

    UP<if_statement> if_stmnt = MAKE_UP(if_statement, std::move(condition), MAKE_UP(block_statement, std::move(body)), UP<object>(new null_object()));

    // ELSIF
    if_statement* prev = if_stmnt.get();
    
    size_t max_loops = 0;
    while (peek_type() == ELSIF && max_loops++ < 100) { 

        UP<object> statement = parse_expression(LOWEST);
        if (statement->type() == ERROR_OBJ) {
            return std::unexpected(CAST_UP(error_object, statement)); }

        if (statement->type() != IF_STATEMENT) {
            push_err_ret_unx_err_obj("ELSIF statement evaluated to non-IF statement"); }

        prev = CAST_UP(if_statement, statement).get();
        prev->other = std::move(statement);
    }

    if (max_loops >= 100) {
        push_err_ret_unx_err_obj("Too many loops during ELSIF statement parsing"); }

    // ELSE
    if (peek_type() == ELSE) {
        UP<object> statement = parse_expression(LOWEST);
        if (statement->type() == ERROR_OBJ) {
            return std::unexpected(CAST_UP(error_object, statement)); }

        if (statement->type() != BLOCK_STATEMENT) {
            push_err_ret_unx_err_obj("ELSE statement evaluated to non-BLOCK statement"); }

        prev->other = std::move(statement);
    }

    if (starter_token.type == IF) {
        UP<object> endif = parse_expression(LOWEST);
        if (endif->type() == ERROR_OBJ) {
            return std::unexpected(CAST_UP(error_object, endif)); }
            
        if (endif->type() != END_IF_STATEMENT) {
            push_err_ret_unx_err_obj("No END IF in " + token_type_to_string(starter_token.type) + " statement, got (" + std::string(endif->inspect()) + ")"); }   
    }

    return if_stmnt;
}

static std::expected<UP<block_statement>, UP<error_object>> parse_block_statement() {

    if (peek_type() != ELSE) {
        push_err_ret_unx_err_obj("parse_block_statement() called with non-ELSE token"); }

    advance_and_check_ret_unx_obj("No tokens after THEN");

    
    // BODY
    avec<UP<object>> body;

    if (peek_type() == BEGIN) {
        size_t max_loops = 0;
        while (max_loops++ < 100) {
            UP<object> expression = parse_expression(LOWEST);
            if (expression->type() == ERROR_OBJ) {
                return std::unexpected(CAST_UP(error_object, expression)); }
            
            if (peek_type() == END) {
                break; }
        }

        if (max_loops >= 100) {
            push_err_ret_unx_err_obj("Too many loops during IF statement parsing"); }

    } else {
        UP<object> expression = parse_expression(LOWEST);
        if (expression->type() == ERROR_OBJ) {
            return std::unexpected(CAST_UP(error_object, expression)); }

        body.push_back(std::move(expression));
    }

    return MAKE_UP(block_statement, std::move(body));
}

static std::expected<UP<block_statement>, UP<error_object>> parse_function() {
    
    if (peek_type() != $$) {
        push_err_ret_unx_err_obj("parse_function() called with non-$$ token"); }

    advance_and_check_ret_unx_obj("No tokens after $$");

    if (peek_type() != BEGIN) {
        push_err_ret_unx_err_obj("No BEGIN in function"); }

    advance_and_check_ret_unx_obj("No tokens after BEGIN");

    avec<UP<object>> statement_list;
    size_t max_loops = 0;
    while (max_loops++ < 100) {
        UP<object> statement = parse_expression(LOWEST);
        if (statement->type() == ERROR_OBJ) {
            return std::unexpected(CAST_UP(error_object, statement)); }

        if (statement->type() == END_STATEMENT) {
            advance_and_check_ret_unx_obj("No values after END");
            if (peek_type() != SEMICOLON) {
                push_err_ret_unx_err_obj("No semicolon after END"); }
            break; 
        } 

        statement_list.push_back(std::move(statement));
    }

    if (max_loops >= 100) {
        push_err_ret_unx_err_obj("Too many loops during function parse"); }

    UP<block_statement> func = MAKE_UP(block_statement, std::move(statement_list));

    if (peek_type() == $$) {
        advance_and_check_ret_unx_obj("No value after closing $$"); }

    if (peek_type() != SEMICOLON) {
        push_err_ret_unx_err_obj("Function declaration missing closing semicolon"); }

    token_position++;

    return func;
}


static std::expected<UP<infix_expr_object>, UP<error_object>> parse_infix_operator(UP<object> left) {

    operator_type op = NULL_OP;
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
    case NOT_EQUAL: // The lexer handles combing ! and =
        op = NOT_EQUALS_OP; break;
    case LESS_THAN:
        // or equal not handled for now cause lazy
        op = LESS_THAN_OP; break;
    case GREATER_THAN:
        // or equal not handled for now cause lazy
        op = GREATER_THAN_OP; break;
    default:
        push_err_ret_unx_err_obj("Unknown infix operator (" + token_type_to_string(peek_type()) + ")");
    }

    UP<operator_object> op_obj = MAKE_UP(operator_object, op);
    
    size_t precedence = numeric_precedence(op_obj);

    advance_and_check_ret_unx_obj("No values after operator (" + op_obj->inspect() + ")");
    
    UP<object> right = parse_expression(precedence);
    if (right->type() == ERROR_OBJ) {
        return std::unexpected(CAST_UP(error_object, right));}
        
    UP<infix_expr_object> expression = MAKE_UP(infix_expr_object, std::move(op_obj), std::move(left), std::move(right));

    return expression;

}


static UP<object> parse_data_type(token tok) {

    if (!is_sql_data_type_token(tok)) {
        FATAL_ERROR_THROW("parse_data_type() called with not SQL data type token", CUR_LOC); }

    token_type prefix = NONE;
    token_type data_type = NONE;
    std::optional<UP<object>> parameter;

    bool unsign = false;
    [[maybe_unused]] bool zerofill = false;

    if (token_position >= tokens.size()) {
        push_err_ret_err_obj("No data type token");}  
    
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

    token_type type = tok.type;
    advance_and_check_ret_obj("Nothing after data type");

    // Basic string
    switch (type) {
    case TINYBLOB: case TINYTEXT: case MEDIUMTEXT: case MEDIUMBLOB: case LONGTEXT: case LONGBLOB:
        data_type = CHAR;
        parameter = UP<object>(new integer_object(255));
        return UP<object>(new SQL_data_type_object(prefix, data_type, std::move(parameter)));
    default:
        break;
    }

    if (type == VARCHAR) { 

        if (peek_type() != OPEN_PAREN) {
            data_type = VARCHAR;
            parameter = UP<object>(new integer_object(255));
            return UP<object>(new SQL_data_type_object(prefix, data_type, std::move(parameter)));
        }

        advance_and_check_ret_obj("VARCHAR missing length");


        UP<object> obj = parse_expression(LOWEST);
        if (obj->type() == ERROR_OBJ) {
            return obj; }

        if (peek_type() != CLOSE_PAREN) {
            push_err_ret_err_obj("VARCHAR missing closing parenthesis");}

        advance_and_check_ret_obj("VARCHAR nothing after closing parenthesis");

        data_type = VARCHAR;
        parameter = std::move(obj);
        return UP<object>(new SQL_data_type_object(prefix, data_type, std::move(parameter)));
    }
    
    // Basic numeric
    if (type == BOOL) {
        data_type = BOOL;
        return UP<object>(new SQL_data_type_object(prefix, data_type, std::move(parameter)));
    } else if (type == BOOLEAN) {
        data_type = BOOL;
        return UP<object>(new SQL_data_type_object(prefix, data_type, std::move(parameter)));
    }

    // Basic time
    if (type == DATE) {
        data_type = DATE;
        return UP<object>(new SQL_data_type_object(prefix, data_type, std::move(parameter)));
    } else if (type == YEAR) {
        data_type = YEAR;
        return UP<object>(new SQL_data_type_object(prefix, data_type, std::move(parameter)));
    }

    if (type == TIMESTAMP) {
        data_type = TIMESTAMP;
        return UP<object>(new SQL_data_type_object(prefix, data_type, std::move(parameter)));
    }

    // String
    if (type == SET) {
        if (peek_type() == OPEN_PAREN) {
            advance_and_check_ret_obj("SET: Missing elements");
            if (peek_type() == CLOSE_PAREN) {
                advance_and_check_ret_obj("No values after SET elements");
                data_type = SET;
                return UP<object>(new SQL_data_type_object(prefix, data_type, std::move(parameter)));
            }

            auto result = parse_comma_seperated_list(CLOSE_PAREN);
            if (!result.has_value()) {
                push_err_ret_err_obj("Failed to parse SET elements"); }

            UP<group_object>& elements = *result;

            if (peek_type() != CLOSE_PAREN) {
                push_err_ret_err_obj("SET missing closing parenthesis");}

            advance_and_check_ret_obj("Missing values after SET()");
            
            data_type = SET;
            parameter = CAST_UP(object, elements);
            return UP<object>(new SQL_data_type_object(prefix, data_type, std::move(parameter)));
        } else {
            data_type = SET;
            return UP<object>(new SQL_data_type_object(prefix, data_type, std::move(parameter)));
        }
    }

    // Numeric
    if (type == BIT) {
        if (peek_type() == OPEN_PAREN) {
            advance_and_check_ret_obj("BIT: Missing elements");
            if (peek_type() == CLOSE_PAREN) {
                advance_and_check_ret_obj("No values after BIT elements");
                data_type = BIT;
                parameter = UP<object>(new integer_object(1));
                return UP<object>(new SQL_data_type_object(prefix, data_type, std::move(parameter)));
            }

            int num = 1;
            if (peek_type() != INTEGER_LITERAL) {
                if (peek_type() != LINE_END) {
                    log_sql_error("BIT contained non-integer");}
            } else {
                if (tokens[token_position].data.length() > 2) {
                    log_sql_error("BIT cannot be greated than 64");
                } else {
                    num = std::stoi(tokens[token_position].data);
                    if (num < 1) {
                        log_sql_error("BIT cannot be less than 1");}
                    if (num > 64) {
                        log_sql_error("BIT cannot be greated than 64");}
                }
            }
            advance_and_check_ret_obj("BIT: Missing clossing parenthesis");

            if (peek_type() != CLOSE_PAREN) {
                log_sql_error("BIT: Missing clossing parenthesis");}
                
            advance_and_check_ret_obj("Missing values after BIT()");

            data_type = BIT;
            parameter = UP<object>(new integer_object(num));
            return UP<object>(new SQL_data_type_object(prefix, data_type, std::move(parameter)));
        } else {
            data_type = BIT;
            parameter = UP<object>(new integer_object(1));
            return UP<object>(new SQL_data_type_object(prefix, data_type, std::move(parameter)));
        }
        
    } else if (type == INT || type == INTEGER) {
        // The number is the display size, can ignore in computations for now. I read the default is 11 for signed and 10 for unsigned but I'm not sure it's standard so...
       if (peek_type() != OPEN_PAREN) {
            if (unsign) {
                data_type = INT;
                parameter = UP<object>(new integer_object(10));
                return UP<object>(new SQL_data_type_object(prefix, data_type, std::move(parameter)));
            } else {
                data_type = INT;
                parameter = UP<object>(new integer_object(11));
                return UP<object>(new SQL_data_type_object(prefix, data_type, std::move(parameter)));
            }
       } else {
            advance_and_check_ret_obj("INT: No values after open parenthesis");
            if (peek_type() == CLOSE_PAREN) {
                advance_and_check_ret_obj("INT: No values after closing parenthesis");
                if (unsign) {
                    data_type = INT;
                    parameter = UP<object>(new integer_object(10));
                    return UP<object>(new SQL_data_type_object(prefix, data_type, std::move(parameter)));
                } else {
                    data_type = INT;
                    parameter = UP<object>(new integer_object(11));
                    return UP<object>(new SQL_data_type_object(prefix, data_type, std::move(parameter)));
                }
            }

            int num = 11;
            if (peek_type() != INTEGER_LITERAL) {
                advance_and_check_ret_obj("INT contained non-integer parameter"); }

            if (tokens[token_position].data.length() > 3) {
                advance_and_check_ret_obj("INT parameter cannot be greater than 255"); }

            num = std::stoi(tokens[token_position].data); // Missing try catch, would be fatal error anyways because integer literal should contain an actual int
            if (num < 1) {
                advance_and_check_ret_obj("INT parameter cannot be less than 1");}
            if (num > 255) {
                advance_and_check_ret_obj("INT parameter cannot be greater than 255");}

            advance_and_check_ret_obj("INT missing closing parenthesis");

            if (peek_type() != CLOSE_PAREN) {
                push_err_ret_err_obj("INT missing closing parenthesis");}

            advance_and_check_ret_obj("No values after INT()");

            data_type = INT;
            parameter = UP<object>(new integer_object(num));
            return UP<object>(new SQL_data_type_object(prefix, data_type, std::move(parameter)));
        }

    } else if (type == DEC || type == DECIMAL) {
        push_err_ret_err_obj_prev_tok("DECIMAL/DEC not supported, too complicated");
    } else if (type == FLOAT) {
        data_type = FLOAT;
        return UP<object>(new SQL_data_type_object(prefix, data_type, std::move(parameter)));
    } else if (type == DOUBLE) {
        data_type = DOUBLE;
        return UP<object>(new SQL_data_type_object(prefix, data_type, std::move(parameter)));
    }

    push_err_ret_err_obj("Unknown SQL data type (" << token_type_to_string(type) << ")");
}

static UP<object> parse_string_literal(token tok) {
    advance_and_check_ret_obj("No values after string prefix");

    if (tok.data == "*") {
        return UP<object>(new star_object()); }

    // Function call
    if (peek_type() == OPEN_PAREN) {

        advance_and_check_ret_obj("No values after open paren in function call");

        auto result = parse_comma_seperated_list(CLOSE_PAREN);
        if (!result.has_value()) {
            push_err_ret_err_obj("Failed to parse function call arguments"); }

        UP<group_object>& arguments = *result;

        advance_and_check_ret_obj("No values after closing parenthesis");
        return UP<object>(new function_call_object(tok.data, std::move(arguments)));
    }

    // Table index
    if (peek_type() == DOT) {

        advance_and_check_ret_obj("No values after dot");

        UP<object> column_name_obj = parse_expression(HIGHEST);
        if (column_name_obj->type() != STRING_OBJ) { // For now only strings (i.e. no AS)
            push_err_ret_err_obj("table.column: Failed to parse column"); }

        UP<column_index_object> obj = MAKE_UP(column_index_object, tok.data, CAST_UP(object, column_name_obj));
        return CAST_UP(object, obj);
    }

    // CREATE TABLE and function parameters. Maybe more idk
    if (is_sql_data_type_token(peek())) {
        auto result = parse_list_until_comma({CLOSE_PAREN, LINE_END}); // For now also line end so can read files
        if (!result.has_value()) {
            push_err_ret_err_obj("Failed to parse SQL Data Type"); }

        UP<group_object> group = CAST_UP(group_object, std::move(*result));

        return UP<object>(new parameter_object(tok.data, std::move(group)));
    }

    return UP<object>(new string_object(tok.data)); 
}

static UP<object> prefix_parse(token tok) {

    if (is_sql_data_type_token(tok)) {
        return parse_data_type(tok); }

    switch (tok.type) {

    // Built-in functions
    case COUNT: {
        advance_and_check_ret_obj("No values after COUNT");

        if (peek_type() != OPEN_PAREN) {
            push_err_ret_err_obj("Function missing open parenthesis"); }

        advance_and_check_ret_obj("No values after open paren in function call");

        auto result = parse_comma_seperated_list(CLOSE_PAREN);
        if (!result.has_value()) {
            push_err_ret_err_obj("Failed to parse function call arguments"); }

        UP<group_object>& arguments = *result;

        advance_and_check_ret_obj("No values after closing parenthesis");
        return UP<object>(new function_call_object(tok.data, std::move(arguments)));
    } break;
    // Built-in functions end

    // SELECT clauses
    case LEFT: {
        advance_and_check_ret_obj("No values after LEFT");

        if (peek_type() != JOIN) {
            push_err_ret_err_obj("LEFT must be followed by JOIN"); }
        
        advance_and_check_ret_obj("No values after JOIN");

        UP<object> left_table_name = parse_expression(LOWEST);
        if (left_table_name->type() != STRING_OBJ) {
            push_err_ret_err_obj("LEFT JOIN: Failed to parse left table name"); }

        if (peek_type() != ON) {
            push_err_ret_err_obj("LEFT JOIN: Only supports ON for now"); }

        advance_and_check_ret_obj("No values after ON");

        UP<object> right_expression = parse_expression(SQL_STATEMENT); // SOMETHING SUSSY HERE!!!!!!!!!!!!!!!!!!!!
        if (right_expression->type() != INFIX_EXPRESSION_OBJ) {
            log_sql_error(right_expression->data());
            push_err_ret_err_obj("LEFT JOIN: Failed to parse right expression"); 
        }

        UP<infix_expr_object> inner = MAKE_UP(infix_expr_object, MAKE_UP(operator_object, ON_OP), std::move(left_table_name), std::move(right_expression));

        return UP<object>(new prefix_expression_object(MAKE_UP(operator_object, LEFT_JOIN_OP), CAST_UP(object, inner)));
    }
    case GROUP: {
        advance_and_check_ret_obj("No values after GROUP");

        if (peek_type() != BY) {
            push_err_ret_err_obj("GROUP must be followed by BY"); }
        
        advance_and_check_ret_obj("No values after BY");

        auto result = parse_comma_seperated_list_end_if_not_comma();
        if (!result.has_value()) {
            push_err_ret_err_obj("GROUP BY: failed to get column indexes"); }

        UP<group_object>& column_indexes_obj = *result;

        return UP<object>(new prefix_expression_object(MAKE_UP(operator_object, GROUP_BY_OP), CAST_UP(object, column_indexes_obj)));
    }
    case ORDER: {
        advance_and_check_ret_obj("No values after ORDER");

        if (peek_type() != BY) {
            push_err_ret_err_obj("ORDER must be followed by BY"); }
        
        advance_and_check_ret_obj("No values after BY");

        auto result = parse_comma_seperated_list_end_if_not_comma();
        if (!result.has_value()) {
            push_err_ret_err_obj("ORDER BY: failed to get column indexes"); }

        UP<group_object>& column_indexes_obj = *result;

        // Can't check if they are specifically Column Index Objects cause they can be aliases, bruh

        return UP<object>(new prefix_expression_object(MAKE_UP(operator_object, ORDER_BY_OP), CAST_UP(object, column_indexes_obj)));
    }
    // SELECT clauses end

    // Table column expressions
    case FOREIGN_KEY: { // !!MAJOR HERE
        advance_and_check_ret_obj("No values after FOREIGN_KEY");
        if (peek_type() != REFERENCES) {
            push_err_ret_err_obj("FOREIGN_KEY: Missing REFERENCES"); }
        advance_and_check_ret_obj("No values after REFERENCES");

        UP<object> reference = parse_expression(HIGHEST); // TODO Not sure is predence is correct
        if (reference->type() == ERROR_OBJ) {
            push_err_ret_err_obj("FOREIGN_KEY: Failed to parse reference"); }
        if (reference->type() != COLUMN_INDEX_OBJ) {
            push_err_ret_err_obj("FOREIGN_KEY: Reference must be in the style of TABLE_NAME.COLUMN_NAME"); }

        auto key_obj = MAKE_UP(foreign_key_object, CAST_UP(column_index_object, reference));
        auto tab_col_expr = MAKE_UP(table_column_expr, CAST_UP(object, key_obj));
        return CAST_UP(object, tab_col_expr);

    } break;
    // Table expressions
    case DELIMITER: {
        advance_and_check_ret_obj("No values after DELIMITER");
        if (peek_type() != OPEN_PAREN) {
            push_err_ret_err_obj("DELIMITER: Missing open parenthesis"); }

        advance_and_check_ret_obj("DELIMITER: Missing column name");
        if (peek_type() != STRING_LITERAL) {
            push_err_ret_err_obj("DELIMITER: Must be in the style of DELIMITER(...)"); }

        astring value = peek_data();

        advance_and_check_ret_obj("DELIMITER: Missing closing parenthesis");
        
        advance_and_check_ret_obj("No values after DELIMITER()");

        auto obj = MAKE_UP(delimiter_object, value);
        auto col_expr = MAKE_UP(table_expr, CAST_UP(object, obj));
        return CAST_UP(object, col_expr);
    } break;
    case PRIMARY_KEY: {
        advance_and_check_ret_obj("No values after PRIMARY_KEY");
        if (peek_type() != OPEN_PAREN) {
            push_err_ret_err_obj("PRIMARY_KEY: Missing open parenthesis"); }

        advance_and_check_ret_obj("PRIMARY_KEY: Missing column name");
        if (peek_type() != STRING_LITERAL) {
            push_err_ret_err_obj("PRIMARY_KEY: Must be in the style of PRIMARY_KEY([COLUMN_NAME])"); }

        astring col_name = peek_data();

        advance_and_check_ret_obj("PRIMARY_KEY: Missing closing parenthesis");
        
        advance_and_check_ret_obj("No values after PRIMARY_KEY()");

        auto obj = MAKE_UP(primary_key_object, col_name);
        auto col_expr = MAKE_UP(table_expr, CAST_UP(object, obj));
        return CAST_UP(object, col_expr);
    } break;
    case CONSTRAINT: {
        advance_and_check_ret_obj("No values after CONSTRAINT");
        UP<object> constraint = parse_expression(HIGHEST); // TODO Not sure is predence is correct
        if (constraint->type() == ERROR_OBJ) {
            push_err_ret_err_obj("CONSTRAINT: Failed to parse constraint"); }

        token_type method = peek_type();
        advance_and_check_ret_obj("No values after CONSTRAINT method");
        // Allowed methods
        switch (method) {
        case IGNORE:
            break;
        default: 
            push_err_ret_err_obj("CONSTRAINT method (" << token_type_to_string(method) << ") not allowed");
        }

        auto constraint_obj = MAKE_UP(constraint_object, std::move(constraint), method);
        auto tab_expr = MAKE_UP(table_expr, CAST_UP(object, constraint_obj));
        return CAST_UP(object, tab_expr);
    } break;
    // Constraints
    case UNIQUE: {
        advance_and_check_ret_obj("No values after UNIQUE");
        if (peek_type() != OPEN_PAREN) {
            push_err_ret_err_obj("UNIQUE: Missing open parenthesis"); }
        advance_and_check_ret_obj("UNIQUE: Missing elements");
        auto result = parse_comma_seperated_list(CLOSE_PAREN);
        if (!result.has_value()) {
            push_err_ret_err_obj("UNIQUE: Failed to parse elements"); }

        UP<group_object> group = std::move(*result);

        if (peek_type() != CLOSE_PAREN) {
            push_err_ret_err_obj("UNIQUE: Missing clossing parenthesis"); }
        advance_and_check_ret_obj("UNIQUE: Missing clossing parenthesis");

        return UP<object>(new unique_object(std::move(group)));
    } break;
    // End table column expressions

    case DEFAULT: {
        advance_and_check_ret_obj("No values after DEFAULT");
        UP<object> value = parse_expression(LOWEST);
        if (value->type() == ERROR_OBJ) {
            return value; }
        return UP<object>(new default_value_object(std::move(value)));
    } break;
    // Default value functions
    case CURRENT_TIMESTAMP: {
        advance_and_check_ret_obj("No values after CURRENT_TIMESTAMP");
        auto cur_time = MAKE_UP(current_timestamp_object);
        auto func = MAKE_UP(default_value_func, CAST_UP(object, cur_time));
        // auto def_val = MAKE_UP(default_value_object, CAST_UP(object, func));
        return CAST_UP(object, func);
    } break;
    case AUTO_INCREMENT: {
        advance_and_check_ret_obj("No values after AUTO_INCREMENT");
        auto auto_inc = MAKE_UP(auto_increment_object);
        auto func = MAKE_UP(default_value_func, CAST_UP(object, auto_inc));
        auto def_val = MAKE_UP(default_value_object, CAST_UP(object, func));
        return CAST_UP(object, def_val);
    } break;
    case HASH: {
        advance_and_check_ret_obj("No values after HASH");
        if (peek_type() != OPEN_PAREN) {
            advance_and_check_ret_obj("HASH: Missing open parenthesis"); }
        advance_and_check_ret_obj("HASH: Missing values");

        auto result = parse_comma_seperated_list_ADVANCED({CLOSE_PAREN, LINE_END});
        if (!result.has_value()) {
            push_err_ret_err_obj("HASH: Failed to parse elements"); }

        auto [group, end_type] = std::move(*result);
        if (group->elements.size() == 0) {
            push_err_ret_err_obj("HASH: Must contain at least one element"); }

        if (peek_type() != CLOSE_PAREN) {
            push_err_ret_err_obj("HASH: Missing clossing parenthesis"); }
        advance_and_check_ret_obj("HASH: Missing clossing parenthesis");
        
        auto hash_obj = MAKE_UP(hash_object, std::move(group));
        auto func = MAKE_UP(default_value_func, CAST_UP(object, hash_obj));

        return UP<object>(new default_value_object(CAST_UP(object, func)));
    } break;
    // End default value functions
    case SELECT:
        return parse_select();
    case VALUES: {
        auto result = parse_values();
        if (result.has_value()) {
            return CAST_UP(object, std::move(*result));
        } else {
            return CAST_UP(object, std::move(result).error());
        }
    } break;
    case SEMICOLON:
        advance_and_check_ret_obj("No values after semicolon prefix");
        return UP<object>(new semicolon_object()); break;
    case MINUS: {
        advance_and_check_ret_obj("No values after - prefix");
        UP<object> right = parse_expression(LOWEST);
        if (right->type() == ERROR_OBJ) {
            return right; }

        return UP<object>(new prefix_expression_object(MAKE_UP(operator_object, NEGATE_OP), std::move(right)));
    } break;
    case INTEGER_LITERAL:
        advance_and_check_ret_obj("No values after integer prefix");
        return UP<object>(new integer_object(tok.data)); break;
    case STRING_LITERAL:
        return parse_string_literal(tok);
    case OPEN_PAREN: {
        advance_and_check_ret_obj("No values after open parenthesis in expression");
        UP<object> expression = parse_expression(LOWEST);
        if (peek_type() != CLOSE_PAREN) {
            push_err_ret_err_obj("Expected closing parenthesis, got (" << token_type_to_string(peek_type()) << ")");}
        advance_and_check_ret_obj("No values after close parenthesis in expression");
        return expression;
    } break;
    case ASTERISK:
        advance_and_check_ret_obj("No values after * prefix");
        return UP<object>(new star_object()); break;
    case AS:
        return parse_prefix_as(); break;
    case IF: case ELSIF: {
        auto result = parse_prefix_if();
        if (result.has_value()) {
            return CAST_UP(object, std::move(*result));
        } else {
            return CAST_UP(object, std::move(result).error());
        }
    } break;
    case LESS_THAN:
        advance_and_check_ret_obj("No values after less than in expression");
        if (token_position < tokens.size()) { // NOT CHECKED !!
            if (peek_type() == EQUAL) {
                advance_and_check_ret_obj("No values after <= in expression");
                return UP<object>(new operator_object(LESS_THAN_OR_EQUAL_TO_OP));
            }
        }
        return UP<object>(new operator_object(LESS_THAN_OP));
    case RETURN: {
        advance_and_check_ret_obj("No values after RETURN in expression");
        UP<object> expression = parse_expression(LOWEST);
        if (peek_type() == SEMICOLON) {
            advance_and_check_ret_obj("No values after SEMICOLON in return statement"); }
        return UP<object>(new return_statement(std::move(expression)));
    } break;
    case ELSE: {
        auto result = parse_block_statement();
        if (result.has_value()) {
            return CAST_UP(object, std::move(*result));
        } else {
            return CAST_UP(object, std::move(result).error());
        }
    } break;
    case END:
        advance_and_check_ret_obj("No values after END in expression");
        if (peek_type() == IF) {
            advance_and_check_ret_obj("No values after END IF in expression");
            if (peek_type() == SEMICOLON) {
                advance_and_check_ret_obj("No values after SEMICOLON in END IF statement"); }
            return UP<object>(new end_if_statement()); 
        }
        if (peek_type() == SEMICOLON) {
            advance_and_check_ret_obj("No values after SEMICOLON in END statement"); }

        return UP<object>(new end_statement()); break;
    case LINE_END:
        FATAL_ERROR_STACK_TRACE_THROW("Received LINE_END token, which should have been dealt with already", CUR_LOC);
    default:
        push_err_ret_err_obj("No prefix function for (" + token_type_to_string(tok.type) + ")");
    }
}

static std::expected<UP<infix_expr_object>, UP<error_object>> infix_parse(UP<object> left) {
    switch (peek_type()) {
    case PLUS: case MINUS: case ASTERISK: case SLASH: case DOT: case EQUAL: case GREATER_THAN: case LESS_THAN: {
        return parse_infix_operator(std::move(left));
    } break;
    case AS: {
        if (left->type() == COLUMN_INDEX_OBJ) {

            advance_and_check_ret_unx_obj("No values after AS");

            UP<object> right = parse_expression(LOWEST);
            if (right->type() == ERROR_OBJ) {
                push_err_ret_unx_err_obj("Failed to parse infix AS"); }

            return MAKE_UP(infix_expr_object, MAKE_UP(operator_object, AS_OP), std::move(left), std::move(right));
        } else {
            push_err_ret_unx_err_obj("AS functionallity not implemented except as column index");
        }
    } break;
    case WHERE: {
        advance_and_check_ret_unx_obj("No values after WHERE");

        UP<object> expression = parse_expression(LOWEST);
        if (expression->type() != INFIX_EXPRESSION_OBJ) {
            push_err_ret_unx_err_obj("WHERE: Failed to parse expression"); }


        return MAKE_UP(infix_expr_object, MAKE_UP(operator_object, WHERE_OP), std::move(left), std::move(expression));  // Might have to make prefix
    }
    case LEFT: {
        advance_and_check_ret_unx_obj("No values after LEFT");

        if (peek_type() != JOIN) {
            push_err_ret_unx_err_obj("LEFT must be followed by JOIN"); }
        
        advance_and_check_ret_unx_obj("No values after JOIN");

        UP<object> left_table_name = parse_expression(LOWEST);
        if (left_table_name->type() != STRING_OBJ) {
            push_err_ret_unx_err_obj("LEFT JOIN: Failed to parse left table name"); }

        if (peek_type() != ON) {
            push_err_ret_unx_err_obj("LEFT JOIN: Only supports ON for now"); }

        advance_and_check_ret_unx_obj("No values after ON");

        UP<object> right_expression = parse_expression(LOWEST);
        if (right_expression->type() != INFIX_EXPRESSION_OBJ) {
            push_err_ret_unx_err_obj("LEFT JOIN: Failed to evaluate right expression"); }

        UP<infix_expr_object> inner = MAKE_UP(infix_expr_object, MAKE_UP(operator_object, ON_OP), std::move(left_table_name), std::move(right_expression));

        return MAKE_UP(infix_expr_object, MAKE_UP(operator_object, LEFT_JOIN_OP), std::move(left), CAST_UP(object, inner));
    }
    default:
        push_err_ret_unx_err_obj("No infix function for (" + token_type_to_string(peek_type()) + ") and left (" + std::string(left->inspect()) + ")");
    }
}   



UP<object> parse_expression(size_t precedence, const token& tok) {
    
    // std::cout << "parse_expression called with " << token_type_to_string(peek_type()) << std::endl;

    UP<object> left = prefix_parse(tok);
    if (left->type() == ERROR_OBJ) {
        return left;}

    while (peek_type() != LINE_END &&
           peek_type() != CLOSE_PAREN &&
           peek_type() != SEMICOLON &&
           peek_type() != COMMA && 
           precedence < numeric_precedence(peek()) ) {
            const UP<object>& prev_left = left; // Having prev_left allows for no-ops
            auto result = infix_parse(std::move(left));
            if (!result.has_value()) {
                return CAST_UP(object, std::move(result).error());
            } else {
                left = CAST_UP(object, std::move(*result));
            }

            if (left == prev_left) {
                break; }
            
           }     
        
    return left;
}
        
// Helpers

token peek() {
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
