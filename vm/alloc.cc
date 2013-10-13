#include "hornet/vm.hh"

#include <cassert>

namespace hornet {

array* gc_new_object_array(klass* klass, size_t length)
{
    thread *current = thread::current();

    auto size = sizeof(array) + length * sizeof(void*);
    auto ret = reinterpret_cast<array*>(current->alloc(size));

    assert(ret != nullptr);

    ret->object.klass = klass;
    ret->length = length;

    return ret;
}

}
