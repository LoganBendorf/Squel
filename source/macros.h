#pragma once

#include <filesystem>
#include <source_location>
#include <array>
#include <stdexcept>
#include <string>
#include <sstream>
#include <iostream>

#define CUR_LOC std::source_location::current()



inline std::string get_relative_path(const std::string& file_path) {
    try {
        return std::filesystem::relative(file_path, PROJECT_ROOT).string(); // PROJECT_ROOT is created by CMake
    } catch (const std::exception&) {
        // Fallback to just filename if relative path fails
        return std::filesystem::path(file_path).filename().string();
    }
}

inline std::string get_function_name(std::string name) { // Maybe use ref?

    // Remove return type
    size_t first_space = name.find(' ');
    if (first_space != std::string::npos) {
        name = name.substr(first_space + 1);
    }

    // Remove parameters
    size_t first_paren = name.find('(');
    size_t last_paren = name.rfind(')');
    if (first_paren != std::string::npos && last_paren != std::string::npos) {
        name = name.substr(0, first_paren + 1) + ")";
    }
    
    return name;
}




class StackTrace {
private:
    static constexpr size_t MAX_FRAMES = 50;
    
    struct FrameInfo {
        void* address;
        std::string binary_name;
        std::string function_name;
        std::string source_file;
        int line_number;
        size_t offset;
        uintptr_t relative_addr;
    };
    
    static std::string demangle_symbol(const char* mangled) {
        if (mangled == nullptr) { return ""; }
        
        int status = -1;
        char* demangled = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
        
        if (status == 0 && demangled != nullptr) {
            std::string result(demangled);
            free(demangled);
            return result;
        }
        
        return {mangled};
    }

    
    static std::pair<std::string, std::string> get_source_and_function(void* addr, const char* binary_path) {
        if (binary_path == nullptr) { return {"", ""}; }
        
        Dl_info info;
        uintptr_t relative_addr = 0;
        
        if (dladdr(addr, &info) != 0) {
            relative_addr = std::bit_cast<uintptr_t>(addr) - std::bit_cast<uintptr_t>(info.dli_fbase);
        } else {
            relative_addr = std::bit_cast<uintptr_t>(addr);
        }
        
        std::stringstream cmd;
        cmd << "addr2line -C -f -e " << binary_path << " 0x" << std::hex << relative_addr << " 2>/dev/null";
        
        FILE* pipe = popen(cmd.str().c_str(), "r");
        if (pipe == nullptr) { return {"", ""}; }
        
        // char buffer[512];
        std::array<char, 512> buffer{};
        std::string function_name;
        std::string source_location;
        
        // First line is function name
        if (fgets(buffer.data(), sizeof(buffer), pipe) != nullptr) {
            function_name = buffer.data();
            if (!function_name.empty() && function_name.back() == '\n') {
                function_name.pop_back();
            }
            
            // Second line is file:line
            if (fgets(buffer.data(), sizeof(buffer), pipe) != nullptr) {
                source_location = buffer.data();
                if (!source_location.empty() && source_location.back() == '\n') {
                    source_location.pop_back();
                }
            }
        }
        
        pclose(pipe);
        
        // Clean up function name if it's valid
        if (function_name == "??" || function_name.empty()) {
            function_name = "";
        }
        
        // Clean up source location if it's invalid
        if (source_location == "??:0" || source_location == "??:?" || source_location.empty()) {
            source_location = "";
        }
        
        return {function_name, source_location};
    }
    
    public:
    // Alternative capture that focuses on getting better symbol resolution
    static std::string capture() {
        void* array[MAX_FRAMES];
        int size_as_int = backtrace(array, MAX_FRAMES); // C func
        size_t size = static_cast<size_t>(size_as_int);
        
        std::stringstream ss;
        ss << "Detailed stack trace (" << size << " frames):\n";
        ss << std::string(80, '=') << "\n";
        
        constexpr size_t trace_info_size = 2;
        constexpr size_t initializer_junk_size = 3;
        const     size_t iterations = size - initializer_junk_size;
        for (size_t i = trace_info_size; i < iterations && size > trace_info_size; i++) {
            Dl_info info;
            // ss << std::setw(2) << i << ": ";
            // ss << "0x" << std::hex << std::setfill('0') << std::setw(12) 
            //    << (uintptr_t)array[i] << std::dec << std::setfill(' ');
            
            if (dladdr(array[i], &info) != 0) {
                // uintptr_t relative_addr = (uintptr_t)array[i] - (uintptr_t)info.dli_fbase;
                // ss << " (+0x" << std::hex << relative_addr << std::dec << ")";
                
                if (info.dli_sname != nullptr) {
                    std::string demangled = demangle_symbol(info.dli_sname);
                    // ss << " in " << demangled;
                    ss << demangled;
                    
                    if (info.dli_saddr != nullptr) {
                        // uintptr_t offset = (uintptr_t)array[i] - (uintptr_t)info.dli_saddr;
                        // ss << " + " << offset;
                    }
                } else {
                    // Try addr2line for static functions
                    auto [func_name, source_loc] = get_source_and_function(array[i], info.dli_fname);
                    if (!func_name.empty()) {
                        ss << func_name;
                    } else {
                        ss << " in <unknown>";
                    }
                }
                
                // ss << "\n    from " << (info.dli_fname ? info.dli_fname : "<unknown>");
                
                // Try to get source location
                auto [func_name, source_loc] = get_source_and_function(array[i], info.dli_fname);
                if (!source_loc.empty()) {
                    ss << "\n    at " << source_loc;
                }
            } else {
                ss << " in <unknown>\n    from <unknown>";
            }
            
            ss << "\n";
        }
        
        return ss.str();
    }
};

inline std::string get_stack_trace() {
    return StackTrace::capture();
}



#define FILE_NAME_STR \
    std::filesystem::path(CUR_LOC.file_name()).filename().string()

#define GET_FILE_NAME_STR(x) \
    std::filesystem::path((x).file_name()).filename().string()

#define GET_ERROR_LOCATION(loc) \
    get_relative_path((loc).file_name()) << ":" << (loc).line() << ":" << (loc).column() 




struct note {
    int freq;
    int dur;
};

constexpr int dur = 80;

constexpr std::array<note, 5> notes{{{349, dur}, {523, dur}, {493, dur}, {440, dur}, {415, dur}}};


#define FATAL_ERROR_EXIT(msg, loc)                                                                      \
    std::cerr << get_relative_path((loc).file_name()) << ":" << (loc).line() << ":" << (loc).column()   \
              << ": FATAL ERROR: " << msg << ". In function "                                           \
              << (loc).function_name() << std::endl;                                                    \
    for (const auto& note : notes) {                                                                    \
        std::stringstream system_stream;                                                                \
        system_stream << "play -n synth " << (note.dur / 1000.0) << " sine " << note.freq << " triangle " << ((note.freq * 3) / 2) << " vol 0.12 2>/dev/null &";\
        std::system(system_stream.str().c_str());                                                       \
        std::this_thread::sleep_for(std::chrono::milliseconds(note.dur * 3));                           \
    }                                                                                                   \
    exit(1);
    // std::raise(SIGTRAP);                       

#define FATAL_ERROR_THROW(msg, loc)                                                                     \
    std::stringstream full_error;                                                                       \
    full_error << get_relative_path((loc).file_name()) << ":" << (loc).line() << ":" << (loc).column()  \
              << ": FATAL ERROR: " << msg << ". In function "                                           \
              << (loc).function_name() << std::endl;                                                    \
    for (const auto& note : notes) {                                                                    \
        std::stringstream system_stream;                                                                \
        system_stream << "play -n synth " << (note.dur / 1000.0) << " sine " << note.freq << " triangle " << ((note.freq * 3) / 2) << " vol 0.12 2>/dev/null &";\
        std::system(system_stream.str().c_str());                                                       \
        std::this_thread::sleep_for(std::chrono::milliseconds(note.dur * 3));                           \
    }                                                                                                   \
    throw std::runtime_error(full_error.str());

// For now runtime error, probably need to make some error aliases 
#define FATAL_ERROR_STACK_TRACE_THROW(msg, loc)                                                         \
    do {                                                                                                \
    std::stringstream full_error;                                                                       \
    full_error << get_relative_path((loc).file_name()) << ":" << (loc).line() << ":" << (loc).column()  \
              << ": FATAL ERROR: " << msg << ". In function "                                           \
              << (loc).function_name() << std::endl;                                                    \
    full_error << get_stack_trace() << std::endl;                                                       \
    for (const auto& note : notes) {                                                                    \
        std::stringstream system_stream;                                                                \
        system_stream << "play -n synth " << (note.dur / 1000.0) << " sine " << note.freq << " triangle " << ((note.freq * 3) / 2) << " vol 0.12 2>/dev/null &";\
        std::system(system_stream.str().c_str());                                                       \
        std::this_thread::sleep_for(std::chrono::milliseconds(note.dur * 3));                           \
    }                                                                                                   \
    throw std::runtime_error(full_error.str());                                                         \
    } while (0)
    // exit(1);

#define FATAL_ERROR_STACK_TRACE_EXIT(msg, loc)                                                          \
    do {                                                                                                \
    std::cerr << get_relative_path((loc).file_name()) << ":" << (loc).line() << ":" << (loc).column()   \
              << ": FATAL ERROR: " << msg << ". In function "                                           \
              << (loc).function_name() << std::endl;                                                    \
    std::cerr << get_stack_trace() << std::endl;                                                        \
    for (const auto& note : notes) {                                                                    \
        std::stringstream system_stream;                                                                \
        system_stream << "play -n synth " << (note.dur / 1000.0) << " sine " << note.freq << " triangle " << ((note.freq * 3) / 2) << " vol 0.12 2>/dev/null &";\
        std::system(system_stream.str().c_str());                                                       \
        std::this_thread::sleep_for(std::chrono::milliseconds(note.dur * 3));                           \
    }                                                                                                   \
    exit(1);                                                                                            \
    } while (0)