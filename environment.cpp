
#include "pch.h"

#include "environment.h"

#include "object.h"



main_alloc<environment> environment::environment_allocator_alias;

environment::environment() : parent(nullptr) {}

environment::environment(environment* par) : parent(SP<environment>(par)) { }
environment::environment(SP<environment> par) : parent(par) { }

environment::~environment() {
}

bool environment::add_function(evaluated_function_object* func) {
    if (is_function(func->name)) {
        return false; }
    functions.push_back(UP<evaluated_function_object>(func));
    return true;
}
bool environment::add_function(UP<evaluated_function_object>&& func) {
    if (is_function(func->name)) {
        return false; }
    functions.push_back(std::move(func));
    return true;
}

void environment::add_or_replace_function(evaluated_function_object* new_func) {
    bool exists = false;
    for (auto& function : functions) {
        if (function->name == new_func->name) {
            function = UP<evaluated_function_object>(new_func);
            exists = true;
            break;
        }
    }

    if (!exists) {
        functions.push_back(SP<evaluated_function_object>(new_func)); }
}
void environment::add_or_replace_function(SP<evaluated_function_object> new_func) {
    bool exists = false;
    for (auto& function : functions) {
        if (function->name == new_func->name) {
            function = std::move(new_func);
            exists = true;
            break;
        }
    }

    if (!exists) {
        functions.push_back(new_func); }
}

bool environment::is_function(const std_and_astring_variant& name) const {

    astring unwrapped_name;
    VISIT(name, unwrapped,
        unwrapped_name = unwrapped;
    );

    for (const auto& func : functions) {
        if (func->name == unwrapped_name) {
            return true; }
    }

    // If not in child scope, maybe in parent scope
    if (parent != nullptr) { 
        return parent->is_function(unwrapped_name); }

    return false;
}

std::pair<SP<evaluated_function_object>, bool> environment::get_function(const std_and_astring_variant& name) const {

    astring unwrapped_name;
    VISIT(name, unwrapped,
        unwrapped_name = unwrapped;
    );

    for (const auto& func : functions) {
        if (func->name == unwrapped_name) {
            return {func, true}; }
    }

    // If not in child scope, maybe in parent scope
    if (parent != nullptr) { 
        return parent->get_function(unwrapped_name); }

    return {nullptr, false};
}

bool environment::add_variables(avec<UP<e_argument_object>>&& args) {
    for (const auto& arg : args) {
        if (is_variable(arg->name)) {
            return false; }
    }

    for (auto& arg : std::move(args)) {
        auto variable = MAKE_UP(e_variable_object, arg->name, std::move(arg->value));
        variables.push_back(std::move(variable));
    }
    return true;
}

UP<object> environment::add_variable(e_variable_object* var) {
    if (is_variable(var->name)) {
        return UP<object>(new error_object("Failed to add variable (" + var->inspect() + ") to environment")); }

    variables.push_back(UP<e_variable_object>(var));
    return UP<object>(new null_object());
}
UP<object> environment::add_variable(UP<e_variable_object>&& var) {
    if (is_variable(var->name)) {
        return UP<object>(new error_object("Failed to add variable (" + var->inspect() + ") to environment")); }

    variables.push_back(std::move(var));
    return UP<object>(new null_object());
}

bool environment::is_variable(const astring& name) const {
    for (const auto& var : variables) {
        if (var->name == name) {
            return true; }
    }

    // If not in child scope, maybe in parent scope
    if (parent != nullptr) { 
        return parent->is_variable(name); }

    return false;
}

std::expected<UP<e_variable_object>, UP<error_object>> environment::get_variable(const std_and_astring_variant& name) const {

    astring unwrapped_name;
    VISIT(name, unwrapped,
        unwrapped_name = unwrapped;
    );

    for (const auto& var : variables) {
        if (var->name == unwrapped_name) {
            return UP<e_variable_object>(var->clone());
        }
    }

    // If not in child scope, maybe in parent scope
    if (parent != nullptr) { 
        return parent->get_variable(unwrapped_name); }

    return std::unexpected(MAKE_UP(error_object, "Variable not found"));
}

avec<astring> environment::inspect_variables() {

    avec<astring> inspected;
    inspected.reserve(variables.size());
    for (const auto& var : variables) {
        inspected.push_back(var->inspect());
    }

    return inspected;
}