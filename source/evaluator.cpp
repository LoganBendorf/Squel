
#include "pch.h"

#include "evaluator.h"

#include "allocator_aliases.h"
#include "node.h"
#include "structs.h"
#include "helpers.h"
#include "object.h"
#include "environment.h"
#include "macros.h"
#include "logger.h"


extern logger<error_msg> sql_errors;
extern logger<warning_msg> sql_warnings;

extern bool DEBUG;



static avec<UP<node>> nodes;
static avec<UP<e_node>> e_nodes;



static void          eval_function    (UP<function>             func,      SP<environment> env);
static UP<evaluated> eval_run_function(UP<function_call_object> func_call, SP<environment> env);

static std::expected<UP<e_assert_node>, UP<error_object>>        eval_assert      (UP<assert_node>      node,    SP<environment> env);
static std::expected<UP<e_alter_table_node>, UP<error_object>>   eval_alter_table (UP<alter_table_node> info,    SP<environment> env);
static void                                                      eval_create_table(UP<create_table>     info,    SP<environment> env);
static void                                                      eval_insert_into (UP<insert_into>      wrapper, SP<environment> env);
static std::expected<UP<e_select_node>, UP<error_object>>        eval_select      (UP<select_object>    info,    SP<environment> env);
static std::expected<UP<e_select_from_object>, UP<error_object>> eval_select_from (UP<select_from>      wrapper, SP<environment> env);


static UP<evaluated> eval_expression_impl(UP<object> expression, SP<environment> env);
static UP<evaluated> eval_prefix_expression(UP<operator_object> op, UP<object> right, SP<environment> env);



#define log_sql_error(x)                               \
    do {                                               \
        std::stringstream err;                         \
        err << x;                                      \
        sql_errors.add_msg(err.str(), CUR_LOC);        \
    } while(0)                

#define eval_push_err_ret(x)                           \
    do {                                               \
        std::stringstream err;                         \
        err << x;                                      \
        sql_errors.add_msg(err.str(), CUR_LOC);        \
        return;                                        \
    } while(0)                


#define push_err_ret_err_obj(x)                        \
    do {                                               \
        std::stringstream err;                         \
        err << x;                                      \
        sql_errors.add_msg(err.str(), CUR_LOC);        \
        return UP<object>(new error_object());         \
    } while(0)

#define push_err_ret_unx_err_obj(x)                    \
    do {                                               \
        std::stringstream err;                         \
        err << x;                                      \
        sql_errors.add_msg(err.str(), CUR_LOC);        \
        return std::unexpected(MAKE_UP(error_object)); \
    } while(0)

#define push_err_ret_eval_err_obj(x)                   \
    do {                                               \
        std::stringstream err;                         \
        err << x;                                      \
        sql_errors.add_msg(err.str(), CUR_LOC);        \
        return UP<evaluated>(new error_object());      \
    } while(0)

#define push_err_break(x)                              \
    std::stringstream err;                             \
    err << x;                                          \
    sql_errors.add_msg(err.str(), CUR_LOC);            \
    break                                              \

void eval_init(avec<UP<node>> nds) {
    nodes = avec<UP<node>>();
    nodes = std::move(nds);
}

avec<UP<e_node>> eval() {

    SP<environment> env = MAKE_SP(environment);
    
    for (auto& node : nodes) {

        switch(node->type()) {
        case INSERT_INTO_NODE:
            if (DEBUG) [[unlikely]] { std::cout << "EVAL INSERT INTO CALLED\n"; }
            eval_insert_into(CAST_UP(insert_into, node), env);
            break;
        case SELECT_NODE: {
            if (DEBUG) [[unlikely]] {  std::cout << "EVAL SELECT CALLED\n"; }
            UP<object> unwrapped = std::move(CAST_UP(select_node, node)->value);
            if (unwrapped->type() != SELECT_OBJ) {
                log_sql_error("Select node contained errors object"); break; }

            UP<select_object> sel_obj = CAST_UP(select_object, unwrapped);
            std::expected<UP<e_select_node>, UP<error_object>> result = eval_select(std::move(sel_obj), env);
            if (!result.has_value()) {
                log_sql_error("Failed to evaluate SELECT"); break; }

            auto nd = std::move(*result);

            e_nodes.push_back(CAST_UP(e_node, std::move(nd)));
            
        } break;
        case SELECT_FROM_NODE: {
            if (DEBUG) [[unlikely]] { std::cout << "EVAL SELECT FROM CALLED\n"; }
            std::expected<UP<e_select_from_object>, UP<error_object>> result = eval_select_from(CAST_UP(select_from, node), env);
            if (!result.has_value()) {
                log_sql_error("Failed to evaluate SELECT FROM"); break; }

            auto nd = MAKE_UP(e_select_from_node, std::move(*result));
            e_nodes.push_back(CAST_UP(e_node, nd));

        } break;
        case CREATE_TABLE_NODE:
            if (DEBUG) [[unlikely]] { std::cout << "EVAL CREATE TABLE CALLED\n"; }
            eval_create_table(CAST_UP(create_table, node), env);
            break;
        case ALTER_TABLE_NODE: {
            if (DEBUG) [[unlikely]] { std::cout << "EVAL ALTER TABLE CALLED" << std::endl; }
            auto result = eval_alter_table(CAST_UP(alter_table_node, node), env);
            if (!result.has_value()) {
                log_sql_error("Failed to evaluate ALTER TABLE"); break; }

            auto nd = std::move(*result);
            e_nodes.push_back(CAST_UP(e_node, nd));
        } break;
        case FUNCTION_NODE:
            if (DEBUG) [[unlikely]] { std::cout << "EVAL FUNCTION CALLED" << std::endl; }
            eval_function(CAST_UP(function, node), env);
            break;
        case ASSERT_NODE: {
            if (DEBUG) [[unlikely]] { std::cout << "EVAL ASSERT CALLED" << std::endl; }
            auto result = eval_assert(CAST_UP(assert_node, node), env);
            if (!result.has_value()) {
                log_sql_error("Failed to evaluate ASSERT"); break; }

            auto nd = std::move(*result);
            e_nodes.push_back(CAST_UP(e_node, nd));
        } break;
        default:
            log_sql_error("eval: unknown node type (" + node->inspect() + ")");
        }
    }

    nodes.clear();
    nodes = avec<UP<node>>();

    return std::move(e_nodes);
}

static std::expected<UP<e_assert_node>, UP<error_object>> eval_assert(UP<assert_node> node, SP<environment> env) {

    UP<assert_object> info = std::move(node->value);

    UP<evaluated> expr = eval_expression(std::move(info->expression), env);
    if (expr->type() == ERROR_OBJ) {
        push_err_ret_unx_err_obj("Failed to evaluate ASSERT expression"); }

    return MAKE_UP(e_assert_node, MAKE_UP(e_assert_object, info->line, std::move(expr)));
}



static std::expected<UP<e_select_node>, UP<error_object>> eval_select(UP<select_object> info, SP<environment> env) {

    if (info->type() != SELECT_OBJ) {
        FATAL_ERROR_THROW("eval_select() called with invalid object (" << object_type_to_astring(info->type()) << ")", CUR_LOC); }

    UP<evaluated> table_value = eval_expression(std::move(info->value), env);
    if (table_value->type() == ERROR_OBJ) {
        push_err_ret_unx_err_obj("Failed to evaluate SELECT value (" + table_value->inspect() + ")"); }

    if (!is_serializable(table_value)) {
        push_err_ret_unx_err_obj("SELECT value (" + table_value->inspect() + ") failed to evaluate to serializable"); }

    auto val = CAST_UP(serializable, table_value);
    return MAKE_UP(e_select_node, std::move(val));
}


// TODO Move to EXEC
static void eval_function([[maybe_unused]] UP<function> func, [[maybe_unused]] SP<environment> env) {

    FATAL_ERROR_STACK_TRACE_THROW("Move to EXECUTE", CUR_LOC);

    // auto params_as_obj = UP<object>(new group_object(std::move(func->func->parameters)));
    // UP<evaluated> eval_parameters = eval_expression(std::move(params_as_obj), env);
    // if (eval_parameters->type() != E_GROUP_OBJ) {
    //     eval_push_err_ret("Failed to evaluate to function parameter"); }

    // avec<UP<evaluated>> params = std::move(CAST_UP(e_group_object, eval_parameters)->elements);
    // avec<UP<e_parameter_object>> evaluated_parameters;
    // evaluated_parameters.reserve(params.size());
    // for (auto& param : params) {
    //     if (param->type() == ERROR_OBJ) {
    //         eval_push_err_ret("Failed to evaluate to function parameter"); }

    //     if (param->type() != E_PARAMETER_OBJ) {
    //         eval_push_err_ret("Function parameter failed to evaluate to parameter object"); }

    //     evaluated_parameters.emplace_back(CAST_UP(e_parameter_object, param));
    // }

    // auto func_obj = std::move(func->func);

    // UP<evaluated> ret_type = eval_expression(CAST_UP(object, func_obj->return_type), env);
    // if (ret_type->type() != E_SQL_DATA_TYPE_OBJ) {
    //     eval_push_err_ret("Failed to evaluated function return type"); }
    // UP<evaluated> body     = eval_expression(CAST_UP(object, func_obj->body), env);
    // if (body->type() == E_BLOCK_STATEMENT) {
    //     eval_push_err_ret("Failed to evaluate function body"); }
        
    // auto new_func = MAKE_SP(e_function_object, func_obj->name, std::move(evaluated_parameters),
    //                                                 CAST_UP(e_SQL_data_type_object, ret_type), CAST_UP(e_block_statement, body));
    
    // env->add_or_replace_function(new_func);

    // // For now just add all functions to global for fun
    // bool found = false;
    // for (auto & s_function : g_functions) {
    //     if (s_function->name == new_func->name) {
    //         s_function = new_func;
    //         found = true;
    //         break;
    //     }
    // }

    // if (!found) {
    //     g_functions.push_back(new_func); }

    // if (DEBUG) [[unlikely]] { std::cout << "!! PRINTING LE FUNCTION !!\n\n" << new_func->inspect() << std::endl; }

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

[[maybe_unused]] static bool is_comparison_operator(const UP<operator_object>& op) {
    switch (op->op_type) {
    case EQUALS_OP: case NOT_EQUALS_OP: case LESS_THAN_OP: case GREATER_THAN_OP:
        return true;
    default:
        return false;
    }
}


// TODO maybe add to execute.cpp???
[[maybe_unused]] static UP<evaluated> eval_infix_column_vs_value([[maybe_unused]] UP<operator_object> op, [[maybe_unused]] UP<column_index_object> col_index_obj, [[maybe_unused]] UP<evaluated> other, [[maybe_unused]] bool left_first, [[maybe_unused]]  SP<environment> env) {
    push_err_ret_eval_err_obj("TODO" << std::source_location::current().line()); 
    /* Shouldn't pass around table objects, use table info instead */

    // Must be comparison operator
    // if (!is_comparison_operator(op)) {
    //     push_err_ret_eval_err_obj("Condition with table index on one side and a single value on the other must be a comparison"); }

    // UP<object> raw_tab = std::move(col_index_obj->table_name);
    // if (raw_tab->type() != TABLE_OBJ) {
    //     push_err_ret_eval_err_obj("eval_infix_column_vs_value(): Column index object contained non-table as table alias, got (" + object_type_to_astring(raw_tab->type()) + ")"); }
    // SP<table_object> table = CAST_UP(table_object, raw_tab);
    
    // UP<object> index_obj = std::move(col_index_obj->column_name);
    // if (index_obj->type() != INDEX_OBJ) {
    //     push_err_ret_eval_err_obj("eval_infix_column_vs_value(): Column index object contained non-index as column alias, got (" + object_type_to_astring(index_obj->type()) + ")"); }
    // size_t index = CAST_UP(index_object, index_obj)->value;

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
    //         if (CAST_UP(boolean_object, obj)->data() == "TRUE") {
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
    //         if (CAST_UP(boolean_object, obj)->data() == "TRUE") {
    //             truth = true; }

    //         if (truth != true) {
    //             return UP<evaluated>(new boolean_object(false)); }
    //     }
    // }

    // return UP<evaluated>(new boolean_object(true));
}



UP<evaluated> eval_expression(object* expression, SP<environment> env,  const std::source_location& loc) {

    auto expr = UP<object>(expression);

    if (!is_object(expr)) {
        FATAL_ERROR_THROW("eval_expression() called with non-object expression", loc); }

    UP<evaluated> result = eval_expression_impl(std::move(expr), env);
    if (!is_evaluated(result)) {
        push_err_ret_eval_err_obj("eval_expression(): Failed to return evaluated object"); }
        
    return CAST_UP(evaluated, result);
}

UP<evaluated> eval_expression(UP<object> expression, SP<environment> env,  const std::source_location& loc) {

    if (!is_object(expression)) {
        if (expression == nullptr) {
            FATAL_ERROR_THROW("eval_expression() called with nullptr expression", loc); }
        FATAL_ERROR_THROW("eval_expression() called with non-object expression", loc); }

    UP<evaluated> result = eval_expression_impl(std::move(expression), env);
    if (!is_evaluated(result)) {
        push_err_ret_eval_err_obj("eval_expression(): Failed to return evaluated object"); }

    return CAST_UP(evaluated, result);
}

static UP<evaluated> eval_expression_impl(UP<object> expression, SP<environment> env) {

    if (!is_object(expression)) {
        FATAL_ERROR_THROW("eval_expression() called with non-object expression", CUR_LOC); }

    switch (expression->type()) {

    // Basic stuff begin
    case DEFAULT_VALUE_FUNC_OBJ: {
        auto obj = CAST_UP(default_value_func, expression);

        auto parameter = eval_expression(std::move(obj->parameter), env);
        if (parameter->type() == ERROR_OBJ) {
            push_err_ret_eval_err_obj("Failed to evaluate default value function parameter"); }
        
        return UP<evaluated>(new e_default_value_func(std::move(parameter)));

    } break;
    case DEFAULT_VALUE_OBJ: {
        auto obj = CAST_UP(default_value_object, expression);
        auto value = eval_expression(std::move(obj->value), env);
        if (value->type() == ERROR_OBJ) {
            push_err_ret_eval_err_obj("Failed to evaluate default value"); }
        return UP<evaluated>(new e_default_value_object(std::move(value)));
    } break;
    case STAR_OBJ:
        return CAST_UP(evaluated, expression); break;
    case INTEGER_OBJ:
        return CAST_UP(evaluated, expression); break;
    case STRING_OBJ:
        return CAST_UP(evaluated, expression); break;
    case PARAMETER_OBJ: {
        UP<parameter_object> param_obj = CAST_UP(parameter_object, expression);
        avec<UP<evaluated>> e_values;
        e_values.reserve(param_obj->values->elements.size());
        for (auto& value : param_obj->values->elements) {
            UP<evaluated> evaled = eval_expression(CAST_UP(object, std::move(value)), env);
            if (evaled->type() == ERROR_OBJ) {
                return evaled; }
            e_values.push_back(std::move(evaled));
        }
        return UP<evaluated>(new e_parameter_object(param_obj->name, MAKE_UP(e_group_object, std::move(e_values)))); 
    } break;
    case RETURN_STATEMENT:
        return CAST_UP(evaluated, expression); break;
    case VARIABLE_OBJ:
        return eval_expression(std::move(CAST_UP(variable_object, (expression))->value), env); break;
    case SQL_DATA_TYPE_OBJ: {
        UP<SQL_data_type_object> cur = CAST_UP(SQL_data_type_object, expression);
        if (!cur->parameter.has_value()) {
            return UP<evaluated>(new e_SQL_data_type_object(cur->prefix, cur->data_type)); }

        UP<evaluated> param = eval_expression(std::move(cur->parameter.value()), env);
        if (param->type() == ERROR_OBJ) {
            return param; }

        if (param->type() != INTEGER_OBJ && param->type() != DECIMAL_OBJ && param->type() != NULL_OBJ) {
            push_err_ret_eval_err_obj("For now parameters of SQL data type must evaluate to integer/decimal/none, can be strings later when working on SET or ENUM"); }

        return UP<evaluated>(new e_SQL_data_type_object(cur->prefix, cur->data_type, std::move(param)));

    } break;
    case NULL_OBJ:
        return CAST_UP(evaluated, expression); break;
    case PREFIX_EXPRESSION_OBJ: {
        auto prefix = CAST_UP(prefix_expression_object, expression);
        auto op     = std::move(prefix->op);
        auto right  = std::move(prefix->right);
        return eval_prefix_expression(std::move(op), std::move(right), env); break;
    } break;
    // Basic stuff end

    // Table column expr stuff
    case TABLE_EXPR_OBJ: {
        auto tab_expr = CAST_UP(table_expr, expression);
        auto e_parameter = eval_expression(std::move(tab_expr->parameter), env);
        if (e_parameter->type() == ERROR_OBJ) {
            push_err_ret_eval_err_obj("Failed to evaluate table expression parameter"); }
        return UP<evaluated>(new e_table_expr(std::move(e_parameter)));
    } break;
    case TABLE_COLUMN_EXPR_OBJ: {
        auto col_expr = CAST_UP(table_column_expr, expression);
        auto e_parameter = eval_expression(std::move(col_expr->parameter), env);
        if (e_parameter->type() == ERROR_OBJ) {
            push_err_ret_eval_err_obj("Failed to evaluate table column expression parameter"); }
        return UP<evaluated>(new e_table_column_expr(std::move(e_parameter)));
    } break;
    case CURRENT_TIMESTAMP_OBJ: 
        return CAST_UP(evaluated, expression);
    case AUTO_INCREMENT_OBJ: 
        return CAST_UP(evaluated, expression);
    case DELIMITER_OBJ: 
        return CAST_UP(evaluated, expression);
    case PRIMARY_KEY_OBJ: 
        return CAST_UP(evaluated, expression);
    case FOREIGN_KEY_OBJ: {
        auto for_key_obj = CAST_UP(foreign_key_object, expression);
        auto reference = eval_expression(CAST_UP(object, for_key_obj->reference), env);
        if (reference->type() == ERROR_OBJ) {
            push_err_ret_eval_err_obj("FOREIGN_KEY: Failed to parse reference"); }
        if (reference->type() != E_COLUMN_INDEX_OBJ) {
            push_err_ret_eval_err_obj("FOREIGN_KEY: Reference failed to evaluate to E Column Index Object"); }

        auto key_obj = MAKE_UP(e_foreign_key_object, CAST_UP(e_column_index_object, reference));
        return CAST_UP(evaluated, key_obj);
    } break;
    case HASH_OBJ: {
        auto hash_obj = CAST_UP(hash_object, expression);
        auto ref = eval_expression(CAST_UP(object, hash_obj->reference), env);
        if (ref->type() == ERROR_OBJ) {
            push_err_ret_eval_err_obj("Failed to evaluate hash reference"); } 
        if (ref->type() != E_GROUP_OBJ) {
            push_err_ret_eval_err_obj("Hash reference failed to evaluate to E Group Object"); } 
        
        auto group = CAST_UP(e_group_object, ref);
        if (group->elements.size() == 0) {
            push_err_ret_eval_err_obj("HASH: Must contain at least one element"); }

        return UP<evaluated>(new e_hash_object(std::move(group)));
    } break;
    case CONSTRAINT_OBJ: {
        auto constraint_obj = CAST_UP(constraint_object, expression);
        auto constraint = eval_expression(std::move(constraint_obj->constraint), env);
        if (constraint->type() == ERROR_OBJ) {
            push_err_ret_eval_err_obj("Failed to evaluate constraint"); } 
        auto method = constraint_obj->method;
        return UP<evaluated>(new e_constraint_object(std::move(constraint), method));
    } break;
    case UNIQUE_OBJ: {
        auto unique_obj = CAST_UP(unique_object, expression);
        auto group = eval_expression(CAST_UP(object, unique_obj->group), env);
        if (group->type() == ERROR_OBJ) {
            push_err_ret_eval_err_obj("Failed to evaluate UNIQUE's elements"); } 
        if (group->type() != E_GROUP_OBJ) {
            push_err_ret_eval_err_obj("UNIQUE elements failed to evaluate to E Group Object"); } 
        return UP<evaluated>(new e_unique_object(CAST_UP(e_group_object, group)));
    }
    // End table column expr stuff

    case SELECT_OBJ: {
        auto result = eval_select(CAST_UP(select_object, expression), env);
        if (result.has_value()) {
            return CAST_UP(evaluated, std::move(*result));
        } else {
            return CAST_UP(evaluated, std::move(result).error());
        }
    }

    case COLUMN_INDEX_OBJ: {
        UP<column_index_object> obj = CAST_UP(column_index_object, expression);

        UP<evaluated> col_name_obj = eval_expression(std::move(obj->column_name), env);
        if (col_name_obj->type() == ERROR_OBJ) {
            push_err_ret_eval_err_obj("Failed to evaluated column index column name"); }

        return UP<evaluated>(new e_column_index_object(obj->table_name, std::move(col_name_obj)));

    } break;

    case SELECT_FROM_OBJ: {
        UP<select_from> wrapper = MAKE_UP(select_from, std::move(expression));
        auto result = eval_select_from(std::move(wrapper), env);
        if (result.has_value()) {
            return CAST_UP(evaluated, std::move(*result));
        } else {
            return CAST_UP(evaluated, std::move(result).error());
        }
    } break;



    case FUNCTION_CALL_OBJ:
        return eval_run_function(CAST_UP(function_call_object, expression), env); break;

    case GROUP_OBJ: {
        UP<group_object> group = CAST_UP(group_object, expression);
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
        UP<block_statement> block = CAST_UP(block_statement, expression);
        UP<e_return_statement> ret_val;
        avec<UP<evaluated>> statements;
        statements.reserve(block->body.size());
        bool has_ret = false;
        for (auto& statement: block->body) {
            UP<evaluated> res = eval_expression(std::move(statement), env);
            if (res->type() == E_RETURN_STATEMENT) {
                if (has_ret) {
                    push_err_ret_eval_err_obj("Block contained multiple (outer) return statements"); }
                ret_val = CAST_UP(e_return_statement, res);
                has_ret = true;
            } else {
                statements.emplace_back(std::move(res)); }
        }
        if (!has_ret) {
            ret_val = MAKE_UP(e_return_statement, UP<evaluated>(new null_object())); }
        return UP<evaluated>(new expression_statement(std::move(statements), std::move(ret_val)));
    } break;

    case INFIX_EXPRESSION_OBJ: {
        UP<infix_expr_object> condition = CAST_UP(infix_expr_object, expression);

        auto evaluate_operand = [&](UP<object> operand) -> UP<evaluated> {
            if (operand->type() == STRING_OBJ) {
                auto result = env->get_variable(operand->data());
                if (result.has_value()) {
                    return eval_expression(CAST_UP(object, std::move(*result)), env);
                } else {
                    return CAST_UP(evaluated, operand);
                }
            } else {
                return eval_expression(std::move(operand), env);
            }
        };

        auto e_left = evaluate_operand(std::move(condition->left));
        if (e_left->type() == ERROR_OBJ) {
            return e_left; }

        auto e_right = evaluate_operand(std::move(condition->right));
        if (e_right->type() == ERROR_OBJ) {
            return e_right; }

        return UP<evaluated>(new e_infix_expr_object(std::move(condition->op), std::move(e_left), std::move(e_right)));
    } break;

    case IF_STATEMENT: {
        UP<if_statement> statement = CAST_UP(if_statement, expression);

        UP<evaluated> obj = eval_expression(std::move(statement->condition), env); // LOWEST or PREFIX??
        if (obj->type() == ERROR_OBJ) {
            return obj; }

        if (obj->type() != BOOLEAN_OBJ) {
            push_err_ret_eval_err_obj("If statement condition returned non-boolean"); }

        UP<boolean_object> condition_result = CAST_UP(boolean_object, obj);

        if (condition_result->data() == "TRUE") { // scuffed
            UP<evaluated> result = eval_expression(CAST_UP(object, statement->body), env);
            return result;
        } else if (statement->other->type() != NULL_OBJ) {
            UP<evaluated> result = eval_expression(std::move(statement->other), env);
            return result;
        }

        return UP<evaluated>(new null_object());

    } break;
    default:
        push_err_ret_eval_err_obj("Cannot evaluate expression. Type (" << object_type_to_astring(expression->type()) << "), value(" << expression->inspect() << ")"); 
    }
}


static avec<UP<e_argument_object>> name_arguments(SP<e_function_object> function, UP<function_call_object> func_call, SP<environment> env) {

    avec<UP<e_argument_object>> named_arguments;

    UP<evaluated> eval_args = eval_expression(CAST_UP(object, func_call->arguments), env);
    if (eval_args->type() != E_GROUP_OBJ) {
        log_sql_error("Failed to evaluate arguments"); return named_arguments; }

    avec<UP<evaluated>> evaluated_arguments = std::move(CAST_UP(e_group_object, eval_args)->elements);

    

    avec<UP<parameter_object>> parameters;
    parameters.reserve(function->parameters.size());
    for (const auto& param: parameters) {
        parameters.push_back(UP<parameter_object>(param->clone()));
    }

    if (evaluated_arguments.size() != parameters.size()) {
        log_sql_error("Function called with incorrect number of arguments, got " + std::to_string(evaluated_arguments.size()) + " wanted " + std::to_string(parameters.size()));
        return named_arguments;
    }

    for (size_t i = 0; i < parameters.size(); i++) {
        astring param_name = parameters[i]->name;
        named_arguments.emplace_back(MAKE_UP(e_argument_object, param_name, std::move(evaluated_arguments[i])));
    }

    return named_arguments;
}


// TODO Needs to be reworked for execute.cpp
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
        return UP<evaluated>(new s_function_call_object("COUNT", MAKE_UP(e_group_object, std::move(e_args))));
    }



    auto&& [function, exists] = env->get_function(func_call->name);
    if (!exists) {
        push_err_ret_eval_err_obj("Function does not exist (" + func_call->name + ")"); }

    size_t error_count = sql_errors.msg_count();
    avec<UP<e_argument_object>> named_args = name_arguments(function, std::move(func_call), env);
    if (error_count < sql_errors.msg_count()) {
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
        //     return eval_expression(std::move(CAST_UP(return_statement, res)->expression), env);
        // }
        
    }

    push_err_ret_eval_err_obj("Failed to find return value");
}


static std::expected<UP<e_alter_table_node>, UP<error_object>> eval_alter_table(UP<alter_table_node> info, SP<environment> env) {

    UP<evaluated> table_name = eval_expression(std::move(info->table_name), env);
    if (table_name->type() == ERROR_OBJ) {
        push_err_ret_unx_err_obj("ALTER TABLE: Failed to evaluate table name (" + info->table_name->inspect() + ")"); }
   
    if (table_name->type() != STRING_OBJ) {
        push_err_ret_unx_err_obj("ALTER TABLE: Table name (" + info->table_name->inspect() + ") failed to evaluate to string"); }

    UP<evaluated> table_edit = eval_expression(std::move(info->table_edit), env);
    if (table_edit->type() == ERROR_OBJ) {
        push_err_ret_unx_err_obj("ALTER TABLE: Failed to evaluate table edit"); }

    return MAKE_UP(e_alter_table_node, table_name->data(), std::move(table_edit));
}


// FIXME
static void eval_create_table(UP<create_table> info, SP<environment> env) {

    astring table_name = info->table_name;

    avec<UP<object>>    details = std::move(info->details->elements);
    avec<UP<evaluated>> e_details; // TODO Maybe should reserve?
    for (auto& detail : details) {

        UP<evaluated> value = eval_expression(UP<object>(detail->clone()), env);
        if (value->type() == ERROR_OBJ) {
            eval_push_err_ret("CREATE TABLE: Failed to evaluate data type (" + detail->inspect() + ")"); }

        e_details.push_back(std::move(value));
    }

    // Verify types
    if (DEBUG) [[unlikely]] { std::cout << "Eval Create Table, types:\n"; }
    for (const auto& obj : e_details) {
        if (DEBUG) [[unlikely]] { std::cout << "\t" << object_type_to_astring(obj->type()) << std::endl; }
        switch (obj->type()) {
            case E_PARAMETER_OBJ: case E_TABLE_COLUMN_EXPR_OBJ: case E_TABLE_EXPR_OBJ: break;
            default: eval_push_err_ret("CREATE TABLE: Invalid object in table details. Type:" << object_type_to_astring(obj->type()) << ", (" << obj->inspect() << ")"); break;
        }
    }

    if (DEBUG) [[unlikely]] { 
        std::cout << "\nEval Create Table, inspected:\n";
        for (const auto& obj : e_details) {
            std::cout << "\t" << obj->inspect() << std::endl;
        }
    }

    // FATAL_ERROR_THROW("debuggin it rn fr fr", CUR_LOC);

    e_nodes.push_back(UP<e_node>(new e_create_table(table_name, MAKE_UP(e_group_object, std::move(e_details)))));
}



static std::expected<UP<e_select_from_object>, UP<error_object>> eval_select_from(UP<select_from> wrapper, SP<environment> env) {

    if (wrapper->value->type() != SELECT_FROM_OBJ) {
        push_err_ret_unx_err_obj("eval_select_from(): Called with invalid object (" + object_type_to_astring(wrapper->value->type()) + ")"); }

    UP<select_from_object> info = CAST_UP(select_from_object, wrapper->value);    

    avec<UP<evaluated>> e_clause_chain;
    e_clause_chain.reserve(info->clause_chain.size());
    if (info->clause_chain.size() != 0) {
        for (const auto& clause : info->clause_chain) {

            UP<evaluated> e_clause = eval_expression(UP<object>(clause->clone()), env);
            if (e_clause->type() == ERROR_OBJ) {
                push_err_ret_unx_err_obj("SELECT FROM: Could not evalute (" + clause->inspect() + ")"); }

            switch (e_clause->type()) {
                case STRING_OBJ: case E_INFIX_EXPRESSION_OBJ: case E_PREFIX_EXPRESSION_OBJ: {
                } break;
                default: {
                    push_err_ret_unx_err_obj("Unsupported clause type, type: (" << object_type_to_astring(e_clause->type()) << "), expression: " << "(" << clause->inspect() << ")"); 
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
        push_err_ret_unx_err_obj("SELECT FROM: No column indexes"); }


    if (column_indexes[0]->type() == STAR_OBJ) {

        if (column_indexes.size() > 1) {
            push_err_ret_unx_err_obj("SELECT FROM: (*) must be alone"); }

        e_column_indexes.push_back(CAST_UP(evaluated, column_indexes[0]));

        return MAKE_UP(e_select_from_object, std::move(e_column_indexes), std::move(e_clause_chain));
    }

    for (const auto& col_index_raw : column_indexes) {

        UP<evaluated> selecter = eval_expression(UP<object>(col_index_raw->clone()), env);
        if (selecter->type() == ERROR_OBJ) {
            push_err_ret_unx_err_obj("SELECT FROM: Could not evalute (" + col_index_raw->inspect() + ")"); }

        switch (selecter->type()) {
        case S_FUNCTION_CALL_OBJ: case E_COLUMN_INDEX_OBJ: case STRING_OBJ: {
        } break;
        default: 
            push_err_ret_unx_err_obj("SELECT FROM: Cannot use (" + col_index_raw->inspect() + ") to index"); }

        e_column_indexes.push_back(std::move(selecter));
    }

    return MAKE_UP(e_select_from_object, std::move(e_column_indexes), std::move(e_clause_chain));
}



static void eval_insert_into(UP<insert_into> wrapper, SP<environment> env) {

    if (wrapper->value->type() != INSERT_INTO_OBJ) {
        eval_push_err_ret("eval_insert_into(): Called with invalid object (" + object_type_to_astring(wrapper->value->type()) + ")"); }

    UP<insert_into_object> info = CAST_UP(insert_into_object, wrapper->value);



    astring table_name = info->table_name;

    UP<evaluated> values_obj = eval_expression(std::move(info->values), env);
    if (values_obj->type() != E_GROUP_OBJ) { // or != TABLE_OBJ???
        eval_push_err_ret("INSERT INTO eval: failed to evaluate values");}

    avec<UP<evaluated>> values = std::move(CAST_UP(e_group_object, values_obj)->elements);
        
    if (values.size() == 0) {
        eval_push_err_ret("INSERT INTO eval: no values");}

    if (info->fields.size() < values.size()) {
        eval_push_err_ret("INSERT INTO eval: more values than field names");}

    // evaluate fields
    avec<UP<evaluated>> field_names;
    field_names.reserve(info->fields.size());
    for (auto& field : info->fields) {
        UP<evaluated> evaluated_field = eval_expression(UP<object>(field->clone()), env);
        if (evaluated_field->type() == ERROR_OBJ) {
            eval_push_err_ret("eval_insert_into(): Failed to evaluate field (" + field->inspect() + ") while inserting rows"); }

        if (evaluated_field->type() != STRING_OBJ) { // For now only accepting string objects
            eval_push_err_ret("INSERT INTO eval: Field (" + field->inspect() + ") evaluated to non-string value"); }

        field_names.push_back(std::move(evaluated_field));
    }


    // evaluate values
    // avec<UP<evaluated>> e_values;
    // e_values.reserve(values.size());
    // for (auto& value : values) {
    //     UP<evaluated> e_value = eval_expression(UP<object>(value->clone()), env);

    //     if (e_value->type() == ERROR_OBJ) {
    //         eval_push_err_ret("eval_insert_into(): Failed to evaluate value (" + value->inspect() + ") while inserting rows"); }

    //     e_values.push_back(std::move(e_value));
    // }

    if (sql_errors.has_msgs()) {
        return;}

    e_nodes.emplace_back(UP<e_node>(new e_insert_into(MAKE_UP(e_insert_into_object, table_name, std::move(field_names), std::move(values)))));
}

