#pragma once

#include <vector>

class object;
class argument_object;
class variable_object;
class evaluated_function_object;

class environment {

    public:
    static void* operator new(std::size_t size, bool use_arena = true);
    static void  operator delete([[maybe_unused]] void* ptr, [[maybe_unused]] bool b) noexcept;
    static void  operator delete(void* ptr) noexcept;
    static void* operator new[](std::size_t size, bool use_arena = true);
    static void  operator delete[]([[maybe_unused]] void* p) noexcept;
    
    environment([[maybe_unused]] bool use_arena = true);
    environment(environment* par, bool use_arena = true);
    ~environment();

    bool add_function(evaluated_function_object* func);
    void add_or_replace_function(evaluated_function_object* new_func);
    bool is_function(std::string name);
    object* get_function(std::string name);

    bool add_variables(std::vector<argument_object*> args);
    bool add_variable(variable_object* var);
    bool is_variable(std::string name);
    object* get_variable(std::string name);

    public:
    environment* parent;
    std::vector<evaluated_function_object*>* functions;     // can change to map later if i want     
    std::vector<variable_object*>* variables;

    protected:
    bool in_arena = true;
};