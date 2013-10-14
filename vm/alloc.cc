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

    auto ret = reinterpret_cast<object*>(current->alloc(sizeof(object)));
    if (!ret) {
        out_of_memory();
    }
    ret->klass = klass;

    return ret;
}

array* gc_new_object_array(klass* klass, size_t length)
{
    thread *current = thread::current();

    auto size = sizeof(array) + length * sizeof(void*);
    auto ret = reinterpret_cast<array*>(current->alloc(size));
    if (!ret) {
        out_of_memory();
    }
    ret->object.klass = klass;
    ret->length = length;

    return ret;
}

}
