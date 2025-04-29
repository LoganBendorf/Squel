
#include "evaluator.h"
#include "node.h"
#include "structs_and_macros.h"
#include "helpers.h"
#include "object.h"
#include "environment.h"

#include <iostream>
#include <vector>



extern std::vector<std::string> errors;
extern std::vector<table> tables;
extern display_table display_tab;

static std::vector<node*> nodes;

static void eval_function(function* func, environment* env);
static object* eval_run_function(function_call_object* func_call, environment* env);

static void eval_alter_table(alter_table* info, environment* env);
static void eval_create_table(const create_table* info);
static void eval_select_from(select_from* info);
static void print_table(table tab);
static void eval_insert_into(const insert_into* info, environment* env);

static object* eval_expression(object* expression, environment* env);

#define eval_push_error_return(x)               \
        std::string error = x;                  \
        errors.push_back(error);                \
        return                                  \

#define push_error_return_error_object(x)       \
    std::string error = x;                  \
    errors.push_back(error);                \
    return new error_object();

environment* eval_init(std::vector<node*> nds) {
    nodes = nds;
    return new environment();
}

void eval(environment* env) {
    
    for (int i = 0; i < nodes.size(); i++) {

        switch(nodes[i]->type()) {
            case INSERT_INTO_NODE:
                eval_insert_into(static_cast<insert_into*>(nodes[i]), env);
                printf("EVAL INSERT INTO CALLED\n");
                break;
            case SELECT_FROM_NODE:
                eval_select_from(static_cast<select_from*>(nodes[i]));
                printf("EVAL SELECT FROM CALLED\n");
                break;
            case CREATE_TABLE_NODE:
                eval_create_table(static_cast<create_table*>(nodes[i]));
                printf("EVAL CREATE TABLE CALLED\n");
                break;
            case ALTER_TABLE_NODE:
                eval_alter_table(static_cast<alter_table*>(nodes[i]), env);
                printf("EVAL ALTER TABLE CALLED\n");
                break;
            case FUNCTION_NODE:
                eval_function(static_cast<function*>(nodes[i]), env);
                printf("EVAL FUNCTION CALLED\n");
                break;
            default:
                eval_push_error_return("eval: unknown node type (" + nodes[i]->inspect() + ")");

        }

    }
}

static void eval_function(function* func, environment* env) {

    std::vector<parameter_object*> evaluated_parameters;
    for (const auto& param : func->func->parameters) {
        object* evaluated_param = eval_expression(param, env);
        if (evaluated_param->type() == ERROR_OBJ) {
            return; }

        if (evaluated_param->type() != PARAMETER_OBJ) {
            errors.push_back("Function parameter failed to evaluate to parameter object");
            return;
        }

        evaluated_parameters.push_back(static_cast<parameter_object*>(evaluated_param));
    }

    evaluated_function_object* evaluated_func = new evaluated_function_object(func->func, evaluated_parameters);

    env->add_function(evaluated_func);

    std::cout << "!! PRINTING LE FUNCTION !!\n\n";

    std::cout << evaluated_func->inspect() << std::endl;
}

static bool is_statement(object* obj) {
    switch(obj->type()) {
    case IF_STATEMENT: case BLOCK_STATEMENT: case END_IF_STATEMENT: case END_STATEMENT: case RETURN_STATEMENT:
        return true;
    default:
        return false;
    }
}


static object* eval_infix_expression(operator_object* op, object* left, object* right) {

    switch (op->op_type) {
        case ADD_OP:
            if (is_numeric_object(left) && is_numeric_object(right)) {
                return new integer_object(static_cast<integer_object*>(left)->value + static_cast<integer_object*>(right)->value);
            } 
            else if (is_string_object(left) && is_string_object(right)) {
                return new string_object(left->data() + right->data());
            } 
            else {
                std::string error_str = "No infix " + op->inspect() + " operation for " + left->inspect() + " and " + right->inspect();
                push_error_return_error_object(error_str);
            }
            break;
        case SUB_OP:
            if (!is_numeric_object(left) || !is_numeric_object(right)) {
                std::string error_str = "No infix " + op->inspect() + " operation for " + left->inspect() + " and " + right->inspect();
                push_error_return_error_object(error_str);}
            return new integer_object(static_cast<integer_object*>(left)->value - static_cast<integer_object*>(right)->value);
            break;
        case MUL_OP:
            if (!is_numeric_object(left) || !is_numeric_object(right)) {
                std::string error_str = "No infix " + op->inspect() + " operation for " + left->inspect() + " and " + right->inspect();
                push_error_return_error_object(error_str);}
            return new integer_object(static_cast<integer_object*>(left)->value * static_cast<integer_object*>(right)->value);
            break;
        case DIV_OP:
            if (!is_numeric_object(left) || !is_numeric_object(right)) {
                std::string error_str = "No infix " + op->inspect() + " operation for " + left->inspect() + " and " + right->inspect();
                push_error_return_error_object(error_str);}
            return new integer_object(static_cast<integer_object*>(left)->value / static_cast<integer_object*>(right)->value);
            break;
        case DOT_OP:
            if (left->type() != INTEGER_OBJ || left->type() != INTEGER_OBJ) {
                std::string error_str = "No infix " + op->inspect() + " operation for " + left->inspect() + " and " + right->inspect();
                push_error_return_error_object(error_str);}
            return new decimal_object(left->data() + "." + right->data());
            break;
        case LESS_THAN_OP:
            if (left->type() != INTEGER_OBJ || left->type() != INTEGER_OBJ) {
                std::string error_str = "No infix " + op->inspect() + " operation for " + left->inspect() + " and " + right->inspect();
                push_error_return_error_object(error_str);}
                return new boolean_object(static_cast<integer_object*>(left)->value < static_cast<integer_object*>(right)->value);
            break;
        default:
            push_error_return_error_object("No infix " + op->inspect() + " operator known");
    }
}


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
        // Basic stuff end

        case FUNCTION_CALL_OBJ:
            return eval_run_function(static_cast<function_call_object*>(expression), env); break;

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

            if (condition->left->type() == STRING_OBJ) {
                object* var = env->get_variable(condition->left->data());
                if (var->type() == ERROR_OBJ) {
                    return var; }

                condition->left = eval_expression(var, env);
            }

            if (condition->right->type() == STRING_OBJ) {
                object* var = env->get_variable(condition->right->data());
                if (var->type() == ERROR_OBJ) {
                    return var; }

                condition->right = eval_expression(var, env);
            }

            object* result = eval_infix_expression(condition->op, condition->left, condition->right);
            if (result->type() == ERROR_OBJ) {
                return result; }
            
            if (result->type() != BOOLEAN_OBJ) {
                errors.push_back("Condition evaluation resulted in non-boolean");
                return new error_object();
            }

            return result;
        } break;

        case IF_STATEMENT: {
            if_statement* statement = static_cast<if_statement*>(expression);

            object* obj = eval_expression(statement->condition, env); // LOWEST or PREFIX??
            if (obj->type() == ERROR_OBJ) {
                return obj; }

            if (obj->type() != BOOLEAN_OBJ) {
                errors.push_back("If statement condition returned non-boolean");
                return new error_object();
            }

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
            errors.push_back("eval_expression(): Cannot evaluate expression " + expression->inspect());
            return new error_object();
    }

    return new error_object();
}

std::vector<argument_object*> name_arguments(evaluated_function_object* function, function_call_object* func_call, environment* env) {

    std::vector<argument_object*> named_arguments;

    std::vector<object*> evaluated_arguments;
    for (const auto& arg : func_call->arguments) {
        object* evaulted_arg = eval_expression(arg, env);
        if (evaulted_arg->type() == ERROR_OBJ) {
            errors.push_back("Failed to evaluate argument");
            return named_arguments; 
        }
        evaluated_arguments.push_back(evaulted_arg);
    }

    if (evaluated_arguments.size() != function->parameters.size()) {
        errors.push_back("Function called with incorrect number of arguments, got " + std::to_string(evaluated_arguments.size()) + " wanted " + std::to_string(function->parameters.size()));
        return named_arguments;
    }

    for (int i = 0; i < function->parameters.size(); i++) {
        std::string param_name = static_cast<parameter_object*>(function->parameters[i])->name;
        named_arguments.push_back(new argument_object(param_name, evaluated_arguments[i]));
    }

    return named_arguments;
}



static object* eval_run_function(function_call_object* func_call, environment* env) {

    if (!env->is_function(func_call->name)) {
        errors.push_back("Called non-existing function (" + func_call->name + ")");
        return new error_object();
    }
    
    object* env_func = env->get_function(func_call->name);
    if (env_func->type() == ERROR_OBJ) {
        errors.push_back("Function returned as error object (" + func_call->name + ")");
        return new error_object();
    }
    evaluated_function_object* function = static_cast<evaluated_function_object*>(env_func);

    int error_count = errors.size();
    std::vector<argument_object*> named_args = name_arguments(function, func_call, env);
    if (error_count < errors.size()) {
        return new error_object(); }


    
    if (named_args.size() != function->parameters.size()) {
        errors.push_back("Function called with incorrect number of arguments, got " + std::to_string(named_args.size()) + " wanted " + std::to_string(function->parameters.size()));
        return new error_object();
    }

    environment* function_env = new environment(env);
    bool ok = function_env->add_variables(named_args);
    if (!ok) {
        errors.push_back("Failed to add function arguments as variables to function environment"); 
        return new error_object();
    }

    for (const auto& line : function->expressions) {
        object* res = eval_expression(line, function_env);
        if (res->type() == ERROR_OBJ) {
            return res; }

        if (res->type() == RETURN_STATEMENT) {
            return eval_expression(static_cast<return_statement*>(res)->expression, env);
        }
        
    }

    errors.push_back("Failed to find return value");
    return new error_object();
}

static object* eval_column(column_object* col, environment* env) {
    object* data_type = eval_expression(col->data_type, env);
    if (data_type->type() == ERROR_OBJ) {
        return new error_object(); }

    if (data_type->type() != SQL_DATA_TYPE_OBJ) {
        errors.push_back("Column data type failed to evaluate to SQL data type object");
        return new error_object(); }

    object* default_value = eval_expression(col->default_value, env);
    if (default_value->type() == ERROR_OBJ) {
        return new error_object(); }

    if (default_value->type() != STRING_OBJ) {
        errors.push_back("Column default value failed to evaluate to string object");
        return new error_object(); }

    return new evaluated_column_object(col->name, static_cast<SQL_data_type_object*>(data_type), default_value->data());
}

static void eval_alter_table(alter_table* info, environment* env) {
    table* tab;
    bool tab_found = false;
    for (int i = 0; i < tables.size(); i++) {
        if (tables[i].name == info->table_name) {
            tab = &tables[i];
            tab_found = true;
            break;}
    }
    
    if (!tab_found) {
        errors.push_back("eval_select_from(): Table not found (" + info->table_name + ")");
        return;}

    switch (info->table_edit->type()) {
    case COLUMN_OBJ: {

        column_object* column_obj = static_cast<column_object*>(info->table_edit);

        for (int i = 0; i < tab->column_datas.size(); i++) {
            if (column_obj->name == tab->column_datas[i].field_name) {
                errors.push_back("eval_alter_table(): Table already contains column with name (" + column_obj->name + ")");
                return;
            }
        }

        object* obj = eval_column(column_obj, env);
        if (obj->type() == ERROR_OBJ) {
            errors.push_back("Failed to evaluated column"); return; }

        if (obj->type() != EVALUATED_COLUMN_OBJ) {
            errors.push_back("Failed to evaluated column, strange return value"); return; }

        evaluated_column_object* evaluated_col = static_cast<evaluated_column_object*>(obj);

        column_data col = {evaluated_col->name, evaluated_col->data_type, evaluated_col->default_value};
        tab->column_datas.push_back(col);
    } break;
    default:
        errors.push_back("eval_alter_table(): Table edit not supported");
        return;
    }
}


static void eval_create_table(const create_table* info) {

    for (int i = 0; i < tables.size(); i++) {
        if (tables[i].name == info->table_name) {
            errors.push_back("Table already exists");
            return;
        }
    }

    table tab;
    tab.name = info->table_name;
    tab.column_datas = info->column_datas;

    tables.push_back(tab);
}


static bool eval_condtion(operator_object* op, object* left, object* right, std::vector<column_data> columns, row tab_row) {

    // Search for column
    int column_index = -1;
    for (int i = 0; i < columns.size(); i++) {
        if (columns[i].field_name == left->inspect()) {
            column_index = i;
            break;
        }
    }

    std::string column_value = tab_row.column_values.at(column_index);

    // For when i actually start using types instead of std::string for everything
    // SQL_data_type_object* type = columns.at(column_index).data_type;


    switch (op->op_type) {
    case EQUALS_OP:
        if (column_value == right->data()) {
            return true;
            break;
        }

        std::cout << "CONDITION FAIL FOR (" + column_value + ", " + right->data() + ")" << std::endl;

        // errors.push_back("eval_select_from(): Unknown = operation for " + left->inspect() + ", " + right->inspect());
        return false;
        break;
    case GREATER_THAN_OP:
        if (std::stoi(column_value) > std::stoi(right->data())) {
            return true;
            break;
        }
        return false;
        break;
    case LESS_THAN_OP:
        if (std::stoi(column_value) < std::stoi(right->data())) {
            return true;
            break;
        }
        return false;
        break;
    default:
        errors.push_back("eval_select_from(): Unknown condition operator, (" + op->inspect() + ")");
        return false;
    }
}

static void eval_select_from(select_from* info) {

    table tab;
    bool found = false;
    for (int i = 0; i < tables.size(); i++) {
        if (tables[i].name == info->table_name) {
            tab = tables[i];
            found = true;
            break;}
    }
    
    if (!found) {
        errors.push_back("eval_select_from(): Table not found (" + info->table_name + ")");
        return;}

    if (info->asterisk == true) {
        for (int i = 0; i < tab.column_datas.size(); i++) {
            info->column_names.push_back(tab.column_datas[i].field_name); }
    }
    


    std::vector<int> row_ids;
    if (info->condition->type() != NULL_OBJ) {

        if (info->condition->type() != INFIX_EXPRESSION_OBJ) {
            errors.push_back("Unsupported condition type, (" + info->condition->inspect() + ")");
            return;
        }

        infix_expression_object* condition = static_cast<infix_expression_object*>(info->condition);


        operator_object* op = condition->op;
        object* left = condition->left;
        object* right = condition->right;

        for (int i = 0; i < tab.rows.size(); i++) {
            int error_count = errors.size();

            bool should_add = eval_condtion(op, left, right, tab.column_datas, tab.rows[i]);

            if (errors.size() > error_count) {
                return;}
            
            if (should_add) {
                row_ids.push_back(i);
            }
        }

    } else {
        for (int i = 0; i < tab.rows.size(); i++) {
            row_ids.push_back(i);
        }
    }

        


    display_tab.to_display = true;
    display_tab.tab = tab;
    display_tab.column_names = info->column_names;
    display_tab.row_ids = row_ids;

    print_table(tab); // CMD line print, QT will do it's own thing in main

}

static void print_table(table tab) {

    std::cout << tab.name << ":\n";
    // for (int i = 0; i < tab.column_datas.size(); i++) {
    //     std::string name = tab.column_datas[i].field_name;
    //     std::string to_print = name.substr(0, 8);
    //     int pad_length = 8 - to_print.length();
    //     std::string pad(pad_length, ' ');
    //     std::string out_string = to_print + pad + " | ";
    //     std::cout << out_string;
    // }

    std::string field_names = "";
    std::vector<int> row_indexes;
    int j = 0;
    for (int i = 0; i < display_tab.column_names.size(); i++) {
        while (tab.column_datas[j].field_name != display_tab.column_names[i]) {
            j++;}
        row_indexes.push_back(j);

        std::string full_name = tab.column_datas[j].field_name;

        std::string name = full_name.substr(0, 10);
        int pad_length = 10 - name.length();
        std::string pad(pad_length, ' ');
        name += pad + " | ";

        field_names += name;
    }

    std::cout << field_names <<std::endl;

    // for (int i = 0; i < tab.rows.size(); i++) {
    //     for(int j = 0; j < tab.rows[0].column_values.size(); j++) {
    //         std::string name = tab.rows[i].column_values[j];
    //         std::string to_print = name.substr(0, 8);
    //         int pad_length = 8 - to_print.length();
    //         std::string pad(pad_length, ' ');
    //         std::string out_string = to_print + pad + " | ";
    //         std::cout << out_string;
    //     }
    //     std::cout << std::endl;
    // }

    for (int i = 0; i < tab.rows.size(); i++) {
        std::string row_values = "";
        for (int j = 0; j < row_indexes.size(); j++) {

            std::string full_name = tab.rows[i].column_values[j];

            std::string name = full_name.substr(0, 10);
            int pad_length = 10 - name.length();
            std::string pad(pad_length, ' ');
            name += pad + " | ";

            row_values += name;
        }
        std::cout << row_values << std::endl;
    }

    std::cout << std::endl;
}

static void eval_insert_into(const insert_into* info, environment* env) {

    table* tab_ptr;
    bool table_found = false;
    for (int i = 0; i < tables.size(); i++) {
        if (tables[i].name == info->table_name) {
            tab_ptr = &tables[i];
            table_found = true;
            break;}
    }

    if (info->values.size() == 0) {
        eval_push_error_return("INSERT INTO, no values");}

    if (info->fields.size() < info->values.size()) {
        eval_push_error_return("INSERT INTO, more values than field names");}

    if (info->fields.size() > tab_ptr->column_datas.size()) {
        eval_push_error_return("INSERT INTO, more field names than table has columns");}

    if (!table_found) { // More important errors should go last so basic mistakes are fixed first???
        eval_push_error_return("INSERT INTO, table not found");}

    row roh;
    int j = 0;
    if (info->fields.size() > 0) {
        for (int i = 0; i < tab_ptr->column_datas.size(); i++) {

            object* obj = eval_expression(info->fields[j], env);
            if (obj->type() == ERROR_OBJ) {
                return; }

            if (obj->type() != STRING_OBJ) {
                errors.push_back("INSERT INTO: Field evaluated to non-string value"); return; }

            std::string name = obj->data();

            if (tab_ptr->column_datas[i].field_name == name) {
                object* value = eval_expression(info->values[j], env);
                if (value->type() == ERROR_OBJ) {
                    errors.push_back("eval_insert_into(): Failed to evaluate (" + info->values[j]->inspect() + ") while inserting rows"); return; }
                    
                if (!can_insert(value, tab_ptr->column_datas[i].data_type)) {
                    errors.push_back("eval_insert_into(): (" + info->values[j]->inspect() + ") evaluated to non-insertable value while inserting rows"); return; }

                roh.column_values.push_back(value->data());
                j++;
                continue;
            }
            roh.column_values.push_back(tab_ptr->column_datas[i].default_value);
        }
    } else {
        for (int i = 0; i < tab_ptr->column_datas.size(); i++) {
            if (j < info->values.size()) {
                object* value = eval_expression(info->values[j], env);
                if (value->type() == ERROR_OBJ) {
                    errors.push_back("eval_insert_into(): Failed to evaluate (" + info->values[j]->inspect() + ") while inserting rows"); return; }
                    
                if (!can_insert(value, tab_ptr->column_datas[i].data_type)) {
                    errors.push_back("eval_insert_into(): (" + info->values[j]->inspect() + ") evaluated to non-insertable value while inserting rows"); return; }

                roh.column_values.push_back(value->data());
                j++;
                continue;
            }
            roh.column_values.push_back(tab_ptr->column_datas[i].default_value);
        }
    }

    if (j < info->values.size()) {
        eval_push_error_return("INSERT INTO, field name (" + info->fields[j]->inspect() + ") not found"); }

    if (j > info->values.size()) {
        eval_push_error_return("eval_insert_into(): werid bug");}

    if (!errors.empty()) {
        return;}

    tab_ptr->rows.push_back(roh);
}
