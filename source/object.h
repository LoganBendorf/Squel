#pragma once

// objects are made in the parser, used to parse and return values from expressions
    // i.e (10 + 10) will return an integer_object with the value 20

#include "token.h"
#include "allocators.h"
#include "allocator_aliases.h"
#include "macros.h"
    
#include <expected>
#include <charconv>
#include <concepts>
#include <type_traits>

enum object_type : std::uint8_t {
    ERROR_OBJ, NULL_OBJ, INFIX_EXPRESSION_OBJ, PREFIX_EXPRESSION_OBJ, INTEGER_OBJ, TIMESTAMP_OBJ, INDEX_OBJ, DECIMAL_OBJ, STRING_OBJ,
    OPERATOR_OBJ, SEMICOLON_OBJ, RETURN_VALUE_OBJ, BOOLEAN_OBJ, AUTO_INCREMENT_OBJ, CURRENT_TIMESTAMP_OBJ, PRIMARY_KEY_OBJ, DELIMITER_OBJ,
    TABLE_OBJ,
    STAR_OBJ, TABLE_AGGREGATE_OBJ,

    IF_STATEMENT, END_IF_STATEMENT, END_STATEMENT, RETURN_STATEMENT,

    INSERT_INTO_OBJ, SELECT_OBJ, SELECT_FROM_OBJ,



    COLUMN_OBJ,         E_COLUMN_OBJ,           S_COLUMN_OBJ,
    COLUMN_INDEX_OBJ,   E_COLUMN_INDEX_OBJ,     S_COLUMN_INDEX_OBJ,
    TABLE_INFO_OBJ,                                                 F_TABLE_INFO_OBJ,
    TABLE_DETAIL_OBJ,   E_TABLE_DETAIL_OBJ,     S_TABLE_DETAIL_OBJ,
    PARAMETER_OBJ,      E_PARAMETER_OBJ,        S_PARAMETER_OBJ, 
    GROUP_OBJ,          E_GROUP_OBJ,            S_GROUP_OBJ,
    SQL_DATA_TYPE_OBJ,  E_SQL_DATA_TYPE_OBJ,    S_SQL_DATA_TYPE_OBJ,
    FUNCTION_OBJ,       E_FUNCTION_OBJ,         S_FUNCTION_OBJ,
    FUNCTION_CALL_OBJ,                          S_FUNCTION_CALL_OBJ,
    // Working on
    TABLE_EXPR_OBJ,     E_TABLE_EXPR_OBJ,       S_TABLE_EXPR_OBJ,
    TABLE_COLUMN_EXPR_OBJ, E_TABLE_COLUMN_EXPR_OBJ, S_TABLE_COLUMN_EXPR_OBJ,
    CONSTRAINT_OBJ,     E_CONSTRAINT_OBJ,       S_CONSTRAINT_OBJ,
    UNIQUE_OBJ,         E_UNIQUE_OBJ,           S_UNIQUE_OBJ,
    DEFAULT_VALUE_OBJ,  E_DEFAULT_VALUE_OBJ,    S_DEFAULT_VALUE_OBJ,
    DEFAULT_VALUE_FUNC_OBJ, E_DEFAULT_VALUE_FUNC_OBJ, S_DEFAULT_VALUE_FUNC_OBJ,
    HASH_OBJ,           E_HASH_OBJ,             S_HASH_OBJ,
    FOREIGN_KEY_OBJ,    E_FOREIGN_KEY_OBJ,      S_FOREIGN_KEY_OBJ,
    VARIABLE_OBJ,       E_VARIABLE_OBJ,         S_VARIABLE_OBJ,
    ARGUMENT_OBJ,       E_ARGUMENT_OBJ,         S_ARGUMENT_OBJ,


    BLOCK_STATEMENT,   E_BLOCK_STATEMENT,   S_BLOCK_STATEMENT,

    
    EXPRESSION_STATEMENT,
    E_RETURN_STATEMENT, E_SELECT_FROM_OBJ,
    E_INFIX_EXPRESSION_OBJ, E_PREFIX_EXPRESSION_OBJ, E_INSERT_INTO_OBJ,
    
    // CUSTOM
    ASSERT_OBJ, E_ASSERT_OBJ,
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
template<typename Visitor>
auto visit(const std_and_astring_variant& to_unwrap, Visitor&& visitor) {
    return std::visit(std::forward<Visitor>(visitor), to_unwrap.get_variant());
}



class object {
    protected:
    static main_alloc<object> object_allocator_alias; 

    public:
    [[nodiscard]] virtual astring inspect()  const = 0;  
    [[nodiscard]] virtual object_type type() const = 0;    
    [[nodiscard]] virtual astring data()     const = 0;    
    [[nodiscard]] virtual object* clone()    const = 0;    
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

template<typename T>
concept PotentiallyInheritsObject = requires {
    typename T;
    sizeof(T*);
} && std::is_class_v<T>;

class evaluated : virtual public object {
    public:
    using object::object;
    [[nodiscard]] virtual evaluated* clone() const override = 0;
};

class serializable : virtual public evaluated {
    public:
    using evaluated::evaluated;
    [[nodiscard]] virtual serializable* clone()     const override = 0;
    [[nodiscard]] virtual astring       serialize() const = 0;
};

class null_object : virtual public serializable {

    public:
    [[nodiscard]] astring      inspect()   const override;
    [[nodiscard]] object_type  type()      const override;
    [[nodiscard]] astring      data()      const override;
    [[nodiscard]] null_object* clone()     const override;
    [[nodiscard]] astring      serialize() const override;
};

class timestamp_object : virtual public serializable {

    public:
    timestamp_object();

    [[nodiscard]] astring      inspect()   const override;
    [[nodiscard]] object_type  type()      const override;
    [[nodiscard]] astring      data()      const override;
    [[nodiscard]] timestamp_object* clone()     const override;
    [[nodiscard]] astring      serialize() const override;

    public:
    size_t value;
};

class auto_increment_object : virtual public serializable {

    public:
    [[nodiscard]] astring                inspect()   const override;
    [[nodiscard]] object_type            type()      const override;
    [[nodiscard]] astring                data()      const override;
    [[nodiscard]] auto_increment_object* clone()     const override;
    [[nodiscard]] astring                serialize() const override;
};

class current_timestamp_object : virtual public serializable {

    public:
    [[nodiscard]] astring                   inspect()   const override;
    [[nodiscard]] object_type               type()      const override;
    [[nodiscard]] astring                   data()      const override;
    [[nodiscard]] current_timestamp_object* clone()     const override;
    [[nodiscard]] astring                   serialize() const override;
};

class primary_key_object : virtual public serializable {

    public:
    primary_key_object(astring set_col_name);

    [[nodiscard]] astring             inspect()   const override;
    [[nodiscard]] object_type         type()      const override;
    [[nodiscard]] astring             data()      const override;
    [[nodiscard]] primary_key_object* clone()     const override;
    [[nodiscard]] astring             serialize() const override;

    astring column_name;
};

class delimiter_object : virtual public serializable {

    public:
    delimiter_object(astring set_value);

    [[nodiscard]] astring           inspect()   const override;
    [[nodiscard]] object_type       type()      const override;
    [[nodiscard]] astring           data()      const override;
    [[nodiscard]] delimiter_object* clone()     const override;
    [[nodiscard]] astring           serialize() const override;

    astring value;
};


template<typename ColumnNameType>
    requires PotentiallyInheritsObject <ColumnNameType>
class column_index_object_mixin {

    public:
    column_index_object_mixin(const std_and_astring_variant& set_tab_name, ColumnNameType* set_column_name) : column_name(UP<ColumnNameType>(set_column_name)) {
        visit(set_tab_name, [&](const auto& unwrapped) {
            table_name = unwrapped;
        });   
    }
    column_index_object_mixin(const std_and_astring_variant& set_tab_name, UP<ColumnNameType> set_column_name) : column_name(std::move(set_column_name)) {
        visit(set_tab_name, [&](const auto& unwrapped) {
            table_name = unwrapped;
        });   
    }


    protected:
    [[nodiscard]] astring inspect_impl(const astring& object_type_prefix) const {
        bool need_space = object_type_prefix.size() != 0;
        return "[" + object_type_prefix + (need_space ? " " : "") + "Column Index: " + table_name + "." + column_name->inspect() + "]";
    }

    public:
    astring table_name;
    UP<ColumnNameType> column_name;
};

template<typename T>
struct is_column_index_object_mixin {
    private:
    template<typename U>
    static std::true_type  test(const column_index_object_mixin<U>*);

    static std::false_type test(...);

    public:
    static constexpr bool value = decltype(test(std::declval<const T*>()))::value;
};

template<typename T>
concept IsColumnIndexObject = is_column_index_object_mixin<std::remove_cvref_t<T>>::value;

class column_index_object : public object, public column_index_object_mixin<object> {
    public:
    using column_index_object_mixin::column_index_object_mixin;

    [[nodiscard]] astring              inspect() const override { return  inspect_impl(""); }
    [[nodiscard]] object_type          type()    const override { return  COLUMN_INDEX_OBJ;  }
    [[nodiscard]] astring              data()    const override { return "COLUMN_INDEX_OBJ"; }
    [[nodiscard]] column_index_object* clone()   const override { return  new column_index_object(table_name, column_name->clone()); }
};

class e_column_index_object : virtual public evaluated, public column_index_object_mixin<evaluated> {
    public:
    using column_index_object_mixin::column_index_object_mixin;

    [[nodiscard]] astring                inspect() const override { return  inspect_impl("E"); }
    [[nodiscard]] object_type            type()    const override { return  E_COLUMN_INDEX_OBJ;  }
    [[nodiscard]] astring                data()    const override { return "E_COLUMN_INDEX_OBJ"; }
    [[nodiscard]] e_column_index_object* clone()   const override { return  new e_column_index_object(table_name, column_name->clone()); }
};

class s_column_index_object : virtual public serializable, public column_index_object_mixin<serializable> {
    public:
    using column_index_object_mixin::column_index_object_mixin;

    [[nodiscard]] astring                inspect()   const override { return  inspect_impl("S"); }
    [[nodiscard]] object_type            type()      const override { return  S_COLUMN_INDEX_OBJ;  }
    [[nodiscard]] astring                data()      const override { return "S_COLUMN_INDEX_OBJ"; }
    [[nodiscard]] s_column_index_object* clone()     const override { return  new s_column_index_object(table_name, column_name->clone()); }
    [[nodiscard]] astring                serialize() const override { 
        return table_name + "." + column_name->serialize();
    }
};



template<typename IndexType>
    requires IsColumnIndexObject <IndexType>
class foreign_key_object_mixin {

    public:
    foreign_key_object_mixin(IndexType*    set_reference) : reference(UP<IndexType>(set_reference)) {}
    foreign_key_object_mixin(UP<IndexType> set_reference) : reference(std::move(set_reference)) {}

    protected:
    [[nodiscard]] astring inspect_impl(const astring& object_type_prefix) const {
        bool need_space = object_type_prefix.size() != 0;
        return "[" + object_type_prefix + (need_space ? " " : "") + "Foreign Key: " + reference->inspect() + "]";
    }

    public:
    UP<IndexType> reference;
};

template<typename T>
struct is_foreign_key_object_mixin {
    private:
    template<typename U>
    static std::true_type  test(const foreign_key_object_mixin<U>*);

    static std::false_type test(...);

    public:
    static constexpr bool value = decltype(test(std::declval<const T*>()))::value;
};

template<typename T>
concept IsForeignKeyObject = is_foreign_key_object_mixin<std::remove_cvref_t<T>>::value;

class foreign_key_object : public object, public foreign_key_object_mixin<column_index_object> {
    public:
    using foreign_key_object_mixin::foreign_key_object_mixin;

    [[nodiscard]] astring             inspect() const override { return  inspect_impl(""); }
    [[nodiscard]] object_type         type()    const override { return  FOREIGN_KEY_OBJ;  }
    [[nodiscard]] astring             data()    const override { return "FOREIGN_KEY_OBJ"; }
    [[nodiscard]] foreign_key_object* clone()   const override { return  new foreign_key_object(reference->clone()); }
};

class e_foreign_key_object : virtual public evaluated, public foreign_key_object_mixin<e_column_index_object> {
    public:
    using foreign_key_object_mixin::foreign_key_object_mixin;

    [[nodiscard]] astring               inspect() const override { return  inspect_impl("E"); }
    [[nodiscard]] object_type           type()    const override { return  E_FOREIGN_KEY_OBJ;  }
    [[nodiscard]] astring               data()    const override { return "E_FOREIGN_KEY_OBJ"; }
    [[nodiscard]] e_foreign_key_object* clone()   const override { return  new e_foreign_key_object(reference->clone()); }
};

class s_foreign_key_object : virtual public serializable, public foreign_key_object_mixin<s_column_index_object> {
    public:
    using foreign_key_object_mixin::foreign_key_object_mixin;

    [[nodiscard]] astring               inspect()   const override { return  inspect_impl("S"); }
    [[nodiscard]] object_type           type()      const override { return  S_FOREIGN_KEY_OBJ;  }
    [[nodiscard]] astring               data()      const override { return "S_FOREIGN_KEY_OBJ"; }
    [[nodiscard]] s_foreign_key_object* clone()     const override { return  new s_foreign_key_object(reference->clone()); }
    [[nodiscard]] astring               serialize() const override { 
        return "FOREIGN_KEY REFERENCES " + reference->serialize();   
    }
};






class operator_object : virtual public serializable {

    public:
    explicit operator_object(operator_type type);

    [[nodiscard]] astring          inspect()   const override;
    [[nodiscard]] object_type      type()      const override;
    [[nodiscard]] astring          data()      const override;
    [[nodiscard]] operator_object* clone()     const override;
    [[nodiscard]] astring          serialize() const override;

    public:
    operator_type op_type;
};


template<typename ParameterType>
    requires PotentiallyInheritsObject <ParameterType>
class table_column_expr_mixin {

    public:
    table_column_expr_mixin(ParameterType*    set_parameter) : parameter(UP<ParameterType>(set_parameter)) {}
    table_column_expr_mixin(UP<ParameterType> set_parameter) : parameter(std::move(set_parameter)) {}

    protected:
    [[nodiscard]] astring inspect_impl(const astring& object_type_prefix) const {
        bool need_space = object_type_prefix.size() != 0;
        return "[" + object_type_prefix + (need_space ? " " : "") + "Coulumn Expr, " + parameter->inspect() + "]"; 
    }

    public:
    UP<ParameterType> parameter;
};

template<typename T>
struct is_table_coulm_expr_mixin {
    private:
    template<typename U>
    static std::true_type  test(const table_column_expr_mixin<U>*);

    static std::false_type test(...);

    public:
    static constexpr bool value = decltype(test(std::declval<const T*>()))::value;
};

template<typename T>
concept IsTableColumnExpr = is_table_coulm_expr_mixin<std::remove_cvref_t<T>>::value;

class table_column_expr : public object, public table_column_expr_mixin<object> {

    public:
    using table_column_expr_mixin::table_column_expr_mixin;

    [[nodiscard]] astring            inspect() const override { return  inspect_impl(""); }
    [[nodiscard]] object_type        type()    const override { return  TABLE_COLUMN_EXPR_OBJ;  }
    [[nodiscard]] astring            data()    const override { return "TABLE_COLUMN_EXPR_OBJ"; }
    [[nodiscard]] table_column_expr* clone()   const override { return  new table_column_expr(parameter->clone()); }

};

class e_table_column_expr : virtual public evaluated, public table_column_expr_mixin<evaluated> {

    public:
    using table_column_expr_mixin::table_column_expr_mixin;

    [[nodiscard]] astring              inspect() const override { return  inspect_impl("E"); }
    [[nodiscard]] object_type          type()    const override { return  E_TABLE_COLUMN_EXPR_OBJ;  }
    [[nodiscard]] astring              data()    const override { return "E_TABLE_COLUMN_EXPR_OBJ"; }
    [[nodiscard]] e_table_column_expr* clone()   const override { return  new e_table_column_expr(parameter->clone()); }

};

class s_table_column_expr : virtual public serializable, public table_column_expr_mixin<serializable> {

    public:
    using table_column_expr_mixin::table_column_expr_mixin;

    [[nodiscard]] astring              inspect() const override { return  inspect_impl("S"); }
    [[nodiscard]] object_type          type()    const override { return  S_TABLE_COLUMN_EXPR_OBJ;  }
    [[nodiscard]] astring              data()    const override { return "S_TABLE_COLUMN_EXPR_OBJ"; }
    [[nodiscard]] s_table_column_expr* clone()   const override { return  new s_table_column_expr(parameter->clone()); }
    [[nodiscard]] astring serialize() const override { 
        return parameter->serialize();
    }

};




template<typename ParameterType>
    requires PotentiallyInheritsObject <ParameterType>
class table_expr_mixin {

    public:
    table_expr_mixin(ParameterType*    set_parameter) : parameter(UP<ParameterType>(set_parameter)) {}
    table_expr_mixin(UP<ParameterType> set_parameter) : parameter(std::move(set_parameter)) {}

    protected:
    [[nodiscard]] astring inspect_impl(const astring& object_type_prefix) const {
        bool need_space = object_type_prefix.size() != 0;
        return "[" + object_type_prefix + (need_space ? " " : "") + "Table Expr, " + parameter->inspect() + "]"; 
    }

    public:
    UP<ParameterType> parameter;
};

template<typename T>
struct is_table_expr_mixin {
    private:
    template<typename U>
    static std::true_type  test(const table_expr_mixin<U>*);

    static std::false_type test(...);

    public:
    static constexpr bool value = decltype(test(std::declval<const T*>()))::value;
};

template<typename T>
concept IsTableExpr = is_table_expr_mixin<std::remove_cvref_t<T>>::value;

class table_expr : public object, public table_expr_mixin<object> {

    public:
    using table_expr_mixin::table_expr_mixin;

    [[nodiscard]] astring            inspect() const override { return  inspect_impl(""); }
    [[nodiscard]] object_type        type()    const override { return  TABLE_EXPR_OBJ;  }
    [[nodiscard]] astring            data()    const override { return "TABLE_EXPR_OBJ"; }
    [[nodiscard]] table_expr* clone()   const override { return  new table_expr(parameter->clone()); }

};

class e_table_expr : virtual public evaluated, public table_expr_mixin<evaluated> {

    public:
    using table_expr_mixin::table_expr_mixin;

    [[nodiscard]] astring       inspect() const override { return  inspect_impl("E"); }
    [[nodiscard]] object_type   type()    const override { return  E_TABLE_EXPR_OBJ;  }
    [[nodiscard]] astring       data()    const override { return "E_TABLE_EXPR_OBJ"; }
    [[nodiscard]] e_table_expr* clone()   const override { return  new e_table_expr(parameter->clone()); }

};

class s_table_expr : virtual public serializable, public table_expr_mixin<serializable> {

    public:
    using table_expr_mixin::table_expr_mixin;

    [[nodiscard]] astring       inspect() const override { return  inspect_impl("S"); }
    [[nodiscard]] object_type   type()    const override { return  S_TABLE_EXPR_OBJ;  }
    [[nodiscard]] astring       data()    const override { return "S_TABLE_EXPR_OBJ"; }
    [[nodiscard]] s_table_expr* clone()   const override { return  new s_table_expr(parameter->clone()); }
    [[nodiscard]] astring serialize() const override { 
        return parameter->serialize();
    }

};




template<typename ParameterType>
    requires PotentiallyInheritsObject <ParameterType>
class default_value_func_mixin {

    public:
    explicit default_value_func_mixin(ParameterType*    set_parameter) : parameter(UP<ParameterType>(set_parameter)) {}
    explicit default_value_func_mixin(UP<ParameterType> set_parameter) : parameter(std::move(set_parameter)) {}

    [[nodiscard]] astring inspect_impl(const astring& object_type_prefix) const { 
        bool need_space = object_type_prefix.size() != 0;
        return object_type_prefix + (need_space ? " " : "") + "Default Value Function: " + parameter->inspect();
    }

    public:
    UP<ParameterType> parameter;
};

template<typename T>
struct is_default_value_func_mixin {
    private:
    template<typename U>
    static std::true_type  test(const default_value_func_mixin<U>*);

    static std::false_type test(...);

    public:
    static constexpr bool value = decltype(test(std::declval<const T*>()))::value;
};

template<typename T>
concept IsDefaultValueFunctionObject = is_default_value_func_mixin<std::remove_cvref_t<T>>::value;

class default_value_func : public object, public default_value_func_mixin<object> {

    public:
    using default_value_func_mixin::default_value_func_mixin;

    [[nodiscard]] astring             inspect() const override { return  inspect_impl(""); }
    [[nodiscard]] object_type         type()    const override { return  DEFAULT_VALUE_FUNC_OBJ;  }
    [[nodiscard]] astring             data()    const override { return "DEFAULT_VALUE_FUNC_OBJ"; }
    [[nodiscard]] default_value_func* clone()   const override { return  new default_value_func(parameter->clone()); }
};

class e_default_value_func : virtual public evaluated, public default_value_func_mixin<evaluated> {

    public:
    using default_value_func_mixin::default_value_func_mixin;

    [[nodiscard]] astring               inspect() const override { return  inspect_impl("E"); }
    [[nodiscard]] object_type           type()    const override { return  E_DEFAULT_VALUE_FUNC_OBJ;  }
    [[nodiscard]] astring               data()    const override { return "E_DEFAULT_VALUE_FUNC_OBJ"; }
    [[nodiscard]] e_default_value_func* clone()   const override { return  new e_default_value_func(parameter->clone()); }
};

class s_default_value_func : virtual public serializable, public default_value_func_mixin<serializable> {

    public:
    using default_value_func_mixin::default_value_func_mixin;

    [[nodiscard]] astring               inspect()   const override { return  inspect_impl("S"); }
    [[nodiscard]] object_type           type()      const override { return  S_DEFAULT_VALUE_FUNC_OBJ;  }
    [[nodiscard]] astring               data()      const override { return "S_DEFAULT_VALUE_FUNC_OBJ"; }
    [[nodiscard]] s_default_value_func* clone()     const override { return  new s_default_value_func(parameter->clone()); }
    [[nodiscard]] astring               serialize() const override { return  parameter->serialize(); }
};



template<typename SelfType, typename ElementType>
    requires PotentiallyInheritsObject <SelfType>
class group_object_mixin {

    public:
    group_object_mixin(ElementType* obj)             { elements.push_back(UP<ElementType>(obj)); }
    group_object_mixin(UP<ElementType> obj)          { elements.push_back(std::move(obj));       }
    group_object_mixin(avec<UP<ElementType>>&& objs) : elements(std::move(objs))                {}

    protected:
    [[nodiscard]] astring inspect_impl() const {
        astringstream stream;
        bool first = true;
        for (const auto& element : elements) {
            if (!first) { stream << ", "; }
            stream << element->inspect();
            first = false;
        }
        return stream.str();
    }

    SelfType* clone_impl() const {
        avec<UP<ElementType>> cloned_elements;
        cloned_elements.reserve(elements.size());
        
        for (const auto& element : elements) {
            cloned_elements.push_back(UP<ElementType>(element->clone())); }
        
        return new SelfType(std::move(cloned_elements));
    }

    public:
    avec<UP<ElementType>> elements;
};

template<typename T>
struct is_group_object_mixin {
    private:
    template<typename U, typename V>
    static std::true_type  test(const group_object_mixin<U,V>*);

    static std::false_type test(...);

    public:
    static constexpr bool value = decltype(test(std::declval<const T*>()))::value;
};

template<typename T>
concept IsGroupObject = is_group_object_mixin<std::remove_cvref_t<T>>::value;

class group_object : public object, public group_object_mixin<group_object, object> {

    public:
    using group_object_mixin::group_object_mixin; // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)

    ~group_object() noexcept override = default;

    [[nodiscard]] astring inspect()     const override { return  inspect_impl();}
    [[nodiscard]] object_type type()    const override { return  GROUP_OBJ;     }
    [[nodiscard]] astring data()        const override { return "GROUP_OBJ";    }
    [[nodiscard]] group_object* clone() const override { return  clone_impl();  }
};

class e_group_object : virtual public evaluated, public group_object_mixin<e_group_object, evaluated> {

    public:
    using group_object_mixin::group_object_mixin; // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
    ~e_group_object() noexcept override = default;

    [[nodiscard]] astring inspect()       const override { return  inspect_impl();}
    [[nodiscard]] object_type type()      const override { return  E_GROUP_OBJ;   }
    [[nodiscard]] astring data()          const override { return "E_GROUP_OBJ";  }
    [[nodiscard]] e_group_object* clone() const override { return  clone_impl();  }

};

class s_group_object : virtual public serializable, public group_object_mixin<s_group_object, serializable> {

    public:
    using group_object_mixin::group_object_mixin; // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
    ~s_group_object() noexcept override = default;

    [[nodiscard]] astring inspect()       const override { return  inspect_impl();}
    [[nodiscard]] object_type type()      const override { return  S_GROUP_OBJ;   }
    [[nodiscard]] astring data()          const override { return "S_GROUP_OBJ";  }
    [[nodiscard]] s_group_object* clone() const override { return  clone_impl();  }
    [[nodiscard]] astring serialize()    const override { 
        astringstream stream;
        bool first = true;
        for (const auto& element : elements) {
            if (!first) { stream << ", "; }
            stream << element->serialize();
            first = false;
        }
        return stream.str();
    }
};




template<typename ConstraintType>
    requires PotentiallyInheritsObject <ConstraintType>
class constraint_object_mixin {
    public:
    constraint_object_mixin(ConstraintType* set_constraint, token_type set_method) : 
        constraint(UP<ConstraintType>(set_constraint)),
        method(set_method)
    {}
    constraint_object_mixin(UP<ConstraintType> set_constraint, token_type set_method) : 
        constraint(std::move(set_constraint)),
        method(set_method)
    {}

    public:
    UP<ConstraintType> constraint;
    token_type method;
};

template<typename T>
struct is_constraint_object_mixin {
    private:
    template<typename U>
    static std::true_type  test(const constraint_object_mixin<U>*);

    static std::false_type test(...);

    public:
    static constexpr bool value = decltype(test(std::declval<const T*>()))::value;
};

template<typename T>
concept IsConstraintObject = is_constraint_object_mixin<std::remove_cvref_t<T>>::value;

class constraint_object : public object, public constraint_object_mixin<object> {

    public:
    using constraint_object_mixin::constraint_object_mixin;

    [[nodiscard]] astring inspect() const override {  
        return "[Constraint: " + constraint->inspect() + ", Method: " + token_type_to_string(method) + "]"; 
    }
    [[nodiscard]] object_type        type()  const override { return  CONSTRAINT_OBJ;  }
    [[nodiscard]] astring            data()  const override { return "CONSTRAINT_OBJ"; }
    [[nodiscard]] constraint_object* clone() const override { return  new constraint_object(constraint->clone(), method); }
};

class e_constraint_object : virtual public evaluated, public constraint_object_mixin<evaluated> {

    public:
    using constraint_object_mixin::constraint_object_mixin;

    [[nodiscard]] astring inspect() const override {  
        return "[E Constraint: " + constraint->inspect() + ", Method: " + token_type_to_string(method) + "]"; 
    }
    [[nodiscard]] object_type          type()  const override { return  E_CONSTRAINT_OBJ;  }
    [[nodiscard]] astring              data()  const override { return "E_CONSTRAINT_OBJ"; }
    [[nodiscard]] e_constraint_object* clone() const override { return  new e_constraint_object(constraint->clone(), method); }
};

class s_constraint_object : virtual public serializable, public constraint_object_mixin<serializable> {

    public:
    using constraint_object_mixin::constraint_object_mixin;

    [[nodiscard]] astring inspect() const override {  
        return "[S Constraint: " + constraint->inspect() + ", Method: " + token_type_to_string(method) + "]"; 
    }
    [[nodiscard]] object_type          type()      const override { return  S_CONSTRAINT_OBJ;  }
    [[nodiscard]] astring              data()      const override { return "S_CONSTRAINT_OBJ"; }
    [[nodiscard]] s_constraint_object* clone()     const override { return  new s_constraint_object(constraint->clone(), method); }
    [[nodiscard]] astring              serialize() const override { 
        return "CONSTRAINT " + constraint->serialize() + " " + token_type_to_string(method); 
    }
};



template<typename ReferenceType>
    requires IsGroupObject <ReferenceType>
class hash_object_mixin {
    public:
    hash_object_mixin(ReferenceType*    set_reference) : reference(UP<ReferenceType>(set_reference)) {}
    hash_object_mixin(UP<ReferenceType> set_reference) : reference(std::move(set_reference)) {}

    public:
    UP<ReferenceType> reference;
};

template<typename T>
struct is_hash_object_mixin {
    private:
    template<typename U>
    static std::true_type  test(const hash_object_mixin<U>*);

    static std::false_type test(...);

    public:
    static constexpr bool value = decltype(test(std::declval<const T*>()))::value;
};

template<typename T>
concept IsHashObject = is_hash_object_mixin<std::remove_cvref_t<T>>::value;

class hash_object : public object, public hash_object_mixin<group_object> {

    public:
    using hash_object_mixin::hash_object_mixin;

    [[nodiscard]] astring      inspect() const override { return "Hash(" + reference->inspect() + ")"; }
    [[nodiscard]] object_type  type()    const override { return  HASH_OBJ;  }
    [[nodiscard]] astring      data()    const override { return "HASH_OBJ"; }
    [[nodiscard]] hash_object* clone()   const override { return  new hash_object(reference->clone()); }
};

class e_hash_object : virtual public evaluated, public hash_object_mixin<e_group_object> {

    public:
    using hash_object_mixin::hash_object_mixin;

    [[nodiscard]] astring        inspect() const override { return "E Hash(" + reference->inspect() + ")"; }
    [[nodiscard]] object_type    type()    const override { return  E_HASH_OBJ;  }
    [[nodiscard]] astring        data()    const override { return "E_HASH_OBJ"; }
    [[nodiscard]] e_hash_object* clone()   const override { return  new e_hash_object(reference->clone()); }
};

class s_hash_object : virtual public serializable, public hash_object_mixin<s_group_object> {

    public:
    using hash_object_mixin::hash_object_mixin;

    [[nodiscard]] astring        inspect()   const override { return "S Hash(" + reference->inspect() + ")"; }
    [[nodiscard]] object_type    type()      const override { return  S_HASH_OBJ;  }
    [[nodiscard]] astring        data()      const override { return "S_HASH_OBJ"; }
    [[nodiscard]] s_hash_object* clone()     const override { return  new s_hash_object(reference->clone());  }
    [[nodiscard]] astring        serialize() const override { return "HASH(" + reference->serialize() + ")"; }
};





class table_object;
class table_info_object : virtual public evaluated {

    public:
    table_info_object(const std_and_astring_variant& set_tab_name, avec<size_t> set_col_ids, avec<size_t> set_row_ids);
    ~table_info_object() noexcept override = default;

    [[nodiscard]] astring            inspect() const override;
    [[nodiscard]] object_type        type()    const override;
    [[nodiscard]] astring            data()    const override;
    [[nodiscard]] table_info_object* clone()   const override;

    public:
    astring table_name;
    avec<size_t> col_ids;
    avec<size_t> row_ids;
};

class f_table_info_object : virtual public serializable {

    public:
    f_table_info_object(SP<table_object> set_table, avec<size_t> set_col_ids, avec<size_t> set_row_ids);
    ~f_table_info_object() noexcept override = default;

    [[nodiscard]] astring              inspect()   const override;
    [[nodiscard]] object_type          type()      const override;
    [[nodiscard]] astring              data()      const override;
    [[nodiscard]] f_table_info_object* clone()     const override;
    [[nodiscard]] astring              serialize() const override;

    public:
    SP<table_object> table;
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

class infix_expr_object : public expression_object {

    public:
    infix_expr_object(operator_object* set_op, object* set_left, object* set_right);
    infix_expr_object(UP<operator_object> set_op, UP<object> set_left, UP<object> set_right);
    ~infix_expr_object() noexcept override = default;
    
    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] infix_expr_object* clone() const override;

    [[nodiscard]] operator_type get_op_type() const override;

    public:
    UP<operator_object> op;
    UP<object> left;
    UP<object> right;
};

class e_infix_expr_object : virtual public e_expression_object {

    public:
    e_infix_expr_object(operator_object* set_op, evaluated* set_left, evaluated* set_right);
    e_infix_expr_object(UP<operator_object> set_op, UP<evaluated> set_left, UP<evaluated> set_right);
    ~e_infix_expr_object() noexcept override = default;
    
    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] e_infix_expr_object* clone() const override;

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

class integer_object : virtual public serializable {

    public:
    integer_object();
    integer_object(int val);
    integer_object(const std::string& val);
    integer_object(const astring& val);

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] integer_object* clone() const override;
    [[nodiscard]] astring serialize() const override;

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

class decimal_object : virtual public serializable {

    public:
    decimal_object();
    decimal_object(double val);
    decimal_object(const std::string& val);
    decimal_object(const astring& val);

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] decimal_object* clone() const override;
    [[nodiscard]] astring serialize() const override;

    public:
    double value; // value can be cast to float later
};

class string_object : virtual public serializable {

    public:
    string_object(const std_and_astring_variant& val);
    ~string_object() noexcept override = default;

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] string_object* clone() const override;
    [[nodiscard]] astring serialize() const override;

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

    [[nodiscard]] astring          inspect() const override;
    [[nodiscard]] object_type      type()    const override;
    [[nodiscard]] astring          data()    const override;
    [[nodiscard]] argument_object* clone()   const override;

    public:
    astring name;
    UP<object> value;
};

class e_argument_object : virtual public evaluated {

    public:
    e_argument_object(const std_and_astring_variant& set_name, evaluated* val);
    e_argument_object(const std_and_astring_variant& set_name, UP<evaluated> val);
    ~e_argument_object() noexcept override = default;

    [[nodiscard]] astring            inspect() const override;
    [[nodiscard]] object_type        type()    const override;
    [[nodiscard]] astring            data()    const override;
    [[nodiscard]] e_argument_object* clone()   const override;

    public:
    astring name;
    UP<evaluated> value;
};

class s_argument_object : virtual public serializable {

    public:
    s_argument_object(const std_and_astring_variant& set_name, serializable* val);
    s_argument_object(const std_and_astring_variant& set_name, UP<serializable> val);
    ~s_argument_object() noexcept override = default;

    [[nodiscard]] astring            inspect()   const override;
    [[nodiscard]] object_type        type()      const override;
    [[nodiscard]] astring            data()      const override;
    [[nodiscard]] s_argument_object* clone()     const override;
    [[nodiscard]] astring            serialize() const override;

    public:
    astring name;
    UP<serializable> value;
};


class variable_object : public object {

    public:
    variable_object(const std_and_astring_variant& set_name, object* val);
    variable_object(const std_and_astring_variant& set_name, UP<object> val);
    ~variable_object() noexcept override = default;

    [[nodiscard]] astring          inspect() const override;
    [[nodiscard]] object_type      type()    const override;
    [[nodiscard]] astring          data()    const override;
    [[nodiscard]] variable_object* clone()   const override;

    public:
    astring name;
    UP<object> value;
};

class e_variable_object : virtual public evaluated {

    public:
    e_variable_object(const std_and_astring_variant& set_name, evaluated* val);
    e_variable_object(const std_and_astring_variant& set_name, UP<evaluated> val);
    ~e_variable_object() noexcept override = default;

    [[nodiscard]] astring            inspect() const override;
    [[nodiscard]] object_type        type()    const override;
    [[nodiscard]] astring            data()    const override;
    [[nodiscard]] e_variable_object* clone()   const override;

    public:
    astring name;
    UP<evaluated> value;
};

// TODO Maybe should be final instead of serializable (f_variable_object not s_variable_object and error if serialize is called)
class s_variable_object : virtual public serializable {

    public:
    s_variable_object(const std_and_astring_variant& set_name, serializable* val);
    s_variable_object(const std_and_astring_variant& set_name, UP<serializable> val);
    ~s_variable_object() noexcept override = default;

    [[nodiscard]] astring            inspect()   const override;
    [[nodiscard]] object_type        type()      const override;
    [[nodiscard]] astring            data()      const override;
    [[nodiscard]] s_variable_object* clone()     const override;
    [[nodiscard]] astring            serialize() const override;

    public:
    astring name;
    UP<serializable> value;
};

class boolean_object : virtual public serializable {

    public:
    boolean_object(bool val);

    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] boolean_object* clone() const override;
    [[nodiscard]] astring serialize() const override;
    
    public:
    bool value;
};


//zerofill is implicitly unsigned im pretty sure
// TODO Maybe want to remove NONE token and use optional instead? Everywhere not just here.
template<typename ParameterType>
class SQL_data_type_mixin {
    public:
    SQL_data_type_mixin(token_type set_prefix, token_type set_data_type, ParameterType* set_parameter) 
        : prefix(set_prefix), parameter(UP<ParameterType>(set_parameter))
    {
        if (set_data_type == NONE) {
            FATAL_ERROR_STACK_TRACE_THROW("SQL data type object constructed without data type token", CUR_LOC); }

        data_type = set_data_type;
    }

    SQL_data_type_mixin(token_type set_prefix, token_type set_data_type, UP<ParameterType> set_parameter) 
        : prefix(set_prefix), parameter(std::move(set_parameter))
    {
        if (set_data_type == NONE) {
            FATAL_ERROR_STACK_TRACE_THROW("SQL data type object constructed without data type token", CUR_LOC); }

        data_type = set_data_type;
    }

    SQL_data_type_mixin(token_type set_prefix, token_type set_data_type, std::optional<UP<ParameterType>> set_parameter) 
        : prefix(set_prefix), parameter(std::move(set_parameter))
    {
        if (set_data_type == NONE) {
            FATAL_ERROR_STACK_TRACE_THROW("SQL data type object constructed without data type token", CUR_LOC); }

        data_type = set_data_type;
    }

    SQL_data_type_mixin(token_type set_prefix, token_type set_data_type) 
        : prefix(set_prefix)
    {
        if (set_data_type == NONE) {
            FATAL_ERROR_STACK_TRACE_THROW("SQL data type object constructed without data type token", CUR_LOC); }

        data_type = set_data_type;
    }

    protected:
    [[nodiscard]] astring inspect_impl(const astring& object_type_prefix) const {
        astringstream stream;

        bool need_space = object_type_prefix.size() != 0;
        stream << "[" << object_type_prefix  << (need_space ? " " : "") << "SQL data type, Data type: ";

        if (prefix != NONE) {
            stream << token_type_to_string(prefix) << ", "; }
            
        stream << token_type_to_string(data_type);
        
        if (parameter.has_value()) {
            stream << "(" + parameter.value()->inspect() + ")"; }

        stream << "]";
        return stream.str();
    }

    public:
    token_type prefix;
    token_type data_type;
    std::optional<UP<ParameterType>> parameter;
};

template<typename T>
struct is_SQL_data_type_mixin {
    private:
    template<typename U>
    static std::true_type  test(const SQL_data_type_mixin<U>*);

    static std::false_type test(...);

    public:
    static constexpr bool value = decltype(test(std::declval<const T*>()))::value;
};

template<typename T>
concept IsSQLDataTypeObject = is_SQL_data_type_mixin<std::remove_cvref_t<T>>::value;

class SQL_data_type_object: public object, public SQL_data_type_mixin<object> {

    public:
    using SQL_data_type_mixin::SQL_data_type_mixin;
    
    ~SQL_data_type_object() noexcept override = default;
    
    [[nodiscard]] astring               inspect() const override { return inspect_impl(""); }
    [[nodiscard]] object_type           type()    const override { return SQL_DATA_TYPE_OBJ; }
    [[nodiscard]] astring               data()    const override { return parameter.has_value() ? parameter.value()->data() : "DT NO PARAM"; }
    [[nodiscard]] SQL_data_type_object* clone()   const override {
        if (parameter.has_value()) {
            return new SQL_data_type_object(prefix, data_type, parameter.value()->clone());
        } else {
            return new SQL_data_type_object(prefix, data_type);
        }
    }
};

class e_SQL_data_type_object : virtual public evaluated, public SQL_data_type_mixin<evaluated> {
    
    public:
    using SQL_data_type_mixin::SQL_data_type_mixin;
    
    ~e_SQL_data_type_object() noexcept override = default;
    
    [[nodiscard]] astring                 inspect() const override { return inspect_impl("E"); }
    [[nodiscard]] object_type             type()    const override { return E_SQL_DATA_TYPE_OBJ; }
    [[nodiscard]] astring                 data()    const override { return parameter.has_value() ? parameter.value()->data() : "DT NO PARAM"; }
    [[nodiscard]] e_SQL_data_type_object* clone()   const override {
        if (parameter.has_value()) {
            return new e_SQL_data_type_object(prefix, data_type, parameter.value()->clone());
        } else {
            return new e_SQL_data_type_object(prefix, data_type);
        }
    }
};

class s_SQL_data_type_object : virtual public serializable, public SQL_data_type_mixin<serializable> {
    
    public:
    using SQL_data_type_mixin::SQL_data_type_mixin;
    
    ~s_SQL_data_type_object() noexcept override = default;
    
    [[nodiscard]] astring                 inspect() const override { return inspect_impl("S"); }
    [[nodiscard]] object_type             type()    const override { return S_SQL_DATA_TYPE_OBJ; }
    [[nodiscard]] astring                 data()    const override { return parameter.has_value() ? parameter.value()->data() : "DT NO PARAM"; }
    [[nodiscard]] s_SQL_data_type_object* clone()   const override {
        if (parameter.has_value()) {
            return new s_SQL_data_type_object(prefix, data_type, parameter.value()->clone());
        } else {
            return new s_SQL_data_type_object(prefix, data_type);
        }
    }
    [[nodiscard]] astring serialize() const override {
        astringstream stream;

        if (prefix != NONE) {
            stream << token_type_to_string(prefix) << ", "; }
            
        stream << token_type_to_string(data_type);
        
        if (parameter.has_value()) {
            stream << "(" << parameter.value()->serialize() << ")"; }

        return stream.str();
    }
};



template<typename GroupType>
    requires IsGroupObject <GroupType>
class unique_object_mixin {
    public:
    unique_object_mixin(GroupType*    set_group) : group(UP<GroupType>(set_group)) {}
    unique_object_mixin(UP<GroupType> set_group) : group(std::move(set_group)) {}

    public:
    UP<GroupType> group;
};

template<typename T>
struct is_unique_object_mixin {
    private:
    template<typename U>
    static std::true_type  test(const unique_object_mixin<U>*);

    static std::false_type test(...);

    public:
    static constexpr bool value = decltype(test(std::declval<const T*>()))::value;
};

template<typename T>
concept IsUniqueObject = is_unique_object_mixin<std::remove_cvref_t<T>>::value;

class unique_object : public object, public unique_object_mixin<group_object> {

    public:
    using unique_object_mixin::unique_object_mixin;

    [[nodiscard]] astring        inspect() const override { return "[Unique: " + group->inspect() + "]"; }
    [[nodiscard]] object_type    type()    const override { return  UNIQUE_OBJ;  }
    [[nodiscard]] astring        data()    const override { return "UNIQUE_OBJ"; }
    [[nodiscard]] unique_object* clone()   const override { return  new unique_object(group->clone()); }
};

class e_unique_object : virtual public evaluated, public unique_object_mixin<e_group_object> {

    public:
    using unique_object_mixin::unique_object_mixin;

    [[nodiscard]] astring        inspect()   const override { return "[E Unique: " + group->inspect() + "]"; }
    [[nodiscard]] object_type    type()      const override { return  E_UNIQUE_OBJ;  }
    [[nodiscard]] astring        data()      const override { return "E_UNIQUE_OBJ"; }
    [[nodiscard]] e_unique_object* clone()   const override { return  new e_unique_object(group->clone()); }
};

class s_unique_object : virtual public serializable, public unique_object_mixin<s_group_object> {

    public:
    using unique_object_mixin::unique_object_mixin;

    [[nodiscard]] astring          inspect()   const override { return "[S Unique: " + group->inspect() + "]"; }
    [[nodiscard]] object_type      type()      const override { return  S_UNIQUE_OBJ;  }
    [[nodiscard]] astring          data()      const override { return "S_UNIQUE_OBJ"; }
    [[nodiscard]] s_unique_object* clone()     const override { return  new s_unique_object(group->clone()); }
    [[nodiscard]] astring          serialize() const override { return "UNIQUE(" + group->serialize() + ")"; }
};




template<typename ValuesType>
    requires IsGroupObject <ValuesType>
class parameter_object_mixin {
    public:
    parameter_object_mixin(const std_and_astring_variant& set_name, ValuesType* set_values) 
        : values(UP<ValuesType>(set_values)) {
        visit(set_name, [&](const auto& unwrapped) {
            name = unwrapped;
        });        
    }
    parameter_object_mixin(const std_and_astring_variant& set_name, UP<ValuesType> set_values) 
        : values(std::move(set_values)) {
        visit(set_name, [&](const auto& unwrapped) {
            name = unwrapped;
        });
    }
   

    protected:
    [[nodiscard]] astring inspect_impl(const astring& object_type_prefix) const {
        bool need_space = object_type_prefix.size() != 0;
        return astring() + "[Type: " + object_type_prefix + (need_space ? " " : "") + "Parameter, Name: " + name + ", Values: " + values->inspect() + "]";
    }

    public:
    astring name;
    UP<ValuesType> values;
};

template<typename T>
struct is_parameter_object_mixin {
    private:
    template<typename U>
    static std::true_type  test(const parameter_object_mixin<U>*);

    static std::false_type test(...);

    public:
    static constexpr bool value = decltype(test(std::declval<const T*>()))::value;
};

template<typename T>
concept IsParameterObject = is_parameter_object_mixin<std::remove_cvref_t<T>>::value;

class parameter_object : public object, public parameter_object_mixin<group_object> {

    public:
    using parameter_object_mixin::parameter_object_mixin;
    ~parameter_object() noexcept override = default;

    [[nodiscard]] astring inspect()     const override     { return  inspect_impl("");}
    [[nodiscard]] object_type type()    const override     { return  PARAMETER_OBJ;   }
    [[nodiscard]] astring data()        const override     { return "PARAMETER_OBJ";  }
    [[nodiscard]] parameter_object* clone() const override {
        return new parameter_object(name, values->clone());
    }
};

class e_parameter_object : virtual public evaluated, public parameter_object_mixin<e_group_object> {

    public:
    using parameter_object_mixin::parameter_object_mixin;
    ~e_parameter_object() noexcept override = default;

    [[nodiscard]] astring inspect()     const override       { return  inspect_impl("E");}
    [[nodiscard]] object_type type()    const override       { return  E_PARAMETER_OBJ;  }
    [[nodiscard]] astring data()        const override       { return "E_PARAMETER_OBJ"; }
    [[nodiscard]] e_parameter_object* clone() const override {
        return new e_parameter_object(name, values->clone());
    }
};

class s_parameter_object : virtual public serializable, public parameter_object_mixin<s_group_object> {

    public:
    using parameter_object_mixin::parameter_object_mixin;
    ~s_parameter_object() noexcept override = default;

    [[nodiscard]] astring inspect()     const override       { return  inspect_impl("S");}
    [[nodiscard]] object_type type()    const override       { return  S_PARAMETER_OBJ;  }
    [[nodiscard]] astring data()        const override       { return "S_PARAMETER_OBJ"; }
    [[nodiscard]] s_parameter_object* clone() const override { 
        return new s_parameter_object(name, values->clone());
    } 
    [[nodiscard]] astring serialize() const override { 
        astringstream stream;
        stream << "(" << name << ", " << values->serialize() << ")";
        return stream.str();
    } 
};



template<typename ValueType>
    requires PotentiallyInheritsObject <ValueType>
class default_value_object_mixin {
    public:
    default_value_object_mixin(ValueType* set_value) 
        : value(UP<ValueType>(set_value)) {}
    default_value_object_mixin(UP<ValueType> set_value) 
        : value(std::move(set_value)) {}

    public:
    UP<ValueType> value;
};

template<typename T>
struct is_default_value_object_mixin {
    private:
    template<typename U>
    static std::true_type  test(const default_value_object_mixin<U>*);

    static std::false_type test(...);

    public:
    static constexpr bool value = decltype(test(std::declval<const T*>()))::value;
};

template<typename T>
concept IsDefaultValueObject = is_default_value_object_mixin<std::remove_cvref_t<T>>::value;

class default_value_object : public object, public default_value_object_mixin<object> {

    public:
    using default_value_object_mixin::default_value_object_mixin;

    [[nodiscard]] astring               inspect() const override { return "[Default Value: " + value->inspect() + "]";}
    [[nodiscard]] object_type           type()    const override { return  DEFAULT_VALUE_OBJ;  }
    [[nodiscard]] astring               data()    const override { return "DEFAULT_VALUE_OBJ"; }
    [[nodiscard]] default_value_object* clone()   const override { return new default_value_object(value->clone()); }
};

class e_default_value_object : public evaluated, public default_value_object_mixin<evaluated> {

    public:
    using default_value_object_mixin::default_value_object_mixin;

    [[nodiscard]] astring                 inspect() const override { return "[E Default Value: " + value->inspect() + "]";}
    [[nodiscard]] object_type             type()    const override { return  E_DEFAULT_VALUE_OBJ;  }
    [[nodiscard]] astring                 data()    const override { return "E_DEFAULT_VALUE_OBJ"; }
    [[nodiscard]] e_default_value_object* clone()   const override { return new e_default_value_object(value->clone()); }
};

class s_default_value_object : public serializable, public default_value_object_mixin<serializable> {

    public:
    using default_value_object_mixin::default_value_object_mixin;

    [[nodiscard]] astring                 inspect()   const override { return "[S Default Value: " + value->inspect() + "]";}
    [[nodiscard]] object_type             type()      const override { return  S_DEFAULT_VALUE_OBJ;  }
    [[nodiscard]] astring                 data()      const override { return "S_DEFAULT_VALUE_OBJ"; }
    [[nodiscard]] s_default_value_object* clone()     const override { return new s_default_value_object(value->clone()); }
    [[nodiscard]] astring                 serialize() const override { 
        if (value->type() == S_DEFAULT_VALUE_FUNC_OBJ) {
            return value->serialize(); }
        return "DEFAULT " + value->serialize(); }
};




template<typename SelfType, typename DataType, typename DefaultType, typename ExprType>
    requires PotentiallyInheritsObject <SelfType>
          && IsSQLDataTypeObject       <DataType>
          && IsDefaultValueObject      <DefaultType>
          && IsTableColumnExpr         <ExprType>
class table_detail_mixin {

    public:
    // Raw
    table_detail_mixin(astring set_name, DataType* set_data_type, DefaultType* set_default_value, avec<UP<ExprType>>&& set_exprs)
        : name(set_name), data_type(UP<DataType>(set_data_type)), default_value(UP<DefaultType>(set_default_value)), exprs(std::move(set_exprs)) {}
    
    table_detail_mixin(astring set_name, DataType* set_data_type, avec<UP<ExprType>>&& set_exprs)
        : name(set_name), data_type(UP<DataType>(set_data_type)), default_value(std::nullopt), exprs(std::move(set_exprs)) {}


    // UP
    table_detail_mixin(astring set_name, UP<DataType> set_data_type, UP<DefaultType> set_default_value,  UP<ExprType> set_exprs)
        : name(set_name), data_type(std::move(set_data_type)), default_value(std::move(set_default_value)) {
            exprs.push_back(std::move(set_exprs));
        }

    table_detail_mixin(astring set_name, UP<DataType> set_data_type, UP<DefaultType> set_default_value)
        : name(set_name), data_type(std::move(set_data_type)), default_value(std::move(set_default_value)) {}

    table_detail_mixin(astring set_name, UP<DataType> set_data_type, UP<ExprType> set_exprs)
        : name(set_name), data_type(std::move(set_data_type)), default_value(std::nullopt) {
            exprs.push_back(std::move(set_exprs));
        }

    table_detail_mixin(astring set_name, UP<DataType> set_data_type)
        : name(set_name), data_type(std::move(set_data_type)), default_value(std::nullopt) {}



    table_detail_mixin(astring set_name, UP<DataType> set_data_type, std::optional<UP<DefaultType>> set_default_value,  avec<UP<ExprType>>&& set_exprs)
        : name(set_name), data_type(std::move(set_data_type)), default_value(std::move(set_default_value)), exprs(std::move(set_exprs)) {}



    protected:
    [[nodiscard]] astring inspect_impl(const astring& object_type_prefix) const {
        astringstream stream;
        stream << "[Type: " << object_type_prefix << " Table detail, Name: " << name << ", " << data_type->inspect();

        if (default_value.has_value()) {
            stream << ", " << default_value.value()->inspect();
        }

        stream << ", Expressions: ";
        bool first = true;
        for (const auto& expr : exprs) {
            if (!first) { stream << ", "; }
            stream << expr->inspect();
            first = false;
        }

        stream << "]";
        return stream.str();
    }

    [[nodiscard]] SelfType* clone_impl() const {
        avec<UP<ExprType>> cloned_exprs;
        cloned_exprs.reserve(exprs.size());
        for (const auto& expr : exprs) {
            cloned_exprs.emplace_back(UP<ExprType>(expr->clone())); }

        if (default_value.has_value()) {
            return new SelfType(name, data_type->clone(), default_value.value()->clone(), std::move(cloned_exprs));
        } else {
            return new SelfType(name, data_type->clone(), std::move(cloned_exprs));
        }
    }

    public:
    astring name;
    UP<DataType> data_type;
    std::optional<UP<DefaultType>> default_value;
    avec<UP<ExprType>> exprs;
};

class table_detail_object : public object, public table_detail_mixin<table_detail_object, SQL_data_type_object, default_value_object, table_column_expr> {

    public:
    using table_detail_mixin::table_detail_mixin; // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
    
    ~table_detail_object() noexcept override = default;
    
    [[nodiscard]] astring              inspect() const override { return inspect_impl(""); }
    [[nodiscard]] object_type          type()    const override { return TABLE_DETAIL_OBJ; }
    [[nodiscard]] astring              data()    const override { return name; }
    [[nodiscard]] table_detail_object* clone()   const override { return clone_impl(); }
};

class e_table_detail_object : virtual public evaluated, public table_detail_mixin<e_table_detail_object, e_SQL_data_type_object, e_default_value_object, e_table_column_expr> {

    public:
    using table_detail_mixin::table_detail_mixin; // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
    
    ~e_table_detail_object() noexcept override = default;
    
    [[nodiscard]] astring                inspect() const override { return inspect_impl("E");  }
    [[nodiscard]] object_type            type()    const override { return E_TABLE_DETAIL_OBJ; }
    [[nodiscard]] astring                data()    const override { return name; }
    [[nodiscard]] e_table_detail_object* clone()   const override { return clone_impl(); }
};

class s_table_detail_object : virtual public serializable, public table_detail_mixin<s_table_detail_object, s_SQL_data_type_object, s_default_value_object, s_table_column_expr> {

    public:
    using table_detail_mixin::table_detail_mixin; // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
    
    ~s_table_detail_object() noexcept override = default;
    
    [[nodiscard]] astring                inspect() const override { return inspect_impl("S"); }
    [[nodiscard]] object_type            type()    const override { return S_TABLE_DETAIL_OBJ; }
    [[nodiscard]] astring                data()    const override { return name; }
    [[nodiscard]] s_table_detail_object* clone()   const override { return clone_impl(); }
    [[nodiscard]] astring serialize() const override { 
        astringstream stream;
        stream << name << " ";
        stream << data_type->serialize();
        if (default_value.has_value()) {
            stream << " " + default_value.value()->serialize(); }

        for (const auto& expr : exprs) {
            stream << " " << expr->serialize(); }

        return stream.str();
    }
};



// Have to put this guy earlier for function_object
template<typename SelfType, typename BodyType>
    requires PotentiallyInheritsObject <SelfType>
class block_statement_mixin {

    public:
    block_statement_mixin(avec<UP<BodyType>>&& set_body) : 
        body(std::move(set_body)) 
    {}

    protected:
    [[nodiscard]] astring inspect_impl() const {
        astringstream stream;
        for (const auto& statement : body) {
            stream << statement->inspect() << "\n";}
        return stream.str();
    }

    [[nodiscard]] SelfType* clone_impl() const {
        avec<UP<BodyType>> cloned_body;
        cloned_body.reserve(body.size());
        
        for (const auto& statement : body) {
            cloned_body.push_back(UP<BodyType>(statement->clone())); }

        return new SelfType(std::move(cloned_body));
    }

    public:
    avec<UP<BodyType>> body;
};

template<typename T>
struct is_block_statement_mixin {
    private:
    template<typename U, typename V>
    static std::true_type  test(const block_statement_mixin<U,V>*);

    static std::false_type test(...);

    public:
    static constexpr bool value = decltype(test(std::declval<const T*>()))::value;
};

template<typename T>
concept IsBlockStatement = is_block_statement_mixin<std::remove_cvref_t<T>>::value;

class block_statement : public object, public block_statement_mixin<block_statement, object> {

    public:
    using block_statement_mixin::block_statement_mixin; // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)

    ~block_statement() noexcept override = default;

    [[nodiscard]] astring inspect()        const override { return  inspect_impl();  }
    [[nodiscard]] object_type type()       const override { return  BLOCK_STATEMENT; }
    [[nodiscard]] astring data()           const override { return "BLOCK_STATEMENT";}
    [[nodiscard]] block_statement* clone() const override { return  clone_impl();    }
};

class e_block_statement : virtual public evaluated, public block_statement_mixin<e_block_statement, evaluated> {

    public:
    using block_statement_mixin::block_statement_mixin; // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)

    ~e_block_statement() noexcept override = default;

    [[nodiscard]] astring inspect()          const override { return  inspect_impl();    }
    [[nodiscard]] object_type type()         const override { return  E_BLOCK_STATEMENT; }
    [[nodiscard]] astring data()             const override { return "E_BLOCK_STATEMENT";}
    [[nodiscard]] e_block_statement* clone() const override { return  clone_impl();      }
};

class s_block_statement : virtual public serializable, public block_statement_mixin<s_block_statement, serializable> {

    public:
    using block_statement_mixin::block_statement_mixin; // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)

    ~s_block_statement() noexcept override = default;

    [[nodiscard]] astring inspect()          const override { return  inspect_impl();    }
    [[nodiscard]] object_type type()         const override { return  S_BLOCK_STATEMENT; }
    [[nodiscard]] astring data()             const override { return "S_BLOCK_STATEMENT";}
    [[nodiscard]] s_block_statement* clone() const override { return  clone_impl();      }
    [[nodiscard]] astring serialize()        const override { 
        astringstream stream;
        for (const auto& stmt : body) {
            stream << stmt->serialize() << "\n";}
        return stream.str();      
    }
};



template<typename SelfType, typename ParameterType, typename ReturnType, typename BodyType>
    requires IsSQLDataTypeObject       <ReturnType>
          && IsBlockStatement          <BodyType>
          && PotentiallyInheritsObject <SelfType>
class function_object_mixin {

    public:
    function_object_mixin(const std_and_astring_variant& set_name, ParameterType* set_parameters, ReturnType* set_return_type, BodyType* set_body)
        : parameters(UP<ParameterType>(set_parameters)), return_type(std::move(UP<ReturnType>(set_return_type))), body(std::move(UP<BodyType>(set_body)))
    {
        visit(set_name, [&](const auto& unwrapped) {
            name = unwrapped;
        }); 
    }
    function_object_mixin(const std_and_astring_variant& set_name, UP<ParameterType> set_parameters, UP<ReturnType> set_return_type, UP<BodyType> set_body) 
        : parameters(std::move(set_parameters)), return_type(std::move(set_return_type)), body(std::move(set_body))
    {
        visit(set_name, [&](const auto& unwrapped) {
            name = unwrapped;
        });
    }
    function_object_mixin(const std_and_astring_variant& set_name, avec<UP<ParameterType>> set_parameters, ReturnType* set_return_type, BodyType* set_body) 
        : parameters(std::move(set_parameters)), return_type(UP<ReturnType>(set_return_type)), body(UP<BodyType>(set_body))
    {
        visit(set_name, [&](const auto& unwrapped) {
            name = unwrapped;
        }); 

    }
    function_object_mixin(const std_and_astring_variant& set_name, avec<UP<ParameterType>> set_parameters, UP<ReturnType> set_return_type, UP<BodyType> set_body) 
        : parameters(std::move(set_parameters)), return_type(std::move(set_return_type)), body(std::move(set_body)) 
    {
        visit(set_name, [&](const auto& unwrapped) {
            name = unwrapped;
        });
    }

    protected:
    [[nodiscard]] astring inspect_impl(const astring& object_type_prefix) const {
        astringstream stream;
        stream << object_type_prefix << "Function name: " << name << "\n";
        stream << "Parameters: (";
        bool first = true;
        for (const auto& param : parameters) {
            if (!first) { stream << ", "; }
            stream << param->inspect();
            first = false;
        }
        stream << ") ";

        stream << "\nReturn type: " << return_type->inspect() << "\n";
        stream << "Body:\n";
        stream << body->inspect();

        return stream.str();
    }

    SelfType* clone_impl() const {
        avec<UP<ParameterType>> cloned_params;
        cloned_params.reserve(parameters.size());
        
        for (const auto& param : parameters) {
            cloned_params.push_back(UP<ParameterType>(param->clone())); }
        
        return new SelfType(name, std::move(cloned_params), return_type->clone(), body->clone());
    }

    public:
    astring name;
    avec<UP<ParameterType>> parameters;
    UP<ReturnType> return_type;
    UP<BodyType> body;
};

class function_object : public object, public function_object_mixin<function_object, object, SQL_data_type_object, block_statement> {

    public:
    using function_object_mixin::function_object_mixin;
    
    ~function_object() noexcept override = default;
    
    [[nodiscard]] astring inspect()        const override { return  inspect_impl(""); }
    [[nodiscard]] object_type type()       const override { return  FUNCTION_OBJ;     }
    [[nodiscard]] astring data()           const override { return "FUNCTION_OBJ";    }
    [[nodiscard]] function_object* clone() const override { return  clone_impl();      }
};

class e_function_object : virtual public evaluated, public function_object_mixin<e_function_object, e_parameter_object, e_SQL_data_type_object, e_block_statement> {

    public:
    using function_object_mixin::function_object_mixin;
    
    ~e_function_object() noexcept override = default;
    
    [[nodiscard]] astring inspect()          const override { return  inspect_impl("E");}
    [[nodiscard]] object_type type()         const override { return  E_FUNCTION_OBJ;   }
    [[nodiscard]] astring data()             const override { return "E_FUNCTION_OBJ";  }
    [[nodiscard]] e_function_object* clone() const override { return clone_impl();      }
};

class s_function_object : virtual public serializable, public function_object_mixin<s_function_object, s_parameter_object, s_SQL_data_type_object, s_block_statement> {

    public:
    using function_object_mixin::function_object_mixin;
    
    ~s_function_object() noexcept override = default;
    
    [[nodiscard]] astring inspect()          const override { return  inspect_impl("S");}
    [[nodiscard]] object_type type()         const override { return  S_FUNCTION_OBJ;   }
    [[nodiscard]] astring data()             const override { return "S_FUNCTION_OBJ";  }
    [[nodiscard]] s_function_object* clone() const override { return clone_impl();      }
    [[nodiscard]] astring serialize() const override {
        astringstream stream;
        stream << "(";
        stream << name << "\n";

        stream << "\t(";
        bool first = true;
        for (const auto& param : parameters) {
            if (!first) { stream << ", "; }
            stream << param->serialize();
            first = false;
        }
        stream << ")";

        stream << "\t" << return_type->serialize() << "\n";
        stream << body->serialize();
        stream << ")";

        return stream.str();
    }
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

class s_function_call_object : virtual public evaluated {

    public:
    s_function_call_object(const std_and_astring_variant& set_name, e_group_object* args);
    s_function_call_object(const std_and_astring_variant& set_name, UP<e_group_object> args);
    ~s_function_call_object() noexcept override = default;

    [[nodiscard]] astring                 inspect() const override;
    [[nodiscard]] object_type             type()    const override;
    [[nodiscard]] astring                 data()    const override;
    [[nodiscard]] s_function_call_object* clone()   const override;

    public:
    astring name;
    UP<e_group_object> arguments;
};



class error_object : virtual public serializable {

    public:
    error_object();
    error_object(const std_and_astring_variant& val);
    ~error_object() noexcept override = default;

    [[nodiscard]] astring       inspect()   const override;
    [[nodiscard]] object_type   type()      const override;
    [[nodiscard]] astring       data()      const override;
    [[nodiscard]] error_object* clone()     const override;
    [[nodiscard]] astring       serialize() const override { return inspect(); }

    public:
    astring value;
};

class semicolon_object : virtual public serializable {

    public:
    [[nodiscard]] astring           inspect()   const override;
    [[nodiscard]] object_type       type()      const override;
    [[nodiscard]] astring           data()      const override;
    [[nodiscard]] semicolon_object* clone()     const override;
    [[nodiscard]] astring           serialize() const override { return ";"; }
};

class star_object : virtual public serializable {

    public:
    [[nodiscard]] astring      inspect()   const override;
    [[nodiscard]] object_type  type()      const override;
    [[nodiscard]] astring      data()      const override;
    [[nodiscard]] star_object* clone()     const override;
    [[nodiscard]] astring      serialize() const override { return "*"; }
};

class table_object : virtual public serializable {

    public:
    table_object(const std_and_astring_variant& set_table_name, avec<UP<s_table_detail_object>>&& set_column_data, 
                 avec<UP<s_table_expr>>&& set_exprs, avec<UP<s_group_object>>&& set_rows);

    table_object(const std_and_astring_variant& set_table_name, avec<UP<s_table_detail_object>>&& set_column_data, 
                 avec<UP<s_table_expr>>&& set_exprs, UP<s_group_object> set_rows);

    table_object(const std_and_astring_variant& set_table_name, UP<s_table_detail_object> set_column_data, 
                 avec<UP<s_table_expr>>&& set_exprs, UP<s_group_object> set_rows);

    table_object(const std_and_astring_variant& set_table_name, UP<s_table_detail_object> set_column_data, 
                 UP<s_table_expr> set_exprs, UP<s_group_object> set_rows);

    ~table_object() noexcept override = default;

    table_object(const table_object&) = delete;
    table_object& operator=(const table_object&) = delete;

    [[nodiscard]] astring       inspect()   const override;
    [[nodiscard]] object_type   type()      const override;
    [[nodiscard]] astring       data()      const override;
    [[nodiscard]] table_object* clone()     const override;
    [[nodiscard]] astring       serialize() const override;

    // Get
    [[nodiscard]] std::pair<const avec<serializable*>&, bool>       get_const_column(size_t index)          const;
    [[nodiscard]] std::pair<const avec<serializable*>&, bool>       get_const_column(const std_and_astring_variant& col_name) const;
    [[nodiscard]] std::pair<astring, bool>                          get_column_name(size_t index)           const;
    [[nodiscard]] std::pair<UP<s_SQL_data_type_object>, bool>       get_column_data_type(size_t index)      const;
    [[nodiscard]] std::pair<size_t, bool>                           get_column_index(const std_and_astring_variant& name) const;
    [[nodiscard]] std::expected<std::optional<UP<s_default_value_object>>, UP<error_object>> get_cloned_column_default_value(size_t index) const;
    [[nodiscard]] std::pair<UP<serializable>, bool>                 get_cell_value(size_t row_index, size_t col_index)    const;
    [[nodiscard]] std::expected<avec<UP<serializable>>*, UP<error_object>> get_row_vec_ptr(size_t index)    const;
    [[nodiscard]] avec<size_t>                                      get_row_ids()                           const;
    [[nodiscard]] astring                                           get_tab_name()                          const;

    [[nodiscard]] bool check_if_field_name_exists(const std_and_astring_variant& name) const;

    public:
    astring table_name;
    avec<UP<s_table_detail_object>> column_data;
    avec<UP<s_table_expr>>   exprs;
    avec<UP<s_group_object>> rows;
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
    [[nodiscard]] avec<size_t>                            get_all_col_ids() const;
    [[nodiscard]] std::pair<size_t, bool>                 get_last_col_id() const;
    [[nodiscard]] std::pair<astring, bool>                get_table_name(size_t index) const;
    [[nodiscard]] std::pair<SP<table_object>, bool>       get_table(size_t index) const;
    [[nodiscard]] SP<table_object>                        combine_tables(const std_and_astring_variant& name) const;
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

    [[nodiscard]] astring inspect()     const override;
    [[nodiscard]] object_type type()    const override;
    [[nodiscard]] astring data()        const override;
    [[nodiscard]] if_statement* clone() const override;

    public:
    UP<object> condition;
    UP<block_statement> body;
    UP<object> other;
};

class end_if_statement : virtual public serializable {

    public:
    [[nodiscard]] astring inspect()         const override;
    [[nodiscard]] object_type type()        const override;
    [[nodiscard]] astring data()            const override;
    [[nodiscard]] end_if_statement* clone() const override;
    [[nodiscard]] astring serialize()       const override { return "END IF"; }
};

class end_statement : virtual public serializable {

    public:
    [[nodiscard]] astring inspect()      const override;
    [[nodiscard]] object_type type()     const override;
    [[nodiscard]] astring data()         const override;
    [[nodiscard]] end_statement* clone() const override;
    [[nodiscard]] astring serialize()    const override { return "END"; }
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
    explicit assert_object(size_t set_line, object* expr);
    explicit assert_object(size_t set_line, UP<object> expr);
    ~assert_object() noexcept override = default;
    
    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] assert_object* clone() const override;

    public:
    size_t line;
    UP<object> expression;
};

class e_assert_object : public evaluated {

    public:
    explicit e_assert_object(size_t set_line, evaluated* expr);
    explicit e_assert_object(size_t set_line, UP<evaluated> expr);
    ~e_assert_object() noexcept override = default;
    
    [[nodiscard]] astring inspect() const override;
    [[nodiscard]] object_type type() const override;
    [[nodiscard]] astring data() const override;
    [[nodiscard]] e_assert_object* clone() const override;

    public:
    size_t line;
    UP<evaluated> expression;
};
