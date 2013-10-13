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

    char* alloc(size_t size) {
        if (_offset + size > _size)
            return nullptr;
        auto offset = _offset;
        _offset += size;
        return _addr + offset;
    }
private:
    char*  _addr;
    size_t _size;
    size_t _offset;
};

}

#endif
