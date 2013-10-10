#include "hornet/vm.hh"

#include "hornet/gc.hh"
#include "hornet/os.hh"

namespace hornet {

thread::thread()
    : _tlb(thread_local_buffer(hugepage_size))
{
}

thread::~thread()
{
}

}
