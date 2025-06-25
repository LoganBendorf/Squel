
#include "pch.h"

#include "evaluator.h"

#include "allocator_aliases.h"
#include "node.h"
#include "structs_and_macros.h"
#include "helpers.h"
#include "object.h"
#include "environment.h"


extern std::vector<std::string> errors;
extern display_table display_tab;

static avec<UP<node>> nodes;

extern std::vector<std::string> warnings;

static avec<SP<table_object>> s_tables;
static avec<SP<evaluated_function_object>> s_functions;

static void eval_function(UP<function> func, SP<environment> env);
static UP<object> eval_run_function(UP<function_call_object> func_call, SP<environment> env);

static UP<object> eval_assert      (UP<assert_node> node,   SP<environment> env);
static void       eval_alter_table (UP<alter_table> info,   SP<environment> env);
static void       eval_create_table(UP<create_table> info,  SP<environment> env);
static void       eval_insert_into (UP<insert_into> wrapper,SP<environment> env);
static std::expected<UP<table_info_object>, UP<error_object>> eval_select     (UP<select_object> info,  SP<environment> env);
static std::expected<UP<table_info_object>, UP<error_object>> eval_select_from(UP<select_from> wrapper, SP<environment> env);
static void       print_table();

[[maybe_unused]] static UP<evaluated> eval_expression (object* expression, SP<environment> env);
static UP<evaluated> eval_expression  (UP<object> expression, SP<environment> env);
static UP<object> eval_expression_impl(UP<object> expression, SP<environment> env);
static UP<object> eval_column         (UP<column_object> col, SP<environment> env);

static UP<object> eval_prefix_expression(UP<operator_object> op, UP<object> right, SP<environment> env);
static UP<object> eval_infix_expression (UP<operator_object> op, UP<object> left, UP<object> right, SP<environment> env);

static std::expected<UP<object>, UP<error_object>> get_insertable(UP<object> insert_obj, const UP<SQL_data_type_object>& data_type);
static std::expected<UP<object>, UP<error_object>> get_insertable(object* insert_obj, const UP<SQL_data_type_object>& data_type);
static std::pair<SP<table_object>, bool> get_table(const std_and_astring_variant& name);

enum ret_code : std::uint8_t{
    SUCCESS, FAIL, ERROR
};
static std::pair<UP<evaluated>, ret_code> convert_table_to_value (const UP<table_object>& tab);
static std::pair<UP<evaluated>, ret_code> convert_table_info_to_value(UP<table_info_object> info);


#define eval_push_error_return(x)               \
    std::stringstream err;                      \
    err << (x);                                 \
    errors.emplace_back(std::move(err).str());  \
    return                        

#define push_err_ret_err_obj(...)               \
    do {                                        \
        std::stringstream err;                  \
        err << __VA_ARGS__;                     \
        errors.emplace_back(std::move(err).str());\
        return UP<object>(new error_object());  \
    } while(0)

#define push_err_ret_unx_err_obj(...)           \
    do {                                        \
        std::stringstream err;                  \
        err << __VA_ARGS__;                     \
        errors.emplace_back(std::move(err).str()); \
        return std::unexpected(MAKE_UP(error_object)); \
    } while(0)

#define push_err_ret_eval_err_obj(...)          \
    do {                                        \
        std::stringstream err;                  \
        err << __VA_ARGS__;                     \
        errors.emplace_back(std::move(err).str()); \
        return UP<evaluated>(new error_object());  \
    } while(0)

#define push_err_break(x)                       \
    std::stringstream err;                      \
    err << (x);                                 \
    errors.push_back(std::move(err).str());     \
    break                                       \

SP<environment> eval_init(avec<UP<node>> nds, avec<SP<evaluated_function_object>>& g_functions, avec<SP<table_object>>& g_tables) {
    nodes       = std::move(nds);
    s_functions = g_functions;
    s_tables    = g_tables;
    return MAKE_UP(environment);
}

static void configure_print_functions(UP<table_info_object> tab_info) {

    if (tab_info->tab->type() != TABLE_OBJECT) {
        eval_push_error_return("SELECT: Failed to evaluate table"); }

    display_tab.to_display = true;
    display_tab.table_info = std::move(tab_info);

    print_table(); // CMD line print, QT will do it's own thing in main
}

std::pair<avec<SP<evaluated_function_object>>, avec<SP<table_object>>> eval(SP<environment> env) {
    
    for (auto& node : nodes) {

        switch(node->type()) {
            case INSERT_INTO_NODE:
                eval_insert_into(cast_UP<insert_into>(std::move(node)), env);
                std::cout << "EVAL INSERT INTO CALLED\n";
                break;
            case SELECT_NODE: {
                UP<object> unwrapped = std::move(cast_UP<select_node>(std::move(node))->value);
                if (unwrapped->type() != SELECT_OBJECT) {
                    errors.emplace_back("Select node contained errors object"); break; }

                UP<select_object> sel_obj = cast_UP<select_object>(std::move(unwrapped));
                auto result = eval_select(std::move(sel_obj), env);
                if (!result.has_value()) {
                    errors.emplace_back("Failed to evaluate SELECT"); break; }

                configure_print_functions(std::move(*result));
                
                std::cout << "EVAL SELECT CALLED\n";
            } break;
            case SELECT_FROM_NODE: {
                std::expected<UP<table_info_object>, UP<error_object>> result = eval_select_from(cast_UP<select_from>(node), env);
                if (!result.has_value()) {
                    errors.emplace_back("Failed to evaluate SELECT FROM"); break; }

                configure_print_functions(std::move(*result));

                std::cout << "EVAL SELECT FROM CALLED\n";
            } break;
            case CREATE_TABLE_NODE:
                eval_create_table(cast_UP<create_table>(node), env);
                std::cout << "EVAL CREATE TABLE CALLED\n";
                break;
            case ALTER_TABLE_NODE:
                eval_alter_table(cast_UP<alter_table>(node), env);
                std::cout << "EVAL ALTER TABLE CALLED\n";
                break;
            case FUNCTION_NODE:
                eval_function(cast_UP<function>(node), env);
                std::cout << "EVAL FUNCTION CALLED\n";
                break;
            case ASSERT_NODE:
                eval_assert(cast_UP<assert_node>(node), env);
                std::cout << "EVAL FUNCTION CALLED\n";
                break;
            default:
                errors.emplace_back("eval: unknown node type (" + node->inspect() + ")");
        }
    }

    return {s_functions, s_tables};
}

static UP<object> eval_assert(UP<assert_node> node, SP<environment> env) {

    UP<assert_object> info = std::move(node->value);

    UP<evaluated> expr = eval_expression(std::move(info->expression), env);
    if (expr->type() == ERROR_OBJ) {
        push_err_ret_err_obj("Failed to evaluate ASSERT expression"); }

    if (expr->type() != BOOLEAN_OBJ) {
        push_err_ret_err_obj("ASSERT expression failed to evaluate to a boolean"); }

    UP<boolean_object> boolean = cast_UP<boolean_object>(std::move(expr));

    if (!boolean->value) {
        push_err_ret_err_obj("ASSERT failed (Line: " << info->line << ", Expression: " << info->inspect() << ")"); }

    return UP<object>(new null_object());
}

static UP<evaluated> assume_data_type(UP<evaluated> obj) {
    switch (obj->type()) {
    case STRING_OBJ:
        return UP<evaluated>(new e_SQL_data_type_object(NONE, VARCHAR, new integer_object(255)));
    case INTEGER_OBJ:
        return UP<evaluated>(new e_SQL_data_type_object(NONE, INT, new integer_object(11)));
    case TABLE_OBJECT: {
        auto&& [cell, rc] = convert_table_to_value(cast_UP<table_object>(std::move(obj)));
        if (rc == SUCCESS) {
            return assume_data_type(std::move(cell));
        }
        push_err_ret_eval_err_obj("Can't assume default data type for (" + obj->inspect() + ")");
    } break;
    case TABLE_INFO_OBJECT: {
        auto&& [cell, rc] = convert_table_info_to_value(cast_UP<table_info_object>(std::move(obj)));
        if (rc == SUCCESS) {
            return assume_data_type(std::move(cell));
        }
        push_err_ret_eval_err_obj("Can't assume default data type for (" + obj->inspect() + ")");
    } break;
    default:
        push_err_ret_eval_err_obj("Can't assume default data type for (" + obj->inspect() + ")");
    }
}

static std::expected<UP<table_info_object>, UP<error_object>> eval_select(UP<select_object> info, SP<environment> env) {

    if (info->type() != SELECT_OBJECT) {
        push_err_ret_unx_err_obj("eval_select(): Called with invalid object (" + object_type_to_astring(info->type()) + ")"); }

    astring table_name = info->value->inspect();
   
    UP<evaluated> table_value = eval_expression(std::move(info->value), env);
    if (table_value->type() == ERROR_OBJ) {
        push_err_ret_unx_err_obj("Failed to evaluate SELECT value (" + table_value->inspect() + ")"); }

    UP<evaluated> assumed_type = assume_data_type(std::move(table_value));
    if (table_value->type() == SQL_DATA_TYPE_OBJ) {
        push_err_ret_unx_err_obj("Couldn't assume SELECT value data type (" + table_value->inspect() + ")"); }
    
    // some nonsense here
    UP<e_SQL_data_type_object> type  = cast_UP<e_SQL_data_type_object>(assumed_type);
    UP<table_detail_object> detail = MAKE_UP(table_detail_object, table_name, std::move(type), UP<object>(new null_object()));
    UP<table_object> tab           = MAKE_UP(table_object, table_name, std::move(detail), MAKE_UP(group_object, std::move(table_value)));
    UP<table_info_object> tab_info = MAKE_UP(table_info_object, std::move(tab), avec<size_t>{0}, avec<size_t>{0});

    return tab_info;
}

static void eval_function (UP<function> func, SP<environment> env) {

    UP<object> eval_parameters = eval_expression(cast_UP<object>(func->func->parameters), env);
    if (eval_parameters->type() != GROUP_OBJ) {
        eval_push_error_return("Failed to evaluate to function parameter"); }

    avec<UP<object>> params = std::move(cast_UP<group_object>(eval_parameters)->elements);
    avec<UP<parameter_object>> evaluated_parameters;
    evaluated_parameters.reserve(params.size());
    for (auto& param : params) {
        if (param->type() == ERROR_OBJ) {
            eval_push_error_return("Failed to evaluate to function parameter"); }

        if (param->type() != PARAMETER_OBJ) {
            eval_push_error_return("Function parameter failed to evaluate to parameter object"); }

        evaluated_parameters.emplace_back(cast_UP<parameter_object>(param));
    }

    SP<evaluated_function_object> new_func = MAKE_SP(evaluated_function_object, std::move(func->func), std::move(evaluated_parameters));
    
    env->add_or_replace_function(new_func);

    // For now just add all functions to global for fun
    bool found = false;
    for (auto & s_function : s_functions) {
        if (s_function->name == new_func->name) {
            s_function = new_func;
            found = true;
            break;
        }
    }
    if (!found) {
        s_functions.push_back(new_func);
    }

    std::cout << "!! PRINTING LE FUNCTION !!\n\n";

    std::cout << new_func->inspect() << std::endl;
}

static UP<object> eval_prefix_expression(UP<operator_object> op, UP<object> right, SP<environment> env) {
    
    UP<object> e_right = eval_expression(std::move(right), env);
    
    switch (op->op_type) {
    case NEGATE_OP:
        switch (e_right->type()) {
        case INTEGER_OBJ:
            return UP<object>(new integer_object( - astring_to_numeric<int>(e_right->data())));
        case DECIMAL_OBJ:
            return UP<object>(new decimal_object( - astring_to_numeric<double>(e_right->data())));
        default:
            push_err_ret_err_obj("No negation operation for type (" + e_right->inspect() + ")");
        }
    default:
        push_err_ret_err_obj("No prefix " + op->inspect() + " operator known");
    }
}


static bool is_values_wrapper(const UP<object>& obj) {
    switch (obj->type()) {
    case COLUMN_VALUES_OBJ:
        return true;

    default: 
        return false;
    }
}

static bool is_comparison_operator(const UP<operator_object>& op) {
    switch (op->op_type) {
    case EQUALS_OP: case NOT_EQUALS_OP: case LESS_THAN_OP: case GREATER_THAN_OP:
        return true;
    default:
        return false;
    }
}

static UP<object> eval_infix_values_condition(UP<operator_object> op, UP<values_wrapper_object> left_wrapper, UP<values_wrapper_object> right_wrapper, SP<environment> env) {
    avec<UP<object>> left  = std::move(left_wrapper->values);
    avec<UP<object>> right = std::move(right_wrapper->values);

    if (left.size() != right.size()) {
        push_err_ret_err_obj("Cannot compare values of different sizes"); }

    if (is_comparison_operator(op)) {
        for (size_t i = 0; i < left.size(); i++) {
            UP<object> obj = eval_infix_expression(std::move(op), std::move(left[i]), std::move(right[i]), env);
            if (obj->type() == ERROR_OBJ) {
                push_err_ret_err_obj(obj->data()); }
            if (obj->type() != BOOLEAN_OBJ) {
                push_err_ret_err_obj("compare_values(): Camparison failed to evaluate to boolean"); }

            bool truth = cast_UP<boolean_object>(obj)->value;

            if (!truth) {
                return UP<object>(new boolean_object(false)); }
        }
        return UP<object>(new boolean_object(true));
    } else {
        avec<UP<object>> new_vec;
        new_vec.reserve(left.size());
        for (size_t i = 0; i < left.size(); i++) {
            UP<object> obj = eval_infix_expression(std::move(op), std::move(left[i]), std::move(right[i]), env);
            if (obj->type() == ERROR_OBJ) {
                push_err_ret_err_obj(obj->data()); }

            new_vec.emplace_back(std::move(obj));
        }
        return UP<object>(new group_object(std::move(new_vec)));
    }
}

static UP<object> eval_infix_column_vs_value(UP<operator_object> op, UP<column_index_object> col_index_obj, UP<object> other, bool left_first, SP<environment> env) {

    // Must be comparison operator
    if (!is_comparison_operator(op)) {
        push_err_ret_err_obj("Condition with table index on one side and a single value on the other must be a comparison"); }

    UP<object> raw_tab = std::move(col_index_obj->table_name);
    if (raw_tab->type() != TABLE_OBJECT) {
        push_err_ret_err_obj("eval_infix_column_vs_value(): Column index object contained non-table as table alias, got (" + object_type_to_astring(raw_tab->type()) + ")"); }
    UP<table_object> table = cast_UP<table_object>(raw_tab);
    
    UP<object> index_obj = std::move(col_index_obj->column_name);
    if (index_obj->type() != INDEX_OBJ) {
        push_err_ret_err_obj("eval_infix_column_vs_value(): Column index object contained non-index as column alias, got (" + object_type_to_astring(index_obj->type()) + ")"); }
    size_t index = cast_UP<index_object>(index_obj)->value;

    if (index >= table->rows.size()) {
        push_err_ret_err_obj("eval_infix_column_vs_value(): Index out-of-bounds"); }



    if (left_first) {
        for (size_t i = 0; i < table->rows.size(); i++) {

            auto&& [cell, ok] = table->get_cell_value(i, index);
            if (!ok) {
                push_err_ret_err_obj("eval_infix_column_vs_value(): " + cell->data()); }

            UP<object> obj = eval_infix_expression(std::move(op), std::move(cell), std::move(other), env);
            if (obj->type() == ERROR_OBJ) {
                push_err_ret_err_obj(obj->data()); }
            if (obj->type() != BOOLEAN_OBJ) {
                push_err_ret_err_obj("eval_infix_column_vs_value(): Camparison failed to evaluate to boolean"); }

            bool truth = false;
            if (cast_UP<boolean_object>(obj)->data() == "TRUE") {
                truth = true; }

            if (truth != true) {
                return UP<object>(new boolean_object(false)); }

        }
    } else {
        for (size_t i = 0; i < table->rows.size(); i++) {

            auto&& [cell, ok] = table->get_cell_value(i, index);
            if (!ok) {
                push_err_ret_err_obj("eval_infix_column_vs_value(): " + cell->data()); }

            UP<object> obj = eval_infix_expression(std::move(op), std::move(other), std::move(cell), env);
            if (obj->type() == ERROR_OBJ) {
                push_err_ret_err_obj(obj->data()); }
            if (obj->type() != BOOLEAN_OBJ) {
                push_err_ret_err_obj("eval_infix_values_condition(): Camparison fail"); }

            bool truth = false;
            if (cast_UP<boolean_object>(obj)->data() == "TRUE") {
                truth = true; }

            if (truth != true) {
                return UP<object>(new boolean_object(false)); }
        }
    }

    return UP<object>(new boolean_object(true));
}

static UP<object> eval_infix_values_vs_value(UP<operator_object> op, UP<values_wrapper_object> values, UP<object> other, bool left_first, SP<environment> env) {

    // Must be comparison operator
    if (!is_comparison_operator(op)) {
        push_err_ret_err_obj("Condition with multiple values on one side and a single value on the other must be a comparison"); }


    if (left_first) {
        for (auto& val : values->values) {
            UP<object> obj = eval_infix_expression(std::move(op), std::move(val), std::move(other), env);
            if (obj->type() == ERROR_OBJ) {
                push_err_ret_err_obj(obj->data()); }
            if (obj->type() != BOOLEAN_OBJ) {
                push_err_ret_err_obj("eval_infix_values_condition(): Camparison failed to evaluate to boolean"); }

            bool truth = false;
            if (cast_UP<boolean_object>(obj)->data() == "TRUE") {
                truth = true; }

            if (truth != true) {
                return UP<object>(new boolean_object(false)); }

        }
    } else {
        for (auto& val : values->values) {
            UP<object> obj =eval_infix_expression(std::move(op), std::move(other), std::move(val), env);
            if (obj->type() == ERROR_OBJ) {
                push_err_ret_err_obj(obj->data()); }
            if (obj->type() != BOOLEAN_OBJ) {
                push_err_ret_err_obj("eval_infix_values_condition(): Camparison fail"); }

            bool truth = false;
            if (cast_UP<boolean_object>(obj)->data() == "TRUE") {
                truth = true; }

            if (truth != true) {
                return UP<object>(new boolean_object(false)); }
        }
    }

    return UP<object>(new boolean_object(true));
}

static std::pair<UP<evaluated>, ret_code> convert_table_info_to_value(UP<table_info_object> info) {
    if (info->col_ids.size() == 1 && info->row_ids.size() == 1) {
        auto&& [cell, ok] = info->tab->get_cell_value(info->col_ids[0], info->row_ids[0]); 
        if (!ok) {
            errors.emplace_back("convert_table_info_to_value(): weird index bug"); 
            return {std::move(cell), ERROR};
        }
        return {std::move(cell), SUCCESS};
    }
    return {UP<evaluated>(new null_object()), FAIL};
}

static std::pair<UP<evaluated>, ret_code> convert_table_to_value(const UP<table_object>& tab) {
    if (tab->column_datas.size() == 1 && tab->rows.size() == 1) {
        auto&& [cell, ok] = tab->get_cell_value(0, 0); 
        if (!ok) {
            errors.emplace_back("eval_infix_expression(): Weird index bug"); 
            return {std::move(cell), ERROR};
        }
        return {std::move(cell), SUCCESS};
    }
    return {UP<evaluated>(new null_object()), FAIL};
}

static UP<object> eval_infix_expression(UP<operator_object> op, UP<object> left, UP<object> right, SP<environment> env) {

    UP<object> e_left  = eval_expression(std::move(left), env);
    UP<object> e_right = eval_expression(std::move(right), env);

    if (e_left->type() == ERROR_OBJ) {
        push_err_ret_err_obj(e_left->data()); }
    if (e_right->type() == ERROR_OBJ) {
        push_err_ret_err_obj(e_right->data()); }

    if (is_values_wrapper(e_left) && is_values_wrapper(e_right)) {
        return eval_infix_values_condition(std::move(op), cast_UP<values_wrapper_object>(std::move(e_left)), cast_UP<values_wrapper_object>(std::move(e_right)), env);
    } else if (is_values_wrapper(e_left)) {
        return eval_infix_values_vs_value(std::move(op), cast_UP<values_wrapper_object>(std::move(e_left)), std::move(e_right), true, env); 
    } else if (is_values_wrapper(e_right)) {
        return eval_infix_values_vs_value(std::move(op), cast_UP<values_wrapper_object>(std::move(e_right)), std::move(e_left), false, env); 
    } 

    if (e_left->type() == COLUMN_INDEX_OBJECT) {
        return eval_infix_column_vs_value(std::move(op), cast_UP<column_index_object>(std::move(e_left)), std::move(e_right), true, env);
    } else if (e_right->type() == COLUMN_INDEX_OBJECT) {
        return eval_infix_column_vs_value(std::move(op), cast_UP<column_index_object>(std::move(e_right)), std::move(e_left), false, env);
    }

    // Functions not dependent on left or right, so can iterate
    std::array<UP<object>*, 2> sides = { &e_left, &e_right };
    for (auto& ptr : sides) {
        switch ((*ptr)->type()) {

        // case TABLE_OBJECT: { // unused also dont think it works
            // auto [cell, rc] = convert_table_to_value(tab);
            // if (rc == SUCCESS) {
            // // put the resulting cell back into the same original variable:
            // uptr = std::move(cell);
            // } else if (rc == ERROR) {
            //    return cell;
            // }
        // } break;

        case TABLE_INFO_OBJECT: {
            auto info = cast_UP<table_info_object>( std::move(*ptr) );
            auto&& [cell, rc] = convert_table_info_to_value(std::move(info));
            if (rc == SUCCESS) {
                *ptr = std::move(cell);
            } else if (rc == ERROR) {
                return std::move(cell);
            }
        } break;
        default:
            continue;
        }
    }


    
    switch (op->op_type) {
        case ADD_OP:
            if (is_numeric_object(e_left) && is_numeric_object(e_right)) {
                return UP<object>(new integer_object(cast_UP<integer_object>(std::move(e_left))->value + cast_UP<integer_object>(std::move(e_right))->value));
            } 
            else if (is_string_object(e_left) && is_string_object(e_right)) {
                return UP<object>(new string_object(e_left->data() + e_right->data()));
            } 
            else {
                push_err_ret_err_obj("No infix " << op->inspect() << " operation for " + e_left->inspect() << " and " << e_right->inspect());
            }
            
        case SUB_OP:
            if (!is_numeric_object(e_left) || !is_numeric_object(e_right)) {
                push_err_ret_err_obj("No infix " << op->inspect() << " operation for " + e_left->inspect() << " and " << e_right->inspect());}
            return UP<object>(new integer_object(cast_UP<integer_object>(std::move(e_left))->value - cast_UP<integer_object>(std::move(e_right))->value));
            break;
        case MUL_OP:
            if (!is_numeric_object(e_left) || !is_numeric_object(e_right)) {
                push_err_ret_err_obj("No infix " << op->inspect() << " operation for " + e_left->inspect() << " and " << e_right->inspect());}
            return UP<object>(new integer_object(cast_UP<integer_object>(std::move(e_left))->value * cast_UP<integer_object>(std::move(e_right))->value));
            break;
        case DIV_OP:
            if (!is_numeric_object(e_left) || !is_numeric_object(e_right)) {
                push_err_ret_err_obj("No infix " << op->inspect() << " operation for " + e_left->inspect() << " and " << e_right->inspect());}
            return UP<object>(new integer_object(cast_UP<integer_object>(std::move(e_left))->value / cast_UP<integer_object>(std::move(e_right))->value));
            break;
        case DOT_OP:
            if (e_left->type() != INTEGER_OBJ || e_left->type() != INTEGER_OBJ) {
                push_err_ret_err_obj("No infix " << op->inspect() << " operation for " + e_left->inspect() << " and " << e_right->inspect());}
            return UP<object>(new decimal_object(e_left->data() + "." + e_right->data()));
            break;
        case EQUALS_OP:
            if (e_left->type() == STRING_OBJ && e_right->type() == STRING_OBJ) {
                return UP<object>(new boolean_object(e_left->data() == e_right->data())); 
            } else if (e_left->type() == INTEGER_OBJ && e_right->type() == INTEGER_OBJ) {
                return UP<object>(new boolean_object(cast_UP<integer_object>(std::move(e_left))->value == cast_UP<integer_object>(std::move(e_right))->value)); 
            } else {
                push_err_ret_err_obj("No infix " << op->inspect() << " operation for " + e_left->inspect() << " and " << e_right->inspect());
            }
            break;
        case NOT_EQUALS_OP:
            if (e_left->type() == STRING_OBJ && e_right->type() == STRING_OBJ) {
                return UP<object>(new boolean_object(e_left->data() != e_right->data())); 
            } else if (e_left->type() == INTEGER_OBJ && e_right->type() == INTEGER_OBJ) {
                return UP<object>(new boolean_object(cast_UP<integer_object>(std::move(e_left))->value != cast_UP<integer_object>(std::move(e_right))->value)); 
            } else {
                push_err_ret_err_obj("No infix " << op->inspect() << " operation for " + e_left->inspect() << " and " << e_right->inspect());
            }
            break;
        case LESS_THAN_OP:
            if (e_left->type() != INTEGER_OBJ || e_left->type() != INTEGER_OBJ) {
                push_err_ret_err_obj("No infix " << op->inspect() << " operation for " + e_left->inspect() << " and " << e_right->inspect());}
            return UP<object>(new boolean_object(cast_UP<integer_object>(std::move(e_left))->value < cast_UP<integer_object>(std::move(e_right))->value));
            break;
        case GREATER_THAN_OP:
            if (e_left->type() != INTEGER_OBJ || e_left->type() != INTEGER_OBJ) {
                push_err_ret_err_obj("No infix " << op->inspect() << " operation for " + e_left->inspect() << " and " << e_right->inspect());}
            return UP<object>(new boolean_object(cast_UP<integer_object>(std::move(e_left))->value > cast_UP<integer_object>(std::move(e_right))->value));
            break;
        default:
            push_err_ret_err_obj("No infix " + op->inspect() + " operator known");
    }
}


static UP<evaluated> eval_expression(object* expression, SP<environment> env) {

    UP<object> expr = UP<object>(expression);

    UP<object> result = eval_expression_impl(std::move(expr), env);
    if (!is_evaluated(result)) {
        errors.push_back("eval_expression(): Failed to return evaluated object");
        return UP<evaluated>(new error_object("eval_expression(): Failed to return evaluated object"));
    }
    return cast_UP<evaluated>(result);
}

static UP<evaluated> eval_expression(UP<object> expression, SP<environment> env) {

    UP<object> result = eval_expression_impl(std::move(expression), env);
    if (!is_evaluated(result)) {
        errors.push_back("eval_expression(): Failed to return evaluated object");
        return UP<evaluated>(new error_object("eval_expression(): Failed to return evaluated object"));
    }

    return cast_UP<evaluated>(result);
}

static UP<object> eval_expression_impl(UP<object> expression, SP<environment> env) {

    switch(expression->type()) {

    // Basic stuff begin
    case STAR_OBJECT:
        return expression; break;
    case INTEGER_OBJ:
        return expression; break;
    case STRING_OBJ:
        return expression; break;
    case PARAMETER_OBJ:
        return expression; break;
    case RETURN_STATEMENT:
        return expression; break;
    case VARIABLE_OBJ:
        return std::move(cast_UP<variable_object>(expression)->value); break;
    case SQL_DATA_TYPE_OBJ: {
        UP<SQL_data_type_object> cur = cast_UP<SQL_data_type_object>(expression);
        UP<object> param = eval_expression(std::move(cur->parameter), env);
        if (param->type() == ERROR_OBJ) {
            return param; }

        if (param->type() != INTEGER_OBJ && param->type() != DECIMAL_OBJ && param->type() != NULL_OBJ) {
            push_err_ret_err_obj("For now parameters of SQL data type must evaluate to integer/decimal/none, can be strings later when working on SET or ENUM"); }

        auto evaled = MAKE_UP(e_SQL_data_type_object, cur->prefix, cur->data_type, std::move(param));

        return cast_UP<object>(evaled);
    } break;
    case NULL_OBJ:
        return expression; break;
    case PREFIX_EXPRESSION_OBJ:
        return eval_prefix_expression(std::move(cast_UP<prefix_expression_object>(expression)->op), std::move(cast_UP<prefix_expression_object>(expression)->right), env); break;
    // Basic stuff end

    case SELECT_OBJECT: {
        auto result = eval_select(cast_UP<select_object>(expression), env);
        if (result.has_value()) {
            return cast_UP<object>(std::move(*result));
        } else {
            return cast_UP<object>(std::move(result).error());
        }
    }

    case COLUMN_INDEX_OBJECT: {
        
        UP<column_index_object> obj = cast_UP<column_index_object>(expression);

        UP<object> tab_expr = eval_expression(std::move(obj->table_name), env);
        if (tab_expr->type() != STRING_OBJ) {
            push_err_ret_err_obj("Column index object: Table name failed to evaluate to string"); }
        astring table_name = tab_expr->data();

        UP<object> col_expr = eval_expression(std::move(obj->column_name), env);
        if (col_expr->type() != STRING_OBJ) {
            push_err_ret_err_obj("Column index object: Column name failed to evaluate to string"); }
        astring column_name = col_expr->data();

        const auto& [table, tab_exists] = get_table(table_name);
        if (!tab_exists) {
            push_err_ret_err_obj("Column index object: Table does not exist"); }

        const auto& [col_index, col_exists]  = table->get_column_index(column_name);
        if (!col_exists) {
            push_err_ret_err_obj("Column index object: Column does not exist"); }

        return UP<object>(new e_column_index_object(table, MAKE_UP(index_object, col_index)));

    } break;

    case SELECT_FROM_OBJECT: {
        UP<select_from> wrapper = MAKE_UP(select_from, std::move(expression));
        auto result = eval_select_from(std::move(wrapper), env);
        if (result.has_value()) {
            return cast_UP<object>(std::move(*result));
        } else {
            return cast_UP<object>(std::move(result).error());
        }
    } break;

    case COLUMN_OBJ:
        return eval_column(cast_UP<column_object>(expression), env);

    case FUNCTION_CALL_OBJ:
        return eval_run_function(cast_UP<function_call_object>(expression), env); break;

    case GROUP_OBJ: {
        UP<group_object> group = cast_UP<group_object>(expression);
        avec<UP<object>> objects;
        for (auto& obj: group->elements) {
            UP<object> evaluated = eval_expression(std::move(obj), env);
            if (evaluated->type() == ERROR_OBJ) {
                return evaluated; }
            objects.emplace_back(std::move(evaluated));
        }
        return UP<object>(new group_object(std::move(objects)));
    } break;
    case BLOCK_STATEMENT: {
        UP<block_statement> block = cast_UP<block_statement>(expression);
        UP<return_statement> ret_val;
        avec<UP<object>> statements;
        statements.reserve(block->body.size());
        bool has_ret = false;
        for (auto& statement: block->body) {
            UP<object> res = eval_expression(std::move(statement), env);
            if (res->type() == RETURN_STATEMENT) {
                if (has_ret) {
                    push_err_ret_err_obj("Block contained multiple (outer) return statements"); }
                ret_val = cast_UP<return_statement>(res); //!!MAJOR might need outer move?
                has_ret = true;
            } else {
                statements.emplace_back(std::move(res)); }
        }
        return UP<object>(new expression_statement(std::move(statements), std::move(ret_val)));
    } break;

    case INFIX_EXPRESSION_OBJ: {
        UP<infix_expression_object> condition = cast_UP<infix_expression_object>(expression);

        UP<object> left = std::move(condition->left);

        if (left->type() == STRING_OBJ) {
            auto result = env->get_variable(left->data());
            if (result.has_value()) {
                left = eval_expression(cast_UP<object>(std::move(*result)), env);
            }
        }

        UP<object> right = std::move(condition->right);

        if (right->type() == STRING_OBJ) {
            auto result = env->get_variable(right->data());
            if (result.has_value()) {
                right = eval_expression(cast_UP<object>(std::move(*result)), env);
            }
        }

        UP<object> result = eval_infix_expression(std::move(condition->op), std::move(left), std::move(right), env);
        if (result->type() == ERROR_OBJ) {
            return result; }

        return result;
    } break;

    case IF_STATEMENT: {
        UP<if_statement> statement = cast_UP<if_statement>(expression);

        UP<object> obj = eval_expression(std::move(statement->condition), env); // LOWEST or PREFIX??
        if (obj->type() == ERROR_OBJ) {
            return obj; }

        if (obj->type() != BOOLEAN_OBJ) {
            push_err_ret_err_obj("If statement condition returned non-boolean"); }

        UP<boolean_object> condition_result = cast_UP<boolean_object>(obj);

        if (condition_result->data() == "TRUE") { // scuffed
            UP<object> result = eval_expression(cast_UP<object>(statement->body), env);
            return result;
        } else if (statement->other->type() != NULL_OBJ) {
            UP<object> result = eval_expression(std::move(statement->other), env);
            return result;
        }

        return UP<object>(new null_object());

    } break;
    default:
        push_err_ret_err_obj("eval_expression(): Cannot evaluate expression. Type (" << object_type_to_astring(expression->type()) << "), value(" << expression->inspect() << ")"); 
    }
}


static avec<UP<argument_object>> name_arguments(SP<evaluated_function_object> function, UP<function_call_object> func_call, SP<environment> env) {

    avec<UP<argument_object>> named_arguments;

    UP<object> eval_args = eval_expression(cast_UP<object>(func_call->arguments), env);
    if (eval_args->type() != GROUP_OBJ) {
        errors.emplace_back("Failed to evaluate arguments"); return named_arguments; }

    avec<UP<object>> evaluated_arguments = std::move(cast_UP<group_object>(eval_args)->elements);

    

    avec<UP<parameter_object>> parameters;
    parameters.reserve(function->parameters.size());
    for (const auto& param: parameters) {
        parameters.push_back(UP<parameter_object>(param->clone()));
    }

    if (evaluated_arguments.size() != parameters.size()) {
        errors.emplace_back("Function called with incorrect number of arguments, got " + std::to_string(evaluated_arguments.size()) + " wanted " + std::to_string(parameters.size()));
        return named_arguments;
    }

    for (size_t i = 0; i < parameters.size(); i++) {
        astring param_name = parameters[i]->name;
        named_arguments.emplace_back(MAKE_UP(argument_object, param_name, std::move(evaluated_arguments[i])));
    }

    return named_arguments;
}



static UP<object> eval_run_function(UP<function_call_object> func_call, SP<environment> env) {

    if (func_call->name == "COUNT") {
        avec<UP<object>> args = std::move(func_call->arguments->elements);
        avec<UP<object>> e_args;
        e_args.reserve(args.size());
        for (auto& arg : args) {
            UP<object> e_arg = eval_expression(std::move(arg), env);
            if (e_arg->type() == ERROR_OBJ) {
                push_err_ret_err_obj("Failed to evaluate (" + arg->inspect() + ")"); }
            e_args.emplace_back(std::move(e_arg));
        }
        return UP<object>(new function_call_object("COUNT", MAKE_UP(group_object, std::move(e_args))));
    }

    bool found = false;
    for (const auto& func : s_functions) {
        if (func->name == func_call->name) {
            found = true; 
        }
    }

    if (!found && !env->is_function(func_call->name)) {
        push_err_ret_err_obj("Called non-existent function (" + func_call->name + ")"); }
    


    auto&& [function, exists] = env->get_function(func_call->name);
    if (!exists) {
        push_err_ret_err_obj("Function does not exist (" + func_call->name + ")"); }

    size_t error_count = errors.size();
    avec<UP<argument_object>> named_args = name_arguments(function, std::move(func_call), env);
    if (error_count < errors.size()) {
        return UP<object>(new error_object()); }



    const avec<UP<parameter_object>>& parameters = function->parameters;
    if (named_args.size() != parameters.size()) {
        push_err_ret_err_obj("Function called with incorrect number of arguments, got " << named_args.size() << " wanted " << parameters.size()); }

    SP<environment> function_env = MAKE_SP(environment, env);
    bool ok = function_env->add_variables(std::move(named_args));
    if (!ok) {
        push_err_ret_err_obj("Failed to add function arguments as variables to function environment"); }

    for (const auto& line : function->body->body) {
        UP<object> res = eval_expression(UP<object>(line->clone()), function_env);
        if (res->type() == ERROR_OBJ) {
            return res; }

        if (res->type() == RETURN_STATEMENT) {
            return eval_expression(std::move(cast_UP<return_statement>(res)->expression), env);
        }
        
    }

    push_err_ret_err_obj("Failed to find return value");
}

static UP<object> eval_column(UP<column_object> col, SP<environment> env) {
    UP<object> parameter = eval_expression(std::move(col->name_data_type), env);
    if (parameter->type() == ERROR_OBJ) {
        return parameter; }

    if (parameter->type() != PARAMETER_OBJ) {
        push_err_ret_err_obj("Column data type (" + col->name_data_type->inspect() + ")failed to evaluate to parameter object"); }

    UP<object> default_value = eval_expression(std::move(col->default_value), env);
    if (default_value->type() == ERROR_OBJ) {
        return default_value; }

    if (default_value->type() != STRING_OBJ && default_value->type() != NULL_OBJ) {
        push_err_ret_err_obj("Column default value (" + col->default_value->inspect() + ") failed to evaluate to string object"); }

    if (default_value->type() == NULL_OBJ) {
        default_value = UP<object>(new string_object("")); }

    return UP<object>(new evaluated_column_object(cast_UP<parameter_object>(parameter)->name, std::move(cast_UP<parameter_object>(parameter)->data_type), default_value->data()));
}

static void eval_alter_table(UP<alter_table> info, SP<environment> env) {

    UP<object> table_name = eval_expression(std::move(info->table_name), env);
    if (table_name->type() == ERROR_OBJ) {
        eval_push_error_return("Failed to evaluate table name (" + info->table_name->inspect() + ")"); }
   
    if (table_name->type() != STRING_OBJ) {
        eval_push_error_return("Table name (" + info->table_name->inspect() + ") failed to evaluate to string"); }
    

    auto&& [table, tab_found] = get_table(table_name->data());
    if (!tab_found) {
        eval_push_error_return("INSERT INTO, table not found");}

    SP<table_object> tab = table;



    UP<object> table_edit = eval_expression(std::move(info->table_edit), env);
    if (table_edit->type() == ERROR_OBJ) {
        eval_push_error_return("eval_alter_table(): Failed to evaluate table edit"); }

    switch (table_edit->type()) {
    case EVALUATED_COLUMN_OBJ: {

        UP<evaluated_column_object> column_obj = cast_UP<evaluated_column_object>(table_edit);

        for (const auto& table_column : tab->column_datas){
            if (column_obj->name == table_column->name) {
                eval_push_error_return("eval_alter_table(): Table already contains column with name (" + column_obj->name + ")"); }
        }

        UP<table_detail_object> col = MAKE_UP(table_detail_object, column_obj->name, std::move(column_obj->data_type), UP<object>(new string_object(column_obj->default_value)));
        tab->column_datas.emplace_back(std::move(col));
    } break;

    default:
        eval_push_error_return("eval_alter_table(): Table edit (" + info->table_edit->inspect() + ") not supported");
    }
}



static void eval_create_table(UP<create_table> info, SP<environment> env) {

    UP<object> table_name = eval_expression(std::move(info->table_name), env);
    if (table_name->type() == ERROR_OBJ) {
        eval_push_error_return("CREATE TABLE: Failed to evaluate table name (" + info->table_name->inspect() + ")"); }
   
    if (table_name->type() != STRING_OBJ) {
        eval_push_error_return("CREATE TABLE: Table name (" + info->table_name->inspect() + ") failed to evaluate to string"); }


    for (const auto& entry : s_tables) {
        if (entry->table_name == table_name->data()) {
            eval_push_error_return("CREATE TABLE: Table (" + table_name->data() + ") already exists"); }
    }

    

    avec<UP<table_detail_object>> e_column_datas;
    for (auto& detail : info->details) {
        astring name = detail->name;

        UP<object> data_type = eval_expression(UP<object>(detail->data_type->clone()), env);
        if (data_type->type() == ERROR_OBJ) {
            eval_push_error_return("CREATE TABLE: Failed to evaluate data type (" + detail->data_type->inspect() + ")"); }
        
        if (data_type->type() != SQL_DATA_TYPE_OBJ) {
            eval_push_error_return("CREATE TABLE: Data type entry failed to evaluate to SQL data type (" + detail->data_type->inspect() + ")"); }

        if (detail->default_value->type() != NULL_OBJ) {
            UP<object> default_value = eval_expression(UP<object>(detail->default_value->clone()), env);
            if (default_value->type() == ERROR_OBJ) {
                eval_push_error_return("CREATE TABLE: Failed to evaluate column name (" + detail->default_value->inspect() + ")"); }
            
            if (default_value->type() != STRING_OBJ && default_value->type() != INTEGER_OBJ) {
                eval_push_error_return("CREATE TABLE: Default value failed to evaluate to a string or a number (" + detail->default_value->inspect() + ")"); }
            
            e_column_datas.emplace_back(MAKE_UP(table_detail_object, name, cast_UP<SQL_data_type_object>(data_type), std::move(default_value)));
        } else {
            e_column_datas.emplace_back(MAKE_UP(table_detail_object, name, cast_UP<SQL_data_type_object>(data_type), UP<object>(new null_object())));
        }

    }

    SP<table_object> tab = MAKE_SP(table_object, table_name->data(), std::move(e_column_datas));

    s_tables.push_back(tab);
}

static std::expected<UP<null_object>, UP<error_object>> eval_where(UP<expression_object> clause, UP<table_aggregate_object> table_aggregate, avec<size_t>& row_ids, SP<environment> env) {
    
    if (clause->type() == PREFIX_EXPRESSION_OBJ) {
        push_err_ret_unx_err_obj("Prefix WHERE not supported yet"); }

    if (clause->type() != INFIX_EXPRESSION_OBJ) {
        push_err_ret_unx_err_obj("Tried to evaluate WHERE with non-infix object, bug"); }

    UP<infix_expression_object> where_infix = cast_UP<infix_expression_object>(clause);
    if (where_infix->get_op_type() != WHERE_OP) {
        push_err_ret_unx_err_obj("eval_where(): Called with non-WHERE operator"); }
    

    UP<object> raw_cond = std::move(where_infix->right);
    if (raw_cond->type() != INFIX_EXPRESSION_OBJ) {
        push_err_ret_unx_err_obj("WHERE condition is not infix condition"); }

    UP<infix_expression_object> condition = cast_UP<infix_expression_object>(raw_cond);
    

    if (clause->type() == INFIX_EXPRESSION_OBJ) { // For SELECT [*] FROM [table] WHERE [CONDITION]
        if (where_infix->left->type() == STRING_OBJ) {
            const auto& [table, tab_found] = get_table(where_infix->left->data());
            if (tab_found) {
                table_aggregate->add_table(table);
            }
        }
    }

    UP<infix_expression_object> cond_clone = UP<infix_expression_object>(condition->clone());
    UP<object> e_left = eval_expression(std::move(cond_clone->left), env);
    if (e_left->type() == ERROR_OBJ) {
        push_err_ret_unx_err_obj("SELECT FROM: Failed to evaluate (" + condition->left->inspect() + ")"); }
    
    UP<object> e_right = eval_expression(std::move(cond_clone->right), env);
    if (e_right->type() == ERROR_OBJ) {
        push_err_ret_unx_err_obj("SELECT FROM: Failed to evaluate (" + condition->right->inspect() + ")"); }

    // Find column index BEGIN 
    size_t where_col_index = SIZE_T_MAX;
    std::array<UP<object>, 2> sides = {std::move(e_left), std::move(e_right)};
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
            UP<column_index_object> col_index = cast_UP<column_index_object>(side);

            UP<object> table_alias_obj = std::move(col_index->table_name);
            if (table_alias_obj->type() != TABLE_OBJECT) {
                push_err_ret_unx_err_obj("WHERE: Column index, table alias is not table object, got (" + table_alias_obj->inspect() + ")"); }
            UP<table_object> table = cast_UP<table_object>(table_alias_obj);

            UP<object> column_alias_obj = std::move(col_index->column_name);
            if (column_alias_obj->type() != INDEX_OBJ) {
                push_err_ret_unx_err_obj("WHERE: Column index, column alias is not index object, got (" + column_alias_obj->inspect() + ")"); }
            UP<index_object> index_obj = cast_UP<index_object>(column_alias_obj);
            size_t index = index_obj->value;
            
            auto result = table_aggregate->get_col_id(table->table_name, index);
            if (!result.has_value()) {
                push_err_ret_unx_err_obj(result.error()->data()); }

            where_col_index = *result;
        } break;
        default:
            break;
        }
    }

    if (where_col_index == SIZE_T_MAX) {
        push_err_ret_unx_err_obj("SELECT FROM: Could not find column alias in WHERE condition"); }

    SP<table_object> table = table_aggregate->combine_tables("Shouldn't be in the end result");


    avec<size_t> new_row_ids;
    for (size_t row_id = 0; row_id < table->rows.size(); row_id++) {
        
        // Add to env
        const auto& [raw_cell_value, cell_in_bounds] = table->get_cell_value(row_id, where_col_index);
        if (!cell_in_bounds) {
            push_err_ret_unx_err_obj("SELECT FROM: Weird index bug"); }
        astring cell_value = raw_cell_value->data();
        const auto& [col_data_type, dt_in_bounds] = table->get_column_data_type(where_col_index);
        if (!dt_in_bounds) {
            push_err_ret_unx_err_obj("SELECT FROM: Weird index bug 2"); }
        token_type col_type = col_data_type->data_type;

        UP<object> value;
        switch (col_type) {
        case INT: case INTEGER:
            value = UP<object>(new integer_object(cell_value)); break;
        case DECIMAL:
            value = UP<object>(new decimal_object(cell_value)); break;
        case VARCHAR:
            value = UP<object>(new string_object(cell_value)); break;
        default:
            push_err_ret_unx_err_obj("Currently WHERE conditions aren't supported for " + token_type_to_string(col_type));
        }

        auto&& [column_name, name_in_bounds] = table->get_column_name(where_col_index);
        if (!name_in_bounds) {
            push_err_ret_unx_err_obj("SELECT FROM: Weird index bug 3"); }

        UP<variable_object> var = MAKE_UP(variable_object, column_name, std::move(value));
        SP<environment> row_env = MAKE_SP(environment, env);
        UP<object> added = row_env->add_variable(std::move(var));
        if (added->type() == ERROR_OBJ) {
            push_err_ret_unx_err_obj("Failed to add variable (" + var->inspect() + ") to environment"); }
        // env done

        UP<object> should_add_obj = eval_expression(cast_UP<object>(condition), row_env);
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
static UP<object> eval_left_join([[maybe_unused]] UP<expression_object> clause, [[maybe_unused]] UP<table_aggregate_object> table_aggregate, [[maybe_unused]] avec<size_t>& row_ids, [[maybe_unused]] SP<environment> env) {
    return UP<object>(new error_object("Left Join should use indexes bruh, need to rewrite, look at Github if u want"));
}

// Row ids are passed by reference cause easier, might have to make tab by reference later as well (for stuff like JOINs)
// If is buggy can just go back to using return values
static UP<object> eval_clause(UP<expression_object> clause, UP<table_aggregate_object> table_aggregate, avec<size_t>& row_ids, SP<environment> env) {

    switch (clause->get_op_type()) {

        case WHERE_OP: {
            // WHERE can be infix or prefix
            auto result = eval_where(std::move(clause), std::move(table_aggregate), row_ids, env);
            if (result.has_value()) {
                return cast_UP<object>( std::move(*result));
            } else {
                return cast_UP<object>( std::move(result).error());
            }
        } break;
        case LEFT_JOIN_OP: {
            return eval_left_join(std::move(clause), std::move(table_aggregate), row_ids, env);
        } break;
        default:
            push_err_ret_err_obj("Unsupported op type (" + operator_type_to_astring(clause->get_op_type()) + ")");
    }
}

static std::expected<UP<table_info_object>, UP<error_object>> eval_select_from(UP<select_from> wrapper, SP<environment> env) {

    if (wrapper->value->type() != SELECT_FROM_OBJECT) {
        push_err_ret_unx_err_obj("eval_select_from(): Called with invalid object (" + object_type_to_astring(wrapper->value->type()) + ")"); }

    UP<select_from_object> info = cast_UP<select_from_object>(wrapper->value);    
    
    // First, use clause chain to obtain the initial table
    // Second, the clause chain will conncect tables together, and narrow the ammount of rows selected
    avec<size_t> row_ids;
    UP<table_aggregate_object> table_aggregate = MAKE_UP(table_aggregate_object);
    if (info->clause_chain.size() != 0) {
        for (auto& clause : info->clause_chain) {

            if (clause->type() == STRING_OBJ) { // To support plain SELECT * FROM table;
                const auto& [table, tab_found] = get_table(clause->data());
                if (!tab_found) {
                    push_err_ret_unx_err_obj("SELECT FROM: Table (" + clause->data() + ") does not exist"); }

                row_ids = table->get_row_ids(); 
                table_aggregate->add_table(table);
                break;
            }

            if (clause->type() != INFIX_EXPRESSION_OBJ && clause->type() != PREFIX_EXPRESSION_OBJ) {
                push_err_ret_unx_err_obj("Unsupported clause type, (" + clause->inspect() + ")"); 
            }

            UP<object> error_val = eval_clause(cast_UP<expression_object>(clause), std::move(table_aggregate), row_ids, env); // Should add table to aggregate by itself
            if (error_val->type() != NULL_OBJ) {
                errors.emplace_back(error_val->data()); return std::unexpected(cast_UP<error_object>(error_val)); }
        }

    }



    // Second, use column indexes the index into the table aggregate, validate
    avec<UP<object>> column_indexes = std::move(info->column_indexes);
    avec<size_t> col_ids;
    col_ids.reserve(column_indexes.size());

    if (column_indexes.size() == 0) {
        push_err_ret_unx_err_obj("SELECT FROM: No column indexes"); }

    // If SELECT * FROM [table], add all columns
    if (column_indexes[0]->type() == STAR_OBJECT) {
        col_ids = table_aggregate->get_all_col_ids();

        if (table_aggregate->tables.size() == 1) {
            const auto& [table_name, success] = table_aggregate->get_table_name(0);
            if (!success) {
                push_err_ret_unx_err_obj("SELECT FROM: Strange bug, couldn't get first table from aggregate, even though size == 1"); }
            SP<table_object> table = table_aggregate->combine_tables(table_name);
            return MAKE_UP(table_info_object, table, std::move(col_ids), row_ids);
        }

        SP<table_object> table = table_aggregate->combine_tables("aggregate");
        return  MAKE_UP(table_info_object, table, std::move(col_ids), row_ids);
    }

    for (auto& col_index_raw : column_indexes) {

        // avec<UP<object>> REALARGS = *static_cast<UP<function_call_object>>(col_index_raw)->arguments->elements; for debug


        UP<object> selecter = eval_expression(std::move(col_index_raw), env);
        if (selecter->type() == ERROR_OBJ) {
            push_err_ret_unx_err_obj("SELECT FROM: Could not evalute (" + col_index_raw->inspect() + ")"); }

        switch (selecter->type()) {
        case FUNCTION_CALL_OBJ: { // Not padding for now cause lazy

            if (column_indexes.size() == 1) {
                avec<size_t> new_row_ids;
                new_row_ids.push_back(0);
                row_ids = new_row_ids;
            }

            avec<UP<object>> args = std::move(cast_UP<function_call_object>(selecter)->arguments->elements);
            if (args.size() != 1) {
                push_err_ret_unx_err_obj("COUNT() bad argument count"); }
            const UP<object>& arg = args[0];
            if (arg->type() != STAR_OBJECT) {
                push_err_ret_unx_err_obj("COUNT() argument must be *, got (" + object_type_to_astring(arg->type()) + ")"); }

            size_t count = (table_aggregate->tables[0])->rows.size(); //!!MAJOR keep track of max size
            
            UP<SQL_data_type_object> type = MAKE_UP(SQL_data_type_object, NONE, INTEGER, UP<object>(new integer_object(11)));
            UP<e_group_object> row = MAKE_UP(e_group_object, UP<evaluated>(new integer_object(static_cast<int>(count)))); //!!MAJOR stinky
            UP<table_detail_object> detail = MAKE_UP(table_detail_object, "COUNT(*)", std::move(type), UP<evaluated>(new null_object()));
            SP<table_object> count_star = MAKE_SP(table_object, "COUNT(*) TABLE", std::move(detail), std::move(row));
            table_aggregate->add_table(count_star);
            const auto& [id, ok] = table_aggregate->get_last_col_id();
            if (!ok) {
                push_err_ret_unx_err_obj("SELECT FROM: Weird bug"); }
            col_ids.push_back(id);
        } break;
        case COLUMN_INDEX_OBJECT: {
            UP<column_index_object> col_index = cast_UP<column_index_object>(selecter);
    
            UP<object> raw_tab = std::move(col_index->table_name);
            if (raw_tab->type() != TABLE_OBJECT) {
                push_err_ret_unx_err_obj("SELECT FROM: Column index table alias is not table object"); }
            astring table_name = cast_UP<table_object>(raw_tab)->table_name;
    
            UP<object> raw_index = std::move(col_index->column_name);
            if (raw_index->type() != INDEX_OBJ) {
                push_err_ret_unx_err_obj("SELECT FROM: Column index column alias is not index object"); }
            size_t index = cast_UP<index_object>(raw_index)->value;
    
            auto result = table_aggregate->get_col_id(table_name, index);
            if (!result.has_value()) {
                push_err_ret_unx_err_obj("SELECT FROM:" + result.error()->data()); }
    
            col_ids.push_back(*result);
             
        } break;
        case STRING_OBJ: {
    
            astring column_name = selecter->data();
    
            auto result = table_aggregate->get_col_id(column_name);
            if (!result.has_value()) {
                push_err_ret_unx_err_obj(result.error()->data()); }
    
            col_ids.push_back(*result);
        } break;

        default: 
            push_err_ret_unx_err_obj("SELECT FROM: Cannot use (" + selecter->inspect() + ") to index");

        }
    }

    if (table_aggregate->tables.size() == 1 || column_indexes.size() == 1) {
        const auto& [table_name, ok] = table_aggregate->get_table_name(0);
        if (!ok) {
            push_err_ret_unx_err_obj("SELECT FROM: Strange bug, couldn't get first table from aggregate, even though size == 1"); }
        SP<table_object> table = table_aggregate->combine_tables(table_name);
        return MAKE_UP(table_info_object, table, col_ids, row_ids);
    }

    SP<table_object> table = table_aggregate->combine_tables("aggregate");
    return MAKE_UP(table_info_object, table, col_ids, row_ids);
}

static void print_table() {

    const UP<table_info_object>& tab_info = display_tab.table_info;

    const SP<table_object> tab = tab_info->tab;

    std::cout << tab->table_name << ":\n";

    astring field_names = "";
    for (const auto& col_id : tab_info->col_ids) {

        const auto& [full_name, col_in_bounds] = tab->get_column_name(col_id);
        if (!col_in_bounds) {
            eval_push_error_return("print_table(): Out of bounds column index"); }

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
            eval_push_error_return("print_table(): Out of bounds row index"); }

        const auto& row = **result;

        for (const auto& col_id : tab_info->col_ids) {

            if (col_id >= row.size()) {
                eval_push_error_return("print_table(): Out of bounds column index"); }  

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

static std::pair<SP<table_object>, bool> get_table(const std_and_astring_variant& name) {

    astring name_unwrapped;
    VISIT(name, unwrapped,
        name_unwrapped = unwrapped;
    );

    for (auto& entry : s_tables) {
        if (entry->table_name == name_unwrapped) {
            return {entry, true};
        }
    }

    return {nullptr, false};
}


static void eval_insert_into(UP<insert_into> wrapper, SP<environment> env) {

    if (wrapper->value->type() != INSERT_INTO_OBJECT) {
        eval_push_error_return("eval_insert_into(): Called with invalid object (" + object_type_to_astring(wrapper->value->type()) + ")"); }

    UP<insert_into_object> info = cast_UP<insert_into_object>(wrapper->value);

    UP<object> table_name = eval_expression(std::move(info->table_name), env);
    if (table_name->type() == ERROR_OBJ) {
        eval_push_error_return("Failed to evaluate table name (" + info->table_name->inspect() + ")"); }
   
    if (table_name->type() != STRING_OBJ) {
        eval_push_error_return("Table name (" + info->table_name->inspect() + ") failed to evaluate to string"); }


        
    const auto& [table, tab_found] = get_table(table_name->data());
    if (!tab_found) {
        eval_push_error_return("INSERT INTO, table not found");}



    UP<object> values_obj = eval_expression(std::move(info->values), env);
    if (values_obj->type() != GROUP_OBJ) { // or != TABLE_OBJ???
        eval_push_error_return("INSERT INTO, failed to evaluate values");}

    avec<UP<object>> values = std::move(cast_UP<group_object>(values_obj)->elements);
        
    if (values.size() == 0) {
        eval_push_error_return("INSERT INTO, no values");}

    if (info->fields.size() < values.size()) {
        eval_push_error_return("INSERT INTO, more values than field names");}

    if (info->fields.size() > table->column_datas.size()) {
        eval_push_error_return("INSERT INTO, more field names than table has columns");}

    if (values.size() > table->column_datas.size()) {
        eval_push_error_return("INSERT INTO, more values than table has columns");}


    // evaluate fields
    avec<astring> field_names;
    field_names.reserve(info->fields.size());
    for (auto& field : info->fields) {
        UP<object> evaluated_field = eval_expression(std::move(field), env);
        if (evaluated_field->type() == ERROR_OBJ) {
            eval_push_error_return("eval_insert_into(): Failed to evaluate field (" + field->inspect() + ") while inserting rows"); }

        if (evaluated_field->type() != STRING_OBJ) { // Can't be null >:(
            eval_push_error_return("INSERT INTO: Field (" + field->inspect() + ") evaluated to non-string value"); }

        field_names.emplace_back(evaluated_field->data());

    }
    // check if fields exist
    for (const auto& name : field_names) {
        bool found = table->check_if_field_name_exists(name);
        if (!found) {
            eval_push_error_return("INSERT INTO: Could not find field (" + name + ") in table + (" + table->table_name + ")"); }
    }

    // evaluate values
    avec<UP<object>> e_values;
    e_values.reserve(values.size());
    for (auto& value : values) {
        UP<object> e_value = eval_expression(std::move(value), env);

        if (e_value->type() == ERROR_OBJ) {
            eval_push_error_return("eval_insert_into(): Failed to evaluate value (" + value->inspect() + ") while inserting rows"); }

        e_values.emplace_back(std::move(e_value));
    }

    avec<UP<object>> row;
    if (info->fields.size() > 0) {

        // If field matches column name, add. else add default value
        for (size_t i = 0; i < table->column_datas.size(); i++) {
            bool found = false;
            size_t id = 0;
            for (size_t j = 0; j < field_names.size(); j++) {
                const auto& [column_name, in_bounds] = table->get_column_name(i);
                if (!in_bounds) {
                    eval_push_error_return("INSERT INTO: Weird index bug");}

                if (column_name == field_names[j]) {
                    id = j;
                    found = true; 
                    break; 
                } 
            }

            if (!found) {
                auto result = table->get_column_default_value(i);
                if (!result.has_value()) {
                    eval_push_error_return("INSERT INTO: Weird index bug 2");}
                row.emplace_back(std::move(*result)); }

            if (found) {
                if (id >= e_values.size()) {
                    eval_push_error_return("INSERT INTO: Weird index bug 3"); }
                UP<object> value = std::move(e_values[id]);

                const auto& [data_type, in_bounds] = table->get_column_data_type(i);
                if (!in_bounds) {
                    eval_push_error_return("INSERT INTO: Weird index bug 4");}

                auto result = get_insertable(value->clone(), data_type);
                if (!result.has_value()) {
                    errors.emplace_back(result.error()->data());
                    eval_push_error_return("INSERT INTO: Value (" + value->inspect() + ") evaluated to non-insertable value while inserting rows"); }

                row.emplace_back(std::move(*result)); 
            }

        }
    } else {
        size_t i = 0;
        for (i = 0; i < e_values.size(); i++) {

            UP<object> value = std::move(e_values[i]);

            const auto& [data_type, in_bounds] = table->get_column_data_type(i);
            if (!in_bounds) {
                eval_push_error_return("INSERT INTO: Weird index bug 4");}
                
            auto result = get_insertable(value->clone(), data_type);
                if (!result.has_value()) {
                    errors.emplace_back(result.error()->data());
                eval_push_error_return("INSERT INTO: Value (" + value->inspect() + ") evaluated to non-insertable value while inserting rows"); }

            row.emplace_back(std::move(*result));
        }

        for (; i < table->column_datas.size(); i++) {
            auto result = table->get_column_default_value(i);
            if (!result.has_value()) {
                eval_push_error_return("INSERT INTO: Weird index bug 5");}

            row.emplace_back(std::move(*result)); }
    }

    if (!errors.empty()) {
        return;}

    table->rows.emplace_back(MAKE_UP(group_object, std::move(row)));
}


static bool get_insertable_both_SQL(const UP<SQL_data_type_object>& insert_obj, const UP<SQL_data_type_object>& data_type) {

    if (insert_obj->data_type == data_type->data_type) {
        return true; }
    return false;
}

static std::expected<UP<object>, UP<error_object>> get_insertable(object* insert_obj, const UP<SQL_data_type_object>& data_type) {
    return get_insertable(UP<object>(insert_obj), data_type);
}

static std::expected<UP<object>, UP<error_object>> get_insertable(UP<object> insert_obj, const UP<SQL_data_type_object>& data_type) {

    if (insert_obj->type() == SQL_DATA_TYPE_OBJ) {
        bool ok = get_insertable_both_SQL(cast_UP<SQL_data_type_object>(insert_obj), data_type);
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
            return UP<object>(new integer_object(insert_obj->data()));
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
            return UP<object>(new decimal_object(insert_obj->data()));
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
            return UP<object>(new decimal_object(insert_obj->data()));
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
            return UP<object>(new string_object(insert_obj->data()));
            break;
        case DECIMAL_OBJ:
            warnings.emplace_back("Decimal implicitly converted to VARCHAR");
            return UP<object>(new string_object(insert_obj->data()));
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