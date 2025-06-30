#pragma once

// objects are made in the parser, used to parse and return values from expressions
    // i.e (10 + 10) will return an integer_object with the value 20

#include "token.h"
#include "allocators.h"
#include "allocator_aliases.h"
#include "structs_and_macros.h" // For table

#include <expected>
#include <charconv>




enum object_type : std::uint8_t {
    ERROR_OBJ, NULL_OBJ, INFIX_EXPRESSION_OBJ, PREFIX_EXPRESSION_OBJ, INTEGER_OBJ, INDEX_OBJ, DECIMAL_OBJ, STRING_OBJ, SQL_DATA_TYPE_OBJ,
    COLUMN_OBJ, EVALUATED_COLUMN_OBJ, FUNCTION_OBJ, OPERATOR_OBJ, SEMICOLON_OBJ, RETURN_VALUE_OBJ, ARGUMENT_OBJ, BOOLEAN_OBJ, FUNCTION_CALL_OBJ,
    GROUP_OBJ, PARAMETER_OBJ, EVALUATED_FUNCTION_OBJ, VARIABLE_OBJ, TABLE_DETAIL_OBJECT, COLUMN_INDEX_OBJECT, TABLE_INFO_OBJECT, TABLE_OBJECT,
    STAR_OBJECT, TABLE_AGGREGATE_OBJECT, COLUMN_VALUES_OBJ,

    IF_STATEMENT, BLOCK_STATEMENT, END_IF_STATEMENT, END_STATEMENT, RETURN_STATEMENT,

    INSERT_INTO_OBJECT, SELECT_OBJECT, SELECT_FROM_OBJECT,

    // EVALUATED
    E_COLUMN_INDEX_OBJECT, EXPRESSION_STATEMENT, E_SQL_DATA_TYPE_OBJ, E_GROUP_OBJ, E_FUNCTION_CALL_OBJ,
    E_ARGUMENT_OBJ, E_VARIABLE_OBJ, E_PARAMETER_OBJ, E_BLOCK_STATEMENT, E_TABLE_DETAIL_OBJECT, E_RETURN_STATEMENT, E_SELECT_FROM_OBJECT,
    E_INFIX_EXPRESSION_OBJ, E_PREFIX_EXPRESSION_OBJ, E_INSERT_INTO_OBJECT,
    

    // CUSTOM

    ASSERT_OBJ,
};

enum operator_type : std::uint8_t {
    ADD_OP, SUB_OP, MUL_OP, DIV_OP, NEGATE_OP,
    EQUALS_OP, NOT_EQUALS_OP, LESS_THAN_OP, LESS_THAN_OR_EQUAL_TO_OP, GREATER_THAN_OP, GREATER_THAN_OR_EQUAL_TO_OP,
    OPEN_PAREN_OP, OPEN_BRACKET_OP, AS_OP, LEFT_JOIN_OP, WHERE_OP, GROUP_BY_OP, ORDER_BY_OP, ON_OP,
    DOT_OP, NULL_OP
};


astring object_type_to_astring(object_type index);
astring operator_type_to_astring(operator_type index);

template<typename T>
T astring_to_numeric(const astring& str) {
    if constexpr (std::is_integral_v<T>) {
        // Use std::from_chars for integral types (works in libc++)
        T result;
        const auto& [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result);
        if (ec == std::errc{}) {
            return result;
        } else {
            throw std::invalid_argument("Invalid numeric string");
        }
    } else {
        // Fallback for floating-point types
        const char* start = str.c_str();
        char* end = nullptr;
        errno = 0;
        
        if constexpr (std::is_same_v<T, float>) {
            float result = std::strtof(start, &end);
            if (errno == ERANGE) {
                throw std::out_of_range("Numeric value out of range");
            }
            if (end == start || *end != '\0') {
                throw std::invalid_argument("Invalid numeric string");
            }
            return result;
        } else if constexpr (std::is_same_v<T, double>) {
            double result = std::strtod(start, &end);
            if (errno == ERANGE) {
                throw std::out_of_range("Numeric value out of range");
            }
            if (end == start || *end != '\0') {
                throw std::invalid_argument("Invalid numeric string");
            }
            return result;
        } else if constexpr (std::is_same_v<T, long double>) {
            long double result = std::strtold(start, &end);
            if (errno == ERANGE) {
                throw std::out_of_range("Numeric value out of range");
            }
            if (end == start || *end != '\0') {
                throw std::invalid_argument("Invalid numeric string");
            }
            return result;
        } else {
            static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");
        }
    }
}

template<typename T>
astring numeric_to_astring(T value) {
    // char buffer[64];  // Larger buffer to handle floating point
    // const auto& [ptr, ec] = std::to_chars(buffer, buffer + sizeof(buffer), value);
    std::array<char, 64> buffer{};  // Larger buffer to handle floating point
    const auto& [ptr, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    
    if (ec == std::errc{}) {
        main_alloc<char> alloc = main_alloc<char>{};
        return {buffer.data(), static_cast<std::size_t>(ptr - buffer.data()), alloc};
    } else {
        throw std::runtime_error("Numeric to string conversion failed");
    }
}


// For std_and_astring_variant
#define VISIT(to_unwrap, var_name, ...)                \
    do {                                               \
        std::visit(                                   \
            [&](const auto& var_name) {                \
                __VA_ARGS__;                           \
            },                                         \
            to_unwrap.get_variant()                    \
        );                                             \
    } while (0)



class object {
    protected:
    static main_alloc<object> object_allocator_alias; 

    public:
    [[nodiscard]] virtual astring inspect() const = 0;  
    [[nodiscard]] virtual object_type type() const = 0;    
    [[nodiscard]] virtual astring data() const = 0;    
    [[nodiscard]] virtual object* clone() const = 0;    
    virtual ~object() noexcept = default; 


    object() = default; 
    object(const object&) = delete;
    object& operator=(const object&) = delete;
    

    static void* operator new(size_t size, bool no_stack = false) {
        return object_allocator_alias.allocate_block_impl(size, no_stack).mem;
    }

    static void operator delete(void* ptr, std::size_t size) noexcept {
        if (ptr == nullptr) { return; }
        
        object_allocator_alias.deallocate_block({size, ptr});
    }
    
    static void* operator new[](std::size_t size, bool no_stack = false) {
        return object_allocator_alias.allocate_block_impl(size, no_stack).mem;
    }
    
    static void operator delete[](void* ptr, std::size_t size) noexcept {
        if (ptr == nullptr) { return; }

        object_allocator_alias.deallocate_block({size, ptr});
    }
    
};

class evaluated : virtual public object {
    public:
    using object::object;
    [[nodiscard]] virtual evaluated* clone() const override = 0;
};

class null_object : virtual public evaluated {

    public:
    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] null_object* clone() const override;
};

class operator_object : virtual public evaluated {
    public:
    explicit operator_object(operator_type type);

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] operator_object* clone() const override;

    public:
    operator_type op_type;
};

class table_object;
class table_info_object : virtual public evaluated {

    public:
    table_info_object(table_object* set_tab, avec<size_t> set_col_ids, avec<size_t> set_row_ids);
    table_info_object(SP<table_object> set_tab, avec<size_t> set_col_ids, avec<size_t> set_row_ids);
    ~table_info_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]]table_info_object* clone() const override;

    public:
    SP<table_object> tab;
    avec<size_t> col_ids;
    avec<size_t> row_ids;
};



class expression_object : public object {

    public:
    virtual ~expression_object() noexcept = default;
    [[nodiscard]] virtual expression_object* clone() const = 0;

    public:
    [[nodiscard]] virtual operator_type get_op_type() const = 0; // <--- The reason why this exists
};

class e_expression_object : virtual public evaluated {

    public:
    virtual ~e_expression_object() noexcept = default;
    [[nodiscard]] virtual e_expression_object* clone() const = 0;

    public:
    [[nodiscard]] virtual operator_type get_op_type() const = 0; // <--- The reason why this exists
};

class infix_expression_object : public expression_object {

    public:
    infix_expression_object(operator_object* set_op, object* set_left, object* set_right);
    infix_expression_object(UP<operator_object> set_op, UP<object> set_left, UP<object> set_right);
    ~infix_expression_object() noexcept override = default;
    
    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] infix_expression_object* clone() const override;

    [[nodiscard]] operator_type get_op_type() const override;

    public:
    UP<operator_object> op;
    UP<object> left;
    UP<object> right;
};

class e_infix_expression_object : virtual public e_expression_object {

    public:
    e_infix_expression_object(operator_object* set_op, evaluated* set_left, evaluated* set_right);
    e_infix_expression_object(UP<operator_object> set_op, UP<evaluated> set_left, UP<evaluated> set_right);
    ~e_infix_expression_object() noexcept override = default;
    
    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] e_infix_expression_object* clone() const override;

    [[nodiscard]] operator_type get_op_type() const override;

    public:
    UP<operator_object> op;
    UP<evaluated> left;
    UP<evaluated> right;
};

class prefix_expression_object : public expression_object {

    public:
    prefix_expression_object(operator_object* set_op, object* set_right);
    prefix_expression_object(UP<operator_object> set_op, UP<object> set_right);
    ~prefix_expression_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] prefix_expression_object* clone() const override;

    [[nodiscard]] operator_type get_op_type() const override;

    public:
    UP<operator_object> op;
    UP<object> right;
};

class e_prefix_expression_object : virtual public e_expression_object {

    public:
    e_prefix_expression_object(operator_object* set_op, evaluated* set_right);
    e_prefix_expression_object(UP<operator_object> set_op, UP<evaluated> set_right);
    ~e_prefix_expression_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] e_prefix_expression_object* clone() const override;

    [[nodiscard]] operator_type get_op_type() const override;

    public:
    UP<operator_object> op;
    UP<evaluated> right;
};

class integer_object : virtual public evaluated {

    public:
    integer_object();
    integer_object(int val);
    integer_object(const std::string& val);
    integer_object(const astring& val);

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] integer_object* clone() const override;

    public:
    int value;
};

class index_object : virtual public evaluated {

    public:
    index_object();
    explicit index_object(size_t val);
    index_object(const std::string& val);
    index_object(const astring& val);

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] index_object* clone() const override;

    public:
    size_t value;
};

class decimal_object : virtual public evaluated {

    public:
    decimal_object();
    decimal_object(double val);
    decimal_object(const std::string& val);
    decimal_object(const astring& val);

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] decimal_object* clone() const override;

    public:
    double value; // value can be cast to float later
};

class string_object : virtual public evaluated {

    public:
    string_object(const std_and_astring_variant& val);
    ~string_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] string_object* clone() const override;

    public:
    astring value;
};

class return_value_object : public object {

    public:
    return_value_object(object* val);
    return_value_object(UP<object> val);
    ~return_value_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] return_value_object* clone() const override;

    public:
    UP<object> value;
};

class argument_object : public object {

    public:
    argument_object(const std_and_astring_variant& set_name, object* val);
    argument_object(const std_and_astring_variant& set_name, UP<object> val);
    ~argument_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] argument_object* clone() const override;

    public:
    astring name;
    UP<object> value;
};

class e_argument_object : virtual public evaluated {

    public:
    e_argument_object(const std_and_astring_variant& set_name, evaluated* val);
    e_argument_object(const std_and_astring_variant& set_name, UP<evaluated> val);
    ~e_argument_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] e_argument_object* clone() const override;

    public:
    astring name;
    UP<evaluated> value;
};


class variable_object : public object {

    public:
    variable_object(const std_and_astring_variant& set_name, object* val);
    variable_object(const std_and_astring_variant& set_name, UP<object> val);
    ~variable_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] variable_object* clone() const override;

    public:
    astring name;
    UP<object> value;
};

class e_variable_object : virtual public evaluated {

    public:
    e_variable_object(const std_and_astring_variant& set_name, evaluated* val);
    e_variable_object(const std_and_astring_variant& set_name, UP<evaluated> val);
    ~e_variable_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] e_variable_object* clone() const override;

    public:
    astring name;
    UP<evaluated> value;
};

class boolean_object : virtual public evaluated {

    public:
    boolean_object(bool val);

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] boolean_object* clone() const override;

    public:
    bool value;
};

//zerofill is implicitly unsigned im pretty sure
class SQL_data_type_object: virtual public object {

    public:
    SQL_data_type_object(token_type set_prefix, token_type set_data_type, object* set_parameter);
    SQL_data_type_object(token_type set_prefix, token_type set_data_type, UP<object> set_parameter);
    ~SQL_data_type_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] SQL_data_type_object* clone() const override;

    public:
    token_type prefix;
    token_type data_type;
    UP<object> parameter;
};

class e_SQL_data_type_object : virtual public evaluated {
    public:
    e_SQL_data_type_object(token_type set_prefix, token_type set_data_type, evaluated* set_parameter);
    e_SQL_data_type_object(token_type set_prefix, token_type set_data_type, UP<evaluated> set_parameter);
    ~e_SQL_data_type_object() noexcept override = default;
    
    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] e_SQL_data_type_object* clone() const override;

    token_type prefix;
    token_type data_type;
    UP<evaluated> parameter;
};

class parameter_object : public object {

    public:
    parameter_object(const std_and_astring_variant& set_name, SQL_data_type_object* set_data_type);
    parameter_object(const std_and_astring_variant& set_name, UP<SQL_data_type_object> set_data_type);
    ~parameter_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] parameter_object* clone() const override;

    public:
    astring name;
    UP<SQL_data_type_object> data_type;
};

class e_parameter_object : virtual public evaluated {

    public:
    e_parameter_object(const std_and_astring_variant& set_name, e_SQL_data_type_object* set_data_type);
    e_parameter_object(const std_and_astring_variant& set_name, UP<e_SQL_data_type_object> set_data_type);
    ~e_parameter_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] e_parameter_object* clone() const override;

    public:
    astring name;
    UP<e_SQL_data_type_object> data_type;
};

class table_detail_object : public object {

    public:
    table_detail_object(const std_and_astring_variant& set_name, SQL_data_type_object* set_data_type, object* set_default_value);
    table_detail_object(const std_and_astring_variant& set_name, UP<SQL_data_type_object> set_data_type, UP<object> set_default_value);
    ~table_detail_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] table_detail_object* clone() const override;

    public:
    astring name;
    UP<SQL_data_type_object> data_type;
    UP<object> default_value;
};

class e_table_detail_object : virtual public evaluated {

    public:
    e_table_detail_object(const std_and_astring_variant& set_name, e_SQL_data_type_object* set_data_type, evaluated* set_default_value);
    e_table_detail_object(const std_and_astring_variant& set_name, UP<e_SQL_data_type_object> set_data_type, UP<evaluated> set_default_value);
    ~e_table_detail_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] e_table_detail_object* clone() const override;

    public:
    astring name;
    UP<e_SQL_data_type_object> data_type;
    UP<evaluated> default_value;
};

class group_object : public object {

    public:
    group_object(object* obj);
    group_object(UP<object> obj);
    group_object(avec<UP<object>>&& objs);
    ~group_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] group_object* clone() const override;

    public:
    avec<UP<object>> elements;
};

class e_group_object : virtual public evaluated {

    public:
    e_group_object(evaluated* obj);
    e_group_object(UP<evaluated> obj);
    e_group_object(avec<UP<evaluated>>&& objs);
    ~e_group_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] e_group_object* clone() const override;

    public:
    avec<UP<evaluated>> elements;
};

class block_statement;

class function_object : public object { //!!MAJOR, switched body and return type, make sure the callers are ok

    public:
    function_object(const std_and_astring_variant& set_name, group_object* set_parameters, SQL_data_type_object* set_return_type, block_statement* set_body);
    function_object(const std_and_astring_variant& set_name, UP<group_object> set_parameters, UP<SQL_data_type_object> set_return_type, UP<block_statement> set_body);
    ~function_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] function_object* clone() const override;

    public:
    astring name;
    UP<group_object> parameters;
    UP<SQL_data_type_object> return_type;
    UP<block_statement> body;
};

class e_block_statement;
class evaluated_function_object : virtual public evaluated {

    public:
    evaluated_function_object(const std_and_astring_variant& set_name, avec<UP<e_parameter_object>>&& set_parameters, e_SQL_data_type_object* set_return_type, e_block_statement* set_body);
    evaluated_function_object(const std_and_astring_variant& set_name, avec<UP<e_parameter_object>>&& set_parameters, UP<e_SQL_data_type_object> set_return_type, UP<e_block_statement> set_body);
    ~evaluated_function_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] evaluated_function_object* clone() const override;

    public:
    astring name;
    avec<UP<e_parameter_object>> parameters;
    UP<e_SQL_data_type_object> return_type;
    UP<e_block_statement> body;
};

class function_call_object : public object {

    public:
    function_call_object(const std_and_astring_variant& set_name, group_object* args);
    function_call_object(const std_and_astring_variant& set_name, UP<group_object> args);
    ~function_call_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] function_call_object* clone() const override;

    public:
    astring name;
    UP<group_object> arguments;
};

class e_function_call_object : virtual public evaluated {

    public:
    e_function_call_object(const std_and_astring_variant& set_name, e_group_object* args);
    e_function_call_object(const std_and_astring_variant& set_name, UP<e_group_object> args);
    ~e_function_call_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] e_function_call_object* clone() const override;

    public:
    astring name;
    UP<e_group_object> arguments;
};


class column_object: public object {

    public:
    column_object(object* name_data_type);
    column_object(UP<object> name_data_type);
    column_object(object* name_data_type, object* default_val);
    column_object(UP<object> name_data_type, UP<object> default_val);
    ~column_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] column_object* clone() const override;

    public:
    UP<object> name_data_type;
    UP<object> default_value;
};

class values_wrapper_object : virtual public evaluated {

    public:
    values_wrapper_object(avec<UP<evaluated>>&& set_values) : values(std::move(set_values)) {}
    avec<UP<evaluated>> values;
};

class column_values_object: public values_wrapper_object {

    public:
    column_values_object(avec<UP<evaluated>>&& set_values);
    ~column_values_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] column_values_object* clone() const override;
};

class evaluated_column_object: virtual public evaluated {

    public:
    evaluated_column_object(const std_and_astring_variant& set_name, e_SQL_data_type_object* type, evaluated* set_default_value);
    evaluated_column_object(const std_and_astring_variant& set_name, UP<e_SQL_data_type_object> type, UP<evaluated> set_default_value);
    ~evaluated_column_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] evaluated_column_object* clone() const override;

    public:
    astring name;
    UP<e_SQL_data_type_object> data_type;
    UP<evaluated> default_value;
};

class error_object : virtual public evaluated {

    public:
    error_object();
    error_object(const std_and_astring_variant& val);
    ~error_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] error_object* clone() const override;

    public:
    astring value;
};

class semicolon_object : virtual public evaluated {

    public:
    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] semicolon_object* clone() const override;
};

class star_object : virtual public evaluated {

    public:
    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] star_object* clone() const override;
};

class column_index_object : public object {
    public:
    column_index_object(object* set_table_name, object* set_column_name);
    column_index_object(UP<object> set_table_name, UP<object> set_column_name);
    ~column_index_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] column_index_object* clone() const override;

    public:
    UP<object> table_name;
    UP<object> column_name;
};

class e_column_index_object : virtual public evaluated {
    public:
    e_column_index_object(table_object* set_table, index_object* set_column_index);
    e_column_index_object(SP<table_object> set_table, UP<index_object> set_column_index);
    ~e_column_index_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] e_column_index_object* clone() const override;

    public:
    SP<table_object> table;
    UP<index_object> index;
};

class table_object : virtual public evaluated {


    public:
    table_object(const std_and_astring_variant& set_table_name, e_table_detail_object* set_column_datas, e_group_object* set_rows);
    table_object(const std_and_astring_variant& set_table_name, UP<e_table_detail_object> set_column_datas, UP<e_group_object> set_rows);
    table_object(const std_and_astring_variant& set_table_name, avec<UP<e_table_detail_object>>&& set_column_datas);
    table_object(const std_and_astring_variant& set_table_name, avec<UP<e_table_detail_object>>&& set_column_datas, avec<UP<e_group_object>>&& set_rows);
    ~table_object() noexcept override = default;

    table_object(const table_object&) = delete;
    table_object& operator=(const table_object&) = delete;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] table_object* clone() const override;

    [[nodiscard]] std::pair<const avec<evaluated*>&, bool>      get_const_column(size_t index)          const;
    [[nodiscard]] std::pair<const avec<evaluated*>&, bool>      get_const_column(const std_and_astring_variant& col_name)   const;
    [[nodiscard]] std::pair<astring, bool>                      get_column_name(size_t index)           const;
    [[nodiscard]] std::pair<UP<e_SQL_data_type_object>, bool>   get_column_data_type(size_t index)      const;
    [[nodiscard]] std::pair<size_t, bool>                       get_column_index(const std_and_astring_variant& name) const;
    [[nodiscard]] std::expected<UP<evaluated>, UP<error_object>>get_cloned_column_default_value(size_t index)     const;
    [[nodiscard]] std::pair<UP<evaluated>, bool>                get_cell_value(size_t row_index, size_t col_index)    const;
    [[nodiscard]] std::expected<avec<UP<evaluated>>*, UP<error_object>> get_row_vec_ptr(size_t index)   const;
    [[nodiscard]] avec<size_t>                                  get_row_ids()                           const;
    [[nodiscard]] astring                                       get_tab_name()                     const;

    [[nodiscard]] bool check_if_field_name_exists(const std_and_astring_variant& name) const;

    public:
    astring table_name;
    avec<UP<e_table_detail_object>> column_datas;
    avec<UP<e_group_object>> rows;
};

class table_aggregate_object : virtual public evaluated {
    public:
    table_aggregate_object();
    table_aggregate_object(avec<SP<table_object>>&& set_tables);
    ~table_aggregate_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] table_aggregate_object* clone() const override;

    [[nodiscard]] std::expected<size_t, UP<error_object>> get_col_id(const std_and_astring_variant& column_name) const;
    [[nodiscard]] std::expected<size_t, UP<error_object>> get_col_id(const std_and_astring_variant& table_name, const std_and_astring_variant& column_name) const;
    [[nodiscard]] std::expected<size_t, UP<error_object>> get_col_id(const std_and_astring_variant& table_name, size_t index) const;
    [[nodiscard]] avec<size_t> get_all_col_ids() const;
    [[nodiscard]] std::pair<size_t, bool> get_last_col_id() const;
    [[nodiscard]] std::pair<astring, bool> get_table_name(size_t index) const;
    [[nodiscard]] std::pair<SP<table_object>, bool> get_table(size_t index) const;
    [[nodiscard]] SP<table_object> combine_tables(const std_and_astring_variant& name) const;
    void add_table(table_object* table);
    void add_table(const SP<table_object>& table);

    public:
    avec<SP<table_object>> tables;
};

// Node objects
class insert_into_object : public object {

    public:
    insert_into_object(const std_and_astring_variant& set_table_name, avec<UP<object>>&& set_fields, object* set_values);
    insert_into_object(const std_and_astring_variant& set_table_name, avec<UP<object>>&& set_fields, UP<object> set_values);
    ~insert_into_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] insert_into_object* clone() const override;

    public:
    astring table_name;
    avec<UP<object>> fields;
    UP<object> values;
};

class e_insert_into_object : virtual public evaluated {

    public:
    e_insert_into_object(astring set_table_name, avec<UP<evaluated>>&& set_fields, avec<UP<evaluated>>&& set_values);
    ~e_insert_into_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] e_insert_into_object* clone() const override;

    public:
    astring table_name;
    avec<UP<evaluated>> fields;
    avec<UP<evaluated>> values;
};

class select_object : public object {
    
    public:
    select_object(object* set_value);
    select_object(UP<object> set_value);
    ~select_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] select_object* clone() const override;

    public:
    UP<object> value;
};

class select_from_object : public object {
    
    public:
    select_from_object(avec<UP<object>>&& set_column_indexes, avec<UP<object>>&& set_clause_chain);
    ~select_from_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] select_from_object* clone() const override;

    public:
    avec<UP<object>> column_indexes;
    avec<UP<object>> clause_chain;
};

class e_select_from_object : virtual public evaluated {
    
    public:
    e_select_from_object(avec<UP<evaluated>>&& set_column_indexes, avec<UP<evaluated>>&& set_clause_chain);
    ~e_select_from_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] e_select_from_object* clone() const override;

    public:
    avec<UP<evaluated>> column_indexes;
    avec<UP<evaluated>> clause_chain;
};



// Statements
class block_statement : public object {

    public:
    block_statement(avec<UP<object>>&& set_body);
    ~block_statement() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] block_statement* clone() const override;

    public:
    avec<UP<object>> body;
};

class e_block_statement : virtual public evaluated {

    public:
    e_block_statement(avec<UP<evaluated>>&& set_body);
    ~e_block_statement() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] e_block_statement* clone() const override;

    public:
    avec<UP<evaluated>> body;
};

class e_return_statement;
class expression_statement : virtual public evaluated {

    public:
    expression_statement(avec<UP<evaluated>>&& set_body, e_return_statement* set_ret_val);
    expression_statement(avec<UP<evaluated>>&& set_body, UP<e_return_statement> set_ret_val);
    ~expression_statement() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] expression_statement* clone() const override;

    public:
    avec<UP<evaluated>> body;
    UP<e_return_statement> ret_val;
};

class if_statement : public object {

    public:
    if_statement(object* set_condition, block_statement* set_body, object* set_other);
    if_statement(UP<object> set_condition, UP<block_statement> set_body, UP<object> set_other);
    ~if_statement() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] if_statement* clone() const override;

    public:
    UP<object> condition;
    UP<block_statement> body;
    UP<object> other;
};

class end_if_statement : virtual public evaluated {

    public:
    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] end_if_statement* clone() const override;
};

class end_statement : virtual public evaluated {

    public:
    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] end_statement* clone() const override;
};

class return_statement : public object {

    public:
    return_statement(object* expr);
    return_statement(UP<object> expr);
    ~return_statement() noexcept override = default;
    
    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] return_statement* clone() const override;

    public:
    UP<object> expression;
};

class e_return_statement : virtual public evaluated {

    public:
    e_return_statement(evaluated* expr);
    e_return_statement(UP<evaluated> expr);
    ~e_return_statement() noexcept override = default;
    
    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] e_return_statement* clone() const override;

    public:
    UP<evaluated> expression;
};



// Custom
class assert_object : public object {

    public:
    assert_object(object*  ) = delete; // stops stupid conversions
    assert_object(UP<object>  ) = delete; // stops stupid conversions
    explicit assert_object(object* expr, size_t set_line);
    explicit assert_object(UP<object> expr, size_t set_line);
    ~assert_object() noexcept override = default;
    
    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] assert_object* clone() const override;

    public:
    size_t line;
    UP<object> expression;
};