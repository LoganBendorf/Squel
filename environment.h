#pragma once

#include "pch.h"
#include "arena.h"
#include "arena_aliases.h"
#include "structs_and_macros.h"

class object;
class argument_object;
class variable_object;
class evaluated_function_object;

class environment {

    protected:
    static arena<environment> environment_arena_alias; // all arenas are alias for the global one

    public:
    // static void* operator new(std::size_t size);
    // static void* operator new(std::size_t size, bool use_arena);
    // static void  operator delete([[maybe_unused]] void* ptr, [[maybe_unused]] bool b) noexcept;
    // static void  operator delete(void* ptr) noexcept;
    // static void* operator new[](std::size_t size, bool use_arena = true);
    // static void  operator delete[]([[maybe_unused]] void* p) noexcept;

    static void* operator new(std::size_t size, bool use_arena) {
        environment* env;
        if (use_arena) {
            env = environment_arena_alias.allocate(size, alignof(environment));
            if (env) { 
                env->in_arena = true; }
        } else {
            auto heap_arena = arena<environment>{HEAP};
            env = heap_arena.allocate(size, alignof(environment));
            if (env) { 
                env->in_arena = false;}
        }
        return env;
    }

    static void* operator new(std::size_t size) {
        environment* env = environment_arena_alias.allocate(size, alignof(environment));
        if (env) { 
            env->in_arena = true; }
        return env;
    }
    
    // Matching delete for placement new above
    static void operator delete([[maybe_unused]] void* ptr, bool) noexcept {
        std::cout << "\n\nDELETE BAD!!!!\n\n";
        exit(1);
    }
    
    // Standard delete
    static void operator delete(void* ptr) noexcept {
        if (!ptr) return;
        
        environment* env = static_cast<environment*>(ptr);
        if (env->in_arena) {
            // Arena environments - the arena handles cleanup
            // In a real implementation, you might want to call:
            // global_environment_arena.deallocate_bytes(ptr);
            return;
        } else {
            ::operator delete(ptr);
        }
    }
    
    // Array versions
    static void* operator new[](std::size_t size, bool use_arena) {
        if (use_arena) {
            return environment_arena_alias.allocate(size, alignof(environment));
        } else {
            return ::operator new[](size);
        }
    }
    
    static void* operator new[](std::size_t size) {
        return ::operator new[](size);
    }
    
    static void operator delete[](void* ptr) noexcept {
        if (!ptr) return;
        // For arrays, we'd need additional metadata to track arena vs heap
        // This is simplified - in practice you'd want to track this
        ::operator delete[](ptr);
    }

    environment();
    environment(environment* par);
    ~environment();

    bool add_function(evaluated_function_object* func);
    void add_or_replace_function(evaluated_function_object* new_func);
    bool is_function(const std_and_astring_variant& name);
    object* get_function(const std_and_astring_variant& name);

    bool add_variables(const avec<argument_object*>& args);
    bool add_variable(variable_object* var);
    bool is_variable(const astring& name);
    object* get_variable(const std_and_astring_variant& name);
    avec<astring> inspect_variables();

    public:
    environment* parent;
    avec<evaluated_function_object*> functions;     // can change to map later if i want     
    avec<variable_object*> variables;

    protected:
    bool in_arena = true;
};