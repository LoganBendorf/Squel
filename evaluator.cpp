
#include "pch.h"

#include "evaluator.h"

#include "node.h"
#include "structs_and_macros.h"
#include "helpers.h"
#include "object.h"
#include "environment.h"



extern std::vector<std::string> errors;
extern display_table display_tab;

static std::vector<node*> nodes;

extern std::vector<std::string> warnings;

static hvec(table_object*, s_tables);
static std::vector<evaluated_function_object*> s_functions;

static void eval_function(function* func, environment* env);
static object* eval_run_function(function_call_object* func_call, environment* env);

static object* eval_assert(assert_node* node, environment* env);
static void eval_alter_table(alter_table* info, environment* env);
static void eval_create_table(const create_table* info, environment* env);
static object* eval_select (select_object* info, environment* env);
static object* eval_select_from(select_from* wrapper, environment* env);
static void print_table();
static void eval_insert_into(const insert_into* info, environment* env);

static object* eval_expression(object* expression, environment* env);
static object* eval_column(column_object* col, environment* env);

static object* eval_prefix_expression(operator_object* op, object* right, environment* env);
static object* eval_infix_expression(operator_object* op, object* left, object* right, environment* env);

static object* can_insert(object* insert_obj, SQL_data_type_object* data_type);
static std::pair<table_object*, bool> get_table(const std_and_astring_variant& name);

enum ret_code {
    SUCCESS, FAIL, ERROR
};
static std::pair<object*, ret_code> convert_table_to_value(table_object* tab);
static std::pair<object*, ret_code> convert_table_info_to_value(table_info_object* info);


#define eval_push_error_return(x)   \
    std::stringstream error;        \
    error << x;                     \
    errors.push_back(error.str());  \
    return                        

#define push_err_ret_err_obj(x)     \
    std::stringstream error;        \
    error << x;                     \
    errors.push_back(error.str());  \
    return new error_object();

#define push_err_break(x)            \
    std::stringstream error;        \
    error << x;                     \
    errors.push_back(error.str());  \
    break                           \

environment* eval_init(std::vector<node*> nds, std::vector<evaluated_function_object*> g_functions, avec<table_object*> g_tables) {
    nodes = nds;
    s_functions.clear();
    for(const auto& func : g_functions) {
        s_functions.push_back(func->clone(ARENA));
    }
    s_tables = g_tables;
    return new environment();
}

static void configure_print_functions(object* result) {

    if (result->type() != TABLE_INFO_OBJECT) {
        eval_push_error_return("Failed to evaluate SELECT FROM"); }

    table_info_object* tab_info = static_cast<table_info_object*>(result);

    if (tab_info->tab->type() != TABLE_OBJECT) {
        eval_push_error_return("SELECT FROM: Failed to evaluate table"); }



    display_tab.to_display = true;
    if (display_tab.table_info != nullptr) {
        delete display_tab.table_info;}
    display_tab.table_info = tab_info->clone(HEAP);

    print_table(); // CMD line print, QT will do it's own thing in main
}

std::pair<std::vector<evaluated_function_object*>, avec<table_object*>> eval(environment* env) {
    
    for (const auto& node : nodes) {

        switch(node->type()) {
            case INSERT_INTO_NODE:
                eval_insert_into(static_cast<insert_into*>(node), env);
                printf("EVAL INSERT INTO CALLED\n");
                break;
            case SELECT_NODE: {
                object* unwrapped = static_cast<select_node*>(node)->value;
                if (unwrapped->type() != SELECT_OBJECT) {
                    errors.push_back("Select node contained errors object"); break; }

                object* result = eval_select(static_cast<select_object*>(unwrapped), env);
                configure_print_functions(result);
                
                printf("EVAL SELECT CALLED\n");
            } break;
            case SELECT_FROM_NODE: {
                object* result = eval_select_from(static_cast<select_from*>(node), env);
                configure_print_functions(result);

                printf("EVAL SELECT FROM CALLED\n");
            } break;
            case CREATE_TABLE_NODE:
                eval_create_table(static_cast<create_table*>(node), env);
                printf("EVAL CREATE TABLE CALLED\n");
                break;
            case ALTER_TABLE_NODE:
                eval_alter_table(static_cast<alter_table*>(node), env);
                printf("EVAL ALTER TABLE CALLED\n");
                break;
            case FUNCTION_NODE:
                eval_function(static_cast<function*>(node), env);
                printf("EVAL FUNCTION CALLED\n");
                break;
            case ASSERT_NODE:
                eval_assert(static_cast<assert_node*>(node), env);
                printf("EVAL FUNCTION CALLED\n");
                break;
            default:
                errors.push_back("eval: unknown node type (" + std::string(node->inspect()) + ")");
        }
    }

    return std::pair(s_functions, s_tables);
}

static object* eval_assert(assert_node* node, environment* env) {

    assert_object* info = node->value;

    object* expr = eval_expression(info->expression, env);
    if (expr->type() == ERROR_OBJ) {
        push_err_ret_err_obj("Failed to evaluate ASSERT expression"); }

    if (expr->type() != BOOLEAN_OBJ) {
        push_err_ret_err_obj("ASSERT expression failed to evaluate to a boolean"); }

    boolean_object* boolean = static_cast<boolean_object*>(expr);

    if (boolean->data() == "FALSE") {
        push_err_ret_err_obj("ASSERT failed (Line: " << std::to_string(info->line) << ", Expression: " << info->inspect() + ")"); }

    return new null_object();
}

static object* assume_data_type(object* obj) {
    switch (obj->type()) {
    case STRING_OBJ:
        return new SQL_data_type_object(NONE, VARCHAR, new integer_object(255));
    case INTEGER_OBJ:
        return new SQL_data_type_object(NONE, INT, new integer_object(11));
    case TABLE_OBJECT: {
        const auto& [cell, rc] = convert_table_to_value(static_cast<table_object*>(obj));
        if (rc == SUCCESS) {
            return assume_data_type(cell);
        }
        push_err_ret_err_obj("Can't assume default data type for (" + obj->inspect() + ")")
    } break;
    case TABLE_INFO_OBJECT: {
        const auto& [cell, rc] = convert_table_info_to_value(static_cast<table_info_object*>(obj));
        if (rc == SUCCESS) {
            return assume_data_type(cell);
        }
        push_err_ret_err_obj("Can't assume default data type for (" + obj->inspect() + ")")
    } break;
    default:
        push_err_ret_err_obj("Can't assume default data type for (" + obj->inspect() + ")");
    }
}

static object* eval_select(select_object* info, environment* env) {

    if (info->type() != SELECT_OBJECT) {
        push_err_ret_err_obj("eval_select(): Called with invalid object (" + object_type_to_astring(info->type()) + ")"); }

    astring table_name = info->value->inspect();
   
    object* table_value = eval_expression(info->value, env);
    if (table_value->type() == ERROR_OBJ) {
        push_err_ret_err_obj("Failed to evaluate SELECT value (" + table_value->inspect() + ")"); }

    object* assumed_type = assume_data_type(table_value);
    if (table_value->type() == SQL_DATA_TYPE_OBJ) {
        push_err_ret_err_obj("Couldn't assume SELECT value data type (" + table_value->inspect() + ")"); }
    
    SQL_data_type_object* type = static_cast<SQL_data_type_object*>(assumed_type); // some nonsense here

    table_detail_object* detail = new table_detail_object(table_name, type, new null_object());

    table_object* tab = new table_object(table_name, {detail}, {new group_object({table_value})});

    table_info_object* tab_info = new table_info_object(tab, avec<size_t>{0}, avec<size_t>{0});

    return tab_info;
}

static void eval_function (function* func, environment* env) {

    object* eval_parameters = eval_expression(func->func->parameters, env);
    if (eval_parameters->type() != GROUP_OBJ) {
        eval_push_error_return("Failed to evaluate to function parameter"); }

    avec<parameter_object*> evaluated_parameters;
    avec<object*> params = static_cast<group_object*>(eval_parameters)->elements;
    for (const auto& param : params) {
        if (param->type() == ERROR_OBJ) {
            eval_push_error_return("Failed to evaluate to function parameter"); }

        if (param->type() != PARAMETER_OBJ) {
            eval_push_error_return("Function parameter failed to evaluate to parameter object"); }

        evaluated_parameters.push_back(static_cast<parameter_object*>(param));
    }

    evaluated_function_object* evaluated_func = new evaluated_function_object(func->func, evaluated_parameters);

    env->add_or_replace_function(evaluated_func);

    // For now just add all functions to global for fun
    bool found = false;
    for (size_t i = 0; i < s_functions.size(); i++) {
        if (s_functions[i]->name == evaluated_func->name) {
            s_functions[i] = evaluated_func;
            found = true;
            break;
        }
    }
    if (!found) {
        s_functions.push_back(evaluated_func);
    }

    std::cout << "!! PRINTING LE FUNCTION !!\n\n";

    std::cout << evaluated_func->inspect() << std::endl;
}

static object* eval_prefix_expression(operator_object* op, object* right, environment* env) {
    
    object* e_right = eval_expression(right, env);
    
    switch (op->op_type) {
    case NEGATE_OP:
        switch (e_right->type()) {
        case INTEGER_OBJ:
            return new integer_object( - astring_to_numeric<int>(e_right->data()));
        case DECIMAL_OBJ:
            return new decimal_object( - astring_to_numeric<double>(e_right->data()));
        default:
            push_err_ret_err_obj("No negation operation for type (" + e_right->inspect() + ")");
        }
    default:
        push_err_ret_err_obj("No prefix " + op->inspect() + " operator known");
    }
}


static bool is_values_wrapper(object* obj) {
    switch (obj->type()) {
    case COLUMN_VALUES_OBJ:
        return true;

    default: 
        return false;
    }
}

static bool is_comparison_operator(operator_object* op) {
    switch (op->op_type) {
    case EQUALS_OP: case NOT_EQUALS_OP: case LESS_THAN_OP: case GREATER_THAN_OP:
        return true;
    default:
        return false;
    }
}

static object* eval_infix_values_condition(operator_object* op, values_wrapper_object* left_wrapper, values_wrapper_object* right_wrapper, environment* env) {
    avec<object*> left = left_wrapper->values;
    avec<object*> right = right_wrapper->values;

    if (left.size() != right.size()) {
        push_err_ret_err_obj("Cannot compare values of different sizes"); }

    if (is_comparison_operator(op)) {
        for (size_t i = 0; i < left.size(); i++) {
            object* obj = eval_infix_expression(op, left[i], right[i], env);
            if (obj->type() == ERROR_OBJ) {
                push_err_ret_err_obj(obj->data()); }
            if (obj->type() != BOOLEAN_OBJ) {
                push_err_ret_err_obj("compare_values(): Camparison failed to evaluate to boolean"); }

            bool truth = false;
            if (static_cast<boolean_object*>(obj)->data() == "TRUE") {
                truth = true; }

            if (truth != true) {
                return new boolean_object(false); }
        }
        return new boolean_object(true);
    } else {
        avec<object*> new_vec;
        new_vec.reserve(left.size());
        for (size_t i = 0; i < left.size(); i++) {
            object* obj = eval_infix_expression(op, left[i], right[i], env);
            if (obj->type() == ERROR_OBJ) {
                push_err_ret_err_obj(obj->data()); }

            new_vec.push_back(obj);
        }
        return new group_object(new_vec);
    }
}

static object* eval_infix_column_vs_value(operator_object* op, column_index_object* col_index_obj, object* other, bool left_first, environment* env) {

    // Must be comparison operator
    if (!is_comparison_operator(op)) {
        push_err_ret_err_obj("Condition with table index on one side and a single value on the other must be a comparison"); }

    object* raw_tab = col_index_obj->table_name;
    if (raw_tab->type() != TABLE_OBJECT) {
        push_err_ret_err_obj("eval_infix_column_vs_value(): Column index object contained non-table as table alias, got (" + object_type_to_astring(raw_tab->type()) + ")"); }
    table_object* table = static_cast<table_object*>(raw_tab);
    
    object* index_obj = col_index_obj->column_name;
    if (index_obj->type() != INDEX_OBJ) {
        push_err_ret_err_obj("eval_infix_column_vs_value(): Column index object contained non-index as column alias, got (" + object_type_to_astring(index_obj->type()) + ")"); }
    size_t index = static_cast<index_object*>(index_obj)->value;

    if (index >= table->rows.size()) {
        push_err_ret_err_obj("eval_infix_column_vs_value(): Index out-of-bounds"); }



    if (left_first) {
        for (size_t i = 0; i < table->rows.size(); i++) {

            const auto& [cell, ok] = table->get_cell_value(i, index);
            if (!ok) {
                push_err_ret_err_obj("eval_infix_column_vs_value(): " + cell->data()); }

            object* obj = eval_infix_expression(op, cell, other, env);
            if (obj->type() == ERROR_OBJ) {
                push_err_ret_err_obj(obj->data()); }
            if (obj->type() != BOOLEAN_OBJ) {
                push_err_ret_err_obj("eval_infix_column_vs_value(): Camparison failed to evaluate to boolean"); }

            bool truth = false;
            if (static_cast<boolean_object*>(obj)->data() == "TRUE") {
                truth = true; }

            if (truth != true) {
                return new boolean_object(false); }

        }
    } else {
        for (size_t i = 0; i < table->rows.size(); i++) {

            const auto& [cell, ok] = table->get_cell_value(i, index);
            if (!ok) {
                push_err_ret_err_obj("eval_infix_column_vs_value(): " + cell->data()); }

            object* obj = eval_infix_expression(op, other, cell, env);
            if (obj->type() == ERROR_OBJ) {
                push_err_ret_err_obj(obj->data()); }
            if (obj->type() != BOOLEAN_OBJ) {
                push_err_ret_err_obj("eval_infix_values_condition(): Camparison fail"); }

            bool truth = false;
            if (static_cast<boolean_object*>(obj)->data() == "TRUE") {
                truth = true; }

            if (truth != true) {
                return new boolean_object(false); }
        }
    }

    return new boolean_object(true);
}

static object* eval_infix_values_vs_value(operator_object* op, values_wrapper_object* values, object* other, bool left_first, environment* env) {

    // Must be comparison operator
    if (!is_comparison_operator(op)) {
        push_err_ret_err_obj("Condition with multiple values on one side and a single value on the other must be a comparison"); }


    if (left_first) {
        for (const auto& val : values->values) {
            object* obj = eval_infix_expression(op, val, other, env);
            if (obj->type() == ERROR_OBJ) {
                push_err_ret_err_obj(obj->data()); }
            if (obj->type() != BOOLEAN_OBJ) {
                push_err_ret_err_obj("eval_infix_values_condition(): Camparison failed to evaluate to boolean"); }

            bool truth = false;
            if (static_cast<boolean_object*>(obj)->data() == "TRUE") {
                truth = true; }

            if (truth != true) {
                return new boolean_object(false); }

        }
    } else {
        for (const auto& val : values->values) {
            object* obj = eval_infix_expression(op, other, val, env);
            if (obj->type() == ERROR_OBJ) {
                push_err_ret_err_obj(obj->data()); }
            if (obj->type() != BOOLEAN_OBJ) {
                push_err_ret_err_obj("eval_infix_values_condition(): Camparison fail"); }

            bool truth = false;
            if (static_cast<boolean_object*>(obj)->data() == "TRUE") {
                truth = true; }

            if (truth != true) {
                return new boolean_object(false); }
        }
    }

    return new boolean_object(true);
}

static std::pair<object*, ret_code> convert_table_info_to_value(table_info_object* info) {
    if (info->col_ids.size() == 1 && info->row_ids.size() == 1) {
        const auto& [cell, ok] = info->tab->get_cell_value(info->col_ids[0], info->row_ids[0]); 
        if (!ok) {
            errors.push_back("convert_table_info_to_value(): weird index bug"); 
            return {cell, ERROR};
        }
        return {cell, SUCCESS};
    }
    return {new null_object(), FAIL};
}

static std::pair<object*, ret_code> convert_table_to_value(table_object* tab) {
    if (tab->column_datas.size() == 1 && tab->rows.size() == 1) {
        const auto& [cell, ok] = tab->get_cell_value(0, 0); 
        if (!ok) {
            errors.push_back("eval_infix_expression(): Weird index bug"); 
            return {cell, ERROR};
        }
        return {cell, SUCCESS};
    }
    return {new null_object(), FAIL};
}

static object* eval_infix_expression(operator_object* op, object* left, object* right, environment* env) {

    object* e_left = eval_expression(left, env);
    object* e_right = eval_expression(right, env);

    if (e_left->type() == ERROR_OBJ) {
        push_err_ret_err_obj(e_left->data()); }
    if (e_right->type() == ERROR_OBJ) {
        push_err_ret_err_obj(e_right->data()); }

    if (is_values_wrapper(e_left) && is_values_wrapper(right)) {
        return eval_infix_values_condition(op, static_cast<values_wrapper_object*>(e_left), static_cast<values_wrapper_object*>(e_right), env);
    } else if (is_values_wrapper(e_left)) {
        return eval_infix_values_vs_value(op, static_cast<values_wrapper_object*>(e_left), e_right, true, env); 
    } else if (is_values_wrapper(e_right)) {
        return eval_infix_values_vs_value(op, static_cast<values_wrapper_object*>(e_right), e_left, false, env); 
    } 

    if (e_left->type() == COLUMN_INDEX_OBJECT) {
        return eval_infix_column_vs_value(op, static_cast<column_index_object*>(e_left), right, true, env);
    } else if (e_right->type() == COLUMN_INDEX_OBJECT) {
        return eval_infix_column_vs_value(op, static_cast<column_index_object*>(e_right), left, false, env);
    }

    // Functions not dependent on left or right, so can iterate
    std::array<object*, 2> pair = {e_left, e_right};
    for (auto& side : pair) {
        switch (side->type()) {

        case TABLE_OBJECT: { // unused
        table_object* tab = static_cast<table_object*>(side);
        const auto& [cell, rc] = convert_table_to_value(tab);
        if (rc == SUCCESS) {
            side = cell; 
        } else if (rc == ERROR) {
            return cell; }
        } break;

        case TABLE_INFO_OBJECT: {
        table_info_object* info = static_cast<table_info_object*>(side);
        const auto& [cell, rc] = convert_table_info_to_value(info);
        if (rc == SUCCESS) {
            side = cell; 
        } else if (rc == ERROR) {
            return cell; }
        } break;
        default:
            continue;
        }
    }


    
    switch (op->op_type) {
        case ADD_OP:
            if (is_numeric_object(e_left) && is_numeric_object(e_right)) {
                return new integer_object(static_cast<integer_object*>(e_left)->value + static_cast<integer_object*>(e_right)->value);
            } 
            else if (is_string_object(e_left) && is_string_object(e_right)) {
                return new string_object(e_left->data() + e_right->data());
            } 
            else {
                push_err_ret_err_obj("No infix " << op->inspect() << " operation for " + e_left->inspect() << " and " << e_right->inspect());
            }
            
        case SUB_OP:
            if (!is_numeric_object(e_left) || !is_numeric_object(e_right)) {
                push_err_ret_err_obj("No infix " << op->inspect() << " operation for " + e_left->inspect() << " and " << e_right->inspect());}
            return new integer_object(static_cast<integer_object*>(e_left)->value - static_cast<integer_object*>(e_right)->value);
            break;
        case MUL_OP:
            if (!is_numeric_object(e_left) || !is_numeric_object(e_right)) {
                push_err_ret_err_obj("No infix " << op->inspect() << " operation for " + e_left->inspect() << " and " << e_right->inspect());}
            return new integer_object(static_cast<integer_object*>(e_left)->value * static_cast<integer_object*>(e_right)->value);
            break;
        case DIV_OP:
            if (!is_numeric_object(e_left) || !is_numeric_object(e_right)) {
                push_err_ret_err_obj("No infix " << op->inspect() << " operation for " + e_left->inspect() << " and " << e_right->inspect());}
            return new integer_object(static_cast<integer_object*>(e_left)->value / static_cast<integer_object*>(e_right)->value);
            break;
        case DOT_OP:
            if (e_left->type() != INTEGER_OBJ || e_left->type() != INTEGER_OBJ) {
                push_err_ret_err_obj("No infix " << op->inspect() << " operation for " + e_left->inspect() << " and " << e_right->inspect());}
            return new decimal_object(e_left->data() + "." + e_right->data());
            break;
        case EQUALS_OP:
            if (e_left->type() == STRING_OBJ && e_right->type() == STRING_OBJ) {
                return new boolean_object(e_left->data() == e_right->data()); 
            } else if (e_left->type() == INTEGER_OBJ && e_right->type() == INTEGER_OBJ) {
                return new boolean_object(static_cast<integer_object*>(e_left)->value == static_cast<integer_object*>(e_right)->value); 
            } else {
                push_err_ret_err_obj("No infix " << op->inspect() << " operation for " + e_left->inspect() << " and " << e_right->inspect());
            }
            break;
        case NOT_EQUALS_OP:
            if (e_left->type() == STRING_OBJ && e_right->type() == STRING_OBJ) {
                return new boolean_object(e_left->data() != e_right->data()); 
            } else if (e_left->type() == INTEGER_OBJ && e_right->type() == INTEGER_OBJ) {
                return new boolean_object(static_cast<integer_object*>(e_left)->value != static_cast<integer_object*>(e_right)->value); 
            } else {
                push_err_ret_err_obj("No infix " << op->inspect() << " operation for " + e_left->inspect() << " and " << e_right->inspect());
            }
            break;
        case LESS_THAN_OP:
            if (e_left->type() != INTEGER_OBJ || e_left->type() != INTEGER_OBJ) {
                push_err_ret_err_obj("No infix " << op->inspect() << " operation for " + e_left->inspect() << " and " << e_right->inspect());}
            return new boolean_object(static_cast<integer_object*>(e_left)->value < static_cast<integer_object*>(e_right)->value);
            break;
        case GREATER_THAN_OP:
            if (e_left->type() != INTEGER_OBJ || e_left->type() != INTEGER_OBJ) {
                push_err_ret_err_obj("No infix " << op->inspect() << " operation for " + e_left->inspect() << " and " << e_right->inspect());}
            return new boolean_object(static_cast<integer_object*>(e_left)->value > static_cast<integer_object*>(e_right)->value);
            break;
        default:
            push_err_ret_err_obj("No infix " + op->inspect() + " operator known");
    }
}

// can turn into while loop instead of recursively calling itself, for perf
static object* eval_expression(object* expression, environment* env) {

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
        return static_cast<variable_object*>(expression)->value; break;
    case SQL_DATA_TYPE_OBJ: {
        SQL_data_type_object* cur = static_cast<SQL_data_type_object*>(expression);
        object* param = eval_expression(cur->parameter, env);
        if (param->type() == ERROR_OBJ) {
            return param; }

        if (param->type() != INTEGER_OBJ && param->type() != DECIMAL_OBJ && param->type() != NULL_OBJ) {
            push_err_ret_err_obj("For now parameters of SQL data type must evaluate to integer/decimal/none, can be strings later when working on SET or ENUM"); }

        cur = new SQL_data_type_object(cur->prefix, cur->data_type, param);

        return cur;
    } break;
    case NULL_OBJ:
        return expression; break;
    case PREFIX_EXPRESSION_OBJ:
        return eval_prefix_expression(static_cast<prefix_expression_object*>(expression)->op, static_cast<prefix_expression_object*>(expression)->right, env); break;
    // Basic stuff end

    case SELECT_OBJECT: {
        return eval_select(static_cast<select_object*>(expression), env);
    }

    case COLUMN_INDEX_OBJECT: {
        
        column_index_object* obj = static_cast<column_index_object*>(expression);

        object* tab_expr = eval_expression(obj->table_name, env);
        if (tab_expr->type() != STRING_OBJ) {
            push_err_ret_err_obj("Column index object: Table name failed to evaluate to string"); }
        astring table_name = tab_expr->data();

        object* col_expr = eval_expression(obj->column_name, env);
        if (col_expr->type() != STRING_OBJ) {
            push_err_ret_err_obj("Column index object: Column name failed to evaluate to string"); }
        astring column_name = col_expr->data();

        const auto& [table, tab_exists] = get_table(table_name);
        if (!tab_exists) {
            push_err_ret_err_obj("Column index object: Table does not exist"); }

        const auto& [col_index, col_exists]  = table->get_column_index(column_name);
        if (!col_exists) {
            push_err_ret_err_obj("Column index object: Column does not exist"); }

        return new column_index_object(table, new index_object(col_index));

    } break;

    case SELECT_FROM_OBJECT: {
        select_from* wrapper = new select_from(static_cast<select_from_object*>(expression));
        return eval_select_from(wrapper, env);
    } break;

    case COLUMN_OBJ:
        return eval_column(static_cast<column_object*>(expression), env);

    case FUNCTION_CALL_OBJ:
        return eval_run_function(static_cast<function_call_object*>(expression), env); break;

    case GROUP_OBJ: {
        group_object* group = static_cast<group_object*>(expression);
        avec<object*> objects;
        for (const auto& obj: group->elements) {
            object* evaluated = eval_expression(obj, env);
            if (evaluated->type() == ERROR_OBJ) {
                return evaluated; }
            objects.push_back(evaluated);
        }
        return new group_object(objects);
    } break;
    case BLOCK_STATEMENT: {
        block_statement* block = static_cast<block_statement*>(expression);
        object* ret_val = new null_object();
        for (const auto& statement: block->body) {
            ret_val = eval_expression(statement, env);
        }
        return ret_val;
    } break;

    case INFIX_EXPRESSION_OBJ: {
        infix_expression_object* condition = static_cast<infix_expression_object*>(expression);

        object* left = condition->left;

        if (left->type() == STRING_OBJ) {
            object* var = env->get_variable(left->data());
            if (var->type() != ERROR_OBJ) {
                left = eval_expression(var, env);
            }
        }

        object* right = condition->right;

        if (right->type() == STRING_OBJ) {
            object* var = env->get_variable(right->data());
            if (var->type() != ERROR_OBJ) {
                right = eval_expression(var, env); 
            }
        }

        object* result = eval_infix_expression(condition->op, left, right, env);
        if (result->type() == ERROR_OBJ) {
            return result; }

        return result;
    } break;

    case IF_STATEMENT: {
        if_statement* statement = static_cast<if_statement*>(expression);

        object* obj = eval_expression(statement->condition, env); // LOWEST or PREFIX??
        if (obj->type() == ERROR_OBJ) {
            return obj; }

        if (obj->type() != BOOLEAN_OBJ) {
            push_err_ret_err_obj("If statement condition returned non-boolean"); }

        boolean_object* condition_result = static_cast<boolean_object*>(obj);

        if (condition_result->data() == "TRUE") { // scuffed
            object* result = eval_expression(statement->body, env);
            return result;
        } else if (statement->other->type() != NULL_OBJ) {
            object* result = eval_expression(statement->other, env);
            return result;
        }

        return new null_object();


    } break;
    default:
        push_err_ret_err_obj("eval_expression(): Cannot evaluate expression. Type (" + object_type_to_astring(expression->type()) + "), value(" + expression->inspect() + ")"); 
    }
}


avec<argument_object*> name_arguments(evaluated_function_object* function, function_call_object* func_call, environment* env) {

    avec<argument_object*> named_arguments;

    object* eval_args = eval_expression(func_call->arguments, env);
    if (eval_args->type() != GROUP_OBJ) {
        errors.push_back("Failed to evaluate arguments"); return named_arguments; }

    avec<object*> evaluated_arguments = static_cast<group_object*>(eval_args)->elements;

    avec<parameter_object*> parameters = function->parameters;

    if (evaluated_arguments.size() != parameters.size()) {
        errors.push_back("Function called with incorrect number of arguments, got " + std::to_string(evaluated_arguments.size()) + " wanted " + std::to_string(parameters.size()));
        return named_arguments;
    }

    for (size_t i = 0; i < parameters.size(); i++) {
        astring param_name = static_cast<parameter_object*>(parameters[i])->name;
        named_arguments.push_back(new argument_object(param_name, evaluated_arguments[i]));
    }

    return named_arguments;
}



static object* eval_run_function(function_call_object* func_call, environment* env) {

    if (func_call->name == "COUNT") {
        avec<object*> args = func_call->arguments->elements;
        avec<object*> e_args;
        e_args.reserve(args.size());
        for (const auto& arg : args) {
            object* e_arg = eval_expression(arg, env);
            if (e_arg->type() == ERROR_OBJ) {
                push_err_ret_err_obj("Failed to evaluate (" + arg->inspect() + ")"); }
            e_args.push_back(e_arg);
        }
        return new function_call_object("COUNT", new group_object(e_args));
    }

    bool found = false;
    for (const auto& func : s_functions) {
        if (func->name == func_call->name) {
            found = true; 
        }
    }

    if (!found && !env->is_function(func_call->name)) {
        push_err_ret_err_obj("Called non-existent function (" + func_call->name + ")"); }
    


    object* env_func = env->get_function(func_call->name);
    if (env_func->type() == ERROR_OBJ) {
        push_err_ret_err_obj("Function returned as error object (" + func_call->name + ")"); }

    evaluated_function_object* function = static_cast<evaluated_function_object*>(env_func);

    size_t error_count = errors.size();
    avec<argument_object*> named_args = name_arguments(function, func_call, env);
    if (error_count < errors.size()) {
        return new error_object(); }



    avec<parameter_object*> parameters = function->parameters;
    if (named_args.size() != parameters.size()) {
        push_err_ret_err_obj("Function called with incorrect number of arguments, got " + std::to_string(named_args.size()) + " wanted " + std::to_string(parameters.size())); }

    environment* function_env = new environment(env);
    bool ok = function_env->add_variables(named_args);
    if (!ok) {
        push_err_ret_err_obj("Failed to add function arguments as variables to function environment"); }

    for (const auto& line : function->body->body) {
        object* res = eval_expression(line, function_env);
        if (res->type() == ERROR_OBJ) {
            return res; }

        if (res->type() == RETURN_STATEMENT) {
            return eval_expression(static_cast<return_statement*>(res)->expression, env);
        }
        
    }

    push_err_ret_err_obj("Failed to find return value");
}

static object* eval_column(column_object* col, environment* env) {
    object* parameter = eval_expression(col->name_data_type, env);
    if (parameter->type() == ERROR_OBJ) {
        return parameter; }

    if (parameter->type() != PARAMETER_OBJ) {
        push_err_ret_err_obj("Column data type (" + col->name_data_type->inspect() + ")failed to evaluate to parameter object"); }

    object* default_value = eval_expression(col->default_value, env);
    if (default_value->type() == ERROR_OBJ) {
        return default_value; }

    if (default_value->type() != STRING_OBJ && default_value->type() != NULL_OBJ) {
        push_err_ret_err_obj("Column default value (" + col->default_value->inspect() + ") failed to evaluate to string object"); }

    if (default_value->type() == NULL_OBJ) {
        default_value = new string_object(""); }

    return new evaluated_column_object(static_cast<parameter_object*>(parameter)->name, static_cast<parameter_object*>(parameter)->data_type, default_value->data());
}

static void eval_alter_table(alter_table* info, environment* env) {

    object* table_name = eval_expression(info->table_name, env);
    if (table_name->type() == ERROR_OBJ) {
        eval_push_error_return("Failed to evaluate table name (" + info->table_name->inspect() + ")"); }
   
    if (table_name->type() != STRING_OBJ) {
        eval_push_error_return("Table name (" + info->table_name->inspect() + ") failed to evaluate to string"); }
    

    const auto& [table, tab_found] = get_table(table_name->data());
    if (!tab_found) {
        eval_push_error_return("INSERT INTO, table not found");}

    table_object* tab = table;



    object* table_edit = eval_expression(info->table_edit, env);
    if (table_edit->type() == ERROR_OBJ) {
        eval_push_error_return("eval_alter_table(): Failed to evaluate table edit"); }

    switch (table_edit->type()) {
    case EVALUATED_COLUMN_OBJ: {

        evaluated_column_object* column_obj = static_cast<evaluated_column_object*>(table_edit);

        for (const auto& table_column : tab->column_datas){
            if (column_obj->name == table_column->name) {
                eval_push_error_return("eval_alter_table(): Table already contains column with name (" + column_obj->name + ")"); }
        }

        table_detail_object* col = new table_detail_object(column_obj->name, column_obj->data_type, new string_object(column_obj->default_value));
        tab->column_datas.push_back(col);
    } break;

    default:
        eval_push_error_return("eval_alter_table(): Table edit (" + info->table_edit->inspect() + ") not supported");
    }
}



static void eval_create_table(const create_table* info, environment* env) {

    object* table_name = eval_expression(info->table_name, env);
    if (table_name->type() == ERROR_OBJ) {
        eval_push_error_return("CREATE TABLE: Failed to evaluate table name (" + info->table_name->inspect() + ")"); }
   
    if (table_name->type() != STRING_OBJ) {
        eval_push_error_return("CREATE TABLE: Table name (" + info->table_name->inspect() + ") failed to evaluate to string"); }


    for (const auto& entry : s_tables) {
        if (entry->table_name == table_name->data()) {
            eval_push_error_return("CREATE TABLE: Table (" + table_name->data() + ") already exists"); }
    }

    

    avec<table_detail_object*> e_column_datas;
    for (const auto& detail : info->details) {
        astring name = detail->name;

        object* data_type = eval_expression(detail->data_type, env);
        if (data_type->type() == ERROR_OBJ) {
            eval_push_error_return("CREATE TABLE: Failed to evaluate data type (" + detail->data_type->inspect() + ")"); }
        
        if (data_type->type() != SQL_DATA_TYPE_OBJ) {
            eval_push_error_return("CREATE TABLE: Data type entry failed to evaluate to SQL data type (" + detail->data_type->inspect() + ")"); }

        if (detail->default_value->type() != NULL_OBJ) {
            object* default_value = eval_expression(detail->default_value, env);
            if (default_value->type() == ERROR_OBJ) {
                eval_push_error_return("CREATE TABLE: Failed to evaluate column name (" + detail->default_value->inspect() + ")"); }
            
            if (default_value->type() != STRING_OBJ && default_value->type() != INTEGER_OBJ) {
                eval_push_error_return("CREATE TABLE: Default value failed to evaluate to a string or a number (" + detail->default_value->inspect() + ")"); }
            
            e_column_datas.push_back(new table_detail_object(name, static_cast<SQL_data_type_object*>(data_type), default_value));
        } else {
            e_column_datas.push_back(new table_detail_object(name, static_cast<SQL_data_type_object*>(data_type), new null_object()));
        }

    }

    table_object* tab = new table_object(table_name->data(), e_column_datas, {});

    s_tables.push_back(tab);
}

// Reference cause easier, hopefully it works;
static object* eval_where(expression_object* clause, table_aggregate_object* table_aggregate, avec<size_t>& row_ids, environment* env) {
    
    if (clause->type() == PREFIX_EXPRESSION_OBJ) {
        push_err_ret_err_obj("Prefix WHERE not supported yet"); }

    if (clause->type() != INFIX_EXPRESSION_OBJ) {
        push_err_ret_err_obj("Tried to evaluate WHERE with non-infix object, bug"); }

    infix_expression_object* where_infix = static_cast<infix_expression_object*>(clause);
    if (where_infix->get_op_type() != WHERE_OP) {
        push_err_ret_err_obj("eval_where(): Called with non-WHERE operator"); }
    

    object* raw_cond = where_infix->right;
    if (raw_cond->type() != INFIX_EXPRESSION_OBJ) {
        push_err_ret_err_obj("WHERE condition is not infix condition"); }

    infix_expression_object* condition = static_cast<infix_expression_object*>(raw_cond);
    

    if (clause->type() == INFIX_EXPRESSION_OBJ) { // For SELECT [*] FROM [table] WHERE [CONDITION]
        if (where_infix->left->type() == STRING_OBJ) {
            const auto& [table, tab_found] = get_table(where_infix->left->data());
            if (tab_found) {
                table_aggregate->add_table(table);
            }
        }
    }

    object* e_left = eval_expression(condition->left, env);
    if (e_left->type() == ERROR_OBJ) {
        push_err_ret_err_obj("SELECT FROM: Failed to evaluate (" + condition->left->inspect() + ")"); }
    
    object* e_right = eval_expression(condition->right, env);
    if (e_right->type() == ERROR_OBJ) {
        push_err_ret_err_obj("SELECT FROM: Failed to evaluate (" + condition->right->inspect() + ")"); }

    // Find column index BEGIN  
    size_t where_col_index = SIZE_T_MAX;
    std::array<object*, 2> sides = {e_left, e_right};
    for (const auto& side : sides) {
        if (where_col_index != SIZE_T_MAX) {
            break; }

        switch(side->type()) {
        case STRING_OBJ: {
            const auto&[id, error_val] = table_aggregate->get_col_id(side->data());
            if (error_val->type() != NULL_OBJ) {
                errors.push_back(std::string(error_val->data())); return error_val; }

            where_col_index = id;
            
        } break;
        case COLUMN_INDEX_OBJECT: {
            column_index_object* col_index = static_cast<column_index_object*>(e_left);

            object* table_alias_obj = col_index->table_name;
            if (table_alias_obj->type() != TABLE_OBJECT) {
                push_err_ret_err_obj("WHERE: Column index, table alias is not table object, got (" + table_alias_obj->inspect() + ")"); }
            table_object* table = static_cast<table_object*>(table_alias_obj);

            object* column_alias_obj = col_index->column_name;
            if (column_alias_obj->type() != INDEX_OBJ) {
                push_err_ret_err_obj("WHERE: Column index, column alias is not index object, got (" + column_alias_obj->inspect() + ")"); }
            index_object* index_obj = static_cast<index_object*>(column_alias_obj);
            size_t index = index_obj->value;
            
            const auto&[id, error_val] = table_aggregate->get_col_id(table->table_name, index);
            if (error_val->type() != NULL_OBJ) {
                push_err_ret_err_obj(error_val->data()); }

            where_col_index = id;
        } break;
        default:
            break;
        }
    }

    if (where_col_index == SIZE_T_MAX) {
        push_err_ret_err_obj("SELECT FROM: Could not find column alias in WHERE condition"); }

    table_object* table = table_aggregate->combine_tables("Shouldn't be in the end result");


    avec<size_t> new_row_ids;
    for (size_t row_id = 0; row_id < table->rows.size(); row_id++) {
        
        // Add to env
        const auto& [raw_cell_value, cell_in_bounds] = table->get_cell_value(row_id, where_col_index);
        if (!cell_in_bounds) {
            push_err_ret_err_obj("SELECT FROM: Weird index bug"); }
        astring cell_value = raw_cell_value->data();
        const auto& [col_data_type, dt_in_bounds] = table->get_column_data_type(where_col_index);
        if (!dt_in_bounds) {
            push_err_ret_err_obj("SELECT FROM: Weird index bug 2"); }
        token_type col_type = col_data_type->data_type;

        object* value;
        switch (col_type) {
        case INT: case INTEGER:
            value = new integer_object(cell_value); break;
        case DECIMAL:
            value = new decimal_object(cell_value); break;
        case VARCHAR:
            value = new string_object(cell_value); break;
        default:
            push_err_ret_err_obj("Currently WHERE conditions aren't supported for " + token_type_to_string(col_type));
        }

        const auto& [column_name, name_in_bounds] = table->get_column_name(where_col_index);
        if (!name_in_bounds) {
            push_err_ret_err_obj("SELECT FROM: Weird index bug 3"); }

        variable_object* var = new variable_object(column_name, value);
        environment* row_env = new environment(env);
        bool added = row_env->add_variable(var);

        if (!added) {
            push_err_ret_err_obj("Failed to add variable (" + var->inspect() + ") to environment"); }
        // env done

        object* should_add_obj = eval_expression(condition, row_env);
        if (should_add_obj->type() == ERROR_OBJ) {
            push_err_ret_err_obj("Failed to evaulate WHERE condition"); }

        if (should_add_obj->type() != BOOLEAN_OBJ) {
            push_err_ret_err_obj("WHERE condition failed to evaluate to boolean"); }

        bool should_add = false;
        if (static_cast<boolean_object*>(should_add_obj)->data() == "TRUE") {
            should_add = true; }
        
        if (should_add) {
            new_row_ids.push_back(row_id);
        }
    }
    row_ids = new_row_ids;

    return new null_object();
}


// Need to work on INFIX
static object* eval_left_join(expression_object* clause, table_aggregate_object* table_aggregate, avec<size_t>& row_ids, environment* env) {

    
    if (clause->type() == INFIX_EXPRESSION_OBJ) {
        push_err_ret_err_obj("Infix LEFT JOIN not supported"); }
        
    if (clause->type() != PREFIX_EXPRESSION_OBJ) {
        push_err_ret_err_obj("Tried to evaluate LEFT JOIN with non-prefix object, bug"); }

    prefix_expression_object* left_join_prefix = static_cast<prefix_expression_object*>(clause);
    if (left_join_prefix->get_op_type() != LEFT_JOIN_OP) {
        push_err_ret_err_obj("eval_where(): Called with non-WHERE operator"); }
    

    object* on_obj = left_join_prefix->right;
    if (on_obj->type() != INFIX_EXPRESSION_OBJ) {
        push_err_ret_err_obj("LEFT JOIN: ON is not infix object"); }

    infix_expression_object* on = static_cast<infix_expression_object*>(on_obj);
    

    if (on->left->type() != STRING_OBJ) {
        push_err_ret_err_obj("Prefix LEFT JOIN: secondary table is not a string"); }

    const auto& [secondary_table, tab_found] = get_table(on->left->data());
    if (!tab_found) {
        push_err_ret_err_obj("Prefix LEFT JOIN: secondary table not found"); }

    

    if (on->right->type() != INFIX_EXPRESSION_OBJ) {
        push_err_ret_err_obj("Prefix LEFT JOIN: right of ON is not infix expression"); }

    infix_expression_object* equals_obj = static_cast<infix_expression_object*>(on->right);
    if (equals_obj->op->op_type   != EQUALS_OP) {
        push_err_ret_err_obj("Prefix LEFT JOIN: right of ON is not comparison, (not x = x)"); }
    if (equals_obj->left->type()  != COLUMN_INDEX_OBJECT) {
        push_err_ret_err_obj("Prefix LEFT JOIN: left of = is not column index object, got (" + equals_obj->left->inspect() + ")"); }
    if (equals_obj->right->type() != COLUMN_INDEX_OBJECT) {
        push_err_ret_err_obj("Prefix LEFT JOIN: right of = is not column index object, got (" + equals_obj->right->inspect() + ")"); }

        

    column_index_object* left = static_cast<column_index_object*>(equals_obj->left);
    if (left->table_name->type()  != STRING_OBJ) {
        push_err_ret_err_obj("Prefix LEFT JOIN: left of ='s column index's table name is not a string"); }
    if (left->column_name->type() != STRING_OBJ) {
        push_err_ret_err_obj("Prefix LEFT JOIN: left of ='s column index's column name is not a string"); }
    
    astring left_table_name  = left->table_name->data();
    astring left_column_name = left->column_name->data();



    column_index_object* right = static_cast<column_index_object*>(equals_obj->right);
    if (right->table_name->type()  != STRING_OBJ) {
        push_err_ret_err_obj("Prefix LEFT JOIN: right of ='s column index's table name is not a string"); }
    if (right->column_name->type() != STRING_OBJ) {
        push_err_ret_err_obj("Prefix LEFT JOIN: right of ='s column index's column name is not a string"); }
    
    astring right_table_name  = right->table_name->data();
    astring right_column_name = right->column_name->data();

    if (left_table_name == right_table_name) {
        push_err_ret_err_obj("Prefix LEFT JOIN: can not join table with itself"); }



    const auto& [left_table, found_left_tab]   = get_table(left_table_name);
    if (!found_left_tab) {
        push_err_ret_err_obj("Prefix LEFT JOIN: left table (" + left_table_name + ") does not exist"); }

    const auto& [right_table, found_right_tab] = get_table(right_table_name);
    if (!found_right_tab) {
        push_err_ret_err_obj("Prefix LEFT JOIN: right table (" + right_table_name + ") does not exist"); }

    
    table_object* primary_table;
    astring primary_table_name;
    astring primary_column_name;
    astring secondary_table_name;
    astring secondary_column_name;
    if (left_table_name == secondary_table->table_name) {
        primary_table         = right_table;
        primary_table_name    = right_table_name;
        primary_column_name   = right_column_name;
        secondary_table_name  = left_table_name;
        secondary_column_name = left_column_name;
    } else if (right_table_name == secondary_table->table_name) {
        primary_table         = left_table;
        primary_table_name    = left_table_name;
        primary_column_name   = left_column_name;
        secondary_table_name  = right_table_name;
        secondary_column_name = right_column_name;
    } else {
        push_err_ret_err_obj("Prefix LEFT JOIN: couldn't assign primary table"); }

    if (secondary_table->table_name != secondary_table_name) {
        push_err_ret_err_obj("Prefix LEFT JOIN: secondary table mismatch, three tables in expression?"); }
    
    if (!primary_table->check_if_field_name_exists(primary_column_name)) {
        push_err_ret_err_obj("Prefix LEFT JOIN: table (" + primary_table_name + ") has no field (" + primary_column_name + ")"); }
    
    if (!secondary_table->check_if_field_name_exists(secondary_column_name)) {
        push_err_ret_err_obj("Prefix LEFT JOIN: table (" + secondary_table_name + ") has no field (" + secondary_column_name + ")"); }

    

    const auto& [prim_col_index, found_prim_col] = primary_table->get_column_index(primary_column_name);
    if (!found_prim_col) {
        push_err_ret_err_obj("Prefix LEFT JOIN: failed to find primary table's column's index"); }

    const auto& [sec_col_index, found_sec_col] = secondary_table->get_column_index(secondary_column_name);
    if (!found_sec_col) {
        push_err_ret_err_obj("Prefix LEFT JOIN: failed to find secondary table's column's index"); }



    const auto& [prim_data_type, prim_dt_in_bounds] = primary_table->get_column_data_type(prim_col_index);
    if (!prim_dt_in_bounds) {
        push_err_ret_err_obj("Prefix LEFT JOIN: Weird index bug");}
    const auto& [sec_data_type, sec_dt_in_bounds] = secondary_table->get_column_data_type(sec_col_index);
    if (!sec_dt_in_bounds) {
        push_err_ret_err_obj("Prefix LEFT JOIN: Weird index bug 2");}

    object* insertable = can_insert(sec_data_type, prim_data_type);
    if (insertable->type() == ERROR_OBJ) {
        push_err_ret_err_obj("Prefix LEFT JOIN: can not insert type (" + sec_data_type->inspect() + ") into (" + prim_data_type->inspect() + ")"); }

    avec<group_object*> prim_rows = primary_table->rows;
    avec<group_object*> sec_rows = secondary_table->rows;
    avec<group_object*> new_rows;
    new_rows.reserve(prim_rows.size());
    for (size_t row_index = 0; row_index < prim_rows.size(); row_index++) {

        avec<object*> prim_row = prim_rows[row_index]->elements;
        avec<object*> sec_row;
        if (row_index < sec_rows.size()) {
            sec_row = sec_rows[row_index]->elements; }
             
        avec<object*> new_row;
        new_row.reserve(prim_row.size() + sec_row.size());
        for (const auto& cell : prim_row) {
            new_row.push_back(cell); }


        const auto& [prim_value, prim_in_bounds] = primary_table->get_cell_value(row_index, prim_col_index);
        if (!prim_in_bounds) {
            push_err_ret_err_obj("Prefix LEFT JOIN: Weird index bug");}
        const auto& [sec_value, sec_in_bounds] = secondary_table->get_cell_value(row_index, sec_col_index);
        if (!sec_in_bounds) {
            new_rows.push_back(new group_object(new_row));
            continue;
        }

        infix_expression_object* condition = new infix_expression_object(new operator_object(EQUALS_OP), prim_value, sec_value);
        
        object* should_add_obj = eval_expression(condition, env);
        if (should_add_obj->type() == ERROR_OBJ) {
            push_err_ret_err_obj("Prefix LEFT JOIN: Failed to evaulate EQUALS condition"); }
        if (should_add_obj->type() != BOOLEAN_OBJ) {
            push_err_ret_err_obj("Prefix LEFT JOIN: Condition failed to evaluate to boolean"); }

        bool should_add = false;
        if (static_cast<boolean_object*>(should_add_obj)->data() == "TRUE") {
            should_add = true; }
        
        if (should_add) {
            for (const auto& cell : sec_row) {
                new_row.push_back(cell);
            } 
        } else {
            for (size_t i = 0; i < secondary_table->column_datas.size(); i++) { // padding if there are no more matches
                new_row.push_back(new null_object());
            }
        }

        new_rows.push_back(new group_object(new_row));
    }

    avec<table_detail_object*> new_column_datas;
    new_column_datas.reserve(primary_table->column_datas.size() + secondary_table->column_datas.size());
    for (const auto& col_data : primary_table->column_datas) {
        new_column_datas.push_back(col_data); }
    for (const auto& col_data : secondary_table->column_datas) {
        new_column_datas.push_back(col_data); }

    
        
    astring table_name = primary_table_name + " + " + secondary_table_name;

    table_object* new_table = new table_object(table_name, new_column_datas, new_rows); // !!MAJOR NOT CORRECT, NEED TO APPEND 2 TABLES
    
    row_ids = new_table->get_row_ids();
    table_aggregate->add_table(new_table);

    return new null_object();
}

// Row ids are passed by reference cause easier, might have to make tab by reference later as well (for stuff like JOINs)
// If is buggy can just go back to using return values
static object* eval_clause(expression_object* clause, table_aggregate_object* table_aggregate, avec<size_t>& row_ids, environment* env) {

    switch (clause->get_op_type()) {

        case WHERE_OP: {
            // WHERE can be infix or prefix
            return eval_where(clause, table_aggregate, row_ids, env);
        } break;
        case LEFT_JOIN_OP: {
            return eval_left_join(clause, table_aggregate, row_ids, env);
        } break;
        default:
            push_err_ret_err_obj("Unsupported op type (" + operator_type_to_astring(clause->get_op_type()) + ")");
    }
    

}

static object* eval_select_from(select_from* wrapper, environment* env) {

    if (wrapper->value->type() != SELECT_FROM_OBJECT) {
        push_err_ret_err_obj("eval_select_from(): Called with invalid object (" + object_type_to_astring(wrapper->value->type()) + ")"); }

    select_from_object* info = static_cast<select_from_object*>(wrapper->value);    
    
    // First, use clause chain to obtain the initial table
    // Second, the clause chain will conncect tables together, and narrow the ammount of rows selected
    avec<size_t> row_ids;
    table_aggregate_object* table_aggregate = new table_aggregate_object();
    if (info->clause_chain.size() != 0) {
        for (const auto& clause : info->clause_chain) {

            if (clause->type() == STRING_OBJ) { // To support plain SELECT * FROM table;
                const auto& [table, tab_found] = get_table(clause->data());
                if (!tab_found) {
                    push_err_ret_err_obj("SELECT FROM: Table (" + clause->data() + ") does not exist"); }

                row_ids = table->get_row_ids(); 
                table_aggregate->add_table(table);
                break;
            }

            if (clause->type() != INFIX_EXPRESSION_OBJ && clause->type() != PREFIX_EXPRESSION_OBJ) {
                push_err_ret_err_obj("Unsupported clause type, (" + clause->inspect() + ")"); 
            }

            object* error_val = eval_clause(static_cast<expression_object*>(clause), table_aggregate, row_ids, env); // Should add table to aggregate by itself
            if (error_val->type() != NULL_OBJ) {
                errors.push_back(std::string(error_val->data())); return error_val; }
        }

    }



    // Second, use column indexes the index into the table aggregate, validate
    avec<object*> column_indexes = info->column_indexes;
    avec<size_t> col_ids;

    if (column_indexes.size() == 0) {
        push_err_ret_err_obj("SELECT FROM: No column indexes"); }

    // If SELECT * FROM [table], add all columns
    if (column_indexes[0]->type() == STAR_OBJECT) {
        col_ids = table_aggregate->get_all_col_ids();

        if (table_aggregate->tables.size() == 1) {
            const auto& [table_name, success] = table_aggregate->get_table_name(0);
            if (!success) {
                push_err_ret_err_obj("SELECT FROM: Strange bug, couldn't get first table from aggregate, even though size == 1"); }
            table_object* table = table_aggregate->combine_tables(table_name);
            return new table_info_object(table, col_ids, row_ids);
        }

        table_object* table = table_aggregate->combine_tables("aggregate");
        return new table_info_object(table, col_ids, row_ids);
    }

    for (const auto& col_index_raw : column_indexes) {

        // avec<object*> REALARGS = *static_cast<function_call_object*>(col_index_raw)->arguments->elements; for debug


        object* selecter = eval_expression(col_index_raw, env);
        if (selecter->type() == ERROR_OBJ) {
            push_err_ret_err_obj("SELECT FROM: Could not evalute (" + col_index_raw->inspect() + ")"); }

        switch (selecter->type()) {
        case FUNCTION_CALL_OBJ: { // Not padding for now cause lazy

            if (column_indexes.size() == 1) {
                avec<size_t> new_row_ids;
                new_row_ids.push_back(0);
                row_ids = new_row_ids;
            }

            avec<object*> args = static_cast<function_call_object*>(selecter)->arguments->elements;
            if (args.size() != 1) {
                push_err_ret_err_obj("COUNT() bad argument count"); }
            object* arg = args[0];
            if (arg->type() != STAR_OBJECT) {
                push_err_ret_err_obj("COUNT() argument must be *, got (" + object_type_to_astring(arg->type()) + ")"); }

            size_t count = (table_aggregate->tables[0])->rows.size(); //!!MAJOR keep track of max size
            
            SQL_data_type_object* type = new SQL_data_type_object(NONE, INTEGER, new integer_object(11));
            group_object* row = new group_object(new integer_object(static_cast<int>(count))); //!!MAJOR stinky
            table_detail_object* detail = new table_detail_object("COUNT(*)", type, new null_object());
            table_object* count_star = new table_object("COUNT(*) TABLE", detail, row);
            table_aggregate->add_table(count_star);
            const auto& [id, ok] = table_aggregate->get_last_col_id();
            if (!ok) {
                push_err_ret_err_obj("SELECT FROM: Weird bug"); }
            col_ids.push_back(id);
        } break;
        case COLUMN_INDEX_OBJECT: {
            column_index_object* col_index = static_cast<column_index_object*>(selecter);
    
            object* raw_tab = col_index->table_name;
            if (raw_tab->type() != TABLE_OBJECT) {
                push_err_ret_err_obj("SELECT FROM: Column index table alias is not table object"); }
            astring table_name = static_cast<table_object*>(raw_tab)->table_name;
    
            object* raw_index = col_index->column_name;
            if (raw_index->type() != INDEX_OBJ) {
                push_err_ret_err_obj("SELECT FROM: Column index column alias is not index object"); }
            size_t index = static_cast<index_object*>(raw_index)->value;
    
            const auto& [id, error_val] = table_aggregate->get_col_id(table_name, index);
            if (error_val->type() != NULL_OBJ) {
                push_err_ret_err_obj("SELECT FROM:" + error_val->data());}
    
            col_ids.push_back(id);
             
        } break;
        case STRING_OBJ: {
    
            astring column_name = selecter->data();
    
            const auto& [id, error_val] = table_aggregate->get_col_id(column_name);
            if (error_val->type() != NULL_OBJ) {
                errors.push_back(std::string(error_val->data())); return error_val; }
    
            col_ids.push_back(id);
        } break;

        default: 
            push_err_ret_err_obj("SELECT FROM: Cannot use (" + selecter->inspect() + ") to index");

        }
    }

    if (table_aggregate->tables.size() == 1 || column_indexes.size() == 1) {
        const auto& [table_name, ok] = table_aggregate->get_table_name(0);
        if (!ok) {
            push_err_ret_err_obj("SELECT FROM: Strange bug, couldn't get first table from aggregate, even though size == 1"); }
        table_object* table = table_aggregate->combine_tables(table_name);
        return new table_info_object(table, col_ids, row_ids);
    }

    table_object* table = table_aggregate->combine_tables("aggregate");
    return new table_info_object(table, col_ids, row_ids);
}

static void print_table() {

    table_info_object* tab_info = display_tab.table_info;

    table_object* tab = tab_info->tab;

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
        const auto& [row, row_index_in_bounds] = tab->get_row_vector(row_index);
        if (!row_index_in_bounds) {
            eval_push_error_return("print_table(): Out of bounds row index"); }
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

static std::pair<table_object*, bool> get_table(const std_and_astring_variant& name) {

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


static void eval_insert_into(const insert_into* wrapper, environment* env) {

    if (wrapper->value->type() != INSERT_INTO_OBJECT) {
        eval_push_error_return("eval_insert_into(): Called with invalid object (" + object_type_to_astring(wrapper->value->type()) + ")"); }

    insert_into_object* info = static_cast<insert_into_object*>(wrapper->value);

    object* table_name = eval_expression(info->table_name, env);
    if (table_name->type() == ERROR_OBJ) {
        eval_push_error_return("Failed to evaluate table name (" + info->table_name->inspect() + ")"); }
   
    if (table_name->type() != STRING_OBJ) {
        eval_push_error_return("Table name (" + info->table_name->inspect() + ") failed to evaluate to string"); }


        
    const auto& [table, tab_found] = get_table(table_name->data());
    if (!tab_found) {
        eval_push_error_return("INSERT INTO, table not found");}



    object* values_obj = eval_expression(info->values, env);
    if (values_obj->type() != GROUP_OBJ) { // or != TABLE_OBJ???
        eval_push_error_return("INSERT INTO, failed to evaluate values");}

    avec<object*> values = static_cast<group_object*>(values_obj)->elements;
        
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
    for (const auto& field : info->fields) {
        object* evaluated_field = eval_expression(field, env);
        if (evaluated_field->type() == ERROR_OBJ) {
            eval_push_error_return("eval_insert_into(): Failed to evaluate field (" + field->inspect() + ") while inserting rows"); }

        if (evaluated_field->type() != STRING_OBJ) { // Can't be null >:(
            eval_push_error_return("INSERT INTO: Field (" + field->inspect() + ") evaluated to non-string value"); }

        field_names.push_back(evaluated_field->data());

    }
    // check if fields exist
    for (const auto& name : field_names) {
        bool found = table->check_if_field_name_exists(name);
        if (!found) {
            eval_push_error_return("INSERT INTO: Could not find field (" + name + ") in table + (" + table->table_name + ")"); }
    }

    // evaluate values
    avec<object*> e_values;
    for (const auto& value : values) {
        object* e_value = eval_expression(value, env);

        if (e_value->type() == ERROR_OBJ) {
            eval_push_error_return("eval_insert_into(): Failed to evaluate value (" + value->inspect() + ") while inserting rows"); }

        e_values.push_back(e_value);
    }

    avec<object*> row;
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
                const auto& [default_value, in_bounds] = table->get_column_default_value(i);
                if (!in_bounds) {
                    eval_push_error_return("INSERT INTO: Weird index bug 2");}
                row.push_back(default_value); }

            if (found) {
                if (id >= e_values.size()) {
                    eval_push_error_return("INSERT INTO: Weird index bug 3"); }
                object* value = e_values[id];

                const auto& [data_type, in_bounds] = table->get_column_data_type(i);
                if (!in_bounds) {
                    eval_push_error_return("INSERT INTO: Weird index bug 4");}

                object* insertable = can_insert(value, data_type);
                if (insertable->type() == ERROR_OBJ) {
                    errors.push_back(std::string(insertable->data()));
                    eval_push_error_return("INSERT INTO: Value (" + value->inspect() + ") evaluated to non-insertable value while inserting rows"); }

                row.push_back(insertable); 
            }

        }
    } else {
        size_t i = 0;
        for (i = 0; i < e_values.size(); i++) {

            object* value = e_values[i];

            const auto& [data_type, in_bounds] = table->get_column_data_type(i);
            if (!in_bounds) {
                eval_push_error_return("INSERT INTO: Weird index bug 4");}
                
            object* insertable = can_insert(value, data_type);
            if (insertable->type() == ERROR_OBJ) {
                errors.push_back(std::string(insertable->data()));
                eval_push_error_return("INSERT INTO: Value (" + value->inspect() + ") evaluated to non-insertable value while inserting rows"); }

            row.push_back(insertable);
        }

        for (; i < table->column_datas.size(); i++) {
            const auto& [default_value, in_bounds] = table->get_column_default_value(i);
            if (!in_bounds) {
                eval_push_error_return("INSERT INTO: Weird index bug 5");}
            row.push_back(default_value); }
    }

    if (!errors.empty()) {
        return;}

    table->rows.push_back(new group_object(row));
}


static bool can_insert_both_SQL(SQL_data_type_object* insert_obj, SQL_data_type_object* data_type) {

    if (insert_obj->data_type == data_type->data_type) {
        return true; }
    return false;
}

static object* can_insert(object* insert_obj, SQL_data_type_object* data_type) {

    if (insert_obj->type() == SQL_DATA_TYPE_OBJ) {
        bool success = can_insert_both_SQL(static_cast<SQL_data_type_object*>(insert_obj), data_type);
        if (!success) {
            return new error_object(); 
        } else {
            return new null_object(); }
    }

    switch (data_type->data_type) {
    case INT:
        switch (insert_obj->type()) {
        case INTEGER_OBJ:
            return insert_obj; 
            break;
        case DECIMAL_OBJ:
            warnings.push_back("Decimal implicitly converted to INT");
            return new integer_object(insert_obj->data());
            break;
        default:
            return new error_object("can_insert(): Value: (" + insert_obj->data() + ") has mismatching type with column (" + data_type->inspect() + ")");
            break;
        }
        break;
    case FLOAT:
        switch (insert_obj->type()) {
        case INTEGER_OBJ:
            warnings.push_back("Integer implicitly converted to FLOAT");
            return new decimal_object(insert_obj->data());
            break;
        case DECIMAL_OBJ:
            return insert_obj; 
            break;
        default:
            return new error_object("can_insert(): Value: (" + insert_obj->data() + ") has mismatching type with column (" + data_type->inspect() + ")");
            break;
        }
        break;
    case DOUBLE:
    switch (insert_obj->type()) {
        case INTEGER_OBJ:
            warnings.push_back("Integer implicitly converted to DOUBLE");
            return new decimal_object(insert_obj->data());
            break;
        case DECIMAL_OBJ:
            return insert_obj; 
            break;
        default:
            return new error_object("can_insert(): Value: (" + insert_obj->data() + ") has mismatching type with column (" + data_type->inspect() + ")");
            break;
        }
        break;
    case VARCHAR:
        switch (insert_obj->type()) {
        case INTEGER_OBJ:
            warnings.push_back("Integer implicitly converted to VARCHAR");
            return new string_object(insert_obj->data());
            break;
        case DECIMAL_OBJ:
            warnings.push_back("Decimal implicitly converted to VARCHAR");
            return new string_object(insert_obj->data());
            break;
        case STRING_OBJ: {
            if (data_type->parameter->type() != INTEGER_OBJ) {
                return new error_object("can_insert(): varchar cannot be inserted into data type with non-integer parameter");
            }
            int max_length = static_cast<integer_object*>(data_type->parameter)->value;
            size_t insert_length = insert_obj->data().length();
            if (insert_length > static_cast<size_t>(max_length)) {
                return new error_object("can_insert(): Value: (" + insert_obj->data() + ") excedes column's max length (" + data_type->parameter->inspect() + ")");
            }
            return insert_obj;
        } break;
        default:
            return new error_object("can_insert(): Value: (" + insert_obj->data() + ") has mismatching type with column (" + data_type->inspect() + ")");
            break;
        }
        break;
    default:
        return new error_object("can_insert(): " + data_type->inspect() + " is not supported YET");
        break;
    }
    
}