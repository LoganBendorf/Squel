#pragma once

#include <object.h>

#include <vector>

class environment {
    public:
    static void* operator new(std::size_t size);
    static void  operator delete(void* p) noexcept;
    static void* operator new[](std::size_t size);
    static void  operator delete[](void* p) noexcept;
    

    environment();
    environment(environment* par);

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
    std::vector<evaluated_function_object*> functions;     // can change to map later if i want     
    std::vector<variable_object*> variables;
};