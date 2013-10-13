#include "hornet/gc.hh"

#include "hornet/system_error.hh"

#include <sys/mman.h>
#include <cassert>

namespace hornet {

memory_block::memory_block(size_t size)
    : _size(size)
    , _offset(0)
{
    constexpr int mmap_prot  = PROT_READ   | PROT_WRITE;
    constexpr int mmap_flags = MAP_PRIVATE | MAP_ANONYMOUS;

    auto addr = mmap(0, size, mmap_prot, mmap_flags, -1, 0);

    if (addr == MAP_FAILED)
        THROW_ERRNO("mmap");

    if (posix_madvise(addr, size, MADV_HUGEPAGE) < 0)
        THROW_ERRNO("posix_madvise");

    _addr = reinterpret_cast<char *>(addr);
}

memory_block::~memory_block()
{
    if (munmap(_addr, _size) < 0)
        THROW_ERRNO("munmap");
}

}
