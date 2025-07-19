#pragma once

#include "allocators.h"
#include "allocator_aliases.h"
// #include "structs.h"

class object;
class e_argument_object;
class e_variable_object;
class e_function_object;
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

    bool add_function(e_function_object* func);
    bool add_function(UP<e_function_object>&& func);

    void add_or_replace_function(e_function_object* new_func);
    void add_or_replace_function(SP<e_function_object> new_func);

    [[nodiscard]] bool is_function(const std_and_astring_variant& name) const;
    [[nodiscard]] std::pair<SP<e_function_object>, bool> get_function(const std_and_astring_variant& name) const;

    bool add_variables(avec<UP<e_argument_object>>&& args);
    UP<object> add_variable(e_variable_object* var);
    UP<object> add_variable(UP<e_variable_object>&& var);

    [[nodiscard]] bool is_variable(const astring& name) const;
    [[nodiscard]] std::expected<UP<e_variable_object>, UP<error_object>> get_variable(const std_and_astring_variant& name) const;
    avec<astring> inspect_variables();

    public:
    SP<environment> parent;
    avec<SP<e_function_object>> functions; 
    avec<UP<e_variable_object>> variables;

};

class serializable;
class s_argument_object;
class s_variable_object;
class s_function_object;

// Just making a new class for now. Pretty sure environment should be removed from evaluator and only have one for execute, but this is easier for now
class s_environment {

    protected:
    static main_alloc<s_environment> s_environment_allocator_alias;

    public:

    static void* operator new(size_t size) {
        return s_environment_allocator_alias.allocate_block(size).mem;
    }

    static void operator delete(void* ptr) noexcept {
        if (ptr == nullptr) { 
            return; }

        s_environment_allocator_alias.deallocate_block({0, ptr});
    }

    static void* operator new[](size_t size) {
        return s_environment_allocator_alias.allocate_block(size).mem;
    }
    
    static void operator delete[](void* ptr) noexcept {
        if (ptr == nullptr) { 
            return; }
        
        s_environment_allocator_alias.deallocate_block({0, ptr});
    }

    s_environment();
    s_environment(s_environment* par);
    s_environment(SP<s_environment> par);

    bool add_function(s_function_object* func);
    bool add_function(UP<s_function_object>&& func);

    void add_or_replace_function(s_function_object* new_func);
    void add_or_replace_function(SP<s_function_object> new_func);

    [[nodiscard]] bool is_function(const std_and_astring_variant& name) const;
    [[nodiscard]] std::pair<SP<s_function_object>, bool> get_function(const std_and_astring_variant& name) const;

    bool add_variables(avec<UP<s_argument_object>>&& args);
    UP<serializable> add_variable(s_variable_object* var);
    UP<serializable> add_variable(UP<s_variable_object>&& var);

    [[nodiscard]] bool is_variable(const astring& name) const;
    [[nodiscard]] std::expected<UP<s_variable_object>, UP<error_object>> get_variable(const std_and_astring_variant& name) const;
    avec<astring> inspect_variables();

    public:
    SP<s_environment> parent;
    avec<SP<s_function_object>> functions; 
    avec<UP<s_variable_object>> variables;

};