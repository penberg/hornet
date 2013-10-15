#include "hornet/vm.hh"

#include <cstdlib>

namespace hornet {

void out_of_memory()
{
    fprintf(stderr, "error: out of memory\n");
    abort();
}

object* gc_new_object(klass* klass)
{
    thread *current = thread::current();

    auto p = current->alloc<object>();
    if (!p) {
        out_of_memory();
    }
    return new (p) object{klass};
}

array* gc_new_object_array(klass* klass, size_t length)
{
    thread *current = thread::current();

    auto p = current->alloc<array>(length * sizeof(void*));
    if (!p) {
        out_of_memory();
    }
    return new (p) array{klass, static_cast<uint32_t>(length)};
}

}
