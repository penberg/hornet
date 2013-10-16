#ifndef HORNET_GC_HH
#define HORNET_GC_HH

#include <cstddef>

namespace hornet {

//
// A memory block that uses pointer-bump for fast thread-local allocation.
// The memory block is backed by mmap'd memory.
//
class memory_block {
public:
    explicit memory_block(size_t size);
    ~memory_block();

    void reset();

    bool is_enough_space(size_t size) {
        return _next + size <= _end;
    }

    // The caller is expected to ensure there's enough space for the
    // allocation.
    char* alloc(size_t size) {
        auto start = _next;
        _next += size;
        return start;
    }

    //
    // Block allocator:
    //
    static memory_block* get();
    static void put(memory_block* block);
    static memory_block* swap(memory_block* block);
private:
    char*  _addr;
    size_t _size;

    char*  _next;
    char*  _end;
};

}

#endif
