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
    std::shared_ptr<klass> klass;
};

using method_list_type = std::valarray<std::shared_ptr<method>>;

struct klass {
    object      object;
    std::string name;

    klass(const method_list_type &methods);
    ~klass();

    std::shared_ptr<method> lookup_method(std::string name, std::string desciptor);

private:
    method_list_type _methods;
};

struct field {
};

struct method {
    std::string name;
    std::string descriptor;
    char*       code;
    uint32_t    code_length;

    method();
    ~method();

    bool matches(std::string name, std::string descriptor);
};

struct array {
    object   object;
    uint32_t length;
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

private:
    thread_local_buffer _tlb;

};

class loader {
public:
    std::shared_ptr<klass> load_class(const char *class_name);
    std::shared_ptr<klass> load_file(const char *file_name);
};

inline loader *system_loader()
{
    static loader loader;

    return &loader;
}

inline void throw_exception(struct object *exception)
{
    thread *current = thread::current();

    current->exception = exception;
}

#define java_lang_NoClassDefFoundError reinterpret_cast<hornet::object *>(0xdeabeef)
#define java_lang_NoSuchMethodError reinterpret_cast<hornet::object *>(0xdeabeef)

}

#endif
