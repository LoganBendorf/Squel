#pragma once

#include "pch.h"

#include "arena_string.h"



template<typename T>
class arena;

template<typename T>
using avec = std::vector<T, arena<T>>;

#define hvec(type, name) \
    avec<type> name{arena<type>{HEAP}}

#define hvec_copy(type) \
    avec<type> {arena<type>{HEAP}}

#define hstring(content) \
    astring(content, HEAP)


using astringstream = std::basic_stringstream<char, std::char_traits<char>, arena<char>>;


#define hstringstream() \
    astringstream(std::ios::out, arena<char>{HEAP})


using string_variant = std::variant<astring, std::string>;

class std_and_astring_variant {
    private:
    string_variant data;
        
    public:
    // Main constructor that stores the variant
    std_and_astring_variant(const string_variant& var) : data(var) {}
    
    std_and_astring_variant(const std::string& str)
        : std_and_astring_variant(string_variant{str}) {}
        
    std_and_astring_variant(const astring& str)
        : std_and_astring_variant(string_variant{str}) {}

    // const char* for string literals
    std_and_astring_variant(const char* str)
        : std_and_astring_variant(std::string{str}) {}

        // Add constructor for arena-allocated strings
    std_and_astring_variant(const std::basic_string<char, std::char_traits<char>, arena<char>>& str)
        : std_and_astring_variant(astring(str)) {}
    
    // Method to get the variant (so variable_object can use it)
    const string_variant& get_variant() const { return data; }
    
    // Or conversion operator
    operator const string_variant&() const { return data; }
};
