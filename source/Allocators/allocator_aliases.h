#pragma once

#include "allocator_string.h"
#include "allocator_structs.h"

#include <memory>
#include <vector>
#include <string>
#include <variant>

template<typename T>
class main_alloc;

template<typename T>
struct main_alloc_deleter {
    void operator()(T* ptr) const {
        if (ptr) {
            // Call destructor first
            ptr->~T();
            // Then deallocate memory
            main_alloc<T> alloc;
            block blk{sizeof(T), ptr};
            alloc.deallocate_block(blk);
        }
    }
};

template<typename T>
using UP = std::unique_ptr<T, main_alloc_deleter<T>>;

#define MAKE_UP(type, ...) \
    UP<type>(new type(__VA_ARGS__))

template<typename To, typename From>
UP<To> cast_UP( UP<From>& ptr ) {
    // simply move it for them
    return cast_UP<To>( std::move(ptr) );
}

template<typename To, typename From>
UP<To> cast_UP( UP<From>&& ptr ) {
    if constexpr (std::is_convertible_v<From*, To*>) {
        return UP<To>(static_cast<To*>(ptr.release()));
    } else {
        // Fall back to dynamic_cast
        To* casted = dynamic_cast<To*>(ptr.get());
        if (casted) {
            ptr.release();
            return UP<To>(casted);
        }
        return nullptr;
    }
}



template<typename T>
using SP = std::shared_ptr<T>;

template<typename To, typename From>
SP<To> cast_SP(const SP<From>& ptr) {
    if constexpr (std::is_convertible_v<From*, To*>) {
        // Static cast - safe conversion
        return std::static_pointer_cast<To>(ptr);
    } else {
        // Dynamic cast - runtime type checking
        return std::dynamic_pointer_cast<To>(ptr);
    }
}

template<typename To, typename From>
SP<To> cast_SP(SP<From>&& ptr) {
    // For rvalue references, we can still use the same logic
    // The shared_ptr will handle the move semantics internally
    return cast_SP<To>(ptr);
}

// For custom allocator (more complex)
template<typename T, typename... Args>
SP<T> make_sp_custom(Args&&... args) {
    main_alloc<T> alloc;
    return std::allocate_shared<T>(alloc, std::forward<Args>(args)...);
}

#define MAKE_SP(type, ...) \
    make_sp_custom<type>(__VA_ARGS__)





template<typename T>
using avec = std::vector<T, main_alloc<T>>;

using astringstream = std::basic_stringstream<char, std::char_traits<char>, main_alloc<char>>;

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

        // Add constructor for main_alloc-allocated strings
    std_and_astring_variant(const std::basic_string<char, std::char_traits<char>, main_alloc<char>>& str)
        : std_and_astring_variant(astring(str)) {}
    
    // Method to get the variant (so variable_object can use it)
    const string_variant& get_variant() const { return data; }
    
    // Or conversion operator
    operator const string_variant&() const { return data; }
};
