#pragma once

#include <object.h>

#include <vector>

class environment {
    public:
    void add_function(function_object* func) {
        functions.push_back(func);
    }

    object* get_function(std::string name) {
        for (int i = 0; i < functions.size(); i++) {
            if (functions[i]->name == name) {
                return functions[i]; }
        }
        return new error_object();
    }

    public:
    std::vector<function_object*> functions;     // can change to map later if i want     
};