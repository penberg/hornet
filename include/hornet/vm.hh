#ifndef HORNET_VM_HH
#define HORNET_VM_HH

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace hornet {

class constant_pool;
struct method;
struct field;
struct klass;
class loader;

class jvm {
public:
    std::shared_ptr<klass> lookup_class(std::string name);
    void register_class(std::shared_ptr<klass> klass);
    void invoke(method* method);

private:
    std::map<std::string, std::shared_ptr<klass>> _classes;
};

extern jvm *_jvm;

typedef uint64_t value_t;

struct object {
    struct object* fwd;
    struct klass*  klass;
    std::vector<value_t> instance_values;

    object(struct klass* _klass);
    ~object();
};

using method_list_type = std::vector<std::shared_ptr<method>>;
using field_list_type = std::vector<std::shared_ptr<field>>;

enum class klass_state {
    loaded,
    initialized,
};

struct klass {
    struct object object;
    std::string   name;
    klass*        super;
    uint16_t      access_flags;
    klass_state   state = klass_state::loaded;
    uint32_t      nr_fields;
    std::vector<value_t> static_values;

    klass(loader* loader, std::shared_ptr<constant_pool> const_pool);
    ~klass();

    void init();
    bool verify();

    void add(std::shared_ptr<method> method);
    void add(std::shared_ptr<field> field);

    std::shared_ptr<constant_pool> const_pool() const {
        return _const_pool;
    }

    bool is_subclass_of(klass* klass) {
        auto* super = this;
        while (super != nullptr) {
            if (super == klass) {
                return true;
            }
            super = super->super;
        }
        return false;
    }

    std::shared_ptr<method> lookup_method(std::string name, std::string desciptor);
    std::shared_ptr<field> lookup_field(std::string name, std::string desciptor);

    std::shared_ptr<klass>  resolve_class (uint16_t idx);
    std::shared_ptr<field>  resolve_field (uint16_t idx);
    std::shared_ptr<method> resolve_method(uint16_t idx);
    std::shared_ptr<method> resolve_interface_method(uint16_t idx);

private:
    std::shared_ptr<constant_pool> _const_pool;
    method_list_type _methods;
    field_list_type _fields;
    loader* _loader;
};

struct field {
    struct klass* klass;
    std::string   name;
    std::string   descriptor;
    uint32_t      offset;
    bool          isstatic;

    field(struct klass* klass_);
    ~field();

    bool is_static() const {
        return isstatic;
    }

    bool matches(std::string name, std::string descriptor);
};

struct method {
    // Method lifecycle is tied to the class it belongs to. Use a pointer to
    // klass instead of a smart pointer to break the cyclic dependency during
    // destruction.
    struct klass* klass;
    uint16_t    access_flags;
    std::string name;
    std::string descriptor;
    struct klass* return_type;
    uint16_t    args_count;
    uint16_t    max_locals;
    char*       code;
    uint32_t    code_length;

    method();
    ~method();

    std::string full_name() const {
        return klass->name + "::" + name + descriptor;
    }

    bool is_init() const {
        return name[0] == '<';
    }

    bool matches(std::string name, std::string descriptor);
};

struct array {
    struct object object;
    uint32_t length;

    array(klass* klass, uint32_t length)
        : object(klass)
        , length(length) {}
};

struct string {
    struct object object;
    const char* data;

    string(const char* data_)
        : object(nullptr)
        , data(data_)
    { }
};

#define java_lang_NoClassDefFoundError reinterpret_cast<hornet::object *>(0xdeabeef)
#define java_lang_NoSuchMethodError reinterpret_cast<hornet::object *>(0xdeabeef)
#define java_lang_VerifyError reinterpret_cast<hornet::object *>(0xdeabeef)

void gc_init();

object* gc_new_object(klass* klass);
array* gc_new_object_array(klass* klass, size_t length);

}

#endif
