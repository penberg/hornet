#include "hornet/gc.hh"

#include "hornet/system_error.hh"
#include "hornet/os.hh"

#include <sys/mman.h>
#include <cstring>
#include <memory>
#include <vector>
#include <mutex>

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

void memory_block::reset()
{
    memset(_addr, 0, _size);
    _next = _addr;
}

std::vector<std::unique_ptr<memory_block>> blocks;
std::mutex block_mutex;

memory_block *memory_block::get()
{
    std::lock_guard<std::mutex> lock{block_mutex};

    if (!blocks.empty()) {
        auto block = blocks.back().release();
        blocks.pop_back();
        block->reset();
        return block;
    }

    block_mutex.unlock();

    return new memory_block(hugepage_size);
}

void memory_block::put(memory_block* block)
{
    std::lock_guard<std::mutex> lock{block_mutex};

    blocks.push_back(std::unique_ptr<memory_block>(block));
}

memory_block* memory_block::swap(memory_block* block)
{
    put(block);
    return get();
}

}
