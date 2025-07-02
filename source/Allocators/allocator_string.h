#pragma once

#include "allocators.h"

#include <string>

class astring {
private:
    // Use the custom allocator for actual string storage
    std::basic_string<char, std::char_traits<char>, main_alloc<char>> m_data;
    
public:
    // Default constructors 
    astring() : m_data(main_alloc<char>{}) {}
    astring(const char* str) : m_data(str, main_alloc<char>{}) {}
    astring(const std::string& str) : m_data(str.c_str(), main_alloc<char>{}) {}
    astring(const astring& other) : m_data(other.m_data) {}
    
    // Constructor for main_alloc-allocated strings (from astringstream)
    astring(const std::basic_string<char, std::char_traits<char>, main_alloc<char>>& str) 
        : m_data(str) {} 
    
    // Constructor for creating string with repeated character
    astring(size_t count, char ch) : m_data(count, ch, main_alloc<char>{}) {}
    astring(size_t count, char ch, const main_alloc<char>& a) : m_data(count, ch, a) {}
    
    // Constructor for creating astring from char buffer with length
    astring(const char* buffer, size_t length, const main_alloc<char>& a) 
        : m_data(buffer, length, a) {}
    
    
    // Assignment operators
    astring& operator=(const astring& other) {
        if (this != &other) {
            m_data = other.m_data;
        }
        return *this;
    }
    
    astring& operator=(const std::string& str) {
        m_data.assign(str.c_str());
        return *this;
    }
    
    astring& operator=(const char* str) {
        m_data.assign(str);
        return *this;
    }
    
    astring& operator=(const std::basic_string<char, std::char_traits<char>, main_alloc<char>>& str) {
        m_data = str;
        return *this;
    }
    
    // Move assignment
    astring& operator=(astring&& other) noexcept {
        if (this != &other) {
            m_data = std::move(other.m_data);
        }
        return *this;
    }
    
    astring& operator=(std::string&& str) noexcept {
        m_data.assign(str.c_str());
        return *this;
    }
    
    // Utility methods
    std::string str() const { return std::string(m_data.c_str()); }
    main_alloc<char> get_allocator() const { return m_data.get_allocator(); }
    
    // Public access to m_data for numeric conversions
    const char* data() const { return m_data.data(); }
    const auto& get_data() const { return m_data; }
    
    // Forward common string operations
    size_t size() const { return m_data.size(); }
    size_t length() const { return m_data.length(); }
    bool empty() const { return m_data.empty(); }
    const char* c_str() const { return m_data.c_str(); }
    char& operator[](size_t pos) { return m_data[pos]; }
    const char& operator[](size_t pos) const { return m_data[pos]; }
    
    // String operations that return new astring objects
    astring substr(size_t pos = 0, size_t len = std::string::npos) const {
        auto sub = m_data.substr(pos, len);
        return astring(sub);
    }
    
    size_t find(const std::string& str, size_t pos = 0) const {
        return m_data.find(str.c_str(), pos);
    }
    
    size_t find(const char* s, size_t pos = 0) const {
        return m_data.find(s, pos);
    }
    
    // Concatenation operators
    astring operator+(const astring& other) const {
        auto result = m_data + other.m_data;
        return astring(result);
    }
    
    astring operator+(const std::string& str) const {
        auto result = m_data + str.c_str();
        return astring(result);
    }
    
    astring operator+(const char* str) const {
        auto result = m_data + str;
        return astring(result);
    }
    
    astring& operator+=(const astring& other) {
        m_data += other.m_data;
        return *this;
    }
    
    astring& operator+=(const std::string& str) {
        m_data += str.c_str();
        return *this;
    }
    
    astring& operator+=(const char* str) {
        m_data += str;
        return *this;
    }
    
    // Comparison operators
    bool operator==(const astring& other) const { return m_data == other.m_data; }
    bool operator==(const std::string& str) const { return m_data == str.c_str(); }
    bool operator==(const char* str) const { return m_data == str; }

    bool operator!=(const astring& other) const { return !(*this == other); }
    bool operator!=(const std::string& str) const { return !(*this == str); }
    bool operator!=(const char* str) const { return !(*this == str); }
    
    // Conversion operators
    operator std::string() const { return std::string(m_data.c_str()); }
    
    // Stream output
    friend std::ostream& operator<<(std::ostream& os, const astring& str) {
        return os << str.m_data;
    }
};

// Global operators for reverse concatenation
inline astring operator+(const char* lhs, const astring& rhs) {
    std::basic_string<char, std::char_traits<char>, main_alloc<char>> temp(lhs, rhs.get_allocator());
    temp += rhs.get_data();
    return astring(temp);
}

inline astring operator+(const std::string& lhs, const astring& rhs) {
    std::basic_string<char, std::char_traits<char>, main_alloc<char>> temp(lhs.c_str(), rhs.get_allocator());
    temp += rhs.get_data();
    return astring(temp);
}