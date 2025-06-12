
#include "pch.h"

#include "environment.h"

#include <object.h>


arena<environment> environment::environment_arena_alias;


environment::environment() {
    parent = NULL; 

    if (in_arena) {
        functions = avec<evaluated_function_object*>();
        variables = avec<variable_object*>();
    } else {
        functions = hvec_copy(evaluated_function_object*);
        variables = hvec_copy(variable_object*);
    }
}

environment::environment(environment* par) {
    
    parent = par; 

    if (in_arena) {
        functions = avec<evaluated_function_object*>();
        variables = avec<variable_object*>();
    } else {
        functions = hvec_copy(evaluated_function_object*);
        variables = hvec_copy(variable_object*);
    }

}

environment::~environment() {
    if (in_arena) {
        for (const auto& func : functions) {
            delete func;
        }
        for (const auto& var : variables) {
            delete var;
        }
    }
}


bool environment::add_function(evaluated_function_object* func) {
    if (is_function(func->name)) {
        return false; }
    functions.push_back(func);
    return true;
}

void environment::add_or_replace_function(evaluated_function_object* new_func) {
    bool exists = false;
    for (size_t i = 0; i < functions.size(); i++) {
        if (functions[i]->name == new_func->name) {
            functions[i] = new_func;
            exists = true;
            break;
        }
    }

    if (!exists) {
        functions.push_back(new_func); }
}

bool environment::is_function(const std_and_astring_variant& name) {

    astring unwrapped_name;
    VISIT(name, unwrapped,
        unwrapped_name = unwrapped;
    );

    for (const auto& func : functions) {
        if (func->name == unwrapped_name) {
            return true; }
    }

    // If not in child scope, maybe in parent scope
    if (parent) { 
        return parent->is_function(unwrapped_name); }

    return false;
    
}

object* environment::get_function(const std_and_astring_variant& name) {

    astring unwrapped_name;
    VISIT(name, unwrapped,
        unwrapped_name = unwrapped;
    );

    for (const auto& func : functions) {
        if (func->name == unwrapped_name) {
            return func; }
    }

    // If not in child scope, maybe in parent scope
    if (parent) { 
        return parent->get_function(unwrapped_name); }

    return new error_object();
}

bool environment::add_variables(const avec<argument_object*>& args) {
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

bool environment::is_variable(const astring& name) {
    for (const auto& var : variables) {
        if (var->name == name) {
            return true; }
    }

    // If not in child scope, maybe in parent scope
    if (parent) { 
        return parent->is_variable(name); }

    return false;
}

object* environment::get_variable(const std_and_astring_variant& name) {

    astring unwrapped_name;
    VISIT(name, unwrapped,
        unwrapped_name = unwrapped;
    );

    for (const auto& var : variables) {
        if (var->name == unwrapped_name) {
            return var;
        }
    }

    // If not in child scope, maybe in parent scope
    if (parent) { 
        return parent->get_variable(unwrapped_name); }

    return new error_object("Variable not found");
}

avec<astring> environment::inspect_variables() {

    avec<astring> inspected;
    inspected.reserve(variables.size());
    for (const auto& var : variables) {
        inspected.push_back(var->inspect());
    }

    return inspected;
}