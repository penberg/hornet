#include "hornet/gc.hh"

#include "hornet/system_error.hh"
#include "hornet/os.hh"

#include <sys/mman.h>
#include <cassert>

namespace hornet {

memory_block::memory_block(size_t size)
    : _size(size)
{
    constexpr int mmap_prot  = PROT_READ   | PROT_WRITE;
    constexpr int mmap_flags = MAP_PRIVATE | MAP_ANONYMOUS;

    auto addr = mmap(0, size, mmap_prot, mmap_flags, -1, 0);

    if (addr == MAP_FAILED)
        THROW_ERRNO("mmap");

    if (posix_madvise(addr, size, MADV_HUGEPAGE) < 0)
        THROW_ERRNO("posix_madvise");

    _addr = _next = reinterpret_cast<char *>(addr);
    _end  = _addr + size;
}

memory_block::~memory_block()
{
    if (munmap(_addr, _size) < 0)
        THROW_ERRNO("munmap");
}

memory_block* memory_block::get()
{
    return new memory_block(hugepage_size);
}

void memory_block::put(memory_block* block)
{
    delete block;
}

memory_block* memory_block::swap(memory_block* block)
{
    put(block);
    return get();
}

}
