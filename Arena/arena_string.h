
#pragma once
#include "pch.h"
#include "arena.h"

class astring {
private:
    std::string m_data;
    arena<char> alloc;
    
public:
    // Default constructors (use ARENA as default)
    inline astring() : m_data(), alloc(arena<char>{ARENA}) {}
    inline astring(const char* str) : m_data(str), alloc(arena<char>{ARENA}) {}
    inline astring(const std::string& str) : m_data(str), alloc(arena<char>{ARENA}) {}
    inline astring(const astring& other) : m_data(other.m_data), alloc(other.alloc) {}
    
    // Constructor for arena-allocated strings (from astringstream)
    inline astring(const std::basic_string<char, std::char_traits<char>, arena<char>>& str) 
        : m_data(str.c_str()), alloc(str.get_allocator()) {} 
    
    // Constructor for creating string with repeated character (fixes pad_length error)
    inline astring(size_t count, char ch) : m_data(count, ch), alloc(arena<char>{ARENA}) {}
    inline astring(size_t count, char ch, heap_or_arena location) : m_data(count, ch), alloc(arena<char>{location}) {}
    inline astring(size_t count, char ch, const arena<char>& a) : m_data(count, ch), alloc(a) {}

    // Add this constructor for creating astring from char buffer with length
    inline astring(const char* buffer, size_t length, const arena<char>& a) 
        : m_data(buffer, length), alloc(a) {}
    
    // Also add the heap_or_arena version
    inline astring(const char* buffer, size_t length, heap_or_arena location) 
        : m_data(buffer, length), alloc(arena<char>{location}) {}
    
    // Location-based constructors
    explicit inline astring(heap_or_arena location) : m_data(), alloc(arena<char>{location}) {}
    inline astring(const char* str, heap_or_arena location) : m_data(str), alloc(arena<char>{location}) {}
    inline astring(const std::string& str, heap_or_arena location) : m_data(str), alloc(arena<char>{location}) {}
    inline astring(const astring& other, heap_or_arena location) : m_data(other.m_data), alloc(arena<char>{location}) {}
    inline astring(const std::basic_string<char, std::char_traits<char>, arena<char>>& str, heap_or_arena location) 
        : m_data(str.c_str()), alloc(arena<char>{location}) {}
    
    // Arena-based constructors (taking arena directly)
    explicit inline astring(const arena<char>& a) : m_data(), alloc(a) {}
    inline astring(const char* str, const arena<char>& a) : m_data(str), alloc(a) {}
    inline astring(const std::string& str, const arena<char>& a) : m_data(str), alloc(a) {}
    inline astring(const astring& other, const arena<char>& a) : m_data(other.m_data), alloc(a) {}
    inline astring(const std::basic_string<char, std::char_traits<char>, arena<char>>& str, const arena<char>& a) 
        : m_data(str.c_str()), alloc(a) {}
    
    // Assignment operators - COPY both m_data and arena
    inline astring& operator=(const astring& other) {
        if (this != &other) {
            m_data = other.m_data;
            alloc = other.alloc;  // Copy the arena too
        }
        return *this;
    }
    
    inline astring& operator=(const std::string& str) {
        m_data = str;
        // alloc stays the same (no arena to copy from std::string)
        return *this;
    }
    
    inline astring& operator=(const char* str) {
        m_data = str;
        // alloc stays the same (no arena to copy from const char*)
        return *this;
    }
    
    inline astring& operator=(const std::basic_string<char, std::char_traits<char>, arena<char>>& str) {
        m_data = str.c_str();  // Convert to regular string
        alloc = str.get_allocator();  // Copy the arena allocator
        return *this;
    }
    
    // Move assignment
    inline astring& operator=(astring&& other) noexcept {
        if (this != &other) {
            m_data = std::move(other.m_data);
            alloc = std::move(other.alloc);  // Move the arena too
        }
        return *this;
    }
    
    inline astring& operator=(std::string&& str) noexcept {
        m_data = std::move(str);
        // alloc unchanged
        return *this;
    }
    
    // Utility methods
    inline const std::string& str() const { return m_data; }
    inline const arena<char>& get_allocator() const { return alloc; }
    
    // Public access to m_data for numeric conversions (fixes private access error)
    inline const char* data() const { return m_data.data(); }
    inline const std::string& get_data() const { return m_data; }
    
    // Forward common string operations
    inline size_t size() const { return m_data.size(); }
    inline size_t length() const { return m_data.length(); }
    inline bool empty() const { return m_data.empty(); }
    inline const char* c_str() const { return m_data.c_str(); }
    inline char& operator[](size_t pos) { return m_data[pos]; }
    inline const char& operator[](size_t pos) const { return m_data[pos]; }
    
    // String operations that return new astring objects
    inline astring substr(size_t pos = 0, size_t len = std::string::npos) const {
        return astring(m_data.substr(pos, len), alloc);  // Use same arena
    }
    
    inline size_t find(const std::string& str, size_t pos = 0) const {
        return m_data.find(str, pos);
    }
    
    inline size_t find(const char* s, size_t pos = 0) const {
        return m_data.find(s, pos);
    }
    
    // Concatenation operators
    inline astring operator+(const astring& other) const {
        return astring(m_data + other.m_data, alloc);  // Use this object's arena
    }
    
    inline astring operator+(const std::string& str) const {
        return astring(m_data + str, alloc);
    }
    
    inline astring operator+(const char* str) const {
        return astring(m_data + str, alloc);
    }
    
    inline astring& operator+=(const astring& other) {
        m_data += other.m_data;
        return *this;
    }
    
    inline astring& operator+=(const std::string& str) {
        m_data += str;
        return *this;
    }
    
    inline astring& operator+=(const char* str) {
        m_data += str;
        return *this;
    }
    
    // Comparison operators
    inline bool operator==(const astring& other) const { return m_data == other.m_data; }
    inline bool operator==(const std::string& str) const { return m_data == str; }
    inline bool operator==(const char* str) const { return m_data == str; }
    
    inline bool operator!=(const astring& other) const { return !(*this == other); }
    inline bool operator!=(const std::string& str) const { return !(*this == str); }
    inline bool operator!=(const char* str) const { return !(*this == str); }
    
    // Conversion operators
    inline operator const std::string&() const { return m_data; }
    
    // Stream output
    friend std::ostream& operator<<(std::ostream& os, const astring& str) {
        return os << str.m_data;
    }
};

// Global operators for reverse concatenation
inline astring operator+(const char* lhs, const astring& rhs) {
    return astring(lhs + rhs.str(), rhs.get_allocator());
}

inline astring operator+(const std::string& lhs, const astring& rhs) {
    return astring(lhs + rhs.str(), rhs.get_allocator());
}