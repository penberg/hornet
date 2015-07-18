#ifndef HORNET_VM_HH
#define HORNET_VM_HH

#include <unordered_map>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <map>

#include <classfile_constants.h>

#include "hornet/types.hh"

namespace hornet {

class constant_pool;
struct method;
struct field;
struct klass;
struct string;
class loader;

class jvm {
public:
    void init();
    std::shared_ptr<klass> lookup_class(std::string name);
    void register_class(std::shared_ptr<klass> klass);
    void invoke(method* method);
    string* intern_string(std::string str);
private:
    std::mutex _intern_mutex;
    std::unordered_map<std::string, std::shared_ptr<string>> _intern;
    std::map<std::string, std::shared_ptr<klass>> _classes;
};

extern bool bootstrap_done;
extern std::shared_ptr<klass> java_lang_Class;
extern std::shared_ptr<klass> java_lang_String;
extern jvm *_jvm;

using value_t = uint64_t;

struct object {
    struct object* fwd;
    struct klass*  klass;
    std::vector<value_t> _fields;
    std::mutex _mutex;

    object(struct klass* _klass);
    ~object();

    object& operator=(const object&) = delete;
    object(const object&) = delete;

    value_t get_field(size_t offset) const {
        return _fields[offset];
    }

    void set_field(size_t offset, value_t value) {
        _fields[offset] = value;
    }

    void lock() {
        _mutex.lock();
    }

    void unlock() {
        _mutex.unlock();
    }
};

using method_list_type = std::vector<std::shared_ptr<method>>;
using field_list_type = std::vector<std::shared_ptr<field>>;

enum class klass_state {
    loaded,
    initialized,
};

struct klass {
    /// The java/lang/Class object representation of this class.
    struct object* object;
    std::string   name;
    klass*        super;
    uint16_t      access_flags;
    klass_state   state = klass_state::loaded;
    uint32_t      nr_fields;
    std::vector<value_t> static_values;
    std::vector<std::shared_ptr<klass>> interfaces;

    klass(const std::string& name_, loader* loader = nullptr, std::shared_ptr<constant_pool> const_pool = nullptr);
    virtual ~klass();

    klass& operator=(const klass&) = delete;
    klass(const klass&) = delete;

    void init();
    void link();
    bool verify();

    void add(std::shared_ptr<klass> iface);
    void add(std::shared_ptr<method> method);
    void add(std::shared_ptr<field> field);

    virtual bool is_primitive() const {
        return false;
    }

    virtual type get_type() const {
        return type::t_ref;
    }

    virtual bool is_void() const {
        return false;
    }

    virtual size_t size() const {
        return sizeof(void*);
    }

    std::shared_ptr<constant_pool> const_pool() const {
        return _const_pool;
    }

    /// Returns the number of fields for an object instantiated from this class.
    ///
    /// Note! The number of fields returned includes fields from this class as
    /// well as all superclasses.
    uint32_t nr_object_fields() const;

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

    std::shared_ptr<klass> load_class(std::string name);

    /// Lookup a method from this class only.
    std::shared_ptr<method> lookup_method_this(std::string name, std::string desciptor);
    /// Lookup a method from this class or any of the superclasses.
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

template<typename T>
class primitive_klass : public klass {
    type _type;
public:
    primitive_klass(const std::string& name, type type_)
        : klass{name}
        , _type{type_}
    { }

    ~primitive_klass() {
    }

    virtual bool is_primitive() const override {
        return true;
    }

    virtual type get_type() const override {
        return _type;
    }

    virtual size_t size() const override {
        return sizeof(T);
    }
};

struct void_klass : public klass {
    void_klass(const std::string& name)
        : klass(name)
    { }

    ~void_klass() {
    }

    virtual bool is_primitive() const override {
        return true;
    }

    virtual type get_type() const override {
        return type::t_void;
    }

    virtual bool is_void() const {
        return true;
    }

    virtual size_t size() const override {
        throw std::logic_error("void class has no size");
    }
};

class array_klass : public klass {
public:
    array_klass(const std::string& name, klass* elem_type)
        : klass(name)
        , _elem_type(elem_type)
    { }

    ~array_klass() {
    }

    virtual size_t size() const override {
        return _elem_type->size();
    }

private:
    klass* _elem_type;
};

struct field {
    struct klass* klass;
    std::string   name;
    std::string   descriptor;
    uint32_t      offset;
    uint16_t      access_flags;

    field(struct klass* klass_)
        : klass(klass_)
    { }

    ~field() {
    }

    field& operator=(const field&) = delete;
    field(const field&) = delete;

    bool is_static() const {
        return access_flags & JVM_ACC_STATIC;
    }

    bool matches(const std::string& n, const std::string& d) const {
        return name == n && descriptor == d;
    }
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
    std::vector<struct klass*> arg_types;
    uint16_t    args_count;
    uint16_t    max_locals;
    char*       code;
    uint32_t    code_length;
    std::vector<uint8_t> trampoline;

    method() {
    }

    ~method() {
        delete[] code;
    }

    method& operator=(const method&) = delete;
    method(const method&) = delete;

    std::string full_name() const {
        return klass->name + "::" + name + descriptor;
    }

    bool is_init() const {
        return name[0] == '<';
    }

    bool is_native() const {
        return access_flags & JVM_ACC_NATIVE;
    }

    bool matches(const std::string& n, const std::string d) const
    {
       return name == n && descriptor == d;
    }

    std::string jni_name() const {
       auto result = klass->name;
       for (size_t i = 0; i < result.size(); i++) {
           if (result[i] == '/') {
               result[i] = '_';
           }
       }
       return "Java_" + result + "_" + name;
    }
};

struct array {
    struct object object;
    uint32_t length;
    char data[];

    array(klass* klass, uint32_t length)
        : object(klass)
        , length(length)
    { }

    ~array()
    { }

    array& operator=(const array&) = delete;
    array(const array&) = delete;

    template<typename T>
    void set(size_t idx, T value) {
        T* p = reinterpret_cast<T*>(data);
        p[idx] = value;
    }

    template<typename T>
    T get(size_t idx) const {
        auto* p = reinterpret_cast<const T*>(data);
        return p[idx];
    }
};

struct string {
    struct object object;
    const char* data;

    string(const char* data_)
        : object(java_lang_String.get())
        , data(data_)
    { assert(java_lang_String.get() != nullptr); }

    ~string()
    { }

    string& operator=(const string&) = delete;
    string(const string&) = delete;
};

inline bool is_array_type_name(std::string name) {
    return !name.empty() && name[0] == '[';
}

#define java_lang_NoClassDefFoundError reinterpret_cast<hornet::object *>(0xdeabeef)
#define java_lang_NoSuchMethodError reinterpret_cast<hornet::object *>(0xdeabeef)
#define java_lang_VerifyError reinterpret_cast<hornet::object *>(0xdeabeef)

void prim_pre_init();
void prim_post_init();
void gc_init();

object* gc_new_object(klass* klass);
array* gc_new_object_array(klass* klass, size_t length);

}

#endif
