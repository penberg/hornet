#ifndef HORNET_VM_HH
#define HORNET_VM_HH

#include <hornet/gc.hh>

#include <valarray>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <mutex>

namespace hornet {

struct method;
struct klass;

class jvm {
public:
    void register_klass(std::shared_ptr<klass> klass);
    void invoke(method* method);

private:
    std::mutex _mutex;
    std::vector<std::shared_ptr<klass>> _klasses;
};

extern jvm *_jvm;

struct object {
    struct klass *klass;

    object(struct klass* _klass) : klass(_klass) {}
};

using method_list_type = std::valarray<std::shared_ptr<method>>;

struct klass {
    struct object object;
    std::string name;

    klass(const method_list_type &methods);
    ~klass();

    bool verify();

    std::shared_ptr<method> lookup_method(std::string name, std::string desciptor);

private:
    method_list_type _methods;
};

struct field {
};

struct method {
    std::string name;
    std::string descriptor;
    uint16_t    args_count;
    uint16_t    max_locals;
    char*       code;
    uint32_t    code_length;

    method();
    ~method();

    bool matches(std::string name, std::string descriptor);
};

struct array {
    struct object object;
    uint32_t length;

    array(klass* klass, uint32_t length)
        : object(klass)
        , length(length) {}
};

class thread {
public:
    thread();
    ~thread();

    object *exception;

    static thread *current() {
        static thread thread;

        return &thread;
    }

    template<typename T>
    T* alloc() { return alloc<T>(0); }

    template<typename T>
    T* alloc(size_t extra) {
        auto size = sizeof(T) + extra;
        if (!_alloc_buffer->is_enough_space(size)) {
            _alloc_buffer = memory_block::swap(_alloc_buffer);
        }
        if (!_alloc_buffer) {
            return nullptr;
        }
        auto p = _alloc_buffer->alloc(size);
        return reinterpret_cast<T*>(p);
    }

private:
    memory_block* _alloc_buffer;

};

inline void throw_exception(struct object *exception)
{
    thread *current = thread::current();

    current->exception = exception;
}

#define java_lang_NoClassDefFoundError reinterpret_cast<hornet::object *>(0xdeabeef)
#define java_lang_NoSuchMethodError reinterpret_cast<hornet::object *>(0xdeabeef)
#define java_lang_VerifyError reinterpret_cast<hornet::object *>(0xdeabeef)

object* gc_new_object(klass* klass);
array* gc_new_object_array(klass* klass, size_t length);

}

#endif
