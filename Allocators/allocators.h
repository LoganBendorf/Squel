#pragma once

#include "pch.h"

#include "allocator_structs.h"

static constexpr size_t size_t_min(size_t a, size_t b) {
    return a < b ? a : b;
}

#ifdef DEBUG_ALLOCATORS
    #include <iostream>
    #include <source_location>
    static constexpr void log(const std::source_location& location = std::source_location::current()) {
        std::cout << "Function: " << location.function_name() << '\n';
        std::cout << "Line: " << location.line() << '\n';
    }
#endif

template<typename T>
concept NotVoid = !std::same_as<T, void>;

template<typename T>
class allocator_base{
    public:

    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    
    template<NotVoid U = T>
    using reference = U&;

    template<NotVoid U = T>
    using const_reference = const U&;

    // For shared pointer
    allocator_base() = default;
    
    template<typename U>
    allocator_base(const allocator_base<U>&) noexcept {}
    
    template<typename U>
    allocator_base(allocator_base<U>&&) noexcept {}
    
    virtual ~allocator_base() = default;


    static constexpr bool good_size([[maybe_unused]] size_t size) { return false; }

    virtual bool can_allocate(size_t size) = 0;
    virtual block allocate_block(size_t size) = 0;
    virtual void allocate_all() = 0;
    virtual void expand(block &blk, size_t delta) = 0;
    virtual void reallocate(block &blk, size_t size) = 0;
    virtual bool owns(block blk) = 0;
    virtual void deallocate_block(block blk) = 0;
    virtual void deallocate_all() = 0;
};

template<typename T>
class mallocator : public allocator_base<T> {
    public:

    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    
    template<NotVoid U = T>
    using reference = U&;

    template<NotVoid U = T>
    using const_reference = const U&;

    // For shared pointer
    mallocator() = default;
    
    template<typename U>
    mallocator(const mallocator<U>&) noexcept {}
    
    template<typename U>
    mallocator(mallocator<U>&&) noexcept {}
    
    virtual ~mallocator() = default;



    // FOR STL
    template<typename U>
    struct rebind {
        using other = mallocator<U>;
    };

    T* allocate(std::size_t n) {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        block blk = allocate_block(n * sizeof(T));
        return static_cast<T*>(blk.mem);
    }
    
    void deallocate(T* p, std::size_t n) {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        block blk{n * sizeof(T), p};
        deallocate_block(blk);
    }


    static constexpr bool good_size([[maybe_unused]] size_t size) {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        return true;
    }

    bool can_allocate([[maybe_unused]] size_t size) override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        return true;
    }

    block allocate_block(size_t size) override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        return block{size, malloc(size)};
    }

    void allocate_all() override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

    }

    void expand(block &blk, size_t delta) override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        if (blk.mem == nullptr) {
            return; }

        block new_block = allocate_block(blk.size + delta);
        std::memcpy(new_block.mem, blk.mem, blk.size);
        deallocate_block(blk);
        blk = new_block;
    }

    void reallocate(block &blk, size_t size) override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        if (blk.mem == nullptr) {
            return; }

        if (!can_allocate(size)) {
            return; }

        block new_block = allocate_block(size);
        std::memcpy(new_block.mem, blk.mem, size_t_min(blk.size, size));
        deallocate_block(blk);
        blk = new_block;
    }

    bool owns([[maybe_unused]] block blk) override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        return true;
    }
        
    void deallocate_block(block blk) override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        if (blk.mem == nullptr) {
            return; }

        free(blk.mem);
    }
        
    void deallocate_all() override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif
    }
};

template<typename T>
class stack_allocator : public allocator_base<T> {
    public:

    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    
    template<NotVoid U = T>
    using reference = U&;

    template<NotVoid U = T>
    using const_reference = const U&;

    // For shared pointer
    stack_allocator() = default;
    
    template<typename U>
    stack_allocator(const stack_allocator<U>&) noexcept {}
    
    template<typename U>
    stack_allocator(stack_allocator<U>&&) noexcept {}
    
    virtual ~stack_allocator() = default;


    
    // FOR STL
    template<typename U>
    struct rebind {
        using other = stack_allocator<U>;
    };

    T* allocate(std::size_t n) {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        block blk = allocate_block(n * sizeof(T));
        return static_cast<T*>(blk.mem);
    }
    
    void deallocate(T* p, std::size_t n) {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        block blk{n * sizeof(T), p};
        deallocate_block(blk);
    }


    static void allocate_stack_memory(std::span<std::byte> stack_span) {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        allocated = true;
        base = stack_span.data(); 
        capacity = stack_span.size();
        top = base;
    }

    static void deallocate_stack_memory() {
        allocated = false;
    }

    static constexpr bool good_size([[maybe_unused]] size_t size) {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        return true;
    }

    bool can_allocate(size_t size) override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        if (base == nullptr) {
            return false; }

        auto available = static_cast<char*>(base) + capacity - static_cast<char*>(top);
        if (static_cast<size_t>(available) < size) {
            return false; }

        return true;
    }

    block allocate_block(size_t size) override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        // Align to 8 bytes
        size = (size + 7) & ~size_t(7);

        if (can_allocate(size)) {
            void* old_top = top;
            top = static_cast<char*>(top) + size;
            return {size, old_top};
        } else {
            return {0, nullptr};
        }
    }

    void allocate_all() override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif
    }

    void expand(block &blk, size_t delta) override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        if (blk.mem == nullptr) {
            return; }

        // Check if we can expand in place
        if (static_cast<char*>(blk.mem) + blk.size == top && can_allocate(delta)) {
            top = static_cast<char*>(top) + delta;
            blk.size += delta;
            return;
        }
        
         if (!can_allocate(blk.size + delta)) {
            return; }

        // Otherwise allocate new block
        block new_block = allocate_block(blk.size + delta);
        if (new_block.mem != nullptr) {
            std::memcpy(new_block.mem, blk.mem, blk.size);
            // Only deallocate if it's the top block
            if (static_cast<char*>(blk.mem) + blk.size == top) {
                top = blk.mem;
            }
            blk = new_block;
        }
    }

    void reallocate(block &blk, size_t size) override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        if (blk.mem == nullptr) {
            return; }

        if (!can_allocate(size)) {
            return; }

        block new_block = allocate_block(size);
        std::memcpy(new_block.mem, blk.mem, size_t_min(blk.size, size));
        deallocate_block(blk);
        blk = new_block;
    }

    bool owns(block blk) override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        if (base == nullptr) {
            return false; }

        if (blk.mem >= base && blk.mem < top) {
            return true;
        } else {
            return false;
        }
    }
        
    void deallocate_block(block blk) override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        if (static_cast<char*>(blk.mem) + blk.size == top) {
            top = blk.mem;
        }
    }
        
    void deallocate_all() override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif
    }

    static inline bool allocated  = false;
    static inline size_t capacity = 0;
    static inline void*  base     = nullptr;
    static inline void*  top      = nullptr;
};

template<typename T>
class fat_allocator : public allocator_base<T> {
    public:

    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    
    template<NotVoid U = T>
    using reference = U&;

    template<NotVoid U = T>
    using const_reference = const U&;

    // For shared pointer
    fat_allocator() = default;
    
    template<typename U>
    fat_allocator(const fat_allocator<U>&) noexcept {}
    
    template<typename U>
    fat_allocator(fat_allocator<U>&&) noexcept {}
    
    virtual ~fat_allocator() = default;



    // FOR STL
    template<typename U>
    struct rebind {
        using other = fat_allocator<U>;
    };

    T* allocate(std::size_t n) {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        block blk = allocate_block(n * sizeof(T));
        return static_cast<T*>(blk.mem);
    }
    
    void deallocate(T* p, std::size_t n) {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        block blk{n * sizeof(T), p};
        deallocate_block(blk);
    }



    static constexpr bool good_size([[maybe_unused]] size_t size) {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        return true;
    }

    bool can_allocate(size_t size) override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif
        
        if (size > size_t(1024 * 2)) {
            return true;
        } else {
            return false;
        }
    }

    block allocate_block(size_t size) override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        if (!can_allocate(size)) {
            return {0, nullptr}; }

        size_t kb_4 = size_t(1024 * 4);
        if (size % kb_4 != 0) {
            size += kb_4 - (size % kb_4);
        }
        block blk = {size, nullptr};
        int rc = posix_memalign(&blk.mem, alignof(std::max_align_t), size);
        if (rc != 0) {
            return {0, nullptr}; }

        return blk;
    }

    void allocate_all() override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif
    }

    void expand(block &blk, size_t delta) override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif
        
        if (blk.mem == nullptr) {
            return; }

        if (!can_allocate(blk.size + delta)) {
            return; }

        block new_block = allocate_block(blk.size + delta);
        std::memcpy(new_block.mem, blk.mem, blk.size);
        deallocate_block(blk);
        blk = new_block;
    }

    void reallocate(block &blk, size_t size) override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        if (blk.mem == nullptr) {
            return; }

        if (!can_allocate(size)) {
            return; }

        block new_block = allocate_block(size);
        std::memcpy(new_block.mem, blk.mem, size_t_min(blk.size, size));
        deallocate_block(blk);
        blk = new_block;
    }

    bool owns(block blk) override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        if (blk.size > size_t(1024 * 2)) {
            return true;
        } else {
            return false;
        }
    }
        
    void deallocate_block(block blk) override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        if (blk.mem == nullptr) {
            return; }

        free(blk.mem);
    }
        
    void deallocate_all() override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif
    }

    static std::vector<void*> alloc_list();
};



template<typename T>
class main_alloc : public allocator_base<T> {
    public:

    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    
    template<NotVoid U = T>
    using reference = U&;

    template<NotVoid U = T>
    using const_reference = const U&;

    // For shared pointer
    main_alloc() = default;
    
    template<typename U>
    main_alloc(const main_alloc<U>&) noexcept {}
    
    template<typename U>
    main_alloc(main_alloc<U>&&) noexcept {}
    
    virtual ~main_alloc() = default;



    // FOR STL
    template<typename U>
    struct rebind {
        using other = main_alloc<U>;
    };

    T* allocate(std::size_t n) {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        block blk = allocate_block(n * sizeof(T));
        return static_cast<T*>(blk.mem);
    }
    
    void deallocate(T* p, std::size_t n) {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        block blk{n * sizeof(T), p};
        deallocate_block(blk);
    }


    static void allocate_stack_memory(std::span<std::byte> stack_span) {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        stack.allocate_stack_memory(stack_span);
    }

    static void deallocate_stack_memory() {
        stack.deallocate_stack_memory();
    }

    static constexpr bool good_size([[maybe_unused]] size_t size) {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        return true;
    }

    bool can_allocate([[maybe_unused]] size_t size) override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        return true;
    }
    
    block allocate_block(size_t size) override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        if (size > size_t(1024 * 2)) {
            return fat.allocate_block(size);
        } else if (stack.can_allocate(size)) {
            return stack.allocate_block(size);
        } else {
            return mal.allocate_block(size);
        }
    }

    void allocate_all() override  {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif
    }

    void expand(block &blk, size_t delta) override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        if (blk.mem == nullptr) {
            return; }

        if (!can_allocate(blk.size + delta)) {
            return; }

        block new_block = allocate_block(blk.size + delta);
        std::memcpy(new_block.mem, blk.mem, blk.size);
        deallocate_block(blk);
        blk = new_block;
    }

    void reallocate(block &blk, size_t size) override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        if (blk.mem == nullptr) {
            return; }

        if (!can_allocate(size)) {
            return; }

        block new_block = allocate_block(size);
        std::memcpy(new_block.mem, blk.mem, size_t_min(blk.size, size));
        deallocate_block(blk);
        blk = new_block;
    }

    bool owns(block blk) override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        return fat.owns(blk) || stack.owns(blk) || mal.owns(blk);
    }
        
    void deallocate_block(block blk) override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif

        if (blk.mem == nullptr) {
            return; }

        if (stack.owns(blk)) {
            stack.deallocate_block(blk);
        } else if (fat.owns(blk)) {
            fat.deallocate_block(blk);
        } else if (mal.owns(blk)) {
            mal.deallocate_block(blk);
        }
    }
        
    void deallocate_all() override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif
    }

    static inline fat_allocator<void>   fat{};
    static inline stack_allocator<void> stack{};
    static inline mallocator<void>      mal{};
};

template<typename T, typename U>
constexpr bool operator==(const main_alloc<T>&, const main_alloc<U>&) noexcept {
    return true;
}

template<typename T, typename U>
constexpr bool operator!=(const main_alloc<T>&, const main_alloc<U>&) noexcept {
    return false;
}