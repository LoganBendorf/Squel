#pragma once

#include <object.h>

#include <vector>

class environment {
    public:

    environment() {
        parent = NULL; }

    environment(environment* par) {
        parent = par; }

    bool add_function(evaluated_function_object* func) {
        if (is_function(func->name)) {
            return false; }
        functions.push_back(func);
        return true;
    }

    bool is_function(std::string name) {
        for (const auto& func : functions) {
            if (func->name == name) {
                return true; }
        }

        // If not in child scope, maybe in parent scope
        if (parent) { 
            return parent->is_function(name); }

        return false;
    }

    object* get_function(std::string name) {
        for (const auto& func : functions) {
            if (func->name == name) {
                return func; }
        }

        // If not in child scope, maybe in parent scope
        if (parent) { 
            return parent->get_function(name); }

        return new error_object();
    }

    bool add_variables(std::vector<argument_object*> args) {
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

    bool is_variable(std::string name) {
        for (const auto& var : variables) {
            if (var->name == name) {
                return true; }
        }

        // If not in child scope, maybe in parent scope
        if (parent) { 
            return parent->is_variable(name); }

        return false;
    }

    object* get_variable(std::string name) {
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

    public:
    environment* parent;
    std::vector<evaluated_function_object*> functions;     // can change to map later if i want     
    std::vector<variable_object*> variables;
};