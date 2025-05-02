
#pragma once



#include <cstddef>   // for std::size_t
#include <cstdlib>   // for std::malloc, std::free

#include <iostream>



class arena {
    public:
    arena()        { init(); }
    ~arena()       { destroy(); }
    void* allocate(size_t size) {

        // 1) Round up next_off to the strictest alignment the platform allows
        size_t align = alignof(std::max_align_t);
        size_t aligned_off = (next_open_slot + align - 1) & ~(align - 1);
        // 2) Check bounds
        if (aligned_off + size > capacity)  {
            std::cout << "Arena failed to allocate memory";
            exit(999);
        }
        // 3) Compute ptr and update offset
        void* ptr = static_cast<char*>(base) + aligned_off;
        next_open_slot = aligned_off + size;
        return ptr;
    }

    void init() {
        base = std::malloc(1 << 20);
        next_open_slot = 0;
        capacity = (1 << 20);
        if (!base) {
            std::cout << "Arena failed to init memory";
            exit(999);
        };
    }

    void destroy() {
        if (base == nullptr) {
            return; }
        std::free(base);
        base = nullptr;
        next_open_slot = -1;
    }

    public:
    void* base = nullptr;
    size_t next_open_slot = 0;
    size_t capacity = 0;

};