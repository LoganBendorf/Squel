module;

#include "pch.h"

#include "structs_and_macros.h"
#include "allocator_aliases.h"
#include "token.h"

#include <array> 

import object;
import helpers;
import node;
import environment;

extern std::vector<std::string> errors;
extern display_table display_tab;

extern std::vector<std::string> warnings;

extern avec<SP<table_object>> g_tables;
extern avec<SP<evaluated_function_object>> g_functions;

module execute;


static avec<UP<e_node>> nodes;


#define exec_push_err_ret(x)                    \
    std::stringstream err;                      \
    err << (x);                                 \
    errors.emplace_back(std::move(err).str());  \
    return                        

#define push_err_ret_err_obj(...)               \
    do {                                        \
        std::stringstream err;                  \
        err << __VA_ARGS__;                     \
        errors.emplace_back(std::move(err).str());\
        return UP<evaluated>(new error_object());  \
    } while(0)


#define push_err_ret_unx_err_obj(...)           \
    do {                                        \
        std::stringstream err;                  \
        err << __VA_ARGS__;                     \
        errors.emplace_back(std::move(err).str()); \
        return std::unexpected(MAKE_UP(error_object)); \
    } while(0)


static void exec_create_table(UP<e_create_table> info);
static void exec_insert_into(UP<e_insert_into> wrapper, SP<environment> env);
static void exec_select_from(UP<e_select_from> wrapper, SP<environment> env);

static std::expected<UP<evaluated>, UP<error_object>> get_insertable(UP<evaluated> insert_obj, const UP<e_SQL_data_type_object>& data_type);
static std::expected<UP<evaluated>, UP<error_object>> get_insertable(evaluated* insert_obj,    const UP<e_SQL_data_type_object>& data_type);
static std::pair<SP<table_object>, bool> get_table(const std_and_astring_variant& name);
static std::pair<const SP<table_object>&, bool> get_table_as_const(const std_and_astring_variant& name);


void execute_init(avec<UP<e_node>> nds) {
    nodes       = std::move(nds);
}

void execute() {
    
    SP<environment> env = MAKE_SP(environment);
    
    for (auto& node : nodes) {

        switch(node->type()) {
        case E_INSERT_INTO_NODE:
            std::cout << "EXECUTE INSERT INTO CALLED\n";
            exec_insert_into(cast_UP<e_insert_into>(node), env);
            break;
        // case E_SELECT_NODE: {
        //     UP<evaluated> unwrapped = std::move(cast_UP<e_select_node>(node)->value);
        //     if (unwrapped->type() != E_SELECT_OBJECT) {
        //         errors.emplace_back("Select node contained errors object"); break; }

        //     UP<e_select_object> sel_obj = cast_UP<e_select_object>(unwrapped);
        //     auto result = execute_select(std::move(sel_obj), env);
        //     std::cout << "EXECUTE SELECT CALLED\n";
        // } break;
        case E_SELECT_FROM_NODE: {
            std::cout << "EXECUTE SELECT FROM CALLED\n";
            exec_select_from(cast_UP<e_select_from>(node), env);
        } break;
        case E_CREATE_TABLE_NODE:
            std::cout << "EXECUTE CREATE TABLE CALLED\n";
            exec_create_table(cast_UP<e_create_table>(node));
            break;
        // case E_ALTER_TABLE_NODE:
        //     execute_alter_table(cast_UP<e_alter_table>(node), env);
        //     std::cout << "EXECUTE ALTER TABLE CALLED\n";
        //     break;
        // case E_ASSERT_NODE:
        //     execute_assert(cast_UP<e_assert_node>(node), env);
        //     std::cout << "EXECUTE FUNCTION CALLED\n";INSERT INTO exec:
        //     break;
        default:
            errors.emplace_back("execute: unknown node type (" + node->inspect() + ")");
        }
    }

    nodes.clear();
    nodes = avec<UP<e_node>>();
}

static void exec_insert_into(UP<e_insert_into> wrapper, [[maybe_unused]] SP<environment> env) {

    if (wrapper->value->type() != E_INSERT_INTO_OBJECT) {
        exec_push_err_ret("exec_insert_into(): Called with invalid object (" + object_type_to_astring(wrapper->value->type()) + ")"); }

    UP<e_insert_into_object> info = cast_UP<e_insert_into_object>(wrapper->value);

    astring table_name = info->table_name;

    const auto& [table, tab_found] = get_table(table_name);
    if (!tab_found) {
        exec_push_err_ret("INSERT INTO exec: table not found");}

    avec<UP<evaluated>> fields = std::move(info->fields);

    avec<UP<evaluated>> values = std::move(info->values);
        
    if (values.size() == 0) {
        exec_push_err_ret("INSERT INTO exec: no values");}

    if (fields.size() < values.size()) {
        exec_push_err_ret("INSERT INTO exec: more values than field names");}

    if (fields.size() > table->column_datas.size()) {
        exec_push_err_ret("INSERT INTO exec: more field names than table has columns");}

    if (values.size() > table->column_datas.size()) {
        exec_push_err_ret("INSERT INTO exec: more values than table has columns");}



    // check if fields exist
    for (const auto& field : fields) {
        bool found = table->check_if_field_name_exists(field->data());
        if (!found) {
            exec_push_err_ret("INSERT INTO exec: Could not find field (" + field->inspect() + ") in table + (" + table->table_name + ")"); }
    }


    avec<UP<evaluated>> row;
    if (info->fields.size() > 0) {

        // If field matches column name, add. else add default value
        for (size_t i = 0; i < table->column_datas.size(); i++) {
            bool found = false;
            size_t id = 0;
            for (size_t j = 0; j < fields.size(); j++) {
                const auto& [column_name, in_bounds] = table->get_column_name(i);
                if (!in_bounds) {
                    exec_push_err_ret("INSERT INTO exec: Weird index bug");}

                if (column_name == fields[j]->data()) {
                    id = j;
                    found = true; 
                    break; 
                } 
            }

            if (!found) {
                auto result = table->get_cloned_column_default_value(i);
                if (!result.has_value()) {
                    exec_push_err_ret("INSERT INTO exec: Weird index bug 2");}
                row.emplace_back(std::move(*result)); }

            if (found) {
                if (id >= values.size()) {
                    exec_push_err_ret("INSERT INTO exec: Weird index bug 3"); }
                UP<evaluated> value = std::move(values[id]);

                const auto& [data_type, in_bounds] = table->get_column_data_type(i);
                if (!in_bounds) {
                    exec_push_err_ret("INSERT INTO exec: Weird index bug 4");}

                auto result = get_insertable(value->clone(), data_type);
                if (!result.has_value()) {
                    errors.emplace_back(result.error()->data());
                    exec_push_err_ret("INSERT INTO exec: Value (" + value->inspect() + ") evaluated to non-insertable value while inserting rows"); }

                row.emplace_back(std::move(*result)); 
            }

        }
    } else {
        size_t i = 0;
        for (i = 0; i < values.size(); i++) {

            UP<evaluated> value = std::move(values[i]);

            const auto& [data_type, in_bounds] = table->get_column_data_type(i);
            if (!in_bounds) {
                exec_push_err_ret("INSERT INTO exec: Weird index bug 4");}
                
            auto result = get_insertable(value->clone(), data_type);
                if (!result.has_value()) {
                    errors.emplace_back(result.error()->data());
                exec_push_err_ret("INSERT INTO exec: Value (" + value->inspect() + ") evaluated to non-insertable value while inserting rows"); }

            row.emplace_back(std::move(*result));
        }

        for (; i < table->column_datas.size(); i++) {
            auto result = table->get_cloned_column_default_value(i);
            if (!result.has_value()) {
                exec_push_err_ret("INSERT INTO exec: Weird index bug 5");}

            row.emplace_back(std::move(*result)); }
    }

    if (!errors.empty()) {
        return;}

    table->rows.emplace_back(MAKE_UP(e_group_object, std::move(row)));
}

// TODO
static UP<evaluated> exec_expression([[maybe_unused]] UP<evaluated> expression, [[maybe_unused]] SP<environment> env) {
    return UP<evaluated>(new null_object());
}

static void exec_create_table(UP<e_create_table> info) {

    astring table_name = info->table_name;

    for (const auto& entry : g_tables) {
        if (entry->get_tab_name() == table_name) {
            exec_push_err_ret("CREATE TABLE: Table (" + table_name + ") already exists"); }
    }

    

    avec<UP<e_table_detail_object>> details = std::move(info->details);
    for (const auto& detail : details) {
        astring name = detail->name;

        const auto& data_type = detail->data_type;
        
        if (data_type->type() != E_SQL_DATA_TYPE_OBJ) {
            exec_push_err_ret("CREATE TABLE: Data type entry failed to evaluate to E SQL data type (" + detail->data_type->inspect() + ")"); }

        if (detail->default_value->type() != NULL_OBJ) {
            UP<evaluated> default_value = UP<evaluated>(detail->default_value->clone());
            
            if (default_value->type() != STRING_OBJ && default_value->type() != INTEGER_OBJ) {
                exec_push_err_ret("CREATE TABLE: Default value failed to evaluate to a string or a number (" + detail->default_value->inspect() + ")"); }
        }
    }

    for (const auto& detail : details) {
        if (detail == nullptr) {
            std::cout << "bruh" << std::source_location::current().line() << std::endl; }
    }

    auto* table = new table_object(table_name, std::move(details));
    g_tables.emplace_back(SP<table_object>(table));
}

static void print_table() {

    const SP<table_info_object>& tab_info = display_tab.table_info;

    const SP<table_object> tab = tab_info->tab;

    for (const auto& detail : tab->column_datas) {
        if (detail == nullptr) {
            std::cout << "bruh, " << std::source_location::current().line() << std::endl; }
    }

    std::cout << tab->table_name << ":\n";

    astring field_names = "";
    for (const auto& col_id : tab_info->col_ids) {

        const auto& [full_name, col_in_bounds] = tab->get_column_name(col_id);
        if (!col_in_bounds) {
            exec_push_err_ret("print_table(): Out of bounds column index"); }

        astring name = full_name.substr(0, 10);
        size_t pad_length = 10 - name.length();
        astring pad(pad_length, ' ');
        name += pad + " | ";

        field_names += name;
    }

    std::cout << field_names <<std::endl;

    
    for (const auto& row_index : tab_info->row_ids) {

        astring row_values = "";
        auto result = tab->get_row_vec_ptr(row_index);
        if (!result.has_value()) {
            exec_push_err_ret("print_table(): Out of bounds row index"); }

        const auto& row = **result;

        for (const auto& col_id : tab_info->col_ids) {

            if (col_id >= row.size()) {
                exec_push_err_ret("print_table(): Out of bounds column index"); }  

            astring full_name = row[col_id]->data();
            if (row[col_id]->type() == NULL_OBJ) {
                full_name = ""; }

            astring name = full_name.substr(0, 10);
            size_t pad_length = 10 - name.length();
            astring pad(pad_length, ' ');
            name += pad + " | ";

            row_values += name;
        }
        std::cout << row_values << "\n";
    }

    std::cout << std::endl;
}


static void configure_print_functions(SP<table_info_object> tab_info) {

    if (tab_info->tab->type() != TABLE_OBJECT) {
        exec_push_err_ret("SELECT: Failed to evaluate table"); }

    display_tab.to_display = true;
    display_tab.table_info = tab_info;

    for (const auto& detail : tab_info->tab->column_datas) {
        if (detail == nullptr) {
            std::cout << "bruh, " << std::source_location::current().line() << std::endl; }
    }

    print_table(); // CMD line print, QT will do it's own thing in main
}

static std::expected<UP<null_object>, UP<error_object>> exec_where(UP<e_expression_object> clause, table_aggregate_object* table_aggregate, avec<size_t>& row_ids, SP<environment> env) {
    
    if (clause->type() == E_PREFIX_EXPRESSION_OBJ) {
        push_err_ret_unx_err_obj("Prefix WHERE not supported yet"); }

    if (clause->type() != E_INFIX_EXPRESSION_OBJ) {
        push_err_ret_unx_err_obj("Tried to evaluate WHERE with non-infix object, bug"); }

    UP<e_infix_expression_object> where_infix = cast_UP<e_infix_expression_object>(clause);
    if (where_infix->get_op_type() != WHERE_OP) {
        push_err_ret_unx_err_obj("eval_where(): Called with non-WHERE operator"); }
    

    UP<evaluated> raw_cond = std::move(where_infix->right);
    if (raw_cond->type() != E_INFIX_EXPRESSION_OBJ) {
        push_err_ret_unx_err_obj("WHERE condition is not infix condition"); }



    if (clause->type() == E_INFIX_EXPRESSION_OBJ) { // For SELECT [*] FROM [table] WHERE [CONDITION]
        if (where_infix->left->type() == STRING_OBJ) {
            const auto& [table, tab_found] = get_table_as_const(where_infix->left->data());
            if (tab_found) {
                table_aggregate->add_table(table);
            }
        }
    }

    UP<e_infix_expression_object> condition = cast_UP<e_infix_expression_object>(raw_cond);
    UP<evaluated> left  = UP<evaluated>(condition->left->clone());
    UP<evaluated> right = UP<evaluated>(condition->right->clone());

    // Find column index BEGIN 
    size_t where_col_index = SIZE_T_MAX;
    std::array<UP<evaluated>, 2> sides = {std::move(left), std::move(right)};
    for (auto& side : sides) {
        if (where_col_index != SIZE_T_MAX) {
            break; }

        switch(side->type()) {
        case STRING_OBJ: {
            auto result = table_aggregate->get_col_id(side->data());
            if (!result.has_value()) {
                push_err_ret_unx_err_obj(result.error()->data()); }

            where_col_index = *result;
            
        } break;
        case COLUMN_INDEX_OBJECT: {

            UP<e_column_index_object> col_index = cast_UP<e_column_index_object>(side);

            SP<table_object> table = col_index->table;

            UP<index_object> index_obj = std::move(col_index->index);
            size_t index = index_obj->value;
            
            auto result = table_aggregate->get_col_id(table->table_name, index);
            if (!result.has_value()) {
                push_err_ret_unx_err_obj(result.error()->data()); }

            where_col_index = *result;
        } break;
        default:
        }
    }

    if (where_col_index == SIZE_T_MAX) {
        push_err_ret_unx_err_obj("SELECT FROM exec: Could not find column alias in WHERE condition"); }

    SP<table_object> table = table_aggregate->combine_tables("Shouldn't be in the end result");


    avec<size_t> new_row_ids;
    for (size_t row_id = 0; row_id < table->rows.size(); row_id++) {
        
        // Add to env
        const auto& [raw_cell_value, cell_in_bounds] = table->get_cell_value(row_id, where_col_index);
        if (!cell_in_bounds) {
            push_err_ret_unx_err_obj("SELECT FROM exec: Weird index bug"); }
        astring cell_value = raw_cell_value->data();
        const auto& [col_data_type, dt_in_bounds] = table->get_column_data_type(where_col_index);
        if (!dt_in_bounds) {
            push_err_ret_unx_err_obj("SELECT FROM exec: Weird index bug 2"); }
        token_type col_type = col_data_type->data_type;

        UP<evaluated> value;
        switch (col_type) {
        case INT: case INTEGER:
            value = UP<evaluated>(new integer_object(cell_value)); break;
        case DECIMAL:
            value = UP<evaluated>(new decimal_object(cell_value)); break;
        case VARCHAR:
            value = UP<evaluated>(new string_object(cell_value)); break;
        default:
            push_err_ret_unx_err_obj("Currently WHERE conditions aren't supported for " + token_type_to_string(col_type));
        }

        auto&& [column_name, name_in_bounds] = table->get_column_name(where_col_index);
        if (!name_in_bounds) {
            push_err_ret_unx_err_obj("SELECT FROM exec: Weird index bug 3"); }

        UP<e_variable_object> var = MAKE_UP(e_variable_object, column_name, std::move(value));
        SP<environment> row_env = MAKE_SP(environment, env);
        UP<object> added = row_env->add_variable(std::move(var));
        if (added->type() == ERROR_OBJ) {
            push_err_ret_unx_err_obj("Failed to add variable (" + var->inspect() + ") to environment"); }
        // env done

        UP<evaluated> should_add_obj = exec_expression(cast_UP<evaluated>(condition), row_env);
        if (should_add_obj->type() == ERROR_OBJ) {
            push_err_ret_unx_err_obj("Failed to evaulate WHERE condition"); }

        if (should_add_obj->type() != BOOLEAN_OBJ) {
            push_err_ret_unx_err_obj("WHERE condition failed to evaluate to boolean"); }

        bool should_add = false;
        if (cast_UP<boolean_object>(should_add_obj)->data() == "TRUE") {
            should_add = true; }
        
        if (should_add) {
            new_row_ids.push_back(row_id);
        }
    }
    row_ids = new_row_ids;

    return MAKE_UP(null_object);
}


// Need to work on INFIX
static UP<evaluated> exec_left_join([[maybe_unused]] UP<e_expression_object> clause, [[maybe_unused]] table_aggregate_object* table_aggregate, [[maybe_unused]] avec<size_t>& row_ids, [[maybe_unused]] SP<environment> env) {
    return UP<evaluated>(new error_object("Left Join should use indexes bruh, need to rewrite, look at Github if u want"));
}

// Row ids are passed by reference cause easier, might have to make tab by reference later as well (for stuff like JOINs)
// If is buggy can just go back to using return values
static UP<evaluated> exec_clause(UP<e_expression_object> clause, table_aggregate_object* table_aggregate, avec<size_t>& row_ids, SP<environment> env) {

    switch (clause->get_op_type()) {

        case WHERE_OP: {
            // WHERE can be infix or prefix
            auto result = exec_where(std::move(clause), table_aggregate, row_ids, env);
            if (result.has_value()) {
                return cast_UP<evaluated>( std::move(*result));
            } else {
                return cast_UP<evaluated>( std::move(result).error());
            }
        } break;
        case LEFT_JOIN_OP: {
            return exec_left_join(std::move(clause), table_aggregate, row_ids, env);
        } break;
        default:
            push_err_ret_err_obj("Unsupported op type (" + operator_type_to_astring(clause->get_op_type()) + ")");
    }
}

static void exec_select_from(UP<e_select_from> wrapper, SP<environment> env) {

    if (wrapper->value->type() != E_SELECT_FROM_OBJECT) {
        exec_push_err_ret("eval_select_from(): Called with invalid object (" + object_type_to_astring(wrapper->value->type()) + ")"); }

    UP<e_select_from_object> info = cast_UP<e_select_from_object>(wrapper->value);    
    
    // First, use clause chain to obtain the initial table
    // Second, the clause chain will conncect tables together, and narrow the ammount of rows selected
    avec<size_t> row_ids;
    table_aggregate_object* table_aggregate = new table_aggregate_object();
    if (info->clause_chain.size() != 0) {
        for (auto& clause : info->clause_chain) {

            if (clause->type() == STRING_OBJ) { // To support plain SELECT * FROM table;
                const auto& [table, tab_found] = get_table_as_const(clause->data());
                if (!tab_found) {
                    exec_push_err_ret("SELECT FROM exec: Table (" + clause->data() + ") does not exist"); }

                row_ids = table->get_row_ids(); 
                table_aggregate->add_table(table);
                break;
            }

            if (clause->type() != INFIX_EXPRESSION_OBJ && clause->type() != PREFIX_EXPRESSION_OBJ) {
                exec_push_err_ret("Unsupported clause type, (" + clause->inspect() + ")"); 
            }

            UP<evaluated> error_val = exec_clause(cast_UP<e_expression_object>(clause), table_aggregate, row_ids, env); // Should add table to aggregate by itself
            if (error_val->type() != NULL_OBJ) {
                exec_push_err_ret(error_val->data()); }
        }

    }



    // Second, use column indexes the index into the table aggregate, validate
    avec<UP<evaluated>> column_indexes = std::move(info->column_indexes);
    avec<size_t> col_ids;
    col_ids.reserve(column_indexes.size());

    if (column_indexes.size() == 0) {
        exec_push_err_ret("SELECT FROM exec: No column indexes"); }

    // If SELECT * FROM [table], add all columns
    if (column_indexes[0]->type() == STAR_OBJECT) {
        col_ids = table_aggregate->get_all_col_ids();

        if (table_aggregate->tables.size() == 1) {
            auto [table, ok] = table_aggregate->get_table(0);
            if (!ok) {
                exec_push_err_ret("SELECT FROM exec: Strange bug, couldn't get first table from aggregate, even though size == 1"); }
            configure_print_functions(MAKE_SP(table_info_object, table, col_ids, row_ids));
            return;
        }

        SP<table_object> table = table_aggregate->combine_tables("aggregate");
        configure_print_functions(MAKE_SP(table_info_object, table, col_ids, row_ids));
        return;
    }

    for (auto& selecter : column_indexes) {

        // avec<UP<object>> REALARGS = *static_cast<UP<function_call_object>>(col_index_raw)->arguments->elements; for debug

        switch (selecter->type()) {
        case E_FUNCTION_CALL_OBJ: { // Not padding for now cause lazy

            if (column_indexes.size() == 1) {
                avec<size_t> new_row_ids;
                new_row_ids.push_back(0);
                row_ids = new_row_ids;
            }

            avec<UP<object>> args = std::move(cast_UP<function_call_object>(selecter)->arguments->elements);
            if (args.size() != 1) {
                exec_push_err_ret("COUNT() bad argument count"); }
            const UP<object>& arg = args[0];
            if (arg->type() != STAR_OBJECT) {
                exec_push_err_ret("COUNT() argument must be *, got (" + object_type_to_astring(arg->type()) + ")"); }

            size_t count = (table_aggregate->tables[0])->rows.size(); //!!MAJOR keep track of max size
            
            auto type = MAKE_UP(e_SQL_data_type_object, NONE, INTEGER, UP<evaluated>(new integer_object(11)));
            auto row          = MAKE_UP(e_group_object, UP<evaluated>(new integer_object(static_cast<int>(count)))); //!!MAJOR stinky
            auto detail  = MAKE_UP(e_table_detail_object, "COUNT(*)", std::move(type), UP<evaluated>(new null_object()));
            auto count_star     = MAKE_SP(table_object, "COUNT(*) TABLE", std::move(detail), std::move(row));
            table_aggregate->add_table(count_star);
            const auto& [id, ok] = table_aggregate->get_last_col_id();
            if (!ok) {
                exec_push_err_ret("SELECT FROM exec: Weird bug"); }
            col_ids.push_back(id);
        } break;
        case E_COLUMN_INDEX_OBJECT: {

            UP<e_column_index_object> col_index = cast_UP<e_column_index_object>(selecter);

            SP<table_object> table = col_index->table;

            UP<index_object> index_obj = std::move(col_index->index);
            size_t index = index_obj->value;
            
            auto result = table_aggregate->get_col_id(table->table_name, index);
            if (!result.has_value()) {
                exec_push_err_ret("SELECT FROM exec:" + result.error()->data()); }

            col_ids.push_back(*result);
             
        } break;
        case STRING_OBJ: {
    
            astring column_name = selecter->data();
    
            auto result = table_aggregate->get_col_id(column_name);
            if (!result.has_value()) {
                exec_push_err_ret(result.error()->data()); }
    
            col_ids.push_back(*result);
        } break;
        default: 
            exec_push_err_ret("SELECT FROM exec: Cannot use (" + selecter->inspect() + ") to index");
        }
    }

    if (table_aggregate->tables.size() == 1 || column_indexes.size() == 1) {
        const auto& [table_name, ok] = table_aggregate->get_table_name(0);
        if (!ok) {
            exec_push_err_ret("SELECT FROM exec: Strange bug, couldn't get first table from aggregate, even though size == 1"); }
        SP<table_object> table = table_aggregate->combine_tables(table_name);
        configure_print_functions(MAKE_SP(table_info_object, table, col_ids, row_ids));
        return;
    }

    SP<table_object> table = table_aggregate->combine_tables("aggregate");

    configure_print_functions(MAKE_SP(table_info_object, table, col_ids, row_ids));
}





static bool get_insertable_both_e_SQL(const UP<e_SQL_data_type_object>& insert_obj, const UP<e_SQL_data_type_object>& data_type) {

    if (insert_obj->data_type == data_type->data_type) {
        return true; }
    return false;
}

static std::expected<UP<evaluated>, UP<error_object>> get_insertable(evaluated* insert_obj, const UP<e_SQL_data_type_object>& data_type) {
    return get_insertable(UP<evaluated>(insert_obj), data_type);
}

static std::expected<UP<evaluated>, UP<error_object>> get_insertable(UP<evaluated> insert_obj, const UP<e_SQL_data_type_object>& data_type) {

    if (insert_obj->type() == SQL_DATA_TYPE_OBJ) {
        bool ok = get_insertable_both_e_SQL(cast_UP<e_SQL_data_type_object>(insert_obj), data_type);
        if (!ok) {
            return std::unexpected(MAKE_UP(error_object, "Could not insert (" + insert_obj->inspect() + ") into (" + data_type->inspect())); 
        } else {
            return insert_obj; }
    }

    switch (data_type->data_type) {
    case INT:
        switch (insert_obj->type()) {
        case INTEGER_OBJ:
            return insert_obj; 
            break;
        case DECIMAL_OBJ:
            warnings.emplace_back("Decimal implicitly converted to INT");
            return UP<evaluated>(new integer_object(insert_obj->data()));
            break;
        default:
            return std::unexpected(MAKE_UP(error_object, "get_insertable(): Value: (" + insert_obj->data() + ") has mismatching type with column (" + data_type->inspect() + ")"));
            break;
        }
        break;
    case FLOAT:
        switch (insert_obj->type()) {
        case INTEGER_OBJ:
            warnings.emplace_back("Integer implicitly converted to FLOAT");
            return UP<evaluated>(new decimal_object(insert_obj->data()));
            break;
        case DECIMAL_OBJ:
            return insert_obj; 
            break;
        default:
            return std::unexpected(MAKE_UP(error_object, "get_insertable(): Value: (" + insert_obj->data() + ") has mismatching type with column (" + data_type->inspect() + ")"));
            break;
        }
        break;
    case DOUBLE:
    switch (insert_obj->type()) {
        case INTEGER_OBJ:
            warnings.emplace_back("Integer implicitly converted to DOUBLE");
            return UP<evaluated>(new decimal_object(insert_obj->data()));
            break;
        case DECIMAL_OBJ:
            return insert_obj; 
            break;
        default:
            return std::unexpected(MAKE_UP(error_object, "get_insertable(): Value: (" + insert_obj->data() + ") has mismatching type with column (" + data_type->inspect() + ")"));
            break;
        }
        break;
    case VARCHAR:
        switch (insert_obj->type()) {
        case INTEGER_OBJ:
            warnings.emplace_back("Integer implicitly converted to VARCHAR");
            return UP<evaluated>(new string_object(insert_obj->data()));
            break;
        case DECIMAL_OBJ:
            warnings.emplace_back("Decimal implicitly converted to VARCHAR");
            return UP<evaluated>(new string_object(insert_obj->data()));
            break;
        case STRING_OBJ: {
            if (data_type->parameter->type() != INTEGER_OBJ) {
                return std::unexpected(MAKE_UP(error_object, "get_insertable(): varchar cannot be inserted into data type with non-integer parameter"));
            }
            int max_length = cast_UP<integer_object>(data_type->parameter)->value;
            size_t insert_length = insert_obj->data().length();
            if (insert_length > static_cast<size_t>(max_length)) {
                return std::unexpected(MAKE_UP(error_object, "get_insertable(): Value: (" + insert_obj->data() + ") excedes column's max length (" + data_type->parameter->inspect() + ")"));
            }
            return insert_obj;
        } break;
        default:
            return std::unexpected(MAKE_UP(error_object, "get_insertable(): Value: (" + insert_obj->data() + ") has mismatching type with column (" + data_type->inspect() + ")"));
            break;
        }
        break;
    default:
        return std::unexpected(MAKE_UP(error_object, "get_insertable(): " + data_type->inspect() + " is not supported YET"));
        break;
    }
}

static std::pair<SP<table_object>, bool> get_table(const std_and_astring_variant& name) {

    astring name_unwrapped;
    visit(name, [&](const auto& unwrapped) {
        name_unwrapped = unwrapped;
    });

    for (const auto& entry : g_tables) {
        if (entry->table_name == name_unwrapped) {
            return {entry, true};
        }
    }

    return {nullptr, false};
}

static std::pair<const SP<table_object>&, bool> get_table_as_const(const std_and_astring_variant& name) {

    astring name_unwrapped;
    visit(name, [&](const auto& unwrapped) {
        name_unwrapped = unwrapped;
    });

    for (const auto& entry : g_tables) {
        if (entry->table_name == name_unwrapped) {
            return {entry, true};
        }
    }

    SP<table_object> garbage;
    return {garbage, false};
}