#include "hornet/vm.hh"

#include "hornet/gc.hh"
#include "hornet/os.hh"

namespace hornet {

thread::thread()
    : _alloc_buffer(memory_block(hugepage_size))
{
}

thread::~thread()
{
}

}
