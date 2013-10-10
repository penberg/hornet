#ifndef HORNET_GC_HH
#define HORNET_GC_HH

#include <cstddef>

namespace hornet {

//
// A memory region that uses pointer-bump for fast thread-local allocation. The
// memory region is usually backed by one or more mmap'd hugepages. The data
// structure is also known as thread-local allocation buffer (TLAB).
//
class thread_local_buffer {
public:
    explicit thread_local_buffer(size_t size);
    ~thread_local_buffer();

    char *alloc(size_t objsize) {
        if (_offset + objsize > _size)
            return nullptr;
        auto offset = _offset;
        _offset += objsize;
        return _addr + offset;
    }
private:
    char *_addr;
    size_t _size;
    size_t _offset;
};

}

#endif
