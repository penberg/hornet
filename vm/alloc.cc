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

    auto ret = current->alloc<object>();
    if (!ret) {
        out_of_memory();
    }
    ret->klass = klass;

    return ret;
}

array* gc_new_object_array(klass* klass, size_t length)
{
    thread *current = thread::current();

    auto ret = current->alloc<array>(length * sizeof(void*));
    if (!ret) {
        out_of_memory();
    }
    ret->object.klass = klass;
    ret->length = length;

    return ret;
}

}
