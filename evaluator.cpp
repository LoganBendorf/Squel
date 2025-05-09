
#include "evaluator.h"

#include "node.h"
#include "structs_and_macros.h"
#include "helpers.h"
#include "object.h"
#include "environment.h"

#include <iostream>
#include <vector>
#include <algorithm>



extern std::vector<std::string> errors;
extern display_table display_tab;

static std::vector<node*> nodes;

extern std::vector<std::string> warnings;

static std::vector<table> tables;
static std::vector<evaluated_function_object*> functions;

static void eval_function(function* func, environment* env);
static object* eval_run_function(function_call_object* func_call, environment* env);

static void eval_alter_table(alter_table* info, environment* env);
static void eval_create_table(const create_table* info, environment* env);
static void eval_select_from(select_from* info, environment* env);
static void print_table();
static void eval_insert_into(const insert_into* info, environment* env);

static object* eval_expression(object* expression, environment* env);
static object* eval_column(column_object* col, environment* env);

static object* eval_prefix_expression(operator_object* op, object* right, environment* env);
static object* eval_infix_expression(operator_object* op, object* left, object* right, environment* env);

static object* can_insert(object* insert_obj, SQL_data_type_object* data_type);

#define eval_push_error_return(x)     \
        std::string error = x;        \
        errors.push_back(error);      \
        return                        \

#define push_err_return_err_obj(x)    \
    std::string error = x;            \
    errors.push_back(error);          \
    return new error_object();

environment* eval_init(std::vector<node*> nds, std::vector<evaluated_function_object*> g_functions, std::vector<table> g_tables) {
    nodes = nds;
    functions.clear();
    for(const auto& func : g_functions) {
        functions.push_back(func->clone(ARENA));
    }
    tables.clear();
    for(const auto& tab : g_tables) {
        std::vector<column_data> column_datas;
        for (const auto& col_data : tab.column_datas) {
            column_datas.push_back({col_data.field_name, col_data.data_type->clone(ARENA), col_data.default_value});
        }
        table new_tab = {tab.name, column_datas, tab.rows};
        tables.push_back(new_tab);
    }
    return new environment();
}

std::pair<std::vector<evaluated_function_object*>, std::vector<table>> eval(environment* env) {
    
    for (const auto& node : nodes) {

        switch(node->type()) {
            case INSERT_INTO_NODE:
                eval_insert_into(static_cast<insert_into*>(node), env);
                printf("EVAL INSERT INTO CALLED\n");
                break;
            case SELECT_FROM_NODE:
                eval_select_from(static_cast<select_from*>(node), env);
                printf("EVAL SELECT FROM CALLED\n");
                break;
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
            default:
                errors.push_back("eval: unknown node type (" + node->inspect() + ")");
        }
    }

    return std::pair(functions, tables);
}

static void eval_function (function* func, environment* env) {

    object* eval_parameters = eval_expression(func->func->parameters, env);
    if (eval_parameters->type() != GROUP_OBJ) {
        eval_push_error_return("Failed to evaluate to function parameter"); }

    std::vector<parameter_object*> evaluated_parameters;
    std::vector<object*> params = *static_cast<group_object*>(eval_parameters)->elements;
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
    for (size_t i = 0; i < functions.size(); i++) {
        if (*functions[i]->name == *evaluated_func->name) {
            functions[i] = evaluated_func;
            found = true;
            break;
        }
    }
    if (!found) {
        functions.push_back(evaluated_func);
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
            return new integer_object( - std::stoi(e_right->data()));
        case DECIMAL_OBJ:
            return new decimal_object( - std::stod(e_right->data()));
        default:
            push_err_return_err_obj("No negation operation for type (" + e_right->inspect() + ")");
        }
    default:
        push_err_return_err_obj("No prefix " + op->inspect() + " operator known");
    }
}

static object* eval_infix_expression(operator_object* op, object* left, object* right, environment* env) {

    object* e_left = eval_expression(left, env);
    object* e_right = eval_expression(right, env);


    switch (op->op_type) {
        case ADD_OP:
            if (is_numeric_object(e_left) && is_numeric_object(e_right)) {
                return new integer_object(static_cast<integer_object*>(e_left)->value + static_cast<integer_object*>(e_right)->value);
            } 
            else if (is_string_object(e_left) && is_string_object(e_right)) {
                return new string_object(e_left->data() + e_right->data());
            } 
            else {
                push_err_return_err_obj("No infix " + op->inspect() + " operation for " + e_left->inspect() + " and " + e_right->inspect());
            }
            
        case SUB_OP:
            if (!is_numeric_object(e_left) || !is_numeric_object(e_right)) {
                std::string error_str = "No infix " + op->inspect() + " operation for " + e_left->inspect() + " and " + e_right->inspect();
                push_err_return_err_obj(error_str);}
            return new integer_object(static_cast<integer_object*>(e_left)->value - static_cast<integer_object*>(e_right)->value);
            break;
        case MUL_OP:
            if (!is_numeric_object(e_left) || !is_numeric_object(e_right)) {
                std::string error_str = "No infix " + op->inspect() + " operation for " + e_left->inspect() + " and " + e_right->inspect();
                push_err_return_err_obj(error_str);}
            return new integer_object(static_cast<integer_object*>(e_left)->value * static_cast<integer_object*>(e_right)->value);
            break;
        case DIV_OP:
            if (!is_numeric_object(e_left) || !is_numeric_object(e_right)) {
                std::string error_str = "No infix " + op->inspect() + " operation for " + e_left->inspect() + " and " + e_right->inspect();
                push_err_return_err_obj(error_str);}
            return new integer_object(static_cast<integer_object*>(e_left)->value / static_cast<integer_object*>(e_right)->value);
            break;
        case DOT_OP:
            if (e_left->type() != INTEGER_OBJ || e_left->type() != INTEGER_OBJ) {
                std::string error_str = "No infix " + op->inspect() + " operation for " + e_left->inspect() + " and " + e_right->inspect();
                push_err_return_err_obj(error_str);}
            return new decimal_object(e_left->data() + "." + e_right->data());
            break;
        case EQUALS_OP:
            if (e_left->type() == STRING_OBJ && e_right->type() == STRING_OBJ) {
                return new boolean_object(e_left->data() == e_right->data()); 
            } else if (e_left->type() == INTEGER_OBJ && e_right->type() == INTEGER_OBJ) {
                return new boolean_object(static_cast<integer_object*>(e_left)->value == static_cast<integer_object*>(e_right)->value); 
            } else {
                std::string error_str = "No infix " + op->inspect() + " operation for " + e_left->inspect() + " and " + e_right->inspect();
                push_err_return_err_obj(error_str);
            }
            break;
        case NOT_EQUALS_OP:
            if (e_left->type() == STRING_OBJ && e_right->type() == STRING_OBJ) {
                return new boolean_object(e_left->data() != e_right->data()); 
            } else if (e_left->type() == INTEGER_OBJ && e_right->type() == INTEGER_OBJ) {
                return new boolean_object(static_cast<integer_object*>(e_left)->value != static_cast<integer_object*>(e_right)->value); 
            } else {
                std::string error_str = "No infix " + op->inspect() + " operation for " + e_left->inspect() + " and " + e_right->inspect();
                push_err_return_err_obj(error_str);
            }
            break;
        case LESS_THAN_OP:
            if (e_left->type() != INTEGER_OBJ || e_left->type() != INTEGER_OBJ) {
                std::string error_str = "No infix " + op->inspect() + " operation for " + e_left->inspect() + " and " + e_right->inspect();
                push_err_return_err_obj(error_str);}
            return new boolean_object(static_cast<integer_object*>(e_left)->value < static_cast<integer_object*>(e_right)->value);
            break;
        case GREATER_THAN_OP:
            if (e_left->type() != INTEGER_OBJ || e_left->type() != INTEGER_OBJ) {
                std::string error_str = "No infix " + op->inspect() + " operation for " + e_left->inspect() + " and " + e_right->inspect();
                push_err_return_err_obj(error_str);}
            return new boolean_object(static_cast<integer_object*>(e_left)->value > static_cast<integer_object*>(e_right)->value);
            break;
        default:
            push_err_return_err_obj("No infix " + op->inspect() + " operator known");
    }
}

// can turn into while loop instead of recursively calling itself, for perf
static object* eval_expression(object* expression, environment* env) {

    switch(expression->type()) {

        // Basic stuff begin
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
                push_err_return_err_obj("For now parameters of SQL data type must evaluate to integer/decimal/none, can be strings later when working on SET or ENUM"); }

            cur = new SQL_data_type_object(cur->prefix, cur->data_type, param);

            return cur;
        } break;
        case NULL_OBJ:
            return expression; break;
        case PREFIX_EXPRESSION_OBJ:
            return eval_prefix_expression(static_cast<prefix_expression_object*>(expression)->op, static_cast<prefix_expression_object*>(expression)->right, env); break;
        // Basic stuff end

        case COLUMN_OBJ:
            return eval_column(static_cast<column_object*>(expression), env);

        case FUNCTION_CALL_OBJ:
            return eval_run_function(static_cast<function_call_object*>(expression), env); break;

        case GROUP_OBJ: {
            group_object* group = static_cast<group_object*>(expression);
            std::vector<object*> objects;
            for (const auto& obj: *group->elements) {
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
            for (const auto& statement: *block->body) {
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
                push_err_return_err_obj("If statement condition returned non-boolean"); }

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
            push_err_return_err_obj("eval_expression(): Cannot evaluate expression. Type (" + object_type_to_string(expression->type()) + "), value(" + expression->inspect() + ")"); }

}

std::vector<argument_object*> name_arguments(evaluated_function_object* function, function_call_object* func_call, environment* env) {

    std::vector<argument_object*> named_arguments;

    object* eval_args = eval_expression(func_call->arguments, env);
    if (eval_args->type() != GROUP_OBJ) {
        errors.push_back("Failed to evaluate arguments"); return named_arguments; }

    std::vector<object*> evaluated_arguments = *static_cast<group_object*>(eval_args)->elements;

    std::vector<parameter_object*> parameters = *function->parameters;

    if (evaluated_arguments.size() != parameters.size()) {
        errors.push_back("Function called with incorrect number of arguments, got " + std::to_string(evaluated_arguments.size()) + " wanted " + std::to_string(parameters.size()));
        return named_arguments;
    }

    for (size_t i = 0; i < parameters.size(); i++) {
        std::string param_name = *static_cast<parameter_object*>(parameters[i])->name;
        named_arguments.push_back(new argument_object(param_name, evaluated_arguments[i]));
    }

    return named_arguments;
}



static object* eval_run_function(function_call_object* func_call, environment* env) {

    bool found = false;
    for (const auto& func : functions) {
        if (*func->name == *func_call->name) {
            found = true; 
        }
    }

    if (!found && !env->is_function(*func_call->name)) {
        push_err_return_err_obj("Called non-existent function (" + *func_call->name + ")"); }
    


    object* env_func = env->get_function(*func_call->name);
    if (env_func->type() == ERROR_OBJ) {
        push_err_return_err_obj("Function returned as error object (" + *func_call->name + ")"); }

    evaluated_function_object* function = static_cast<evaluated_function_object*>(env_func);

    size_t error_count = errors.size();
    std::vector<argument_object*> named_args = name_arguments(function, func_call, env);
    if (error_count < errors.size()) {
        return new error_object(); }



    std::vector<parameter_object*> parameters = *function->parameters;
    if (named_args.size() != parameters.size()) {
        push_err_return_err_obj("Function called with incorrect number of arguments, got " + std::to_string(named_args.size()) + " wanted " + std::to_string(parameters.size())); }

    environment* function_env = new environment(env);
    bool ok = function_env->add_variables(named_args);
    if (!ok) {
        push_err_return_err_obj("Failed to add function arguments as variables to function environment"); }

    for (const auto& line : *function->body->body) {
        object* res = eval_expression(line, function_env);
        if (res->type() == ERROR_OBJ) {
            return res; }

        if (res->type() == RETURN_STATEMENT) {
            return eval_expression(static_cast<return_statement*>(res)->expression, env);
        }
        
    }

    push_err_return_err_obj("Failed to find return value");
}

static object* eval_column(column_object* col, environment* env) {
    object* parameter = eval_expression(col->name_data_type, env);
    if (parameter->type() == ERROR_OBJ) {
        return parameter; }

    if (parameter->type() != PARAMETER_OBJ) {
        push_err_return_err_obj("Column data type (" + col->name_data_type->inspect() + ")failed to evaluate to parameter object"); }

    object* default_value = eval_expression(col->default_value, env);
    if (default_value->type() == ERROR_OBJ) {
        return default_value; }

    if (default_value->type() != STRING_OBJ && default_value->type() != NULL_OBJ) {
        push_err_return_err_obj("Column default value (" + col->default_value->inspect() + ") failed to evaluate to string object"); }

    if (default_value->type() == NULL_OBJ) {
        default_value = new string_object(""); }

    return new evaluated_column_object(*static_cast<parameter_object*>(parameter)->name, static_cast<parameter_object*>(parameter)->data_type, default_value->data());
}

static void eval_alter_table(alter_table* info, environment* env) {

    object* table_name = eval_expression(info->table_name, env);
    if (table_name->type() == ERROR_OBJ) {
        eval_push_error_return("Failed to evaluate table name (" + info->table_name->inspect() + ")"); }
   
    if (table_name->type() != STRING_OBJ) {
        eval_push_error_return("Table name (" + info->table_name->inspect() + ") failed to evaluate to string"); }



    table* tab;
    bool tab_found = false;
    for (auto& entry : tables) {
        if (entry.name == table_name->data()) {
            tab = &entry;
            tab_found = true;
            break;
        }
    }
    
    if (!tab_found) {
        eval_push_error_return("eval_alter_table(): Table not found (" + table_name->inspect() + ")"); }

    object* table_edit = eval_expression(info->table_edit, env);
    if (table_edit->type() == ERROR_OBJ) {
        eval_push_error_return("eval_alter_table(): Failed to evaluate table edit"); }

    switch (table_edit->type()) {
    case EVALUATED_COLUMN_OBJ: {

        evaluated_column_object* column_obj = static_cast<evaluated_column_object*>(table_edit);

        for (const auto& table_column : tab->column_datas){
            if (*column_obj->name == table_column.field_name) {
                eval_push_error_return("eval_alter_table(): Table already contains column with name (" + *column_obj->name + ")"); }
        }

        column_data col = {*column_obj->name, column_obj->data_type, *column_obj->default_value};
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


    for (const auto& entry : tables) {
        if (entry.name == table_name->data()) {
            eval_push_error_return("CREATE TABLE: Table (" + table_name->data() + ") already exists"); }
    }

    // Make sure column_datas values are on the heap
    std::vector<column_data> column_datas;
    for (const auto& detail : *info->details) {
        object* name = eval_expression(detail->name, env);
        if (name->type() == ERROR_OBJ) {
            eval_push_error_return("CREATE TABLE: Failed to evaluate column name (" + detail->name->inspect() + ")"); }
        
        if (name->type() != STRING_OBJ) {
            eval_push_error_return("CREATE TABLE: Column name failed to evaluate to string (" + detail->name->inspect() + ")"); }

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
                
            column_data column(name->data(), static_cast<SQL_data_type_object*>(data_type), default_value->data());
            column_datas.push_back(column);
        } else {
            column_data column(name->data(), static_cast<SQL_data_type_object*>(data_type), ""); // todo
            column_datas.push_back(column);
        }

    }

    table tab;
    tab.name = table_name->data();
    tab.column_datas = column_datas;

    tables.push_back(tab);
}


static void eval_select_from(select_from* info, environment* env) {

    object* table_name = eval_expression(info->table_name, env);
    if (table_name->type() == ERROR_OBJ) {
        eval_push_error_return("Failed to evaluate table name (" + info->table_name->inspect() + ")"); }
   
    if (table_name->type() != STRING_OBJ) {
        eval_push_error_return("Table name (" + info->table_name->inspect() + ") failed to evaluate to string"); }



    table tab;
    bool found = false;
    for (const auto& entry : tables) {
        if (entry.name == table_name->data()) {
            tab = entry;
            found = true;
            break;
        }
    }
    
    if (!found) {
        eval_push_error_return("eval_select_from(): Table not found (" + table_name->inspect() + ")"); }

    std::vector<std::string> column_names;
    if (info->asterisk == true) {
        for (const auto& col_data : tab.column_datas) {
            column_names.push_back(col_data.field_name); 
        }
    } else {
        for (const auto& column : *info->column_names) {
            object* column_name = eval_expression(column, env);
            if (column_name->type() != STRING_OBJ) {
                eval_push_error_return("SELECT FROM: Column (" + column->inspect() + "> failed to evaulated to a string"); }

            column_names.push_back(column->data()); 
        }
    }
    
    std::vector<size_t> col_ids;
    for (size_t i = 0; i < tab.column_datas.size(); i++) {
        for (const auto& col_name : column_names) {
            if (tab.column_datas[i].field_name == col_name) {
                col_ids.push_back(i);
                break;
            }
        }
    }
     
    std::vector<size_t> row_ids;
    if (info->condition->type() != NULL_OBJ) {

        if (info->condition->type() != INFIX_EXPRESSION_OBJ) {
            eval_push_error_return("Unsupported condition type, (" + info->condition->inspect() + ")"); }

        infix_expression_object* condition = static_cast<infix_expression_object*>(info->condition);

        object* e_left = eval_expression(condition->left, env);
        if (e_left->type() == ERROR_OBJ) {
            eval_push_error_return("SELECT FROM: Failed to evaluate (" + condition->left->inspect() + ")"); }
        
        object* e_right = eval_expression(condition->right, env);
        if (e_right->type() == ERROR_OBJ) {
            eval_push_error_return("SELECT FROM: Failed to evaluate (" + condition->right->inspect() + ")"); }

        size_t where_col_index = SIZE_T_MAX;
        if (e_left->type() == STRING_OBJ) {
            for (const auto& col_id : col_ids) {
                if (tab.column_datas[col_id].field_name == e_left->data()) {
                    where_col_index = col_id;
                }
            }
        } else if (e_right->type() == STRING_OBJ) {
            for (const auto& col_id : col_ids) {
                if (tab.column_datas[col_id].field_name == e_left->data()) {
                    where_col_index = col_id;
                }
            }
        } else {
            eval_push_error_return("SELECT FROM: WHERE condition must contain a column name"); }

        if (where_col_index == SIZE_T_MAX) {
            eval_push_error_return("SELECT FROM: Could not find column name in WHERE condition"); }



        for (size_t i = 0; i < tab.rows.size(); i++) {
            
            // Add to env
            std::string cell_value = tab.rows[i].column_values[where_col_index];
            token_type col_type = tab.column_datas[where_col_index].data_type->data_type;

            object* value;
            switch (col_type) {
            case INT: case INTEGER:
                value = new integer_object(std::stoi(cell_value)); break;
            case DECIMAL:
                value = new decimal_object(std::stod(cell_value)); break;
            case VARCHAR:
                value = new string_object(cell_value); break;
            default:
                eval_push_error_return("Currently WHERE conditions aren't supported for " + token_type_to_string(col_type));
            }

            variable_object* var = new variable_object(tab.column_datas[where_col_index].field_name, value);
            environment* row_env = new environment(env);
            bool added = row_env->add_variable(var);

            if (!added) {
                eval_push_error_return("Failed to add variable (" + var->inspect() + ") to environment"); }
            // env done

            object* should_add_obj = eval_expression(condition, row_env);
            if (should_add_obj->type() == ERROR_OBJ) {
                eval_push_error_return("Failed to evaulate WHERE condition"); }

            if (should_add_obj->type() != BOOLEAN_OBJ) {
                eval_push_error_return("WHERE condition failed to evaluate to boolean"); }

            bool should_add = false;
            if (static_cast<boolean_object*>(should_add_obj)->data() == "TRUE") {
                should_add = true; }
            
            if (should_add) {
                row_ids.push_back(i);
            }
        }

    } else {
        for (size_t i = 0; i < tab.rows.size(); i++) {
            row_ids.push_back(i);
        }
    }



    display_tab.to_display = true;
    display_tab.tab = tab;
    display_tab.col_ids = col_ids;
    display_tab.row_ids = row_ids;

    print_table(); // CMD line print, QT will do it's own thing in main

}

static void print_table() {

    table tab = display_tab.tab;

    std::cout << tab.name << ":\n";

    std::string field_names = "";
    for (const auto& col_id : display_tab.col_ids) {
        std::string full_name = tab.column_datas[col_id].field_name;

        std::string name = full_name.substr(0, 10);
        size_t pad_length = 10 - name.length();
        std::string pad(pad_length, ' ');
        name += pad + " | ";

        field_names += name;
    }

    std::cout << field_names <<std::endl;

    
    for (const auto& row_index : display_tab.row_ids) {
        std::string row_values = "";
        std::vector<std::string> column_values = display_tab.tab.rows[row_index].column_values;
        for (const auto& col_id : display_tab.col_ids) {

            std::string full_name = column_values[col_id];

            std::string name = full_name.substr(0, 10);
            size_t pad_length = 10 - name.length();
            std::string pad(pad_length, ' ');
            name += pad + " | ";

            row_values += name;
        }
        std::cout << row_values << "\n";
    }

    std::cout << std::endl;
}

static void eval_insert_into(const insert_into* info, environment* env) {

    object* table_name = eval_expression(info->table_name, env);
    if (table_name->type() == ERROR_OBJ) {
        eval_push_error_return("Failed to evaluate table name (" + info->table_name->inspect() + ")"); }
   
    if (table_name->type() != STRING_OBJ) {
        eval_push_error_return("Table name (" + info->table_name->inspect() + ") failed to evaluate to string"); }


        
    table* tab_ptr;
    bool table_found = false;
    for (auto& entry : tables) {
        if (entry.name == table_name->data()) {
            tab_ptr = &entry;
            table_found = true;
            break;
        }
    }

    if (!table_found) { // More important errors should go last so basic mistakes are fixed first???
        eval_push_error_return("INSERT INTO, table not found");}
        
    if (info->values->size() == 0) {
        eval_push_error_return("INSERT INTO, no values");}

    if (info->fields->size() < info->values->size()) {
        eval_push_error_return("INSERT INTO, more values than field names");}

    if (info->fields->size() > tab_ptr->column_datas.size()) {
        eval_push_error_return("INSERT INTO, more field names than table has columns");}

    if (info->values->size() > tab_ptr->column_datas.size()) {
        eval_push_error_return("INSERT INTO, more values than table has columns");}


    // evaluate fields
    std::vector<std::string> field_names;
    for (const auto& field : *info->fields) {
        object* evaluated_field = eval_expression(field, env);
        if (evaluated_field->type() == ERROR_OBJ) {
            eval_push_error_return("eval_insert_into(): Failed to evaluate field (" + field->inspect() + ") while inserting rows"); }

        if (evaluated_field->type() != STRING_OBJ) { // Can't be null >:(
            eval_push_error_return("INSERT INTO: Field (" + field->inspect() + ") evaluated to non-string value"); }

        field_names.push_back(evaluated_field->data());

    }
    // check if fields exist
    for (const auto& name : field_names) {
        bool found = false;
        for (const auto& col_data : tab_ptr->column_datas) {
            if (col_data.field_name == name) {
                found = true; break; }
        }

        if (!found) {
            eval_push_error_return("INSERT INTO: Could not find field (" + name + ") in table + (" + tab_ptr->name + ")"); }
    }

    // evaluate values
    std::vector<object*> values;
    for (const auto& value : *info->values) {
        object* evaluated_value = eval_expression(value, env);

        if (evaluated_value->type() == ERROR_OBJ) {
            eval_push_error_return("eval_insert_into(): Failed to evaluate value (" + value->inspect() + ") while inserting rows"); }

        values.push_back(evaluated_value);
    }

    row roh;
    if (info->fields->size() > 0) {

        // If field matches column name, add. else add default value
        for (size_t i = 0; i < tab_ptr->column_datas.size(); i++) {
            bool found = false;
            size_t id = 0;
            for (size_t j = 0; j < field_names.size(); j++) {
                if (tab_ptr->column_datas[i].field_name == field_names[j]) {
                    id = j;
                    found = true; 
                    break; 
                } 
            }

            if (!found) {
                roh.column_values.push_back(tab_ptr->column_datas[i].default_value); }

            if (found) {
                object* value = values[id];

                object* insertable = can_insert(value, tab_ptr->column_datas[i].data_type);
                if (insertable->type() == ERROR_OBJ) {
                    errors.push_back(insertable->data());
                    eval_push_error_return("INSERT INTO: Value (" + value->inspect() + ") evaluated to non-insertable value while inserting rows"); }

                roh.column_values.push_back(insertable->data()); 
            }

        }
    } else {
        size_t i = 0;
        for (i = 0; i < info->values->size(); i++) {

            object* value = values[i];

            object* insertable = can_insert(value, tab_ptr->column_datas[i].data_type);
            if (insertable->type() == ERROR_OBJ) {
                errors.push_back(insertable->data());
                eval_push_error_return("INSERT INTO: Value (" + value->inspect() + ") evaluated to non-insertable value while inserting rows"); }

            roh.column_values.push_back(insertable->data());
        }

        for (; i < tab_ptr->column_datas.size(); i++) {
            roh.column_values.push_back(tab_ptr->column_datas[i].default_value); }
    }

    if (!errors.empty()) {
        return;}

    tab_ptr->rows.push_back(roh);
}



static object* can_insert(object* insert_obj, SQL_data_type_object* data_type) {

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