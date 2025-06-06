#pragma once
#include <cstdlib>
#include <iostream>
#include <vector>
#include <functional>
#include <algorithm>

template<typename T>
struct arena {
    using value_type = T;
    
    arena() noexcept { }

    // To use heap
    arena(bool use_arena) {
        if (!use_arena) {
            use_std_alloc = true; }
    }
    
    template<typename U>
    arena(const arena<U>&) noexcept { }
    
    void construct() {
        if (has_memory) {
            std::cout << "Arena has memory constructed\n"; exit(1); }

        base = std::malloc(1 << 20);
        if (!base) {
            std::cerr << "Arena failed to init memory\n"; exit(1); }

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
            std::cout << "Cannot custom initialize arena when it is already constructed\n"; exit(1); }

        base = set_base;
        capacity = size;
        next_open_slot = 0;
        has_memory = true;
    }
    
    T* allocate(std::size_t count, size_t align = alignof(std::max_align_t)) {
        if (!has_memory) {
            std::cout << "Tried to use arena without memory in it\n"; exit(1); }
        if (use_std_alloc) {
            return static_cast<T*>(std::malloc(sizeof(T) * count)); 
        }
        
        size_t aligned_off = (next_open_slot + align - 1) & ~(align - 1);
        size_t end_point = aligned_off + sizeof(T) * count;
        if (end_point > capacity) {
            std::cerr << "Arena OOM\n"; exit(1); }
        
        void* ptr = static_cast<char*>(base) + aligned_off;
        next_open_slot = end_point;
        // __lsan_ignore_object(ptr);
        return static_cast<T*>(ptr);
    }
    
    void deallocate(T* p, [[maybe_unused]] std::size_t n) noexcept {
        if (!has_memory) {
            std::cout << "Tried to use arena without memory in it\n"; exit(1); }
        
        if (use_std_alloc) {
            std::free(p); 
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
    // Static members - shared across all instances
    static inline bool has_memory = false;
    static inline bool heap_constructed = false;
    static inline void* base = nullptr;
    static inline size_t next_open_slot = 0;
    static inline size_t capacity = 0;
    static inline std::vector<std::function<void()>> destructors;
};

template<typename T, typename U>
bool operator==(const arena<T>&, const arena<U>&) noexcept { 
    return true; 
}

template<typename T, typename U>
bool operator!=(const arena<T>& a, const arena<U>& b) noexcept {
    return !(a == b);
}