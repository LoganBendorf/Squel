
#include "pch.h"

#include "environment.h"

#include <object.h>


class arena;
extern arena arena_inst;

void* environment::operator new(std::size_t size, bool use_arena) {
    if (use_arena) {
        return arena_inst.allocate(size, alignof(environment)); // maybe get rid of second parameter
    }
    else {
        return ::operator new(size);
    }
}
void environment::operator delete([[maybe_unused]] void* ptr, [[maybe_unused]] bool b) noexcept {
    std::cout << "\n\nDELETE BAD!!!!\n\n";
    exit(1);
}
void environment::operator delete(void* ptr) noexcept {
    // if it came from the heap, free it; if arena, do nothing
    ::operator delete(ptr);
}
void* environment::operator new[](std::size_t size, bool use_arena) {
    if (use_arena) {
        return arena_inst.allocate(size, alignof(environment));
    }
    else {
        return ::operator new(size);
    }
}
void environment::operator delete[]([[maybe_unused]] void* p) noexcept {
    std::cout << "\n\nDELETE BAD!!!!\n\n";
    exit(1);
}



environment::environment([[maybe_unused]] bool use_arena) {
    in_arena = use_arena;
    parent = NULL; 

    void* mem = arena_inst.allocate(sizeof(std::vector<evaluated_function_object*>), alignof(std::vector<evaluated_function_object*>)); 
    auto vec_ptr = new (mem) std::vector<evaluated_function_object*>();                                  
    functions = vec_ptr;                                                             
    arena_inst.register_dtor([vec_ptr]() {                                      
        vec_ptr->std::vector<evaluated_function_object*>::~vector();                                              \
    });

    mem = arena_inst.allocate(sizeof(std::vector<variable_object*>), alignof(std::vector<variable_object*>)); 
    auto var_vec_ptr = new (mem) std::vector<variable_object*>();                                  
    variables = var_vec_ptr;                                                             
    arena_inst.register_dtor([var_vec_ptr]() {                                      
        var_vec_ptr->std::vector<variable_object*>::~vector();                                              \
    });
}

environment::environment(environment* par, [[maybe_unused]] bool use_arena) {
    in_arena = use_arena;
    parent = par; 

    void* mem = arena_inst.allocate(sizeof(std::vector<evaluated_function_object*>), alignof(std::vector<evaluated_function_object*>)); 
    auto vec_ptr = new (mem) std::vector<evaluated_function_object*>();                                  
    functions = vec_ptr;                                                             
    arena_inst.register_dtor([vec_ptr]() {                                      
        vec_ptr->std::vector<evaluated_function_object*>::~vector();                                              \
    });

    mem = arena_inst.allocate(sizeof(std::vector<variable_object*>), alignof(std::vector<variable_object*>)); 
    auto var_vec_ptr = new (mem) std::vector<variable_object*>();                                  
    variables = var_vec_ptr;                                                             
    arena_inst.register_dtor([var_vec_ptr]() {                                      
        var_vec_ptr->std::vector<variable_object*>::~vector();                                              \
    });
}

environment::~environment() {
    if (in_arena) {
        for (const auto& func : *functions) {
            delete func;
        }
        delete functions;
        for (const auto& var : *variables) {
            delete var;
        }
        delete variables; 
    }
}


bool environment::add_function(evaluated_function_object* func) {
    if (is_function(*func->name)) {
        return false; }
    functions->push_back(func);
    return true;
}

void environment::add_or_replace_function(evaluated_function_object* new_func) {
    bool exists = false;
    for (size_t i = 0; i < functions->size(); i++) {
        if ((*functions)[i]->name == new_func->name) {
            (*functions)[i] = new_func;
            exists = true;
            break;
        }
    }

    if (!exists) {
        functions->push_back(new_func); }
}

bool environment::is_function(std::string name) {
    for (const auto& func : *functions) {
        if (*func->name == name) {
            return true; }
    }

    // If not in child scope, maybe in parent scope
    if (parent) { 
        return parent->is_function(name); }

    return false;
}

object* environment::get_function(std::string name) {
    for (const auto& func : *functions) {
        if (*func->name == name) {
            return func; }
    }

    // If not in child scope, maybe in parent scope
    if (parent) { 
        return parent->get_function(name); }

    return new error_object();
}

bool environment::add_variables(std::vector<argument_object*> args) {
    for (const auto& arg : args) {
        if (is_variable(*arg->name)) {
            return false; }
    }

    for (const auto& arg : args) {
        variable_object* variable = new variable_object(*arg->name, arg->value);
        variables->push_back(variable);
    }
    return true;
}

bool environment::add_variable(variable_object* var) {
    if (is_variable(*var->name)) {
        return false; 
    }

    variables->push_back(var);
    return true;
}

bool environment::is_variable(std::string name) {
    for (const auto& var : *variables) {
        if (*var->name == name) {
            return true; }
    }

    // If not in child scope, maybe in parent scope
    if (parent) { 
        return parent->is_variable(name); }

    return false;
}

object* environment::get_variable(std::string name) {
    for (const auto& var : *variables) {
        if (*var->name == name) {
            return var;
        }
    }

    // If not in child scope, maybe in parent scope
    if (parent) { 
        return parent->get_variable(name); }

    return new error_object("Variable not found");
}

std::vector<std::string> environment::inspect_variables() {

    std::vector<std::string> inspected;
    inspected.reserve(variables->size());
    for (const auto& var : *variables) {
        inspected.push_back(var->inspect());
    }

    return inspected;
}