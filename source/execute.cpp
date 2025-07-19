#include "pch.h"

#include "execute.h"

#include "helpers.h"
#include "structs.h"
#include "allocator_aliases.h"
#include "object.h"
#include "node.h"
#include "environment.h"
#include "macros.h"
#include "hash.h"
#include "token.h"
#include "logger.h"

extern logger<error_msg> sql_errors;
extern logger<warning_msg> sql_warnings;

extern display_table display_tab;

extern avec<table_cache> table_caches;
extern avec<SP<e_function_object>> g_functions;

extern bool DEBUG;

static avec<UP<e_node>> nodes;


#define log_sql_error(x)                               \
    do {                                               \
        std::stringstream sql_err;                     \
        sql_err << x;                                  \
        sql_errors.add_msg(sql_err.str(), CUR_LOC);    \
    } while(0)                

#define exec_push_err_ret(x)                           \
    do {                                               \
        std::stringstream err;                         \
        err << x;                                      \
        sql_errors.add_msg(err.str(), CUR_LOC);        \
        return;                                        \
    } while(0)                   

#define push_err_ret_eval_err(x)                       \
    do {                                               \
        std::stringstream err;                         \
        err << x;                                      \
        sql_errors.add_msg(err.str(), CUR_LOC);        \
        return UP<evaluated>(new error_object());      \
    } while(0)

#define push_err_ret_ser_err(x)                        \
    do {                                               \
        std::stringstream err;                         \
        err << x;                                      \
        sql_errors.add_msg(err.str(), CUR_LOC);        \
        return UP<serializable>(new error_object());   \
    } while(0)


#define push_err_ret_unx_err_obj(x)                    \
    do {                                               \
        std::stringstream err;                         \
        err << x;                                      \
        sql_errors.add_msg(err.str(), CUR_LOC);        \
        return std::unexpected(MAKE_UP(error_object)); \
    } while(0)


static void exec_create_table(UP<e_create_table>     info,    SP<s_environment> env);
static void exec_alter_table (UP<e_alter_table_node> info,    SP<s_environment> env);
static void exec_insert_into (UP<e_insert_into>      wrapper, SP<s_environment> env);
static void exec_select_from (UP<e_select_from_node> wrapper, SP<s_environment> env);
static void exec_assert      (UP<e_assert_node>      node,    SP<s_environment> env); 

static UP<serializable> exec_infix_expression(operator_object*    op, serializable*    left, serializable*    right, SP<s_environment> env);
static UP<serializable> exec_infix_expression(UP<operator_object> op, UP<serializable> left, UP<serializable> right, SP<s_environment> env);

static std::expected<UP<serializable>, UP<error_object>> get_insertable(UP<serializable> insert_obj, const UP<s_SQL_data_type_object>& data_type);

static std::pair<table_cache*, bool>            get_table_cache   (const std_and_astring_variant& name);
static std::pair<SP<table_object>, bool>        get_table         (const std_and_astring_variant& name);
static std::pair<const SP<table_object>&, bool> get_table_as_const(const std_and_astring_variant& name);

[[maybe_unused]] static std::expected<UP<s_SQL_data_type_object>, UP<error_object>> assume_data_type(UP<serializable> obj);

enum ret_code : std::uint8_t{
    SUCCESS, FAIL, ERROR
};
[[maybe_unused]] static std::pair<UP<serializable>, ret_code> convert_table_to_value (const SP<table_object>& tab);
static std::pair<UP<serializable>, ret_code> convert_table_info_to_value(UP<f_table_info_object> info);


void execute_init(avec<UP<e_node>> nds) {
    nodes       = std::move(nds);
}

void execute() {
    
    SP<s_environment> env = MAKE_SP(s_environment);
    
    for (auto& node : nodes) {

        switch(node->type()) {
        case E_INSERT_INTO_NODE:
            if (DEBUG) [[unlikely]] { std::cout << "EXECUTE INSERT INTO CALLED\n"; }
            exec_insert_into(CAST_UP(e_insert_into, node), env);
            break;
        // case E_SELECT_NODE: {
        //     UP<evaluated> unwrapped = std::move(CAST_UP(e_select_node, node)->value);
        //     if (unwrapped->type() != E_SELECT_OBJ) {
        //         log_sql_error("Select node contained errors object"); break; }

        //     UP<e_select_object> sel_obj = CAST_UP(e_select_object, unwrapped);
        //     auto result = execute_select(std::move(sel_obj), env);
        //     std::cout << "EXECUTE SELECT CALLED\n";
        // } break;
        case E_SELECT_FROM_NODE: {
            if (DEBUG) [[unlikely]] { std::cout << "EXECUTE SELECT FROM CALLED\n"; }
            exec_select_from(CAST_UP(e_select_from_node, node), env);
        } break;
        case E_CREATE_TABLE_NODE:
            // std::cout << "EXECUTE CREATE TABLE CALLED\n";
            exec_create_table(CAST_UP(e_create_table, node), env);
            break;
        case E_ALTER_TABLE_NODE:
            if (DEBUG) [[unlikely]] { std::cout << "EXECUTE ALTER TABLE CALLED\n"; }
            exec_alter_table(CAST_UP(e_alter_table_node, node), env);
            break;
        case E_ASSERT_NODE:
            if (DEBUG) [[unlikely]] { std::cout << "EXECUTE ASSERT CALLED" << std::endl; }
            exec_assert(CAST_UP(e_assert_node, node), env);
            break;
        default:
            log_sql_error("execute: unknown node type (" + node->inspect() + ")");
        }
    }

    nodes.clear();
    nodes = avec<UP<e_node>>();
}

// TODO Needs to be reworked
// static UP<evaluated> eval_run_function(UP<function_call_object> func_call, SP<s_environment> env) {

//     if (func_call->name == "COUNT") {
//         avec<UP<object>> args = std::move(func_call->arguments->elements);
//         avec<UP<evaluated>> e_args;
//         e_args.reserve(args.size());
//         for (auto& arg : args) {
//             UP<evaluated> e_arg = eval_expression(std::move(arg), env);
//             if (e_arg->type() == ERROR_OBJ) {
//                 push_err_ret_eval_err_obj("Failed to evaluate (" + arg->inspect() + ")"); }
//             e_args.emplace_back(std::move(e_arg));
//         }
//         return UP<evaluated>(new s_function_call_object("COUNT", MAKE_UP(e_group_object, std::move(e_args))));
//     }

//     bool found = false;
//     for (const auto& func : g_functions) {
//         if (func->name == func_call->name) {
//             found = true; 
//         }
//     }

    // if (!found && !env->is_function(func_call->name)) {
    //     push_err_ret_eval_err_obj("Called non-existent function (" + func_call->name + ")"); }
    


//     auto&& [function, exists] = env->get_function(func_call->name);
//     if (!exists) {
//         push_err_ret_eval_err_obj("Function does not exist (" + func_call->name + ")"); }

//     size_t error_count = errors.size();
//     avec<UP<e_argument_object>> named_args = name_arguments(function, std::move(func_call), env);
//     if (error_count < errors.size()) {
//         return UP<evaluated>(new error_object()); }



//     const avec<UP<e_parameter_object>>& parameters = function->parameters;
//     if (named_args.size() != parameters.size()) {
//         push_err_ret_eval_err_obj("Function called with incorrect number of arguments, got " << named_args.size() << " wanted " << parameters.size()); }

//     SP<s_environment> function_env = MAKE_SP(s_environment, env);
//     bool ok = function_env->add_variables(std::move(named_args));
//     if (!ok) {
//         push_err_ret_eval_err_obj("Failed to add function arguments as variables to function s_environment"); }

//     for (const auto& line : function->body->body) {
//         UP<evaluated> res = eval_expression(UP<object>(line->clone()), function_env);
//         if (res->type() == ERROR_OBJ) {
//             return res; }

//         /* I think it's just this */
//         if (res->type() == RETURN_STATEMENT) {
//             return res;
//         }

//         /* Not sure this is needed */
//         // if (res->type() == RETURN_STATEMENT) {
//         //     return eval_expression(std::move(CAST_UP(return_statement, res)->expression), env);
//         // }
        
//     }

//     push_err_ret_eval_err_obj("Failed to find return value");
// }


static void exec_assert(UP<e_assert_node> node, SP<s_environment> env) {

    UP<e_assert_object> info = std::move(node->value);

    UP<serializable> expr = make_serializable(UP<evaluated>(info->expression->clone()), env);
    if (expr->type() == ERROR_OBJ) {
        exec_push_err_ret("Failed to evaluate ASSERT expression"); }
    if (expr->type() != BOOLEAN_OBJ) {
        exec_push_err_ret("ASSERT expression failed to evaluate to a boolean"); }

    UP<boolean_object> boolean = CAST_UP(boolean_object, expr);

    if (!boolean->value) {
        exec_push_err_ret("ASSERT failed (Line: " << info->line << ", Expression: " << info->inspect() << ")"); }
}

UP<serializable> make_serializable(UP<evaluated> object, SP<s_environment> env) {
    if (is_serializable(object)) {
        return CAST_UP(serializable, object); }

    switch (object->type()) {
        case E_DEFAULT_VALUE_FUNC_OBJ: {
            auto obj = CAST_UP(e_default_value_func, object);

            auto parameter = make_serializable(std::move(obj->parameter), env);
            if (parameter->type() == ERROR_OBJ) {
                push_err_ret_ser_err("Failed to make default value function parameter serializable"); }
            
            return UP<serializable>(new s_default_value_func(std::move(parameter)));

        } break;
        case E_DEFAULT_VALUE_OBJ: {
            auto obj = CAST_UP(e_default_value_object, object);
            auto value = make_serializable(std::move(obj->value), env);
            if (value->type() == ERROR_OBJ) {
                push_err_ret_ser_err("Failed to make default value serializable"); }

            return UP<serializable>(new s_default_value_object(std::move(value)));

        } break;
        case E_UNIQUE_OBJ: {
            auto obj = CAST_UP(e_unique_object, object);
            auto group = make_serializable(CAST_UP(evaluated, obj->group), env);
            if (group->type() == ERROR_OBJ) {
                push_err_ret_ser_err("Failed to make UNIQUE elements serializable"); }
            if (group->type() != S_GROUP_OBJ) {
                push_err_ret_ser_err("UNIQUE elements failed to be made serializable"); }

            return UP<serializable>(new s_unique_object(CAST_UP(s_group_object, group)));

        } break;
        case CURRENT_TIMESTAMP_OBJ: 
            return CAST_UP(serializable, object);
        case AUTO_INCREMENT_OBJ: 
            return CAST_UP(serializable, object);
        case E_FOREIGN_KEY_OBJ: {
            auto for_key_obj = CAST_UP(e_foreign_key_object, object);
            auto reference = make_serializable(CAST_UP(evaluated, for_key_obj->reference), env);
            if (reference->type() == ERROR_OBJ) {
                push_err_ret_ser_err("FOREIGN_KEY: Failed to parse reference"); }
            if (reference->type() != S_COLUMN_INDEX_OBJ) {
                push_err_ret_ser_err("FOREIGN_KEY: Reference failed to serialize to E Column Index Object"); }

            auto key_obj = MAKE_UP(s_foreign_key_object, CAST_UP(s_column_index_object, reference));
            return CAST_UP(serializable, key_obj);
        } break;
        case E_HASH_OBJ: {
            auto hash = CAST_UP(e_hash_object, object);
            auto ref = make_serializable(CAST_UP(evaluated, hash->reference), env);
            if (ref->type() == ERROR_OBJ) {
                push_err_ret_ser_err("Failed to make hash reference serializable"); }
            if (ref->type() != S_GROUP_OBJ) {
                push_err_ret_ser_err("Hash reference failed to become S Group Object, got (" << object_type_to_astring(ref->type()) << ")"); }

            auto group = CAST_UP(s_group_object, ref);
            if (group->elements.size() == 0) {
                push_err_ret_ser_err("HASH: Must contain at least one element"); }

            return UP<serializable>(new s_hash_object(std::move(group)));

        } break;
        case E_CONSTRAINT_OBJ: {
            auto obj = CAST_UP(e_constraint_object, object);
            auto constraint = make_serializable(std::move(obj->constraint), env);
            if (constraint->type() == ERROR_OBJ) {
                push_err_ret_ser_err("Failed to make constraint serializable"); }

            token_type method = obj->method;
            return UP<serializable>(new s_constraint_object(std::move(constraint), method));

        } break;
        case E_TABLE_EXPR_OBJ: {
            auto obj = CAST_UP(e_table_expr, object);
            auto parameter = make_serializable(std::move(obj->parameter), env);
            if (parameter->type() == ERROR_OBJ) {
                push_err_ret_ser_err("Failed to make table expression parameter serializable"); }

            return UP<serializable>(new s_table_expr(std::move(parameter)));

        } break;
        case E_TABLE_COLUMN_EXPR_OBJ: {
            auto obj = CAST_UP(e_table_column_expr, object);
            auto parameter = make_serializable(std::move(obj->parameter), env);
            if (parameter->type() == ERROR_OBJ) {
                push_err_ret_ser_err("Failed to make table coulumn expression parameter serializable"); }

            return UP<serializable>(new s_table_column_expr(std::move(parameter)));

        } break;
        case E_GROUP_OBJ: {
            auto e_group_obj = CAST_UP(e_group_object, object);
            avec<UP<evaluated>>    e_elements = std::move(e_group_obj->elements);
            avec<UP<serializable>> s_elements;
            s_elements.reserve(e_elements.size());
            for (auto& element : e_elements) {
                auto ser = make_serializable(std::move(element), env);
                if (ser->type() == ERROR_OBJ) {
                    return ser; }
                s_elements.push_back(std::move(ser));
            }
            return UP<serializable>(new s_group_object(std::move(s_elements)));

        } break;
        case E_PARAMETER_OBJ: {
            auto e_param_obj = CAST_UP(e_parameter_object, object);
            auto name = e_param_obj->name;
            auto values = make_serializable(CAST_UP(evaluated, e_param_obj->values), env);
            if (values->type() == ERROR_OBJ) {
                push_err_ret_ser_err("Could not make E Parameter Object values serializable"); }
            if (values->type() != S_GROUP_OBJ) {
                push_err_ret_ser_err("E Parameter Object values failed to be made into S Group Object"); }
            UP<s_group_object> group_obj = CAST_UP(s_group_object, values);
            auto& vec = group_obj->elements;
            // Verify
            for (const auto& value : vec) {
                if (!is_serializable(value)) {
                    FATAL_ERROR_THROW("Bruh", CUR_LOC); }
            }
            return UP<serializable>(new s_parameter_object(name, std::move(group_obj)));

        } break;
        case E_COLUMN_INDEX_OBJ: {
            auto obj = CAST_UP(e_column_index_object, object);

            UP<serializable> column_name_obj = make_serializable(std::move(obj->column_name), env);
            if (column_name_obj->type() == ERROR_OBJ) {
                push_err_ret_ser_err("Failed to evaluate column index column name"); }

            return UP<serializable>(new s_column_index_object(obj->table_name, std::move(column_name_obj)));

        } break;
        case F_TABLE_INFO_OBJ: {
            auto obj = CAST_UP(f_table_info_object, object);
            if (obj->row_ids.size() != 1) {
                push_err_ret_ser_err("Table info with row size != 1 could not be serialized (" + obj->inspect() + ")"); }
            if (obj->col_ids.size() != 1) {
                push_err_ret_ser_err("Table info with column size != 1 could not be serialized (" + obj->inspect() + ")"); }

            const auto& [cell, not_needed] = obj->table->get_cell_value(0, 0);
            return UP<serializable>(cell->clone());
            
        }  break;
        case E_INFIX_EXPRESSION_OBJ: {
            auto obj = CAST_UP(e_infix_expr_object, object);
            auto left  = make_serializable(CAST_UP(evaluated, obj->left), env);
            if (left->type() == ERROR_OBJ) {
                push_err_ret_ser_err("Failed to make infix left serializable"); }
            auto right = make_serializable(CAST_UP(evaluated, obj->right), env);
            if (right->type() == ERROR_OBJ) {
                push_err_ret_ser_err("Failed to make infix right serializable"); }
            return exec_infix_expression(std::move(obj->op), std::move(left), std::move(right), env);

        } break;
        case E_TABLE_DETAIL_OBJ: {
            UP<e_table_detail_object> obj = CAST_UP(e_table_detail_object, object);

            astring name = obj->name;

            UP<serializable> data_type = make_serializable(CAST_UP(evaluated, obj->data_type), env);
            if (data_type->type() == ERROR_OBJ) {
                return data_type; 
            } else if (data_type->type() != S_SQL_DATA_TYPE_OBJ) {
                push_err_ret_ser_err("E SQL Data Type Object failed to become serializable"); } 
            
            // TODO Make sure default value fits with the data type
            std::optional<UP<s_default_value_object>> default_value;
            if (obj->default_value.has_value()) {
                auto ser = make_serializable(CAST_UP(evaluated, obj->default_value.value()), env);
                if (ser->type() != S_DEFAULT_VALUE_OBJ) {
                    push_err_ret_ser_err("E_TABLE_DETAIL_OBJ default value failed to become serializable"); } 
                default_value = CAST_UP(s_default_value_object, ser);
            }

            avec<UP<s_table_column_expr>> exprs;
            for (auto& expr : obj->exprs) {
                auto ser = make_serializable(CAST_UP(evaluated, expr), env);
                if (ser->type() != S_TABLE_COLUMN_EXPR_OBJ) {
                    push_err_ret_ser_err("E_TABLE_DETAIL_OBJ column expression failed to become serializable"); } 

                exprs.push_back(CAST_UP(s_table_column_expr, ser));
            }

            return UP<serializable>(new s_table_detail_object(name, CAST_UP(s_SQL_data_type_object, data_type), std::move(default_value), std::move(exprs)));
            
        } break;
        case E_SQL_DATA_TYPE_OBJ: {
            UP<e_SQL_data_type_object> obj = CAST_UP(e_SQL_data_type_object, object);
            if (!obj->parameter.has_value()) {
                return UP<serializable>(new s_SQL_data_type_object(obj->prefix, obj->data_type)); }

            if (is_serializable(obj->parameter.value())) {
                return UP<serializable>(new s_SQL_data_type_object(obj->prefix, obj->data_type, CAST_UP(serializable, obj->parameter.value())));
            } else {
                push_err_ret_ser_err("SQL data type could not convert to serializable (" + obj->inspect() + ")");
            }

        } break;
        default: {
            push_err_ret_ser_err("Could not convert to serializable (Type: " << object_type_to_astring(object->type()) << ". " << object->inspect() << ")");
        }
    }
}


// FIXME Needs to work with table_detail.
static void exec_alter_table(UP<e_alter_table_node> info, [[maybe_unused]] SP<s_environment> env) {

    astring table_name = info->table_name;

    const auto& [table, tab_found] = get_table_as_const(table_name);
    if (!tab_found) {
        exec_push_err_ret("ALTER TABLE: Table not found");}

    SP<table_object> tab = table;

    UP<evaluated> table_edit = std::move(info->table_edit);

    switch (table_edit->type()) {
    case E_TABLE_DETAIL_OBJ: {} break;
    default:
        exec_push_err_ret("ALTER TABLE: Table edit (" + info->table_edit->inspect() + ") not supported");
    }
}




static UP<integer_object> integer_hash(const astring& blob) {

    return MAKE_UP(integer_object, raw_hash<int>(blob.c_str(), blob.size(), DEBUG));
}

#define exec_table_exprs_push_err_ret(x)                 \
    do {                                                 \
        std::stringstream err;                           \
        err << GET_ERROR_LOCATION(CUR_LOC) << ": " << x; \
        log_sql_error(std::move(err).str());             \
        return {false, false};                           \
    } while(0)                   

// returns [should_add, ok]
static std::pair<bool, bool> exec_table_exprs(const SP<table_object>& table, const avec<UP<serializable>>& row) {
    
    for (const auto& expr : table->exprs) {
        const auto& expr_type = expr->parameter->type();
        switch (expr_type) {
        case PRIMARY_KEY_OBJ: break;
        case DELIMITER_OBJ:   break;
        case S_CONSTRAINT_OBJ: {
            const auto& constraint_wrapper = CAST_UP_TO_CONST_RAW(s_constraint_object, expr->parameter);
            const auto& constraint_obj = CAST_UP_TO_CONST_RAW(serializable, constraint_wrapper->constraint);
            const auto& constraint_type = constraint_obj->type();
            const auto& constraint_method = constraint_wrapper->method;
            switch (constraint_type) {
            case S_UNIQUE_OBJ: {
                const s_unique_object* unq_obj = CAST_CONST_RAW_TO_CONST_RAW(s_unique_object, constraint_obj);
                const auto& group_obj = unq_obj->group;
                const auto& fields = group_obj->elements;

                avec<size_t> unique_indexes;
                for (const auto& field : fields) {
                    const auto& [index, ok] = table->get_column_index(field->data());
                    if (!ok) { FATAL_ERROR_THROW("INSERT INTO: Bad table constraint unique index, likely a CREATE TABLE error", CUR_LOC); }
                    unique_indexes.push_back(index);
                }

                // All have to be not-unique, currently fails if just one is not-unique

                avec<UP<serializable>> contained;
                contained.reserve(unique_indexes.size());
                bool unique_fail = false;
                for (const auto& table_row_obj : table->rows) {

                    const auto& table_row = table_row_obj->elements;

                    bool is_unique = false;
                    for (const auto& unique_index : unique_indexes) {
                        const auto& current_value = row[unique_index];
                        const auto& to_compare = table_row[unique_index];
                        // memcpr didn't work so this is the next best thing I guess. Maybe not so bad if I ever make a binary serialize
                        if (current_value->serialize() != to_compare->serialize()) { // Different
                            is_unique = true;
                            break;
                        }
                        contained.push_back(UP<serializable>(to_compare->clone()));
                    }

                    if (is_unique) {
                        contained.clear(); 
                    } else {
                        unique_fail = true;
                    }

                }

                if (unique_fail) {
                    std::stringstream ss;
                    ss << "Fields (" + group_obj->inspect() + ") were not UNIQUE in table (" + table->table_name + "). Values (";
                    bool first = true;
                    for (const auto& val : contained) {
                        if (!first) { ss << ", "; }
                        ss << val->inspect(); 
                        first = false;
                    }
                    ss << ")";
                    switch (constraint_method) {
                    case IGNORE: 
                        ss << " IGNORED";
                        break;
                    default: exec_table_exprs_push_err_ret("INSERT INTO: Unsupported table constraint method (" << token_type_to_string(constraint_method) << ")");
                    }
                    sql_warnings.add_msg(ss.str());
                    return {false, true};
                }

                
            } break;
            default: exec_table_exprs_push_err_ret("INSERT INTO: Unsupported table constraint (" << object_type_to_astring(constraint_type) << ")");
            }
        } break;
        default: exec_table_exprs_push_err_ret("INSERT INTO: Unsupported table expression (" << object_type_to_astring(expr_type) << ")");
        }
    }

    return {true, true};
}

// TODO For now fields and columns must be lined up together
static void exec_insert_into(UP<e_insert_into> wrapper, [[maybe_unused]] SP<s_environment> env) {

    if (wrapper->value->type() != E_INSERT_INTO_OBJ) { // Fatal error?
        exec_push_err_ret("exec_insert_into(): Called with invalid object (" + object_type_to_astring(wrapper->value->type()) + ")"); }

    UP<e_insert_into_object> info = CAST_UP(e_insert_into_object, wrapper->value);

    const astring table_name = info->table_name;

    auto [table_cache_ptr, tab_found] = get_table_cache(table_name);
    if (!tab_found) {
        exec_push_err_ret("INSERT INTO: table not found"); }
    const auto& table = table_cache_ptr->table;
    bool* dirty = &(table_cache_ptr->dirty); 

    avec<UP<evaluated>> fields = std::move(info->fields);

    avec<UP<evaluated>> values = std::move(info->values);
        
    if (values.size() == 0) {
        exec_push_err_ret("INSERT INTO: No values"); }

    if (values.size() == 0) {
        exec_push_err_ret("INSERT INTO: No field names"); }

    if (fields.size() < values.size()) {
        exec_push_err_ret("INSERT INTO: More values than field names");}

    if (fields.size() > values.size()) {
        exec_push_err_ret("INSERT INTO: More field names than values");}

    if (fields.size() > table->column_data.size()) {
        exec_push_err_ret("INSERT INTO: More field names than table has columns");}

    if (values.size() > table->column_data.size()) {
        exec_push_err_ret("INSERT INTO: More values than table has columns");}



    // Check if fields exist
    for (const auto& field : fields) {
        const bool found = table->check_if_field_name_exists(field->data());
        if (!found) {
            exec_push_err_ret("INSERT INTO: Could not find field (" + field->inspect() + ") in table + (" + table->table_name + ")"); }
    }

    // Serialize
    avec<UP<serializable>> row(table->column_data.size()); // Should default construct will nullptr
    for (size_t i = 0; i < values.size(); i++) {
        auto value = std::move(values[i]);
        auto field = std::move(fields[i]);

        auto ser_field = make_serializable(UP<evaluated>(field->clone()), env);
        if (ser_field->type() == ERROR_OBJ) {
            exec_push_err_ret("INSERT INTO: Failed to make field serializable. Type: " << field->type() << ", (" << field->inspect() << ")"); }
        if (ser_field->type() != STRING_OBJ) {
            exec_push_err_ret("INSERT INTO: Field failed to become String Object. Type: " << field->type() << ", (" << field->inspect() << ")"); }

        auto ser_val = make_serializable(UP<evaluated>(value->clone()), env);
        if (ser_val->type() == ERROR_OBJ) {
            exec_push_err_ret("INSERT INTO: Failed to make value serializable. Type: " << value->type() << ", (" << value->inspect() << ")"); }

        const auto& [column_index, ok] = table->get_column_index(ser_field->data());

        const auto& [data_type, in_bounds] = table->get_column_data_type(column_index);
        if (!in_bounds) {
            exec_push_err_ret("INSERT INTO: Coulumn index was not in bounds. Maybe error should be fatal");}
            
        auto result = get_insertable(std::move(ser_val), data_type);
        if (!result.has_value()) {
            exec_push_err_ret("INSERT INTO: Value (" + value->inspect() + ") evaluated to non-insertable value while inserting rows"); }

        UP<serializable> insertable = std::move(*result);

        // Check column expressions for constraints on insertable values
        for (const auto& col_expr_wrapper : table->column_data[i]->exprs) {
            const auto& col_expr = col_expr_wrapper->parameter;
            switch (col_expr->type()) {
            case S_FOREIGN_KEY_OBJ: {
                const auto& foreign_key_obj = CAST_UP_TO_CONST_RAW(s_foreign_key_object, col_expr);
                const auto& foreign_column_index = CAST_UP_TO_CONST_RAW(s_column_index_object, foreign_key_obj->reference);

                // Verify, there might be an issue in CREATE TABLE if one of these go off. The user might have also done something stupid with ALTER
                if (foreign_column_index->column_name->type() != STRING_OBJ) {
                    FATAL_ERROR_THROW("INSERT INTO: Foreign key' column index's column name was not a string object. Got (" << 
                                      object_type_to_astring(foreign_column_index->column_name->type()) << ") Likely an issue in CREATE TABLE", CUR_LOC); 
                }

                const auto& foreign_table_name = foreign_column_index->table_name;
                const auto& foreign_col_name = CAST_UP_TO_CONST_RAW(string_object, foreign_column_index->column_name)->data();

                const auto& [foreign_table, foreign_tab_found] = get_table(foreign_table_name);
                if (!foreign_tab_found) {
                    FATAL_ERROR_THROW("INSERT INTO: Foreign key' column index's table name was not a real table. Likely an issue in CREATE TABLE", CUR_LOC); }

                const bool foreign_col_found = foreign_table->check_if_field_name_exists(foreign_col_name);
                if (!foreign_col_found) {
                    FATAL_ERROR_THROW("INSERT INTO: Foreign key' column index's column name (" << foreign_col_name << ") was not found in table (" << foreign_table_name << ")", CUR_LOC); }

                const auto& [foreign_col_index, foreign_ok] = foreign_table->get_column_index(foreign_col_name);
                if (!foreign_ok) {
                    FATAL_ERROR_STACK_TRACE_THROW("INSERT INTO: IDK weird bug", CUR_LOC); }

                // Check if insertable exists in foreign values
                bool foreign_exists = false;
                for (const auto& foreign_row_obj : foreign_table->rows) {
                    const auto& foreign_row = foreign_row_obj->elements;
                    const auto& foreign_value = foreign_row[foreign_col_index];

                    if (sizeof(foreign_value) != sizeof(insertable)) { continue; }
                    // Memcmp cause lazy, idk if there's something better
                    if (memcmp(static_cast<void*>(foreign_value.get()), static_cast<void*>(insertable.get()), sizeof(foreign_value)) == 0) {
                        foreign_exists = true; break; }
                    
                }

                // If it doesn't exist error, else go on with life
                if (!foreign_exists) {
                    exec_push_err_ret("INSERT INTO: Foreign value for (" << insertable->inspect() << "), in foreign table (" << foreign_table_name << "), does not exist"); }


            } break;
            default:
                exec_push_err_ret("INSERT INTO: Unsupported column expression (" << object_type_to_astring(col_expr->type()) << ")"); 
            }
        }

        row[column_index] = std::move(insertable);
    }

    // For now only HASH is defered
    // Field indexes, row index 
    avec<std::pair<avec<size_t>, size_t>> deferred_hashes;

    // Check if non-inserted rows have default values, if they do, insert them, else error
    for (size_t i = 0; i < table->column_data.size(); i++) {
        
        if (row[i] != nullptr) { continue; }

        auto result = table->get_cloned_column_default_value(i);
        if (!result.has_value()) {
            exec_push_err_ret("INSERT INTO:" + result.error()->data()); }
        
        auto default_value_result = std::move(*result);
        if (!default_value_result.has_value()) {
            exec_push_err_ret("INSERT INTO: Field (" << table->column_data[i]->name << ") was not given a value and does not have a default value"); }

        UP<s_default_value_object> default_value_wrapper = std::move(*default_value_result);

        UP<serializable> default_value = std::move(default_value_wrapper->value);

        if (default_value->type() == S_DEFAULT_VALUE_FUNC_OBJ) {
            auto func = CAST_UP(s_default_value_func, default_value);
            switch (func->parameter->type()) {
            case S_HASH_OBJ: {
                auto hash_obj = CAST_UP(s_hash_object, func->parameter);
                auto reference_obj = std::move(hash_obj->reference);
                avec<UP<serializable>> references = std::move(reference_obj->elements);

                // Check if reference fields exist
                avec<size_t> reference_indexes;
                for (const auto& ref_field : references) {
                    if (ref_field->type() != STRING_OBJ) {
                        exec_push_err_ret("INSERT INTO: HASH field (Type: " << ref_field->type() << ", Value: " << ref_field->inspect() << " did not become String Object"); }

                    auto [index, found] = table->get_column_index(ref_field->data());
                    if (!found) {
                        exec_push_err_ret("INSERT INTO: Could not find HASH field (" + ref_field->inspect() + ") in table + (" + table->table_name + ")"); }

                    reference_indexes.emplace_back(index);
                }

                deferred_hashes.emplace_back(std::move(reference_indexes), i);

            } break;
            case CURRENT_TIMESTAMP_OBJ: {
                auto timestamp_obj = MAKE_UP(timestamp_object);
                default_value = CAST_UP(serializable, timestamp_obj);

            } break;
            case AUTO_INCREMENT_OBJ: {
                constexpr int integer_default = 0;
                auto row_result = table->get_row_vec_ptr(table->rows.size() - 1);
                if (!row_result.has_value()) {
                    auto insert_result = get_insertable(UP<serializable>(new integer_object(integer_default)), table->column_data[i]->data_type);
                    if (!insert_result.has_value()) {
                        exec_push_err_ret("INSERT INTO: Can not AUTO_INCREMENT type (" << table->column_data[i]->data_type->inspect() << ")"); }
                    
                    default_value = CAST_UP(serializable, *insert_result);
                    break;
                }

                const avec<UP<serializable>>& table_row = **row_result;
                const auto& previous = table_row[i];
                UP<serializable> to_add;
                switch (previous->type()) {
                case INTEGER_OBJ: {
                    auto prev_clone = UP<serializable>(previous->clone());
                    auto int_obj = CAST_UP(integer_object, prev_clone);
                    int_obj->value += 1;
                    to_add = CAST_UP(serializable, int_obj);

                } break;
                default:
                    exec_push_err_ret("INSERT INTO: Can not AUTO_INCREMENT type (" << previous->type() << ")"); 
                }          
                
                default_value = CAST_UP(serializable, to_add);

            } break;
            default: 
                exec_push_err_ret("INSERT INTO: Default value function (" << object_type_to_astring(func->parameter->type()) << ") not supported");
            }
        }

        row[i] = std::move(default_value);
    }


    // Deferred
    for (auto& hash_info : deferred_hashes) {

        const auto& [indexes, row_index] = hash_info;

        astringstream ss;
        for (const auto& index : indexes) {
            ss << row[index]->data(); }

        UP<serializable> hash_obj;
        const token_type data_type = table->column_data[row_index]->data_type->data_type;

        switch (data_type) {
        case INT: case INTEGER: {
            UP<integer_object> int_hash_obj = integer_hash(ss.str());
            hash_obj = CAST_UP(serializable, int_hash_obj);

        } break;
        default:
            exec_push_err_ret("INSERT INTO: Hash is unsupported for column type (" << token_type_to_string(data_type) << ")");
        }

        row[row_index] = std::move(hash_obj);
    }

    

    // Constraints
    const auto& [should_add, ok] = exec_table_exprs(table, row);
    if (!ok) {
        exec_push_err_ret("INSERT INTO: Failed to execute table expressions"); }

    if (!should_add) { // Not an error, some condition like UNIQUE failed to pass so don't insert
        return; }


    size_t row_size = 0;
    for (size_t i = 0; i < table->column_data.size(); i++) {
        if (row[i] != nullptr) { row_size++; continue; }
        log_sql_error("INSERT INTO: Could not get a value for field (" + table->column_data[i]->name + ")");
    }

    if (sql_errors.has_msgs()) {
        exec_push_err_ret("INSERT INTO: Failed"); }
    if (row_size < table->column_data.size()) {
        exec_push_err_ret("INSERT INTO: Not enough elements. Expected (" << table->column_data.size() << "), got (" << row_size << ")"); }
    if (row_size > table->column_data.size()) {
        exec_push_err_ret("INSERT INTO: Too many elements. Expected (" << table->column_data.size() << "), got (" << row_size << ")"); }

    table->rows.emplace_back(MAKE_UP(s_group_object, std::move(row)));
    *dirty = true;
}



#define param_to_tab_err(x)                                                  \
        std::stringstream err;                                               \
        err << GET_ERROR_LOCATION(CUR_LOC) << ": " << x;                     \
        return std::unexpected(MAKE_UP(error_object, std::move(err).str())); 

std::expected<UP<s_table_detail_object>, UP<error_object>> parameter_to_table_detail(UP<s_parameter_object> param_obj) {

    astring col_name = param_obj->name;
    auto values = std::move(param_obj->values->elements);
    if (values.size() == 0) {
        param_to_tab_err("Table column detail did not contain enough info"); }
    if (values.size() > 4) {
        param_to_tab_err("Table column detail contains too many items. For now at most 4, got " << values.size()); }

    auto data_type = std::move(values[0]);
    if (data_type->type() != S_SQL_DATA_TYPE_OBJ) {
        param_to_tab_err("Table column data type failed to become S_SQL_DATA_TYPE_OBJ"); }
    if (values.size() == 1) {
        auto detail = MAKE_UP(s_table_detail_object, col_name, CAST_UP(s_SQL_data_type_object, data_type));
        return std::move(detail);
    }

    std::optional<UP<s_default_value_object>> default_value;
    avec<UP<s_table_column_expr>> col_exprs;
    for (size_t i = 1; i < values.size(); i++) {
        auto value = std::move(values[i]);
        switch (value->type()) {
        case S_DEFAULT_VALUE_OBJ: {
            if (default_value.has_value()) {
                param_to_tab_err("Table column cannot have multiple DEFAULT values"); }

            default_value = CAST_UP(s_default_value_object, value);
        } break;
        case S_DEFAULT_VALUE_FUNC_OBJ: {
            if (default_value.has_value()) {
                param_to_tab_err("Table column cannot have multiple DEFAULT values"); }

            default_value = MAKE_UP(s_default_value_object, CAST_UP(serializable, value));
        } break;
        case S_TABLE_COLUMN_EXPR_OBJ: {
            col_exprs.push_back(CAST_UP(s_table_column_expr, value));
        } break;
        case S_TABLE_EXPR_OBJ: {
            FATAL_ERROR_THROW("Table expression are no longer allowed in parameter object, only table column expressions are allowed", CUR_LOC);
        } break;
        default: {
            param_to_tab_err("Table column unknown value. Type: " << object_type_to_astring(value->type()) << ", (" << value->inspect() << ")");
        }
        }
    }

    auto detail = MAKE_UP(s_table_detail_object, col_name, CAST_UP(s_SQL_data_type_object, data_type), 
                                        std::move(default_value), std::move(col_exprs));

    return std::move(detail);
}

static void exec_create_table(UP<e_create_table> info, SP<s_environment> env) {

    astring table_name = info->table_name;

    for (const auto& entry : table_caches) {
        if (entry.table->get_tab_name() == table_name) { // TODO Add cache for table names and read save file location for table names
            exec_push_err_ret("CREATE TABLE: Table (" + table_name + ") already exists"); }
    }

    avec<UP<evaluated>>    e_details = std::move(info->details->elements);
    avec<UP<serializable>> s_details;
    s_details.reserve(e_details.size()); // TODO Not sure should reserve
    for (auto& detail : e_details) {

        UP<serializable> val = make_serializable(std::move(detail), env);
        if (val->type() == ERROR_OBJ) {
            exec_push_err_ret("CREATE TABLE: Table detail failed to become serializable"); }

        s_details.push_back(std::move(val));
    }

    for (const auto& detail : s_details) {
        if (detail == nullptr) {
            FATAL_ERROR_THROW("TABLE DETAIL NULLPTR", CUR_LOC);
        }
    }

    // Verify types
    if (DEBUG) [[unlikely]] { std::cout << "Exec Create Table, types:\n"; }
    for (const auto& obj : s_details) {
         if (DEBUG) [[unlikely]] { std::cout << "\t" << object_type_to_astring(obj->type()) << std::endl; }
        switch (obj->type()) {
            case S_PARAMETER_OBJ: case S_TABLE_COLUMN_EXPR_OBJ: case S_TABLE_EXPR_OBJ: break;
            default: exec_push_err_ret("CREATE TABLE: Invalid object in table details. Type:" << object_type_to_astring(obj->type()) << ", (" << obj->inspect() << ")"); break;
        }
    }

    if (DEBUG) [[unlikely]] {
        std::cout << "\nExec Create Table, inspected:\n";
        for (const auto& obj : s_details) {
            std::cout << "\t" << obj->inspect() << std::endl;
        }
    }

    avec<UP<s_table_detail_object>> column_data;
    avec<UP<s_table_expr>> table_exprs;
    for (auto& obj : s_details) {
        switch (obj->type()) {
            case S_PARAMETER_OBJ: {
                auto result = parameter_to_table_detail(CAST_UP(s_parameter_object, obj));
                if (!result.has_value()) {
                    exec_push_err_ret(result.error()->data()); }

                column_data.push_back(std::move(*result));

            } break;
            case S_TABLE_EXPR_OBJ: {
                table_exprs.push_back(CAST_UP(s_table_expr, obj));
            } break;
            default: exec_push_err_ret("CREATE TABLE: Invalid object in table details. Type:" << object_type_to_astring(obj->type()) << ", (" << obj->inspect() << ")"); break;
        }
    }

    auto empty_rows = avec<UP<s_group_object>>();
    auto* table = new table_object(table_name, std::move(column_data), std::move(table_exprs), std::move(empty_rows));

    table_caches.emplace_back(true, SP<table_object>(table));
}

static void print_table() {

    const SP<f_table_info_object>& tab_info = display_tab.table_info;

    const SP<table_object> tab = tab_info->table;

    for (const auto& detail : tab->column_data) {
        if (detail == nullptr) {
            std::cout << FILE_NAME_STR << ": " << CUR_LOC.function_name() << ". ";
            std::cout << "bruh, " << std::source_location::current().line() << std::endl; }
    }

    if (DEBUG) [[unlikely]] {
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

    }

    // Search for delimiter
    std::string delimiter = ", ";
    for (const auto& expr_wrapper : tab->exprs) {
        const auto& expr = expr_wrapper->parameter;
        if (expr->type() == DELIMITER_OBJ) {
            delimiter = CAST_UP_TO_CONST_RAW(delimiter_object, expr)->value; }
    }
    
    bool first_row = true;
    for (const auto& row_index : tab_info->row_ids) {

        std::stringstream row_values;
        auto result = tab->get_row_vec_ptr(row_index);
        if (!result.has_value()) [[unlikely]] {
            exec_push_err_ret("print_table(): Out of bounds row index"); }

        const auto& row = **result;

        for (const auto& col_id : tab_info->col_ids) {

            if (col_id >= row.size()) [[unlikely]] {
                exec_push_err_ret("print_table(): Out of bounds column index"); }  

            astring full_name = row[col_id]->data();
            if (row[col_id]->type() == NULL_OBJ) {
                full_name = ""; }

            std::string name;
            if (DEBUG) [[unlikely]] {
                name = full_name.substr(0, 10);
                size_t pad_length = 10 - name.length();
                astring pad(pad_length, ' ');
                name += pad + " | ";
            } else {
                name = full_name + delimiter;
            }

            row_values << name;
        }

        if (!first_row) { std::cout << "\n"; }
        std::cout << row_values.str();
        first_row = false;
    }

    std::cout << std::endl;
}


static void configure_print_functions(SP<f_table_info_object> tab_info) {

    display_tab.to_display = true;
    display_tab.table_info = tab_info;

    for (const auto& detail : tab_info->table->column_data) {
        if (detail == nullptr) {
            std::cout << "bruh, " << std::source_location::current().line() << std::endl; }
    }

    print_table(); // CMD line print, QT will do it's own thing in main
}

static std::expected<UP<null_object>, UP<error_object>> exec_where(UP<e_expression_object> clause, table_aggregate_object* table_aggregate, avec<size_t>& row_ids, SP<s_environment> env) {
    
    if (clause->type() == E_PREFIX_EXPRESSION_OBJ) {
        push_err_ret_unx_err_obj("Prefix WHERE not supported yet"); }

    if (clause->type() != E_INFIX_EXPRESSION_OBJ) {
        push_err_ret_unx_err_obj("Tried to evaluate WHERE with non-infix object, bug"); }

    UP<e_infix_expr_object> where_infix = CAST_UP(e_infix_expr_object, clause);
    if (where_infix->get_op_type() != WHERE_OP) {
        push_err_ret_unx_err_obj("eval_where(): Called with non-WHERE operator"); }
    

    UP<evaluated> raw_cond = std::move(where_infix->right);
    if (raw_cond->type() != E_INFIX_EXPRESSION_OBJ) {
        push_err_ret_unx_err_obj("WHERE condition is not infix condition"); }



    if (where_infix->type() == E_INFIX_EXPRESSION_OBJ) { // For SELECT [*] FROM [table] WHERE [CONDITION]
        if (where_infix->left->type() == STRING_OBJ) {
            const auto& [table, tab_found] = get_table_as_const(where_infix->left->data());
            if (tab_found) {
                table_aggregate->add_table(table);
            }
        }
    }

    UP<e_infix_expr_object> e_condition = CAST_UP(e_infix_expr_object, raw_cond);
    UP<operator_object> op    = std::move(e_condition->op);
    UP<serializable>    left  = make_serializable(CAST_UP(evaluated, e_condition->left), env);
    UP<serializable>    right = make_serializable(CAST_UP(evaluated, e_condition->right), env);

    // Find column index BEGIN 
    size_t where_col_index = SIZE_T_MAX;
    const std::array<serializable*, 2> sides = {left.get(), right.get()};
    for (const auto* side : sides) {
        if (where_col_index != SIZE_T_MAX) {
            break; }

        switch(side->type()) {
        case STRING_OBJ: {
            const auto result = table_aggregate->get_col_id(side->data());
            if (!result.has_value()) {
                push_err_ret_unx_err_obj(result.error()->data()); }

            where_col_index = *result;
            
        } break;
        case S_COLUMN_INDEX_OBJ: {

            const auto* col_index = CAST_CONST_RAW_TO_CONST_RAW(s_column_index_object, side);

            const auto& [table, exists] = get_table(col_index->table_name);
            if (!exists) {
                push_err_ret_unx_err_obj("WHERE: Table does not exist"); }

            const auto& index_obj = col_index->column_name;
            if (index_obj->type() != INDEX_OBJ) {
                push_err_ret_unx_err_obj("WHERE: Column index failed to evauluate to index object"); }
            const size_t index = CAST_UP_TO_CONST_RAW(index_object, index_obj)->value;
            
            const auto result = table_aggregate->get_col_id(table->table_name, index);
            if (!result.has_value()) {
                push_err_ret_unx_err_obj(result.error()->data()); }

            where_col_index = *result;
        } break;
        default:
        }
    }

    if (where_col_index == SIZE_T_MAX) {
        push_err_ret_unx_err_obj("SELECT FROM: Could not find column alias in WHERE condition"); }

    SP<table_object> table = table_aggregate->combine_tables("Shouldn't be in the end result");


    avec<size_t> new_row_ids;
    for (size_t row_id = 0; row_id < table->rows.size(); row_id++) {
        
        // Add to env
        auto [cloned_cell_value, cell_in_bounds] = table->get_cell_value(row_id, where_col_index);
        if (!cell_in_bounds) {
            FATAL_ERROR_STACK_TRACE_THROW("Weird index bug", CUR_LOC); }

        auto [column_name, name_in_bounds] = table->get_column_name(where_col_index);
        if (!name_in_bounds) {
            FATAL_ERROR_STACK_TRACE_THROW("Weird index bug", CUR_LOC); }

        auto var = MAKE_UP(s_variable_object, column_name, std::move(cloned_cell_value));
        SP<s_environment> row_env = MAKE_SP(s_environment, env);
        UP<serializable> added = row_env->add_variable(std::move(var));
        if (added->type() == ERROR_OBJ) {
            push_err_ret_unx_err_obj(added->data()); }
        // env done

        UP<serializable> should_add_obj = exec_infix_expression(op->clone(), left->clone(), right->clone(), row_env);
        if (should_add_obj->type() == ERROR_OBJ) {
            push_err_ret_unx_err_obj("Failed to evaulate WHERE condition"); }
        if (should_add_obj->type() != BOOLEAN_OBJ) {
            push_err_ret_unx_err_obj("WHERE condition failed to evaluate to boolean, got (" << object_type_to_astring(should_add_obj->type()) << ")"); }

        bool should_add = false;
        if (CAST_UP(boolean_object, should_add_obj)->data() == "TRUE") {
            should_add = true; }
        
        if (should_add) {
            new_row_ids.push_back(row_id);
        }
    }
    row_ids = new_row_ids;

    return MAKE_UP(null_object);
}


// Need to work on INFIX
static UP<serializable> exec_left_join([[maybe_unused]] UP<e_expression_object> clause, [[maybe_unused]] table_aggregate_object* table_aggregate, [[maybe_unused]] avec<size_t>& row_ids, [[maybe_unused]] SP<s_environment> env) {
    return UP<serializable>(new error_object("Left Join should use indexes bruh, need to rewrite, look at Github if u want"));
}

// Row ids are passed by reference cause easier, might have to make tab by reference later as well (for stuff like JOINs)
// If is buggy can just go back to using return values
static UP<serializable> exec_clause(UP<e_expression_object> clause, table_aggregate_object* table_aggregate, avec<size_t>& row_ids, SP<s_environment> env) {

    switch (clause->get_op_type()) {

        case WHERE_OP: {
            // WHERE can be infix or prefix
            auto result = exec_where(std::move(clause), table_aggregate, row_ids, env);
            if (result.has_value()) {
                return CAST_UP(serializable, std::move(*result));
            } else {
                return CAST_UP(serializable, std::move(result).error());
            }
        } break;
        case LEFT_JOIN_OP: {
            return exec_left_join(std::move(clause), table_aggregate, row_ids, env);
        } break;
        default:
            push_err_ret_ser_err("Unsupported op type (" + operator_type_to_astring(clause->get_op_type()) + ")");
    }
}

static void exec_select_from(UP<e_select_from_node> wrapper, SP<s_environment> env) {

    if (wrapper->value->type() != E_SELECT_FROM_OBJ) {
        exec_push_err_ret("eval_select_from(): Called with invalid object (" + object_type_to_astring(wrapper->value->type()) + ")"); }

    UP<e_select_from_object> info = CAST_UP(e_select_from_object, wrapper->value);    
    
    // First, use clause chain to obtain the initial table
    // Second, the clause chain will conncect tables together, and narrow the ammount of rows selected
    avec<size_t> row_ids;
    table_aggregate_object* table_aggregate = new table_aggregate_object();
    if (info->clause_chain.size() != 0) {
        for (auto& clause : info->clause_chain) {

            if (clause->type() == STRING_OBJ) { // To support plain SELECT * FROM table;
                const auto& [table, tab_found] = get_table_as_const(clause->data());
                if (!tab_found) {
                    exec_push_err_ret("SELECT FROM: Table (" + clause->data() + ") does not exist"); }

                row_ids = table->get_row_ids(); 
                table_aggregate->add_table(table);
                break;
            }

            if (clause->type() != E_INFIX_EXPRESSION_OBJ && clause->type() != E_PREFIX_EXPRESSION_OBJ) {
                exec_push_err_ret("Unsupported clause type, type: (" << object_type_to_astring(clause->type()) << "), expression: " << "(" << clause->inspect() << ")"); 
            }

            UP<serializable> error_val = exec_clause(CAST_UP(e_expression_object, clause), table_aggregate, row_ids, env); // Should add table to aggregate by itself
            if (error_val->type() != NULL_OBJ) {
                exec_push_err_ret(error_val->data()); }
        }

    }



    // Second, use column indexes the index into the table aggregate, validate
    avec<UP<evaluated>> column_indexes = std::move(info->column_indexes);
    avec<size_t> col_ids;
    col_ids.reserve(column_indexes.size());

    if (column_indexes.size() == 0) {
        exec_push_err_ret("SELECT FROM: No column indexes"); }

    // If SELECT * FROM [table], add all columns
    if (column_indexes[0]->type() == STAR_OBJ) {
        col_ids = table_aggregate->get_all_col_ids();

        if (table_aggregate->tables.size() == 1) {
            auto [table, ok] = table_aggregate->get_table(0);
            if (!ok) {
                exec_push_err_ret("SELECT FROM: Strange bug, couldn't get first table from aggregate, even though size == 1"); }
            configure_print_functions(MAKE_SP(f_table_info_object, table, col_ids, row_ids));
            return;
        }

        SP<table_object> table = table_aggregate->combine_tables("aggregate");
        configure_print_functions(MAKE_SP(f_table_info_object, table, col_ids, row_ids));
        return;
    }

    for (auto& selecter : column_indexes) {

        // avec<UP<object>> REALARGS = *static_cast<UP<function_call_object>>(col_index_raw)->arguments->elements; for debug

        switch (selecter->type()) {
        case S_FUNCTION_CALL_OBJ: { // Not padding for now cause lazy

            if (column_indexes.size() == 1) {
                avec<size_t> new_row_ids;
                new_row_ids.push_back(0);
                row_ids = new_row_ids;
            }

            auto func_call_obj = CAST_UP(s_function_call_object, selecter);
            auto args = std::move(func_call_obj->arguments->elements);
            if (args.size() != 1) {
                exec_push_err_ret("COUNT() bad argument count"); }
            const UP<object>& arg = args[0];
            if (arg->type() != STAR_OBJ) {
                exec_push_err_ret("COUNT() argument must be *, got (" + object_type_to_astring(arg->type()) + ")"); }

            size_t count = (table_aggregate->tables[0])->rows.size(); // TODO keep track of max size
            
            auto type       = MAKE_UP(s_SQL_data_type_object, NONE, INTEGER, UP<serializable>(new integer_object(11)));
            auto row        = MAKE_UP(s_group_object, UP<serializable>(new integer_object(static_cast<int>(count)))); // FIXME stinky cast
            auto detail     = MAKE_UP(s_table_detail_object, "COUNT(*)", std::move(type));
            auto empty_exprs = avec<UP<s_table_expr>>();
            auto count_star = MAKE_SP(table_object, "COUNT(*) TABLE", std::move(detail), std::move(empty_exprs), std::move(row));
            table_aggregate->add_table(count_star);
            const auto& [id, ok] = table_aggregate->get_last_col_id();
            if (!ok) {
                exec_push_err_ret("SELECT FROM: Weird bug"); }
            col_ids.push_back(id);
        } break;
        case E_COLUMN_INDEX_OBJ: {

            UP<e_column_index_object> col_index = CAST_UP(e_column_index_object, selecter);

            const auto& [table, exists] = get_table(col_index->table_name);
            if (!exists) {
                exec_push_err_ret("SELECT FROM: Table does not exist"); }

            UP<evaluated> index_obj = std::move(col_index->column_name);
            if (index_obj->type() != INDEX_OBJ) {
                exec_push_err_ret("WHERE: Column index failed to evauluate to index object"); }
            size_t index = CAST_UP(index_object, index_obj)->value;
            
            auto result = table_aggregate->get_col_id(table->table_name, index);
            if (!result.has_value()) {
                exec_push_err_ret("SELECT FROM:" + result.error()->data()); }

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
            exec_push_err_ret("SELECT FROM: Cannot use (" + selecter->inspect() + ") to index");
        }
    }

    if (table_aggregate->tables.size() == 1 || column_indexes.size() == 1) {
        const auto& [table_name, ok] = table_aggregate->get_table_name(0);
        if (!ok) {
            exec_push_err_ret("SELECT FROM: Strange bug, couldn't get first table from aggregate, even though size == 1"); }
        SP<table_object> table = table_aggregate->combine_tables(table_name);
        configure_print_functions(MAKE_SP(f_table_info_object, table, col_ids, row_ids));
        return;
    }

    SP<table_object> table = table_aggregate->combine_tables("aggregate");

    configure_print_functions(MAKE_SP(f_table_info_object, table, col_ids, row_ids));
}






static std::expected<UP<serializable>, UP<error_object>> get_insertable(UP<serializable> insert_obj, const UP<s_SQL_data_type_object>& data_type) {

    if (insert_obj->type() == S_SQL_DATA_TYPE_OBJ) {
        auto insert_dt = CAST_UP(s_SQL_data_type_object, insert_obj);
        bool ok = insert_dt->data_type == data_type->data_type;
        if (!ok) {
            push_err_ret_unx_err_obj( "Could not insert (" + insert_obj->inspect() + ") into (" + data_type->inspect()); } 
        return CAST_UP(serializable, insert_obj); 
    }

    switch (data_type->data_type) {
    case INT:
        switch (insert_obj->type()) {
        case INTEGER_OBJ:
            return CAST_UP(serializable, insert_obj); 
        case DECIMAL_OBJ:
            sql_warnings.add_msg("Decimal implicitly converted to INT");
            return UP<serializable>(new integer_object(insert_obj->data()));
        default:
            push_err_ret_unx_err_obj("Value: (" + insert_obj->data() + ") has mismatching type with column (" + data_type->inspect() + ")");
        }
        break;
    case FLOAT:
        switch (insert_obj->type()) {
        case INTEGER_OBJ:
            sql_warnings.add_msg("Integer implicitly converted to FLOAT");
            return UP<serializable>(new decimal_object(insert_obj->data()));
        case DECIMAL_OBJ:
            return CAST_UP(serializable, insert_obj); 
        default:
            push_err_ret_unx_err_obj( "Value: (" + insert_obj->data() + ") has mismatching type with column (" + data_type->inspect() + ")");
        }
        break;
    case DOUBLE:
    switch (insert_obj->type()) {
        case INTEGER_OBJ:
            sql_warnings.add_msg("Integer implicitly converted to DOUBLE");
            return UP<serializable>(new decimal_object(insert_obj->data()));
        case DECIMAL_OBJ:
            return CAST_UP(serializable, insert_obj); 
        default:
            push_err_ret_unx_err_obj( "Value: (" + insert_obj->data() + ") has mismatching type with column (" + data_type->inspect() + ")");
        }
        break;
    case VARCHAR:
        switch (insert_obj->type()) {
        case INTEGER_OBJ:
            sql_warnings.add_msg("Integer implicitly converted to VARCHAR");
            return UP<serializable>(new string_object(insert_obj->data()));
        case DECIMAL_OBJ:
            sql_warnings.add_msg("Decimal implicitly converted to VARCHAR");
            return UP<serializable>(new string_object(insert_obj->data()));
        case STRING_OBJ: {
            if (!data_type->parameter.has_value()) {
                FATAL_ERROR_THROW("SQL Data Type VARCHAR has to have parameter, cannot get insertable", CUR_LOC); }
            if (data_type->parameter.value()->type() != INTEGER_OBJ) {
                FATAL_ERROR_THROW("SQL Data Type VARCHAR must have integer parameter, cannot get insertable", CUR_LOC); }
                
            int max_length = CAST_UP(integer_object, data_type->parameter.value())->value;
            size_t insert_length = insert_obj->data().length();
            if (insert_length > static_cast<size_t>(max_length)) {
                push_err_ret_unx_err_obj("Value: (" + insert_obj->data() + ") excedes column's max length (" + data_type->parameter.value()->inspect() + ")"); }

            return CAST_UP(serializable, insert_obj); 
        } break;
        default:
            push_err_ret_unx_err_obj( "Value: (" + insert_obj->data() + ") has mismatching type with column (" + data_type->inspect() + ")");
        }
        break;
    default:
        push_err_ret_unx_err_obj( "get_insertable(): " + data_type->inspect() + " is not supported YET");
    }
}

// TODO Maybe put dirty flag in table_object itself
static std::pair<table_cache*, bool> get_table_cache(const std_and_astring_variant& name) {

    astring name_unwrapped;
    visit(name, [&](const auto& unwrapped) {
        name_unwrapped = unwrapped;
    });

    for (auto& entry : table_caches) {
        if (entry.table->table_name == name_unwrapped) {
            return {&entry, true};
        }
    }

    return {nullptr, false};
}

static std::pair<SP<table_object>, bool> get_table(const std_and_astring_variant& name) {

    astring name_unwrapped;
    visit(name, [&](const auto& unwrapped) {
        name_unwrapped = unwrapped;
    });

    for (const auto& entry : table_caches) {
        if (entry.table->table_name == name_unwrapped) {
            return {entry.table, true};
        }
    }

    return {nullptr, false};
}

static std::pair<const SP<table_object>&, bool> get_table_as_const(const std_and_astring_variant& name) {

    astring name_unwrapped;
    visit(name, [&](const auto& unwrapped) {
        name_unwrapped = unwrapped;
    });

    for (const auto& entry : table_caches) {
        if (entry.table->table_name == name_unwrapped) {
            return {entry.table, true};
        }
    }

    SP<table_object> garbage;
    return {garbage, false};
}

static std::pair<UP<serializable>, ret_code> convert_table_info_to_value(UP<f_table_info_object> info) {
    if (info->col_ids.size() == 1 && info->row_ids.size() == 1) {
        auto&& [cell, ok] = info->table->get_cell_value(info->col_ids[0], info->row_ids[0]); 
        if (!ok) {
            log_sql_error("convert_table_info_to_value(): weird index bug"); 
            return {std::move(cell), ERROR};
        }
        return {std::move(cell), SUCCESS};
    }
    return {UP<serializable>(new null_object()), FAIL};
}

static std::pair<UP<serializable>, ret_code> convert_table_to_value(const SP<table_object>& tab) {
    if (tab->column_data.size() == 1 && tab->rows.size() == 1) {
        auto&& [cell, ok] = tab->get_cell_value(0, 0); 
        if (!ok) {
            log_sql_error("eval_infix_expression(): Weird index bug"); 
            return {std::move(cell), ERROR};
        }
        return {std::move(cell), SUCCESS};
    }
    return {UP<serializable>(new null_object()), FAIL};
}

static UP<serializable> exec_infix_expression(operator_object* op, serializable* left, serializable* right, SP<s_environment> env) {
    return exec_infix_expression(UP<operator_object>(op), UP<serializable>(left), UP<serializable>(right), env);
}

static UP<serializable> exec_infix_expression(UP<operator_object> op, UP<serializable> left, UP<serializable> right, SP<s_environment> env) {


    if (left->type() == STRING_OBJ) {
        auto env_var_result = env->get_variable(left->data());
        if (env_var_result.has_value()) {
            UP<s_variable_object> var = std::move(*env_var_result);
            left = std::move(var->value);
        }
    }

    if (right->type() == STRING_OBJ) {
        auto env_var_result = env->get_variable(right->data());
        if (env_var_result.has_value()) {
            UP<s_variable_object> var = std::move(*env_var_result);
            right = std::move(var->value);
        }
    }


    /* Just an idea */
    // if (!is_numeric_object(left)) {
    //     auto num_result = make_numeric(UP<serializable>(left->clone()));
    //     if (!num_result.has_value()) {
    //         auto str_result = make_string(std::move)
    //     }
    // }

    switch (op->op_type) {
        case ADD_OP:
            if (is_numeric_object(left) && is_numeric_object(right)) {
                return UP<serializable>(new integer_object(CAST_UP(integer_object, left)->value + CAST_UP(integer_object, right)->value)); 
            } else if (is_string_object(left) && is_string_object(right)) {
                return UP<serializable>(new string_object(left->data() + right->data())); 
            }
            push_err_ret_ser_err("No infix " << op->inspect() << " operation for " << left->inspect() << " and " << right->inspect());
            
        case SUB_OP:
            if (!is_numeric_object(left) || !is_numeric_object(right)) {
                push_err_ret_ser_err("No infix " << op->inspect() << " operation for " + left->inspect() << " and " << right->inspect());}
            return UP<serializable>(new integer_object(CAST_UP(integer_object, left)->value - CAST_UP(integer_object, right)->value));

        case MUL_OP:
            if (!is_numeric_object(left) || !is_numeric_object(right)) {
                push_err_ret_ser_err("No infix " << op->inspect() << " operation for " + left->inspect() << " and " << right->inspect());}
            return UP<serializable>(new integer_object(CAST_UP(integer_object, left)->value * CAST_UP(integer_object, right)->value));

        case DIV_OP:
            if (!is_numeric_object(left) || !is_numeric_object(right)) {
                push_err_ret_ser_err("No infix " << op->inspect() << " operation for " + left->inspect() << " and " << right->inspect());}
            return UP<serializable>(new integer_object(CAST_UP(integer_object, left)->value / CAST_UP(integer_object, right)->value));

        case DOT_OP:
            if (left->type() != INTEGER_OBJ || left->type() != INTEGER_OBJ) {
                push_err_ret_ser_err("No infix " << op->inspect() << " operation for " + left->inspect() << " and " << right->inspect());}
            return UP<serializable>(new decimal_object(left->data() + "." + right->data()));

        case EQUALS_OP:
            if (left->type() == STRING_OBJ && right->type() == STRING_OBJ) {
                return UP<serializable>(new boolean_object(left->data() == right->data())); 
            } else if (left->type() == INTEGER_OBJ && right->type() == INTEGER_OBJ) {
                return UP<serializable>(new boolean_object(CAST_UP(integer_object, left)->value == CAST_UP(integer_object, right)->value)); 
            }
            push_err_ret_ser_err("No infix " << op->inspect() << " operation for " + left->inspect() << " and " << right->inspect());

        case NOT_EQUALS_OP:
            if (left->type() == STRING_OBJ && right->type() == STRING_OBJ) {
                return UP<serializable>(new boolean_object(left->data() != right->data())); 
            } else if (left->type() == INTEGER_OBJ && right->type() == INTEGER_OBJ) {
                return UP<serializable>(new boolean_object(CAST_UP(integer_object, left)->value != CAST_UP(integer_object, right)->value)); 
            } 
            push_err_ret_ser_err("No infix " << op->inspect() << " operation for " + left->inspect() << " and " << right->inspect());

        case LESS_THAN_OP:
            if (left->type() != INTEGER_OBJ || right->type() != INTEGER_OBJ) {
                push_err_ret_ser_err("No infix " << op->inspect() << " operation for " + left->inspect() << " and " << right->inspect());}
            return UP<serializable>(new boolean_object(CAST_UP(integer_object, left)->value < CAST_UP(integer_object, right)->value)); 

        case GREATER_THAN_OP:
            if (left->type() != INTEGER_OBJ || right->type() != INTEGER_OBJ) {
                push_err_ret_ser_err("No infix " << op->inspect() << " operation for " + left->inspect() << " and " << right->inspect());}
            return UP<serializable>(new boolean_object(CAST_UP(integer_object, left)->value > CAST_UP(integer_object, right)->value)); 

        default:
            push_err_ret_ser_err("No infix " + op->inspect() + " operator known");
    }
}


static std::expected<UP<s_SQL_data_type_object>, UP<error_object>> assume_data_type(UP<serializable> obj) {
    switch (obj->type()) {
    case STRING_OBJ:
        return MAKE_UP(s_SQL_data_type_object, NONE, VARCHAR, UP<serializable>(new integer_object(255)));
    case INTEGER_OBJ:
        return MAKE_UP(s_SQL_data_type_object, NONE, INT,     UP<serializable>(new integer_object(11)));
    case TABLE_OBJ: {
        /* Shouldn't pass around table objects*/
        push_err_ret_unx_err_obj("BAD" << std::source_location::current().line()); 
        // auto&& [cell, rc] = convert_table_to_value(cast_SP<table_object>(obj));
        // if (rc == SUCCESS) {
        //     return assume_data_type(std::move(cell));
        // }
        push_err_ret_unx_err_obj("Can't assume default data type for (" + obj->inspect() + ")");
    } break;
    case F_TABLE_INFO_OBJ: {
        auto [cell, rc] = convert_table_info_to_value(CAST_UP(f_table_info_object, obj));
        if (rc == SUCCESS) {
            return assume_data_type(std::move(cell));
        }
        push_err_ret_unx_err_obj("Can't assume default data type for (" + obj->inspect() + ")");
    } break;
    default:
        push_err_ret_unx_err_obj("Can't assume default data type for (" + obj->inspect() + ")");
    }
}