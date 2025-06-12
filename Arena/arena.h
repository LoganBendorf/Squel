#pragma once

#include "pch.h"

#include "structs_and_macros.h"



inline void print_stack_trace() {
    std::cerr << "Crashing to get ASan stack trace:\n";
    // Force a crash that ASan will catch and print a stack trace for
    volatile char* p = nullptr;
    *p = 0;  // This will trigger ASan and show the stack
}

// Global values
class arena_base {
    protected:
    static inline bool has_memory = false;
    static inline bool heap_constructed = false;
    static inline void* base = nullptr;
    static inline size_t next_open_slot = 0;
    static inline size_t capacity = 0;
    static inline std::vector<std::function<void()>> destructors;
};


template<typename T>
struct arena  : arena_base {
    using value_type = T;
    
    arena() noexcept { }

    // To use heap
    arena(bool use_arena) {
        if (use_arena == HEAP) {
            use_std_alloc = true; }
    }
    
    // so use std_alloc is preserved across copy
    template<typename U>
    arena(const arena<U>& other) noexcept : use_std_alloc(other.use_std_alloc) { }

    arena(const arena& other) noexcept : use_std_alloc(other.use_std_alloc) { }

    // Assignment operators - ADD THESE to fix the deprecated-copy warning
    arena& operator=(const arena& other) noexcept {
        if (this != &other) {
            use_std_alloc = other.use_std_alloc;
        }
        return *this;
    }
    
    template<typename U>
    arena& operator=(const arena<U>& other) noexcept {
        use_std_alloc = other.use_std_alloc;
        return *this;
    }
    
    // Move assignment operator
    arena& operator=(arena&& other) noexcept {
        if (this != &other) {
            use_std_alloc = other.use_std_alloc;
        }
        return *this;
    }
        
        
    void construct() {
        if (has_memory) {
            std::cout << "Arena has memory constructed\n"; 
            print_stack_trace();
            exit(1); 
        }

        base = std::malloc(1 << 20);
        if (!base) {
            std::cerr << "Arena failed to init memory\n"; 
            print_stack_trace();
            exit(1); 
        }

        capacity = (1 << 20);
        has_memory = true;
        heap_constructed = true;
        next_open_slot = 0;
        std::cout << "\nArena Allocated\n";
    }
    
    void teardown() {
        // Call all registered destructors
        for (auto& dtor : destructors) {
            dtor();
        }
        destructors.clear();
        
        std::free(base);
        base = nullptr;
        has_memory = false;
        heap_constructed = false;
        next_open_slot = static_cast<size_t>(-1);
    }

    void destroy() {
        if (!base) return;
        // run destructors in reverse order
        for (auto it = destructors.rbegin(); it != destructors.rend(); ++it) {
            (*it)();
        }
        destructors.clear();
        next_open_slot = static_cast<size_t>(-1);
        has_memory = false;
    }
    
    void register_dtor(std::function<void()> fn) {
        destructors.emplace_back(std::move(fn));
    }
    
    void init() {
        next_open_slot = 0;
    }
    
    void init(void* set_base, size_t size) {
        if (heap_constructed) {
            std::cout << "Cannot custom initialize arena when it is already constructed\n"; 
            print_stack_trace();
            exit(1); 
        }

        base = set_base;
        capacity = size;
        next_open_slot = 0;
        has_memory = true;
    }
    
    T* allocate(std::size_t count, size_t align = alignof(std::max_align_t)) {
        if (use_std_alloc) {
            return static_cast<T*>(std::malloc(sizeof(T) * count)); 
        }

        if (!has_memory) {
            std::cout << "Tried to allocate with arena without memory in it. Type: " << typeid(T).name() << "\n"; 
            print_stack_trace();
            exit(1); 
        }
        
        size_t aligned_off = (next_open_slot + align - 1) & ~(align - 1);
        size_t end_point = aligned_off + sizeof(T) * count;
        if (end_point > capacity) {
            std::cerr << "Arena OOM\n"; 
            print_stack_trace();
            exit(1);
         }
        
        void* ptr = static_cast<char*>(base) + aligned_off;
        next_open_slot = end_point;
        // __lsan_ignore_object(ptr);
        return static_cast<T*>(ptr);
    }
    
    void deallocate(T* p, [[maybe_unused]] std::size_t n = 0) noexcept {
        std::cout << "Deallocating on: " << (use_std_alloc ? "ARENA" : "HEAP") << std::endl;
        if (use_std_alloc) {
            std::free(p); 
            return;
        }

        if (!has_memory) {
            std::cout << "Tried to deallocate with arena without memory in it. Type: " << typeid(T).name() << "\n"; 
            print_stack_trace();
            exit(1); 
        }
        
        // For arena allocation, we don't actually free individual objects
    }

    size_t get_usage() {
        return next_open_slot;
    }
    
    // (for older STL impls)
    template<typename U>
    struct rebind { using other = arena<U>; };
    
    bool use_std_alloc = false; // to disable the allocator and just use malloc    
};

template<typename T, typename U>
bool operator==(const arena<T>&, const arena<U>&) noexcept { 
    return true; 
}

template<typename T, typename U>
bool operator!=(const arena<T>& a, const arena<U>& b) noexcept {
    return !(a == b);
}
