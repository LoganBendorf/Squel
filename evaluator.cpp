
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
static avec<UP<e_node>> e_nodes;

extern std::vector<std::string> warnings;

extern avec<SP<table_object>> g_tables;
extern std::vector<SP<evaluated_function_object>> g_functions;

static void eval_function(UP<function> func, SP<environment> env);
static UP<evaluated> eval_run_function(UP<function_call_object> func_call, SP<environment> env);

static UP<object> eval_assert      (UP<assert_node> node,   SP<environment> env);
static void       eval_alter_table (UP<alter_table> info,   SP<environment> env);
static void       eval_create_table(UP<create_table> info,  SP<environment> env);
static void       eval_insert_into (UP<insert_into> wrapper,SP<environment> env);
static std::expected<UP<table_info_object>, UP<error_object>> eval_select     (UP<select_object> info,  SP<environment> env);
static std::expected<UP<e_select_from_object>, UP<error_object>> eval_select_from(UP<select_from> wrapper, SP<environment> env);
static void       print_table() {
    std::cout << "MOVED TO EXECUTE SECTIONS" << std::endl;
}

[[maybe_unused]] static UP<evaluated> eval_expression (object* expression, SP<environment> env);
static UP<evaluated> eval_expression     (UP<object> expression, SP<environment> env);
static UP<evaluated> eval_expression_impl(UP<object> expression, SP<environment> env);
static UP<evaluated> eval_column         (UP<column_object> col, SP<environment> env);

static UP<evaluated> eval_prefix_expression(UP<operator_object> op, UP<object> right, SP<environment> env);
static UP<evaluated> eval_infix_expression (UP<operator_object> op, UP<object> left, UP<object> right, SP<environment> env);
static UP<evaluated> eval_infix_expression (UP<operator_object> op, UP<evaluated> left, UP<evaluated> right, SP<environment> env);

static std::pair<const SP<table_object>&, bool> get_table_as_const(const std_and_astring_variant& name);

enum ret_code : std::uint8_t{
    SUCCESS, FAIL, ERROR
};
[[maybe_unused]] static std::pair<UP<evaluated>, ret_code> convert_table_to_value (const SP<table_object>& tab);
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

void eval_init(avec<UP<node>> nds) {
    nodes = avec<UP<node>>();
    nodes = std::move(nds);
}

static void configure_print_functions(UP<table_info_object> tab_info) {

    if (tab_info->tab->type() != TABLE_OBJECT) {
        eval_push_error_return("SELECT: Failed to evaluate table"); }

    display_tab.to_display = true;
    display_tab.table_info = std::move(tab_info);

    print_table(); // CMD line print, QT will do it's own thing in main
}

avec<UP<e_node>> eval() {

    SP<environment> env = MAKE_SP(environment);
    
    for (auto& node : nodes) {

        switch(node->type()) {
        case INSERT_INTO_NODE:
            std::cout << "EVAL INSERT INTO CALLED\n";
            eval_insert_into(cast_UP<insert_into>(node), env);
            break;
        case SELECT_NODE: {
            std::cout << "EVAL SELECT CALLED\n";
            UP<object> unwrapped = std::move(cast_UP<select_node>(node)->value);
            if (unwrapped->type() != SELECT_OBJECT) {
                errors.emplace_back("Select node contained errors object"); break; }

            UP<select_object> sel_obj = cast_UP<select_object>(unwrapped);
            auto result = eval_select(std::move(sel_obj), env);
            if (!result.has_value()) {
                errors.emplace_back("Failed to evaluate SELECT"); break; }

            configure_print_functions(std::move(*result));
            
        } break;
        case SELECT_FROM_NODE: {
            std::cout << "EVAL SELECT FROM CALLED\n";
            std::expected<UP<e_select_from_object>, UP<error_object>> result = eval_select_from(cast_UP<select_from>(node), env);
            if (!result.has_value()) {
                errors.emplace_back("Failed to evaluate SELECT FROM"); break; }

            auto nd = MAKE_UP(e_select_from, std::move(*result));

            e_nodes.emplace_back(cast_UP<e_node>(std::move(nd)));

        } break;
        case CREATE_TABLE_NODE:
            std::cout << "EVAL CREATE TABLE CALLED\n";
            eval_create_table(cast_UP<create_table>(node), env);
            break;
        case ALTER_TABLE_NODE:
            std::cout << "EVAL ALTER TABLE CALLED\n";
            eval_alter_table(cast_UP<alter_table>(node), env);
            break;
        case FUNCTION_NODE:
            std::cout << "EVAL FUNCTION CALLED\n";
            eval_function(cast_UP<function>(node), env);
            break;
        case ASSERT_NODE:
            std::cout << "EVAL FUNCTION CALLED\n";
            eval_assert(cast_UP<assert_node>(node), env);
            break;
        default:
            errors.emplace_back("eval: unknown node type (" + node->inspect() + ")");
        }
    }

    nodes.clear();
    nodes = avec<UP<node>>();

    return std::move(e_nodes);
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
        /* Shouldn't pass around table objects*/
        push_err_ret_eval_err_obj("BAD" << std::source_location::current().line()); 
        // auto&& [cell, rc] = convert_table_to_value(cast_SP<table_object>(obj));
        // if (rc == SUCCESS) {
        //     return assume_data_type(std::move(cell));
        // }
        push_err_ret_eval_err_obj("Can't assume default data type for (" + obj->inspect() + ")");
    } break;
    case TABLE_INFO_OBJECT: {
        auto&& [cell, rc] = convert_table_info_to_value(cast_UP<table_info_object>(obj));
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
    UP<e_table_detail_object> detail = MAKE_UP(e_table_detail_object, table_name, std::move(type), UP<evaluated>(new null_object()));
    SP<table_object> tab           = MAKE_SP(table_object, table_name, std::move(detail), MAKE_UP(e_group_object, std::move(table_value)));
    UP<table_info_object> tab_info = MAKE_UP(table_info_object, std::move(tab), avec<size_t>{0}, avec<size_t>{0});

    return tab_info;
}

static void eval_function (UP<function> func, SP<environment> env) {

    UP<evaluated> eval_parameters = eval_expression(cast_UP<object>(func->func->parameters), env);
    if (eval_parameters->type() != E_GROUP_OBJ) {
        eval_push_error_return("Failed to evaluate to function parameter"); }

    avec<UP<evaluated>> params = std::move(cast_UP<e_group_object>(eval_parameters)->elements);
    avec<UP<e_parameter_object>> evaluated_parameters;
    evaluated_parameters.reserve(params.size());
    for (auto& param : params) {

        if (param->type() == ERROR_OBJ) {
            eval_push_error_return("Failed to evaluate to function parameter"); }

        if (param->type() != E_PARAMETER_OBJ) {
            eval_push_error_return("Function parameter failed to evaluate to parameter object"); }

        evaluated_parameters.emplace_back(cast_UP<e_parameter_object>(param));
    }

    auto func_obj = std::move(func->func);

    UP<evaluated> ret_type = eval_expression(cast_UP<object>(func_obj->return_type), env);
    if (ret_type->type() != E_SQL_DATA_TYPE_OBJ) {
        eval_push_error_return("Failed to evaluated function return type"); }
    UP<evaluated> body     = eval_expression(cast_UP<object>(func_obj->body), env);
    if (body->type() == E_BLOCK_STATEMENT) {
        eval_push_error_return("Failed to evaluate function body"); }
        
    auto new_func = MAKE_SP(evaluated_function_object, func_obj->name, std::move(evaluated_parameters),
                                                    cast_UP<e_SQL_data_type_object>(ret_type), cast_UP<e_block_statement>(body));
    
    env->add_or_replace_function(new_func);

    // For now just add all functions to global for fun
    bool found = false;
    for (auto & s_function : g_functions) {
        if (s_function->name == new_func->name) {
            s_function = new_func;
            found = true;
            break;
        }
    }
    if (!found) {
        g_functions.push_back(new_func);
    }

    std::cout << "!! PRINTING LE FUNCTION !!\n\n";

    std::cout << new_func->inspect() << std::endl;
}

static UP<evaluated> eval_prefix_expression(UP<operator_object> op, UP<object> right, SP<environment> env) {
    
    UP<evaluated> e_right = eval_expression(std::move(right), env);
    
    switch (op->op_type) {
    case NEGATE_OP:
        switch (e_right->type()) {
        case INTEGER_OBJ:
            return UP<evaluated>(new integer_object( - astring_to_numeric<int>(e_right->data())));
        case DECIMAL_OBJ:
            return UP<evaluated>(new decimal_object( - astring_to_numeric<double>(e_right->data())));
        default:
            push_err_ret_eval_err_obj("No negation operation for type (" + e_right->inspect() + ")");
        }
    default:
        push_err_ret_eval_err_obj("No prefix " + op->inspect() + " operator known");
    }
}


static bool is_values_wrapper(const UP<evaluated>& obj) {
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

static UP<evaluated> eval_infix_values_condition(UP<operator_object> op, UP<values_wrapper_object> left_wrapper, UP<values_wrapper_object> right_wrapper, SP<environment> env) {
    avec<UP<evaluated>> left  = std::move(left_wrapper->values);
    avec<UP<evaluated>> right = std::move(right_wrapper->values);

    if (left.size() != right.size()) {
        push_err_ret_eval_err_obj("Cannot compare values of different sizes"); }

    if (is_comparison_operator(op)) {
        for (size_t i = 0; i < left.size(); i++) {
            UP<evaluated> obj = eval_infix_expression(std::move(op), std::move(left[i]), std::move(right[i]), env);
            if (obj->type() == ERROR_OBJ) {
                push_err_ret_eval_err_obj(obj->data()); }
            if (obj->type() != BOOLEAN_OBJ) {
                push_err_ret_eval_err_obj("compare_values(): Camparison failed to evaluate to boolean"); }

            bool truth = cast_UP<boolean_object>(obj)->value;

            if (!truth) {
                return UP<evaluated>(new boolean_object(false)); }
        }
        return UP<evaluated>(new boolean_object(true));
    } else {
        avec<UP<evaluated>> new_vec;
        new_vec.reserve(left.size());
        for (size_t i = 0; i < left.size(); i++) {
            UP<evaluated> obj = eval_infix_expression(std::move(op), std::move(left[i]), std::move(right[i]), env);
            if (obj->type() == ERROR_OBJ) {
                push_err_ret_eval_err_obj(obj->data()); }

            new_vec.emplace_back(std::move(obj));
        }
        return UP<evaluated>(new e_group_object(std::move(new_vec)));
    }
}

static UP<evaluated> eval_infix_column_vs_value([[maybe_unused]] UP<operator_object> op, [[maybe_unused]] UP<column_index_object> col_index_obj, [[maybe_unused]] UP<evaluated> other, [[maybe_unused]] bool left_first, [[maybe_unused]]  SP<environment> env) {
    push_err_ret_eval_err_obj("TODO" << std::source_location::current().line()); 
    /* Shouldn't pass around table objects, use table info instead */

    // Must be comparison operator
    // if (!is_comparison_operator(op)) {
    //     push_err_ret_eval_err_obj("Condition with table index on one side and a single value on the other must be a comparison"); }

    // UP<object> raw_tab = std::move(col_index_obj->table_name);
    // if (raw_tab->type() != TABLE_OBJECT) {
    //     push_err_ret_eval_err_obj("eval_infix_column_vs_value(): Column index object contained non-table as table alias, got (" + object_type_to_astring(raw_tab->type()) + ")"); }
    // SP<table_object> table = cast_UP<table_object>(raw_tab);
    
    // UP<object> index_obj = std::move(col_index_obj->column_name);
    // if (index_obj->type() != INDEX_OBJ) {
    //     push_err_ret_eval_err_obj("eval_infix_column_vs_value(): Column index object contained non-index as column alias, got (" + object_type_to_astring(index_obj->type()) + ")"); }
    // size_t index = cast_UP<index_object>(index_obj)->value;

    // if (index >= table->rows.size()) {
    //     push_err_ret_eval_err_obj("eval_infix_column_vs_value(): Index out-of-bounds"); }



    // if (left_first) {
    //     for (size_t i = 0; i < table->rows.size(); i++) {

    //         auto&& [cell, ok] = table->get_cell_value(i, index);
    //         if (!ok) {
    //             push_err_ret_eval_err_obj("eval_infix_column_vs_value(): " + cell->data()); }

    //         UP<evaluated> obj = eval_infix_expression(std::move(op), std::move(cell), std::move(other), env);
    //         if (obj->type() == ERROR_OBJ) {
    //             push_err_ret_eval_err_obj(obj->data()); }
    //         if (obj->type() != BOOLEAN_OBJ) {
    //             push_err_ret_eval_err_obj("eval_infix_column_vs_value(): Camparison failed to evaluate to boolean"); }

    //         bool truth = false;
    //         if (cast_UP<boolean_object>(obj)->data() == "TRUE") {
    //             truth = true; }

    //         if (truth != true) {
    //             return UP<evaluated>(new boolean_object(false)); }

    //     }
    // } else {
    //     for (size_t i = 0; i < table->rows.size(); i++) {

    //         auto&& [cell, ok] = table->get_cell_value(i, index);
    //         if (!ok) {
    //             push_err_ret_eval_err_obj("eval_infix_column_vs_value(): " + cell->data()); }

    //         UP<evaluated> obj = eval_infix_expression(std::move(op), std::move(other), std::move(cell), env);
    //         if (obj->type() == ERROR_OBJ) {
    //             push_err_ret_eval_err_obj(obj->data()); }
    //         if (obj->type() != BOOLEAN_OBJ) {
    //             push_err_ret_eval_err_obj("eval_infix_values_condition(): Camparison fail"); }

    //         bool truth = false;
    //         if (cast_UP<boolean_object>(obj)->data() == "TRUE") {
    //             truth = true; }

    //         if (truth != true) {
    //             return UP<evaluated>(new boolean_object(false)); }
    //     }
    // }

    // return UP<evaluated>(new boolean_object(true));
}

static UP<evaluated> eval_infix_values_vs_value(UP<operator_object> op, UP<values_wrapper_object> values, UP<evaluated> other, bool left_first, SP<environment> env) {

    // Must be comparison operator
    if (!is_comparison_operator(op)) {
        push_err_ret_eval_err_obj("Condition with multiple values on one side and a single value on the other must be a comparison"); }


    if (left_first) {
        for (auto& val : values->values) {
            UP<evaluated> obj = eval_infix_expression(std::move(op), std::move(val), std::move(other), env);
            if (obj->type() == ERROR_OBJ) {
                push_err_ret_eval_err_obj(obj->data()); }
            if (obj->type() != BOOLEAN_OBJ) {
                push_err_ret_eval_err_obj("eval_infix_values_condition(): Camparison failed to evaluate to boolean"); }

            bool truth = false;
            if (cast_UP<boolean_object>(obj)->data() == "TRUE") {
                truth = true; }

            if (truth != true) {
                return UP<evaluated>(new boolean_object(false)); }

        }
    } else {
        for (auto& val : values->values) {
            UP<evaluated> obj = eval_infix_expression(std::move(op), std::move(other), std::move(val), env);
            if (obj->type() == ERROR_OBJ) {
                push_err_ret_eval_err_obj(obj->data()); }
            if (obj->type() != BOOLEAN_OBJ) {
                push_err_ret_eval_err_obj("eval_infix_values_condition(): Camparison fail"); }

            bool truth = false;
            if (cast_UP<boolean_object>(obj)->data() == "TRUE") {
                truth = true; }

            if (truth != true) {
                return UP<evaluated>(new boolean_object(false)); }
        }
    }

    return UP<evaluated>(new boolean_object(true));
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

static std::pair<UP<evaluated>, ret_code>  convert_table_to_value(const SP<table_object>& tab) {
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

static UP<evaluated> eval_infix_expression(UP<operator_object> op, UP<object> left, UP<object> right, SP<environment> env) {
    UP<evaluated> e_left  = eval_expression(std::move(left), env);
    UP<evaluated> e_right = eval_expression(std::move(right), env);
    return eval_infix_expression(std::move(op), std::move(e_left), std::move(e_right), env);
}

static UP<evaluated> eval_infix_expression(UP<operator_object> op, UP<evaluated> left, UP<evaluated> right, SP<environment> env) {

    UP<evaluated> e_left  = std::move(left);
    UP<evaluated> e_right = std::move(right);

    if (e_left->type() == ERROR_OBJ) {
        push_err_ret_eval_err_obj(e_left->data()); }
    if (e_right->type() == ERROR_OBJ) {
        push_err_ret_eval_err_obj(e_right->data()); }

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
    std::array<UP<evaluated>*, 2> sides = { &e_left, &e_right };
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
                return UP<evaluated>(new integer_object(cast_UP<integer_object>(std::move(e_left))->value + cast_UP<integer_object>(std::move(e_right))->value));
            } 
            else if (is_string_object(e_left) && is_string_object(e_right)) {
                return UP<evaluated>(new string_object(e_left->data() + e_right->data()));
            } 
            else {
                push_err_ret_eval_err_obj("No infix " << op->inspect() << " operation for " + e_left->inspect() << " and " << e_right->inspect());
            }
            
        case SUB_OP:
            if (!is_numeric_object(e_left) || !is_numeric_object(e_right)) {
                push_err_ret_eval_err_obj("No infix " << op->inspect() << " operation for " + e_left->inspect() << " and " << e_right->inspect());}
            return UP<evaluated>(new integer_object(cast_UP<integer_object>(std::move(e_left))->value - cast_UP<integer_object>(std::move(e_right))->value));
            break;
        case MUL_OP:
            if (!is_numeric_object(e_left) || !is_numeric_object(e_right)) {
                push_err_ret_eval_err_obj("No infix " << op->inspect() << " operation for " + e_left->inspect() << " and " << e_right->inspect());}
            return UP<evaluated>(new integer_object(cast_UP<integer_object>(std::move(e_left))->value * cast_UP<integer_object>(std::move(e_right))->value));
            break;
        case DIV_OP:
            if (!is_numeric_object(e_left) || !is_numeric_object(e_right)) {
                push_err_ret_eval_err_obj("No infix " << op->inspect() << " operation for " + e_left->inspect() << " and " << e_right->inspect());}
            return UP<evaluated>(new integer_object(cast_UP<integer_object>(std::move(e_left))->value / cast_UP<integer_object>(std::move(e_right))->value));
            break;
        case DOT_OP:
            if (e_left->type() != INTEGER_OBJ || e_left->type() != INTEGER_OBJ) {
                push_err_ret_eval_err_obj("No infix " << op->inspect() << " operation for " + e_left->inspect() << " and " << e_right->inspect());}
            return UP<evaluated>(new decimal_object(e_left->data() + "." + e_right->data()));
            break;
        case EQUALS_OP:
            if (e_left->type() == STRING_OBJ && e_right->type() == STRING_OBJ) {
                return UP<evaluated>(new boolean_object(e_left->data() == e_right->data())); 
            } else if (e_left->type() == INTEGER_OBJ && e_right->type() == INTEGER_OBJ) {
                return UP<evaluated>(new boolean_object(cast_UP<integer_object>(std::move(e_left))->value == cast_UP<integer_object>(std::move(e_right))->value)); 
            } else {
                push_err_ret_eval_err_obj("No infix " << op->inspect() << " operation for " + e_left->inspect() << " and " << e_right->inspect());
            }
            break;
        case NOT_EQUALS_OP:
            if (e_left->type() == STRING_OBJ && e_right->type() == STRING_OBJ) {
                return UP<evaluated>(new boolean_object(e_left->data() != e_right->data())); 
            } else if (e_left->type() == INTEGER_OBJ && e_right->type() == INTEGER_OBJ) {
                return UP<evaluated>(new boolean_object(cast_UP<integer_object>(std::move(e_left))->value != cast_UP<integer_object>(std::move(e_right))->value)); 
            } else {
                push_err_ret_eval_err_obj("No infix " << op->inspect() << " operation for " + e_left->inspect() << " and " << e_right->inspect());
            }
            break;
        case LESS_THAN_OP:
            if (e_left->type() != INTEGER_OBJ || e_left->type() != INTEGER_OBJ) {
                push_err_ret_eval_err_obj("No infix " << op->inspect() << " operation for " + e_left->inspect() << " and " << e_right->inspect());}
            return UP<evaluated>(new boolean_object(cast_UP<integer_object>(std::move(e_left))->value < cast_UP<integer_object>(std::move(e_right))->value));
            break;
        case GREATER_THAN_OP:
            if (e_left->type() != INTEGER_OBJ || e_left->type() != INTEGER_OBJ) {
                push_err_ret_eval_err_obj("No infix " << op->inspect() << " operation for " + e_left->inspect() << " and " << e_right->inspect());}
            return UP<evaluated>(new boolean_object(cast_UP<integer_object>(std::move(e_left))->value > cast_UP<integer_object>(std::move(e_right))->value));
            break;
        default:
            push_err_ret_eval_err_obj("No infix " + op->inspect() + " operator known");
    }
}


static UP<evaluated> eval_expression(object* expression, SP<environment> env) {

    UP<object> expr = UP<object>(expression);

    UP<evaluated> result = eval_expression_impl(std::move(expr), env);
    if (!is_evaluated(result)) {
        push_err_ret_eval_err_obj("eval_expression(): Failed to return evaluated object"); }
        
    return cast_UP<evaluated>(result);
}

static UP<evaluated> eval_expression(UP<object> expression, SP<environment> env) {

    UP<evaluated> result = eval_expression_impl(std::move(expression), env);
    if (!is_evaluated(result)) {
        push_err_ret_eval_err_obj("eval_expression(): Failed to return evaluated object"); }

    return cast_UP<evaluated>(result);
}

static UP<evaluated> eval_expression_impl(UP<object> expression, SP<environment> env) {

    switch(expression->type()) {

    // Basic stuff begin
    case STAR_OBJECT:
        return cast_UP<evaluated>(expression); break;
    case INTEGER_OBJ:
        return cast_UP<evaluated>(expression); break;
    case STRING_OBJ:
        return cast_UP<evaluated>(expression); break;
    case PARAMETER_OBJ:
        return cast_UP<evaluated>(expression); break;
    case RETURN_STATEMENT:
        return cast_UP<evaluated>(expression); break;
    case VARIABLE_OBJ:
        return eval_expression(std::move(cast_UP<variable_object>(expression)->value), env); break;
    case SQL_DATA_TYPE_OBJ: {
        UP<SQL_data_type_object> cur = cast_UP<SQL_data_type_object>(expression);
        UP<evaluated> param = eval_expression(std::move(cur->parameter), env);
        if (param->type() == ERROR_OBJ) {
            return param; }

        if (param->type() != INTEGER_OBJ && param->type() != DECIMAL_OBJ && param->type() != NULL_OBJ) {
            push_err_ret_eval_err_obj("For now parameters of SQL data type must evaluate to integer/decimal/none, can be strings later when working on SET or ENUM"); }

        auto evaled = MAKE_UP(e_SQL_data_type_object, cur->prefix, cur->data_type, std::move(param));

        return cast_UP<evaluated>(evaled);
    } break;
    case NULL_OBJ:
        return cast_UP<evaluated>(expression); break;
    case PREFIX_EXPRESSION_OBJ:
        return eval_prefix_expression(std::move(cast_UP<prefix_expression_object>(expression)->op), std::move(cast_UP<prefix_expression_object>(expression)->right), env); break;
    // Basic stuff end

    case SELECT_OBJECT: {
        auto result = eval_select(cast_UP<select_object>(expression), env);
        if (result.has_value()) {
            return cast_UP<evaluated>(std::move(*result));
        } else {
            return cast_UP<evaluated>(std::move(result).error());
        }
    }

    case COLUMN_INDEX_OBJECT: {
        
        UP<column_index_object> obj = cast_UP<column_index_object>(expression);

        UP<evaluated> tab_expr = eval_expression(std::move(obj->table_name), env);
        if (tab_expr->type() != STRING_OBJ) {
            push_err_ret_eval_err_obj("Column index object: Table name failed to evaluate to string"); }
        astring table_name = tab_expr->data();

        UP<evaluated> col_expr = eval_expression(std::move(obj->column_name), env);
        if (col_expr->type() != STRING_OBJ) {
            push_err_ret_eval_err_obj("Column index object: Column name failed to evaluate to string"); }
        astring column_name = col_expr->data();

        const auto& [table, tab_exists] = get_table_as_const(table_name);
        if (!tab_exists) {
            push_err_ret_eval_err_obj("Column index object: Table does not exist"); }

        const auto& [col_index, col_exists]  = table->get_column_index(column_name);
        if (!col_exists) {
            push_err_ret_eval_err_obj("Column index object: Column does not exist"); }

        return UP<evaluated>(new e_column_index_object(table, MAKE_UP(index_object, col_index)));

    } break;

    case SELECT_FROM_OBJECT: {
        UP<select_from> wrapper = MAKE_UP(select_from, std::move(expression));
        auto result = eval_select_from(std::move(wrapper), env);
        if (result.has_value()) {
            return cast_UP<evaluated>(std::move(*result));
        } else {
            return cast_UP<evaluated>(std::move(result).error());
        }
    } break;

    case COLUMN_OBJ:
        return eval_column(cast_UP<column_object>(expression), env);

    case FUNCTION_CALL_OBJ:
        return eval_run_function(cast_UP<function_call_object>(expression), env); break;

    case GROUP_OBJ: {
        UP<group_object> group = cast_UP<group_object>(expression);
        avec<UP<evaluated>> objects;
        for (auto& obj: group->elements) {
            UP<evaluated> evaled = eval_expression(std::move(obj), env);
            if (evaled->type() == ERROR_OBJ) {
                return evaled; }
            objects.emplace_back(std::move(evaled));
        }
        return UP<evaluated>(new e_group_object(std::move(objects)));
    } break;
    case BLOCK_STATEMENT: {
        UP<block_statement> block = cast_UP<block_statement>(expression);
        UP<e_return_statement> ret_val;
        avec<UP<evaluated>> statements;
        statements.reserve(block->body.size());
        bool has_ret = false;
        for (auto& statement: block->body) {
            UP<evaluated> res = eval_expression(std::move(statement), env);
            if (res->type() == E_RETURN_STATEMENT) {
                if (has_ret) {
                    push_err_ret_eval_err_obj("Block contained multiple (outer) return statements"); }
                ret_val = cast_UP<e_return_statement>(res);
                has_ret = true;
            } else {
                statements.emplace_back(std::move(res)); }
        }
        if (!has_ret) {
            ret_val = MAKE_UP(e_return_statement, UP<evaluated>(new null_object())); }
        return UP<evaluated>(new expression_statement(std::move(statements), std::move(ret_val)));
    } break;

    case INFIX_EXPRESSION_OBJ: {
        UP<infix_expression_object> condition = cast_UP<infix_expression_object>(expression);

        UP<evaluated> e_left = nullptr;
        UP<evaluated> e_right = nullptr;

        UP<object> left = std::move(condition->left);

        if (left->type() == STRING_OBJ) {
            auto result = env->get_variable(left->data());
            if (result.has_value()) {
                e_left = eval_expression(cast_UP<object>(std::move(*result)), env);
            }
        } else {
            e_left = eval_expression(std::move(left), env);
        }

        UP<object> right = std::move(condition->right);

        if (right->type() == STRING_OBJ) {
            auto result = env->get_variable(right->data());
            if (result.has_value()) {
                e_right = eval_expression(cast_UP<object>(std::move(*result)), env);
            }
        } else {
            e_right = eval_expression(std::move(right), env);
        }

        UP<evaluated> result = eval_infix_expression(std::move(condition->op), std::move(left), std::move(right), env);
        if (result->type() == ERROR_OBJ) {
            return result; }

        return result;
    } break;

    case IF_STATEMENT: {
        UP<if_statement> statement = cast_UP<if_statement>(expression);

        UP<evaluated> obj = eval_expression(std::move(statement->condition), env); // LOWEST or PREFIX??
        if (obj->type() == ERROR_OBJ) {
            return obj; }

        if (obj->type() != BOOLEAN_OBJ) {
            push_err_ret_eval_err_obj("If statement condition returned non-boolean"); }

        UP<boolean_object> condition_result = cast_UP<boolean_object>(obj);

        if (condition_result->data() == "TRUE") { // scuffed
            UP<evaluated> result = eval_expression(cast_UP<object>(statement->body), env);
            return result;
        } else if (statement->other->type() != NULL_OBJ) {
            UP<evaluated> result = eval_expression(std::move(statement->other), env);
            return result;
        }

        return UP<evaluated>(new null_object());

    } break;
    default:
        push_err_ret_eval_err_obj("eval_expression(): Cannot evaluate expression. Type (" << object_type_to_astring(expression->type()) << "), value(" << expression->inspect() << ")"); 
    }
}


static avec<UP<e_argument_object>> name_arguments(SP<evaluated_function_object> function, UP<function_call_object> func_call, SP<environment> env) {

    avec<UP<e_argument_object>> named_arguments;

    UP<evaluated> eval_args = eval_expression(cast_UP<object>(func_call->arguments), env);
    if (eval_args->type() != E_GROUP_OBJ) {
        errors.emplace_back("Failed to evaluate arguments"); return named_arguments; }

    avec<UP<evaluated>> evaluated_arguments = std::move(cast_UP<e_group_object>(eval_args)->elements);

    

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
        named_arguments.emplace_back(MAKE_UP(e_argument_object, param_name, std::move(evaluated_arguments[i])));
    }

    return named_arguments;
}



static UP<evaluated> eval_run_function(UP<function_call_object> func_call, SP<environment> env) {

    if (func_call->name == "COUNT") {
        avec<UP<object>> args = std::move(func_call->arguments->elements);
        avec<UP<evaluated>> e_args;
        e_args.reserve(args.size());
        for (auto& arg : args) {
            UP<evaluated> e_arg = eval_expression(std::move(arg), env);
            if (e_arg->type() == ERROR_OBJ) {
                push_err_ret_eval_err_obj("Failed to evaluate (" + arg->inspect() + ")"); }
            e_args.emplace_back(std::move(e_arg));
        }
        return UP<evaluated>(new e_function_call_object("COUNT", MAKE_UP(e_group_object, std::move(e_args))));
    }

    bool found = false;
    for (const auto& func : g_functions) {
        if (func->name == func_call->name) {
            found = true; 
        }
    }

    if (!found && !env->is_function(func_call->name)) {
        push_err_ret_eval_err_obj("Called non-existent function (" + func_call->name + ")"); }
    


    auto&& [function, exists] = env->get_function(func_call->name);
    if (!exists) {
        push_err_ret_eval_err_obj("Function does not exist (" + func_call->name + ")"); }

    size_t error_count = errors.size();
    avec<UP<e_argument_object>> named_args = name_arguments(function, std::move(func_call), env);
    if (error_count < errors.size()) {
        return UP<evaluated>(new error_object()); }



    const avec<UP<e_parameter_object>>& parameters = function->parameters;
    if (named_args.size() != parameters.size()) {
        push_err_ret_eval_err_obj("Function called with incorrect number of arguments, got " << named_args.size() << " wanted " << parameters.size()); }

    SP<environment> function_env = MAKE_SP(environment, env);
    bool ok = function_env->add_variables(std::move(named_args));
    if (!ok) {
        push_err_ret_eval_err_obj("Failed to add function arguments as variables to function environment"); }

    for (const auto& line : function->body->body) {
        UP<evaluated> res = eval_expression(UP<object>(line->clone()), function_env);
        if (res->type() == ERROR_OBJ) {
            return res; }

        /* I think it's just this */
        if (res->type() == RETURN_STATEMENT) {
            return res;
        }

        /* Not sure this is needed */
        // if (res->type() == RETURN_STATEMENT) {
        //     return eval_expression(std::move(cast_UP<return_statement>(res)->expression), env);
        // }
        
    }

    push_err_ret_eval_err_obj("Failed to find return value");
}

static UP<evaluated> eval_column(UP<column_object> col, SP<environment> env) {
    UP<evaluated> parameter = eval_expression(std::move(col->name_data_type), env);
    if (parameter->type() == ERROR_OBJ) {
        return parameter; }

    if (parameter->type() != E_PARAMETER_OBJ) {
        push_err_ret_eval_err_obj("Column data type (" + col->name_data_type->inspect() + ")failed to evaluate to E parameter object"); }

    UP<evaluated> default_value = eval_expression(std::move(col->default_value), env);
    if (default_value->type() == ERROR_OBJ) {
        return default_value; }

    if (default_value->type() != STRING_OBJ && default_value->type() != NULL_OBJ) {
        push_err_ret_eval_err_obj("Column default value (" + col->default_value->inspect() + ") failed to evaluate to string object"); }

    if (default_value->type() == NULL_OBJ) {
        default_value = UP<evaluated>(new string_object("")); }

    return UP<evaluated>(new evaluated_column_object(cast_UP<e_parameter_object>(parameter)->name, std::move(cast_UP<e_parameter_object>(parameter)->data_type), std::move(default_value)));
}

static void eval_alter_table(UP<alter_table> info, SP<environment> env) {

    UP<evaluated> table_name = eval_expression(std::move(info->table_name), env);
    if (table_name->type() == ERROR_OBJ) {
        eval_push_error_return("Failed to evaluate table name (" + info->table_name->inspect() + ")"); }
   
    if (table_name->type() != STRING_OBJ) {
        eval_push_error_return("Table name (" + info->table_name->inspect() + ") failed to evaluate to string"); }
    

    const auto& [table, tab_found] = get_table_as_const(table_name->data());
    if (!tab_found) {
        eval_push_error_return("INSERT INTO: table not found");}

    SP<table_object> tab = table;



    UP<evaluated> table_edit = eval_expression(std::move(info->table_edit), env);
    if (table_edit->type() == ERROR_OBJ) {
        eval_push_error_return("eval_alter_table(): Failed to evaluate table edit"); }

    switch (table_edit->type()) {
    case EVALUATED_COLUMN_OBJ: {

        UP<evaluated_column_object> column_obj = cast_UP<evaluated_column_object>(table_edit);

        for (const auto& table_column : tab->column_datas){
            if (column_obj->name == table_column->name) {
                eval_push_error_return("eval_alter_table(): Table already contains column with name (" + column_obj->name + ")"); }
        }

        UP<e_table_detail_object> col = MAKE_UP(e_table_detail_object, column_obj->name, std::move(column_obj->data_type), std::move(column_obj->default_value));
        tab->column_datas.emplace_back(std::move(col));
    } break;

    default:
        eval_push_error_return("eval_alter_table(): Table edit (" + info->table_edit->inspect() + ") not supported");
    }
}



static void eval_create_table(UP<create_table> info, SP<environment> env) {

    astring table_name = info->table_name;

    avec<UP<e_table_detail_object>> e_column_datas;
    for (auto& detail : info->details) {
        astring name = detail->name;

        UP<evaluated> data_type = eval_expression(UP<object>(detail->data_type->clone()), env);
        if (data_type->type() == ERROR_OBJ) {
            eval_push_error_return("CREATE TABLE: Failed to evaluate data type (" + detail->data_type->inspect() + ")"); }
        
        if (data_type->type() != E_SQL_DATA_TYPE_OBJ) {
            eval_push_error_return("CREATE TABLE: Data type entry failed to evaluate to E SQL data type (" + detail->data_type->inspect() + ")"); }

        if (detail->default_value->type() != NULL_OBJ) {
            UP<evaluated> default_value = eval_expression(UP<object>(detail->default_value->clone()), env);
            if (default_value->type() == ERROR_OBJ) {
                eval_push_error_return("CREATE TABLE: Failed to evaluate column name (" + detail->default_value->inspect() + ")"); }
            
            if (default_value->type() != STRING_OBJ && default_value->type() != INTEGER_OBJ) {
                eval_push_error_return("CREATE TABLE: Default value failed to evaluate to a string or a number (" + detail->default_value->inspect() + ")"); }
            
            e_column_datas.emplace_back(MAKE_UP(e_table_detail_object, name, cast_UP<e_SQL_data_type_object>(data_type), std::move(default_value)));
        } else {
            e_column_datas.emplace_back(MAKE_UP(e_table_detail_object, name, cast_UP<e_SQL_data_type_object>(data_type), UP<evaluated>(new null_object())));
        }

    }

    e_nodes.push_back(UP<e_node>(new e_create_table(table_name, std::move(e_column_datas))));
}



static std::expected<UP<e_select_from_object>, UP<error_object>> eval_select_from(UP<select_from> wrapper, SP<environment> env) {

    if (wrapper->value->type() != SELECT_FROM_OBJECT) {
        push_err_ret_unx_err_obj("eval_select_from(): Called with invalid object (" + object_type_to_astring(wrapper->value->type()) + ")"); }

    UP<select_from_object> info = cast_UP<select_from_object>(wrapper->value);    

    avec<UP<evaluated>> e_clause_chain;
    e_clause_chain.reserve(info->clause_chain.size());
    if (info->clause_chain.size() != 0) {
        for (const auto& clause : info->clause_chain) {

            UP<evaluated> e_clause = eval_expression(UP<object>(clause->clone()), env);
            if (e_clause->type() == ERROR_OBJ) {
                push_err_ret_unx_err_obj("SELECT FROM eval: Could not evalute (" + clause->inspect() + ")"); }

            switch (e_clause->type()) {
                case STRING_OBJ: case INFIX_EXPRESSION_OBJ: case PREFIX_EXPRESSION_OBJ: {
                } break;
                default: {
                    push_err_ret_unx_err_obj("Unsupported clause type, (" + clause->inspect() + ")"); 
                }
            }

            e_clause_chain.push_back(std::move(e_clause));
        }   
    }



    // Second, use column indexes the index into the table aggregate, validate
    avec<UP<object>> column_indexes = std::move(info->column_indexes);
    avec<UP<evaluated>> e_column_indexes;
    e_column_indexes.reserve(column_indexes.size());

    if (column_indexes.size() == 0) {
        push_err_ret_unx_err_obj("SELECT FROM eval: No column indexes"); }


    if (column_indexes[0]->type() == STAR_OBJECT) {

        if (column_indexes.size() > 1) {
            push_err_ret_unx_err_obj("SELECT FROM eval: (*) must be alone"); }

        e_column_indexes.push_back(cast_UP<evaluated>(column_indexes[0]));

        return MAKE_UP(e_select_from_object, std::move(e_column_indexes), std::move(e_clause_chain));
    }

    for (const auto& col_index_raw : column_indexes) {

        UP<evaluated> selecter = eval_expression(UP<object>(col_index_raw->clone()), env);
        if (selecter->type() == ERROR_OBJ) {
            push_err_ret_unx_err_obj("SELECT FROM eval: Could not evalute (" + col_index_raw->inspect() + ")"); }

        switch (selecter->type()) {
        case FUNCTION_CALL_OBJ: case COLUMN_INDEX_OBJECT: case STRING_OBJ: {
        } break;
        default: 
            push_err_ret_unx_err_obj("SELECT FROM eval: Cannot use (" + col_index_raw->inspect() + ") to index"); }

        e_column_indexes.push_back(std::move(selecter));
    }

    return MAKE_UP(e_select_from_object, std::move(e_column_indexes), std::move(e_clause_chain));
}



static std::pair<const SP<table_object>&, bool> get_table_as_const(const std_and_astring_variant& name) {

    std::string name_unwrapped;
    VISIT(name, unwrapped,
        name_unwrapped = unwrapped;
    );

    for (const auto& entry : g_tables) {
        if (entry->table_name == name_unwrapped) {
            return {entry, true};
        }
    }

    SP<table_object> garbage;
    return {garbage, false};
}


static void eval_insert_into(UP<insert_into> wrapper, SP<environment> env) {

    if (wrapper->value->type() != INSERT_INTO_OBJECT) {
        eval_push_error_return("eval_insert_into(): Called with invalid object (" + object_type_to_astring(wrapper->value->type()) + ")"); }

    UP<insert_into_object> info = cast_UP<insert_into_object>(wrapper->value);



    astring table_name = info->table_name;

    UP<evaluated> values_obj = eval_expression(std::move(info->values), env);
    if (values_obj->type() != E_GROUP_OBJ) { // or != TABLE_OBJ???
        eval_push_error_return("INSERT INTO eval: failed to evaluate values");}

    avec<UP<evaluated>> values = std::move(cast_UP<e_group_object>(values_obj)->elements);
        
    if (values.size() == 0) {
        eval_push_error_return("INSERT INTO eval: no values");}

    if (info->fields.size() < values.size()) {
        eval_push_error_return("INSERT INTO eval: more values than field names");}

    // evaluate fields
    avec<UP<evaluated>> field_names;
    field_names.reserve(info->fields.size());
    for (auto& field : info->fields) {
        UP<evaluated> evaluated_field = eval_expression(UP<object>(field->clone()), env);
        if (evaluated_field->type() == ERROR_OBJ) {
            eval_push_error_return("eval_insert_into(): Failed to evaluate field (" + field->inspect() + ") while inserting rows"); }

        if (evaluated_field->type() != STRING_OBJ) { // For now only accepting string objects
            eval_push_error_return("INSERT INTO eval: Field (" + field->inspect() + ") evaluated to non-string value"); }

        field_names.push_back(std::move(evaluated_field));
    }


    // evaluate values
    // avec<UP<evaluated>> e_values;
    // e_values.reserve(values.size());
    // for (auto& value : values) {
    //     UP<evaluated> e_value = eval_expression(UP<object>(value->clone()), env);

    //     if (e_value->type() == ERROR_OBJ) {
    //         eval_push_error_return("eval_insert_into(): Failed to evaluate value (" + value->inspect() + ") while inserting rows"); }

    //     e_values.push_back(std::move(e_value));
    // }

    if (!errors.empty()) {
        return;}

    e_nodes.emplace_back(UP<e_node>(new e_insert_into(MAKE_UP(e_insert_into_object, table_name, std::move(field_names), std::move(values)))));
}

