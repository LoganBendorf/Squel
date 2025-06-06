#pragma once

#include <sanitizer/lsan_interface.h>

#include <cstddef>
#include <cstdlib>
#include <vector>
#include <functional>
#include <iostream>

// Forward declare global arena instance
class arena;
extern arena arena_inst;

class arena {
public:
    arena() {}
    ~arena(){}

    void construct() {
        base = std::malloc(1 << 20);
        if (!base) {
            std::cerr << "Arena failed to init memory\n";
            std::exit(1);
        }
        capacity = (1 << 20);
        constructed = true;
        std::cout << "\nArena Allocated\n";
    }

    void teardown() {
        std::free(base);
        base = nullptr;
        constructed = false;
    }
    // Raw bump-allocation
    void* allocate(size_t size, size_t align = alignof(std::max_align_t)) {
        size_t aligned_off = (next_open_slot + align - 1) & ~(align - 1);
        if (aligned_off + size > capacity) {
            std::cerr << "Arena OOM\n";
            std::exit(1);
        }
        void* ptr = static_cast<char*>(base) + aligned_off;
        next_open_slot = aligned_off + size;
        // __lsan_ignore_object(ptr);
        return ptr;
    }

    // Record a destructor for later
    void register_dtor(std::function<void()> fn) {
        destructors.emplace_back(std::move(fn));
    }


    void init() {
        next_open_slot = 0;
    }

    void init(void* set_base, size_t size) {
        if (constructed) {
            std::cout << "Cannot custom initalize arena when it is already constructed\n";
            exit(1);
        }
        base = set_base;
        capacity = size;
        next_open_slot = 0;
    }

    void destroy() {
        if (!base) return;
        // run destructors in reverse order
        for (auto it = destructors.rbegin(); it != destructors.rend(); ++it) {
            (*it)();
        }
        destructors.clear();
        next_open_slot = static_cast<size_t>(-1);
    }

    size_t get_usage() {
        return next_open_slot;
    }

    private:
    bool use_std_alloc = false; // to disable the allocator and just use malloc
    bool constructed = false;
    void* base = nullptr;
    size_t next_open_slot = 0;
    size_t capacity = 0;
    std::vector<std::function<void()>> destructors;
};


