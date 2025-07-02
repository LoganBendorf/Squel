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
        if (n == 0) return nullptr;
        
        block blk = allocate_block(n * sizeof(T));
        if (blk.mem == nullptr) {
            throw std::bad_alloc();
        }
        return static_cast<T*>(blk.mem);
    }
    
    void deallocate(T* p, std::size_t n) {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif
        if (p == nullptr || n == 0) return;
        
        block blk{n * sizeof(T), p};
        deallocate_block(blk);
    }
    
    static void allocate_stack_memory(std::span<std::byte> stack_span) {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif
        if (stack_span.empty()) {
            throw std::invalid_argument("Stack span cannot be empty");
        }
        
        get_allocated() = true;
        get_base() = stack_span.data(); 
        get_capacity() = stack_span.size();
        get_top() = get_base();
        
        // Verify capacity is what we expect
        if (get_capacity() != (1 << 20)) {
            std::cout << "WARNING: Expected capacity " << (1 << 20) 
                      << " but got " << get_capacity() << std::endl;
        }
        
        #ifdef DEBUG_ALLOCATORS
            std::cout << "Stack allocated: base=" << get_base() 
                      << ", capacity=" << get_capacity() << " (" << (get_capacity() >> 10) << "KB)"
                      << ", end=" << (static_cast<char*>(get_base()) + get_capacity()) << std::endl;
            std::cout << "Expected end should be around: " << std::hex 
                      << (reinterpret_cast<uintptr_t>(get_base()) + get_capacity()) << std::dec << std::endl;
        #endif
    }
    
    static void deallocate_stack_memory() {
        get_allocated() = false;
        get_base() = nullptr;
        get_top() = nullptr;
        get_capacity() = 0;
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
        if (!get_allocated() || get_base() == nullptr || size == 0) {
            return false;
        }
        
        // Align size to 8 bytes for consistency with allocate_block
        size = (size + 7) & ~size_t(7);
        
        char* stack_end = static_cast<char*>(get_base()) + get_capacity();
        char* current_top = static_cast<char*>(get_top());
        
        // Bounds checking
        if (current_top < static_cast<char*>(get_base()) || current_top > stack_end) {
            #ifdef DEBUG_ALLOCATORS
                std::cout << "ERROR: Stack corruption detected in can_allocate!" << std::endl;
                std::cout << "  base=" << get_base() << ", top=" << get_top() << ", end=" << stack_end << std::endl;
            #endif
            return false;
        }
        
        size_t available = static_cast<size_t>(stack_end - current_top);
        return available >= size;
    }
    
    block allocate_block(size_t size) override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif
        if (size == 0) {
            return {0, nullptr};
        }
        
        // Align to 8 bytes
        size = (size + 7) & ~size_t(7);
        
        if (!can_allocate(size)) {
            #ifdef DEBUG_ALLOCATORS
                std::cout << "Cannot allocate " << size << " bytes" << std::endl;
            #endif
            return {0, nullptr};
        }
        
        void* old_top = get_top();
        get_top() = static_cast<char*>(get_top()) + size;
        
        #ifdef DEBUG_ALLOCATORS
            std::cout << "Allocated " << size << " bytes at " << old_top 
                      << ", new top=" << get_top() << std::endl;
        #endif
        
        return {size, old_top};
    }
    
    void allocate_all() override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif
        // Move top to end of stack
        if (get_allocated() && get_base() != nullptr) {
            get_top() = static_cast<char*>(get_base()) + get_capacity();
        }
    }
    
    void expand(block &blk, size_t delta) override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif
        if (blk.mem == nullptr || delta == 0) {
            return;
        }
        
        // Align delta
        delta = (delta + 7) & ~size_t(7);
        
        // Check if we can expand in place (block is at the top)
        if (static_cast<char*>(blk.mem) + blk.size == get_top() && can_allocate(delta)) {
            get_top() = static_cast<char*>(get_top()) + delta;
            blk.size += delta;
            #ifdef DEBUG_ALLOCATORS
                std::cout << "Expanded block in place by " << delta << " bytes" << std::endl;
            #endif
            return;
        }
        
        // Check if we can allocate a new larger block
        if (!can_allocate(blk.size + delta)) {
            #ifdef DEBUG_ALLOCATORS
                std::cout << "Cannot expand block: not enough space" << std::endl;
            #endif
            return;
        }
        
        // Allocate new block and copy data
        block new_block = allocate_block(blk.size + delta);
        if (new_block.mem != nullptr) {
            std::memcpy(new_block.mem, blk.mem, blk.size);
            
            // Deallocate old block if it's at the top
            if (static_cast<char*>(blk.mem) + blk.size == static_cast<char*>(get_top()) - (blk.size + delta)) {
                // The old block was right before the new block, so we can merge them
                get_top() = static_cast<char*>(blk.mem);
                get_top() = static_cast<char*>(get_top()) + blk.size + delta;
            }
            
            blk = new_block;
            #ifdef DEBUG_ALLOCATORS
                std::cout << "Expanded block by allocating new block" << std::endl;
            #endif
        }
    }
    
    void reallocate(block &blk, size_t size) override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif
        if (blk.mem == nullptr) {
            blk = allocate_block(size);
            return;
        }
        
        if (size == 0) {
            deallocate_block(blk);
            blk = {0, nullptr};
            return;
        }
        
        // Align size
        size = (size + 7) & ~size_t(7);
        
        if (size == blk.size) {
            return; // No change needed
        }
        
        if (!can_allocate(size)) {
            #ifdef DEBUG_ALLOCATORS
                std::cout << "Cannot reallocate: not enough space" << std::endl;
            #endif
            return;
        }
        
        block new_block = allocate_block(size);
        if (new_block.mem != nullptr) {
            std::memcpy(new_block.mem, blk.mem, std::min(blk.size, size));
            deallocate_block(blk);
            blk = new_block;
        }
    }
    
    bool owns(block blk) override {
        char* stack_end = static_cast<char*>(get_base()) + get_capacity();
        #ifdef DEBUG_ALLOCATORS
            log();
            std::cout << "\tStack is " << (get_allocated() ? "allocated" : "not allocated") << std::endl;
            std::cout << "\tCapacity: " << get_capacity() << " bytes (" << (get_capacity() >> 10) << "KB)" << std::endl;
            std::cout << "\tStack start: " << get_base() << ". Stack top: " << get_top() 
                      << ". Stack end: " << (static_cast<char*>(get_base()) + get_capacity()) << std::endl; 
            std::cout << "\tExpected end (hex): " << std::hex 
                      << (reinterpret_cast<uintptr_t>(get_base()) + get_capacity()) << std::dec << std::endl;
            std::cout << "\tChecking ownership of: " << blk.mem << std::endl;
            
            // CRITICAL: Check for memory corruption
            if (get_capacity() != (1 << 20) && get_allocated()) {
                std::cout << "\t*** MEMORY CORRUPTION DETECTED ***" << std::endl;
                std::cout << "\tCapacity should be " << (1 << 20) 
                          << " but is " << get_capacity() << std::endl;
                std::cout << "\tThis indicates static variable corruption!" << std::endl;
                
                // Try to detect what corrupted it
                uintptr_t actual_end = reinterpret_cast<uintptr_t>(get_base()) + get_capacity();
                uintptr_t expected_end = reinterpret_cast<uintptr_t>(get_base()) + (1 << 20);
                std::cout << "\tActual end: " << std::hex << actual_end << std::dec << std::endl;
                std::cout << "\tExpected end: " << std::hex << expected_end << std::dec << std::endl;
                std::cout << "\tDifference: " << (expected_end - actual_end) << " bytes" << std::endl;
            }
            
            // Detect stack overflow condition
            if (get_top() > stack_end) {
                std::cout << "\t*** STACK OVERFLOW DETECTED ***" << std::endl;
                std::cout << "\tTop (" << get_top() << ") is beyond end (" << stack_end << ")" << std::endl;
                std::cout << "\tOverflow by: " << (static_cast<char*>(get_top()) - stack_end) << " bytes" << std::endl;
            }
        #endif
        
        if (!get_allocated() || get_base() == nullptr || blk.mem == nullptr) {
            return false;
        }
        
        return (blk.mem >= get_base() && blk.mem < stack_end);
    }
        
    void deallocate_block(block blk) override {
        return;
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif
        if (blk.mem == nullptr || !owns(blk)) {
            return;
        }
        
        // Only deallocate if it's the top block (LIFO behavior)
        if (static_cast<char*>(blk.mem) + blk.size == get_top()) {
            get_top() = static_cast<char*>(blk.mem);
            #ifdef DEBUG_ALLOCATORS
                std::cout << "Deallocated top block, new top=" << get_top() << std::endl;
            #endif
        }
        #ifdef DEBUG_ALLOCATORS
        else {
            std::cout << "Cannot deallocate non-top block (LIFO violation)" << std::endl;
        }
        #endif
    }
        
    void deallocate_all() override {
        #ifdef DEBUG_ALLOCATORS
            log();
        #endif
        if (get_allocated() && get_base() != nullptr) {
            get_top() = get_base();
            #ifdef DEBUG_ALLOCATORS
                std::cout << "Deallocated all blocks, reset top to base" << std::endl;
            #endif
        }
    }
    
    private:
    // Shared stack memory across all template instantiations
    struct stack_memory {
        static inline bool allocated  = false;
        static inline size_t capacity = 0;
        static inline void*  base     = nullptr;
        static inline void*  top      = nullptr;
    };
    
    // Access the shared memory
    static bool& get_allocated() { return stack_memory::allocated; }
    static size_t& get_capacity() { return stack_memory::capacity; }
    static void*& get_base() { return stack_memory::base; }
    static void*& get_top() { return stack_memory::top; }
};