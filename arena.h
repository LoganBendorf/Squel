
#include <cstddef>   // for std::size_t
#include <cstdlib>   // for std::malloc, std::free

class arena {
    public:
    void* allocate(int size) {
        int ret_val = next_open_slot;
        next_open_slot += size;
        return base + ret_val;
    }

    void init() {
        base = std::malloc(1 << 20);
        next_open_slot = 0;
    }

    void destroy() {
        std::free(base);
        next_open_slot = -1;
    }

    public:
    void* base;
    int next_open_slot;

};