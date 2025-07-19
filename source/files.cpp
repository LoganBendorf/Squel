
#include "pch.h"

#include "files.h"

#include "print.h" // For DEBUG

#include "allocator_aliases.h"
#include "object.h"
#include "lexer.h"
#include "helpers.h"
#include "environment.h"
#include "parser.h"
#include "evaluator.h"
#include "execute.h"

extern bool DEBUG;

extern std::vector<std::string> errors;



#define parse_table_details_err(x)                                              \
    std::cerr << GET_ERROR_LOCATION(CUR_LOC) << ": Error: " << x << std::endl;  \
    for (const auto& err: errors) {                                             \
        std::cerr << err << std::endl; }                                        \
    errors.clear();                                                             \
    return {avec<UP<s_table_detail_object>>{}, avec<UP<s_table_expr>>{}, false}    

#define parse_table_exprs_err(x)                                                \
    std::cerr << GET_ERROR_LOCATION(CUR_LOC) << ": Error: " << x << std::endl;  \
    for (const auto& err: errors) {                                             \
        std::cerr << err << std::endl; }                                        \
    errors.clear();                                                             \
    return {{}, false}    

#define deserialize_table_err(x)                                                \
    std::cerr << GET_ERROR_LOCATION(CUR_LOC) << ": Error: " << x << std::endl;  \
    for (const auto& err: errors) {                                             \
        std::cerr << err << std::endl; }                                        \
    errors.clear();                                                             \
    return {nullptr, false}                                     



static std::pair<UP<s_group_object>, bool> parse_row(const avec<UP<s_table_detail_object>>& column_details, std::string& raw);

static constexpr bool is_digit(char a) {
    return (std::isdigit(a) != 0);
}

static void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

static void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

[[maybe_unused]] static std::string trim(std::string&& s) {
    std::string str = std::move(s);
    rtrim(str);
    ltrim(str);
    return str;
}

static std::string trim_copy(std::string str) {
    rtrim(str);
    ltrim(str);
    return str;
}

bool save_tables(const avec<SP<table_object>>& tables, const std::string& out_path) {
    
    if (DEBUG) [[unlikely]] { std::cout << "Saving tables to " << out_path << "..." << std::endl; }

    /* Create directory if it doesn't exist */
    const std::filesystem::path path(out_path);
    std::filesystem::create_directories(path.parent_path());

    std::ofstream out_file(out_path);
    if (!out_file) {
        std::cerr << "Error: Could not open " << out_path << " for writing" << std::endl;;
        return false;
    }

    for (const auto& table : tables) {
        out_file << table->serialize();
        if (out_file.fail()) {
            std::cerr << "Error: Failed to write table data" << std::endl;
            return false;
        }
    } 

    out_file.close();

    if (DEBUG) [[unlikely]] { std::cout << "Saved " << tables.size() << " table" << (tables.size() > 1 ? "s" : "") << " to " << out_path << std::endl; }
    return true;
}



static std::tuple<avec<UP<s_table_detail_object>>, avec<UP<s_table_expr>>, bool> parse_table_details(std::string& raw, size_t col_count) {

    std::vector<std::string> raw_details;
    std::stringstream ss(raw);
    std::string item;
    while (std::getline(ss, item, '|')) {
        raw_details.push_back(item); }

    if (raw_details.size() != col_count) [[unlikely]] {
        parse_table_details_err("Table detail column count mismatch. Expected " << col_count << ", got " << raw_details.size()); } 

    avec<UP<s_table_detail_object>> details;
    avec<UP<s_table_expr>> tab_exprs;
    for (const auto& detail : raw_details) {
        std::vector<std::string> pieces;
        std::stringstream stream(detail);
        while (std::getline(stream, item, ' ')) {
            pieces.push_back(item); }
        
        if (pieces.size() < 2) {
            parse_table_details_err("Table detail did not contain enough info, got " << pieces.size() << " from (" << trim_copy(detail) << ")"); }

        auto tokens = lexer(detail);
        if (tokens.size() == 0) [[unlikely]] {
            parse_table_details_err("Could not lex column detail (" << trim_copy(detail) << "), no tokens"); }
        if (!errors.empty()) [[unlikely]] {
            parse_table_details_err("Could not lex column detail (" << trim_copy(detail) << ")"); }

        // std::vector<token> needed_tokens(tokens.begin() + 1, tokens.end());
        parser_init(tokens);
        UP<object> obj = parse_expression(0, tokens[0]);
        if (obj->type() == ERROR_OBJ) {
            parse_table_details_err("Parser, " << obj->inspect()); }
        if (DEBUG) [[unlikely]] { std::cout << obj->inspect() << std::endl; }

        UP<evaluated> evl = eval_expression(std::move(obj), MAKE_SP(environment));
        if (evl->type() == ERROR_OBJ) {
            parse_table_details_err("Evaluator, " << evl->inspect()); }
        if (DEBUG) [[unlikely]] { std::cout << evl->inspect() << std::endl; }

        UP<serializable> ser = make_serializable(std::move(evl), MAKE_SP(s_environment));
        if (ser->type() == ERROR_OBJ) {
            parse_table_details_err("Serializer, " << ser->inspect()); }
        if (DEBUG) [[unlikely]] { std::cout << ser->inspect() << std::endl; }
        
        if (ser->type() != S_PARAMETER_OBJ) {
            parse_table_details_err("Could not make column detail serializable. Got Type: " << object_type_to_astring(ser->type()) << " (" << ser->inspect() << ")"); }

        auto result = parameter_to_table_detail(CAST_UP(s_parameter_object, ser));
        if (!result.has_value()) {
            parse_table_details_err(result.error()->data()); }

        details.push_back(std::move(*result));

    }

    return {std::move(details), std::move(tab_exprs), true};
}

static std::pair<avec<UP<s_table_expr>>, bool> parse_table_exprs(std::string& raw, size_t expr_count) {

    std::vector<std::string> raw_exprs;
    std::stringstream ss(raw);
    std::string item;
    while (std::getline(ss, item, '|')) {
        raw_exprs.push_back(item); }

    if (raw_exprs.size() != expr_count) {
        parse_table_exprs_err("Table expression count mismatch. Expected " << expr_count << ", got " << raw_exprs.size()); } 

    avec<UP<s_table_expr>> exprs;
    exprs.reserve(raw_exprs.size());
    for (const auto& expr : raw_exprs) {

        auto tokens = lexer(expr);
        if (tokens.size() == 0) [[unlikely]] {
            parse_table_exprs_err("Could not lex expression (" << trim_copy(expr) << "), no tokens"); }
        if (!errors.empty()) [[unlikely]] {
            parse_table_exprs_err("Could not lex expression (" << trim_copy(expr) << ")"); }

        parser_init(tokens);
        UP<object> obj = parse_expression(0, tokens[0]);
        if (obj->type() == ERROR_OBJ) {
            parse_table_exprs_err("Parser, " << obj->inspect()); }
        if (DEBUG) [[unlikely]] { std::cout << obj->inspect() << std::endl; }

        UP<evaluated> evl = eval_expression(std::move(obj), MAKE_SP(environment));
        if (evl->type() == ERROR_OBJ) {
            parse_table_exprs_err("Evaluator, " << evl->inspect()); }
        if (DEBUG) [[unlikely]] { std::cout << evl->inspect() << std::endl; }

        UP<serializable> ser = make_serializable(std::move(evl), MAKE_SP(s_environment));
        if (ser->type() == ERROR_OBJ) {
            parse_table_exprs_err("Serializer, " << ser->inspect()); }
        if (DEBUG) [[unlikely]] { std::cout << ser->inspect() << std::endl; }
        
        if (ser->type() != S_TABLE_EXPR_OBJ) {
            parse_table_exprs_err("Could not make expression serializable. Got Type: " << object_type_to_astring(ser->type()) << " (" << ser->inspect() << ")"); }

        exprs.push_back(CAST_UP(s_table_expr, ser));
    }

    return {std::move(exprs), true};
}

static std::pair<avec<UP<s_group_object>>, bool> parse_rows(const avec<UP<s_table_detail_object>>& column_details, std::vector<std::string>& raw, size_t line_start) {

    constexpr size_t row_info_index = 5;
    std::string row_count_str = std::move(raw[row_info_index]);

    const size_t row_count_end_index = row_count_str.size() - 1;
    size_t row_count_start_index = 0;
    for (size_t i = 0; i < row_count_str.size(); i++) {
        if (is_digit(row_count_str[i])) { row_count_start_index = i; break; }
    }
    row_count_str = row_count_str.substr(row_count_start_index, row_count_end_index);

    size_t row_count = 0;
    try {
        row_count = std::stoull(row_count_str);
    } catch (const std::invalid_argument& ia) { // string isn't a valid unsigned integer
        FATAL_ERROR_THROW("Could not convert (" << row_count_str << ") to size_t", CUR_LOC); } 
    catch (const std::out_of_range& oor) { // numeric value is too large
        FATAL_ERROR_THROW("Value out of range for size_t (" << row_count_str << ")", CUR_LOC); }

    const size_t real_size = raw.size() - (row_info_index + 1);

    if (real_size != row_count) {
        std::cerr << "Error: Row count mismatch. Expected " << row_count << ", got " << real_size << std::endl; 
        return {{}, false};
    } 


    avec<UP<s_group_object>> rows;
    rows.reserve(real_size);
    for (size_t i = (row_info_index + 1); i < raw.size(); i++) {
        std::string line = std::move(raw[i]);
        if (DEBUG) [[unlikely]] { std::cout << "Reading row: " << line << std::endl; };
        auto [row, ok] = parse_row(column_details, line);
        if (!ok) {
            std::cerr << "Error: Could not parse table row. Line " << (line_start + i) << std::endl; return {{}, false};
        }

        rows.push_back(std::move(row));        
    }

    return {std::move(rows), true};
    
}

static std::pair<UP<s_group_object>, bool> parse_row(const avec<UP<s_table_detail_object>>& column_details, std::string& raw) {

    std::vector<std::string> cells;
    std::stringstream ss(raw);
    std::string item;
    while (std::getline(ss, item, '|')) {
        cells.push_back(item); }

    if (cells.size() != column_details.size()) {
        FATAL_ERROR_STACK_TRACE_THROW("Row size did not match correct number of columns", CUR_LOC); }

    avec<UP<serializable>> row;
    row.reserve(cells.size());
    for (size_t i = 0; i < cells.size(); i++) {

        // FIXME Kind of hacky fix, should probably remove once paramterized queries work.
        // Has the side effect of eating whitespace
        if (is_string_data_type(column_details[i]->data_type->data_type)) {
            auto str_obj = MAKE_UP(string_object, trim_copy(cells[i]));
            row.push_back(CAST_UP(serializable, str_obj));
            continue;
        }

        const auto& cell = cells[i];

        std::vector<token> tokens = lexer(cell);
        if (tokens.size() == 0) {
            std::cerr << "Error: Row could not parse value (" << cell << ")" << std::endl; return {{}, false}; }

        if (DEBUG) [[unlikely]] { print_tokens(tokens); }

        parser_init(tokens);
        UP<object> obj       = parse_expression(0, tokens[0]);
        UP<evaluated> evaled = eval_expression(std::move(obj), MAKE_SP(environment));
        UP<serializable> ser;
        if (!is_serializable(evaled)) {
            ser = make_serializable(std::move(evaled), MAKE_SP(s_environment));
            if (ser->type() == ERROR_OBJ) {
                std::cerr << "Error: Row could not parse value" << std::endl; return {{}, false};
            }
        } else {
            ser = CAST_UP(serializable, evaled);
        }

        row.push_back(std::move(ser));
    }

    return {MAKE_UP(s_group_object, std::move(row)), true};
}

static std::pair<SP<table_object>, bool> deserialize_table(std::vector<std::string> raw, size_t line_start) {
    
    constexpr size_t table_name_index = 0;
    std::string table_name = std::move(raw[table_name_index]);

    constexpr size_t column_info_index = 1;
    std::string col_count_str = std::move(raw[column_info_index]);

    const size_t col_count_end_index = col_count_str.size() - 1;
    size_t col_count_start_index = 0;
    for (size_t i = 0; i < col_count_str.size(); i++) {
        if (is_digit(col_count_str[i])) { col_count_start_index = i; break; }
    }
    col_count_str = col_count_str.substr(col_count_start_index, col_count_end_index);

    size_t col_count = 0;
    try {
        col_count = std::stoull(col_count_str);
    } catch (const std::invalid_argument& ia) { // string isn't a valid unsigned integer
        FATAL_ERROR_THROW("Could not convert (" << col_count_str << ") to size_t", CUR_LOC); } 
    catch (const std::out_of_range& oor) { // numeric value is too large
        FATAL_ERROR_THROW("Value out of range for size_t (" << col_count_str << ")", CUR_LOC); }



    constexpr size_t expr_info_index = 3;
    std::string expr_count_str = std::move(raw[expr_info_index]);

    const size_t expr_count_end_index = expr_count_str.size() - 1;
    size_t expr_count_start_index = 0;
    for (size_t i = 0; i < expr_count_str.size(); i++) {
        if (is_digit(expr_count_str[i])) { expr_count_start_index = i; break; }
    }
    expr_count_str = expr_count_str.substr(expr_count_start_index, expr_count_end_index);

    size_t expr_count = 0;
    try {
        expr_count = std::stoull(expr_count_str);
    } catch (const std::invalid_argument& ia) { // string isn't a valid unsigned integer
        FATAL_ERROR_THROW("Could not convert (" << expr_count_str << ") to size_t", CUR_LOC); } 
    catch (const std::out_of_range& oor) { // numeric value is too large
        FATAL_ERROR_THROW("Value out of range for size_t (" << expr_count_str << ")", CUR_LOC); }




    constexpr size_t tab_col_details_index = column_info_index + 1;
    auto [table_details, table_exprs, details_ok] = parse_table_details(raw[tab_col_details_index], col_count);
    if (!details_ok) {
        deserialize_table_err("Failed to parse table details. Line " << (tab_col_details_index + 1)); }

    constexpr size_t expr_index = expr_info_index + 1;
    auto [exprs, exprs_ok] = parse_table_exprs(raw[expr_index], expr_count);
    if (!exprs_ok) {
        deserialize_table_err("Failed to parse table expressions. Line " << (expr_index + 1)); }

    auto [rows, rows_ok] = parse_rows(table_details, raw, line_start);
    if (!rows_ok) {
        deserialize_table_err("Failed to parse table rows"); }

    
    
    avec<UP<s_table_expr>> combined_exprs;
    combined_exprs.reserve(table_exprs.size() + exprs.size());
    for (auto& expr : table_exprs) {
        combined_exprs.push_back(std::move(expr)); }
    for (auto& expr : exprs) {
        combined_exprs.push_back(std::move(expr)); }

    return {MAKE_SP(table_object, table_name, std::move(table_details), std::move(combined_exprs), std::move(rows)), true};
}

#define read_saved_files_err(x)          \
    for (const auto& err: errors) {      \
        std::cerr << err << std::endl; } \
    FATAL_ERROR_STACK_TRACE_THROW(x, CUR_LOC)

bool read_saved_files(avec<SP<table_object>>& tables, const std::filesystem::path& in_path) {

    // Nothing to read from
    if (!std::filesystem::exists(in_path)) {
        if (DEBUG) [[unlikely]] { std::cout << "No files to read from" << std::endl; }
        return true;
    }

    if (DEBUG) [[unlikely]] { std::cout << "Loading tables from " << in_path << "..." << std::endl; }

    std::ifstream in_file(in_path);
    if (!in_file) {
        std::cerr << "Error: Could not open " << in_path << " for reading" << std::endl;
        return false;
    }

    int table_count = 0;
    std::string line;
    std::vector<std::string> raw;
    size_t line_count = 0;
    size_t line_start = 0;
    std::getline(in_file, line); // Remove first empty lien
    while (std::getline(in_file, line)) {
        line_count++;
        if (line.empty() ) {
            if (raw.size() < 3) {
                read_saved_files_err("Deserialize table called with incorrect size. Line " << line_start); }

            if (DEBUG) [[unlikely]] { std::cout << "Table (" << raw[0] << ") starting on row " << line_start << std::endl; }

            auto [table, ok] = deserialize_table(std::move(raw), line_start);
            if (!ok) {
                read_saved_files_err("Failed to deserialize table. Line " << line_start); }

            tables.push_back(SP<table_object>(table));
            table_count++;
            raw = std::vector<std::string>();
            line_start = line_count;
        } else {
            raw.push_back(std::move(line)); // Maybe dont move if it doesnt work
        }
    }
    if (line.empty()) {
        line_count++;
        if (line.empty() ) {
            auto [table, ok] = deserialize_table(std::move(raw), line_start);
            if (!ok) {
                read_saved_files_err("Failed to deserialize table. Line " << line_start); }

            tables.push_back(SP<table_object>(table));
            table_count++;
            raw.clear();
            line_start = line_count;
        }
    }

    in_file.close();
    
    if (DEBUG) [[unlikely]] { std::cout << "Loaded " << table_count << " tables from " << in_path << std::endl; }
    
    return true;
}