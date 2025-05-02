
#include "environment.h"

#include <object.h>

#include <vector>

class arena;
extern arena arena_inst;

void* environment::operator new(std::size_t size) {
    return arena_inst.allocate(size);
}
void environment::operator delete([[maybe_unused]] void* p) noexcept {
    std::cout << "\n\nDELETE BAD!!!!\n\n";
    exit(1);
}
void* environment::operator new[](std::size_t size) {
    return arena_inst.allocate(size);
}
void environment::operator delete[] ([[maybe_unused]] void* p) noexcept {
    std::cout << "\n\nDELETE BAD!!!!\n\n";
    exit(1);
}


environment::environment() {
    parent = NULL; }
environment::environment(environment* par) {
    parent = par; }

bool environment::add_function(evaluated_function_object* func) {
    if (is_function(func->name)) {
        return false; }
    functions.push_back(func);
    return true;
}

void environment::add_or_replace_function(evaluated_function_object* new_func) {
    bool exists = false;
    for (int i = 0; i < functions.size(); i++) {
        if (functions[i]->name == new_func->name) {
            functions[i] = new_func;
            exists = true;
            break;
        }
    }

    if (!exists) {
        functions.push_back(new_func); }
}

bool environment::is_function(std::string name) {
    for (const auto& func : functions) {
        if (func->name == name) {
            return true; }
    }

    // If not in child scope, maybe in parent scope
    if (parent) { 
        return parent->is_function(name); }

    return false;
}

object* environment::get_function(std::string name) {
    for (const auto& func : functions) {
        if (func->name == name) {
            return func; }
    }

    // If not in child scope, maybe in parent scope
    if (parent) { 
        return parent->get_function(name); }

    return new error_object();
}

bool environment::add_variables(std::vector<argument_object*> args) {
    for (const auto& arg : args) {
        if (is_variable(arg->name)) {
            return false; }
    }

    for (const auto& arg : args) {
        variable_object* variable = new variable_object(arg->name, arg->value);
        variables.push_back(variable);
    }
    return true;
}

bool environment::add_variable(variable_object* var) {
    if (is_variable(var->name)) {
        return false; 
    }

    variables.push_back(var);
    return true;
}

bool environment::is_variable(std::string name) {
    for (const auto& var : variables) {
        if (var->name == name) {
            return true; }
    }

    // If not in child scope, maybe in parent scope
    if (parent) { 
        return parent->is_variable(name); }

    return false;
}

object* environment::get_variable(std::string name) {
    for (const auto& var : variables) {
        if (var->name == name) {
            return var;
        }
    }

    // If not in child scope, maybe in parent scope
    if (parent) { 
        return parent->get_variable(name); }

    return new error_object("Variable not found");
}
