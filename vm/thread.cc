#include "hornet/java.hh"

#include "hornet/system_error.hh"

#include <sys/mman.h>

namespace hornet {

thread::thread()
    : _stack(mmap_stack(_stack_max))
{
}

thread::~thread()
{
    munmap_stack(_stack, _stack_max);
}

char* thread::mmap_stack(size_t size)
{
    auto* p = mmap(nullptr, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
    if (p == MAP_FAILED) {
        THROW_ERRNO("mmap");
    }
    return static_cast<char*>(p);
}

void thread::munmap_stack(char* p, size_t size)
{
    if (munmap(p, size) < 0) {
        THROW_ERRNO("munmap");
    }
}

}
