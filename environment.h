#pragma once

#include "pch.h"
#include "allocators.h"
#include "allocator_aliases.h"
#include "structs_and_macros.h"

class object;
class argument_object;
class variable_object;
class evaluated_function_object;
class error_object;

class environment {

    protected:
    static main_alloc<environment> environment_allocator_alias;

    public:

    static void* operator new(size_t size) {
        return environment_allocator_alias.allocate_block(size).mem;
    }

    static void operator delete(void* ptr) noexcept {
        if (ptr == nullptr) { 
            return; }

        environment_allocator_alias.deallocate_block({0, ptr});
    }

    static void* operator new[](size_t size) {
        return environment_allocator_alias.allocate_block(size).mem;
    }
    
    static void operator delete[](void* ptr) noexcept {
        if (ptr == nullptr) { 
            return; }
        
        environment_allocator_alias.deallocate_block({0, ptr});
    }

    environment();
    environment(environment* par);
    environment(SP<environment> par);
    ~environment();

    bool add_function(evaluated_function_object* func);
    bool add_function(UP<evaluated_function_object>&& func);

    void add_or_replace_function(evaluated_function_object* new_func);
    void add_or_replace_function(SP<evaluated_function_object> new_func);

    [[nodiscard]] bool is_function(const std_and_astring_variant& name) const;
    [[nodiscard]] std::pair<SP<evaluated_function_object>, bool> get_function(const std_and_astring_variant& name) const;

    bool add_variables(avec<UP<argument_object>>&& args);

    UP<object> add_variable(variable_object* var);
    UP<object> add_variable(UP<variable_object>&& var);

    [[nodiscard]] bool is_variable(const astring& name) const;
    [[nodiscard]] std::expected<UP<variable_object>, UP<error_object>> get_variable(const std_and_astring_variant& name) const;
    avec<astring> inspect_variables();

    public:
    SP<environment> parent;
    avec<SP<evaluated_function_object>> functions; 
    avec<UP<variable_object>> variables;

};