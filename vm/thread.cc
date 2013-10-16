#include "hornet/vm.hh"

#include "hornet/gc.hh"

namespace hornet {

thread::thread()
    : _alloc_buffer(memory_block::get())
{
}

thread::~thread()
{
    memory_block::put(_alloc_buffer);
}

}
