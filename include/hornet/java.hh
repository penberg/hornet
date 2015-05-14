#ifndef HORNET_JAVA_HH
#define HORNET_JAVA_HH

#include "hornet/zip.hh"
#include "hornet/vm.hh"

#include <jni.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <utility>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <stack>

namespace hornet {

struct method;
struct field;
struct klass;

enum class cp_tag {
    const_class,
    const_fieldref,
    const_methodref,
    const_interface_methodref,
    const_string,
    const_integer,
    const_float,
    const_long,
    const_double,
    const_name_and_type,
    const_utf8,
    const_method_handle,
    const_method_type,
    const_invoke_dynamic,
};

struct cp_info final {
    cp_tag tag;

    cp_info(cp_tag tag) : tag(tag) { }

    ~cp_info()
    {
        switch (tag) {
        case cp_tag::const_utf8:
            delete[] bytes;
            break;
        }
    }

    cp_info(const cp_info&) = delete;
    cp_info& operator=(const cp_info&) = delete;

    union {
        struct {
            uint16_t name_index;
            uint16_t descriptor_index;
        };
        struct {
            uint16_t class_index;
            uint16_t name_and_type_index;
        };
        struct {
            uint16_t string_index;
        };
        jint     int_value;
        jlong    long_value;
        jfloat   float_value;
        jdouble  double_value;
        char*    bytes;
    };

    static inline
    std::shared_ptr<cp_info> make_class(uint16_t name_index) {
        auto ret = std::make_shared<cp_info>(cp_tag::const_class);
        ret->name_index = name_index;
        return ret;
    }

    static inline
    std::shared_ptr<cp_info> make_fieldref(uint16_t class_index, uint16_t name_and_type_index) {
        auto ret = std::make_shared<cp_info>(cp_tag::const_fieldref);
        ret->class_index = class_index;
        ret->name_and_type_index = name_and_type_index;
        return ret;
    }

    static inline
    std::shared_ptr<cp_info> make_methodref(uint16_t class_index, uint16_t name_and_type_index) {
        auto ret = std::make_shared<cp_info>(cp_tag::const_methodref);
        ret->class_index = class_index;
        ret->name_and_type_index = name_and_type_index;
        return ret;
    }

    static inline
    std::shared_ptr<cp_info> make_interface_methodref(uint16_t class_index, uint16_t name_and_type_index) {
        auto ret = std::make_shared<cp_info>(cp_tag::const_interface_methodref);
        ret->class_index = class_index;
        ret->name_and_type_index = name_and_type_index;
        return ret;
    }

    static inline
    std::shared_ptr<cp_info> make_string(uint16_t string_index) {
        auto ret = std::make_shared<cp_info>(cp_tag::const_string);
        ret->string_index = string_index;
        return ret;
    }

    static inline
    std::shared_ptr<cp_info> make_integer(jint value) {
        auto ret = std::make_shared<cp_info>(cp_tag::const_integer);
        ret->int_value = value;
        return ret;
    }

    static inline
    std::shared_ptr<cp_info> make_long(jlong value) {
        auto ret = std::make_shared<cp_info>(cp_tag::const_long);
        ret->long_value = value;
        return ret;
    }

    static inline
    std::shared_ptr<cp_info> make_float(jfloat value) {
        auto ret = std::make_shared<cp_info>(cp_tag::const_float);
        ret->float_value = value;
        return ret;
    }

    static inline
    std::shared_ptr<cp_info> make_double(jdouble value) {
        auto ret = std::make_shared<cp_info>(cp_tag::const_double);
        ret->double_value = value;
        return ret;
    }

    static inline
    std::shared_ptr<cp_info> make_name_and_type(uint16_t name_index, uint16_t descriptor_index) {
        auto ret = std::make_shared<cp_info>(cp_tag::const_name_and_type);
        ret->name_index = name_index;
        ret->descriptor_index = descriptor_index;
        return ret;
    }

    static inline
    std::shared_ptr<cp_info> make_utf8_info(char* bytes) {
        auto ret = std::make_shared<cp_info>(cp_tag::const_utf8);
        ret->bytes = bytes;
        return ret;
    }
};

class constant_pool {
    std::vector<std::shared_ptr<cp_info>> _entries;
public:
    explicit constant_pool(uint16_t size);
    ~constant_pool();

    void set(uint16_t idx, std::shared_ptr<cp_info> entry);

    const cp_info& get(uint16_t idx) const;
    const cp_info& get_class(uint16_t idx) const;
    const cp_info& get_fieldref(uint16_t idx) const;
    const cp_info& get_methodref(uint16_t idx) const;
    const cp_info& get_interface_methodref(uint16_t idx) const;
    const cp_info& get_name_and_type(uint16_t idx) const;
    jint get_integer(uint16_t idx) const;
    jlong get_long(uint16_t idx) const;
    jfloat get_float(uint16_t idx) const;
    jdouble get_double(uint16_t idx) const;
    const cp_info& get_utf8(uint16_t idx) const;
    string *get_string(uint16_t idx) const;

private:
    template<typename Type, cp_tag Tag>
    const Type& get_ty(uint16_t idx) const;
};

enum class attr_type {
    code,
    unknown
};

struct attr_info {
    attr_type type;

    attr_info(attr_type type) : type(type) {}
};

struct code_attr : attr_info {
    uint16_t max_locals;
    char*    code;
    uint32_t code_length;

    code_attr() : attr_info(attr_type::code) {}
};

struct unknown_attr : attr_info {
    unknown_attr() : attr_info(attr_type::unknown) {}
};

string* intern_string(std::string str);

class class_file {
public:
    explicit class_file(void *data, size_t size);
    ~class_file();

    std::shared_ptr<klass> parse();
private:

    std::shared_ptr<constant_pool> read_constant_pool();
    std::shared_ptr<cp_info> read_const_class();
    std::shared_ptr<cp_info> read_const_fieldref();
    std::shared_ptr<cp_info> read_const_methodref();
    std::shared_ptr<cp_info> read_const_interface_methodref();
    std::shared_ptr<cp_info> read_const_string();
    std::shared_ptr<cp_info> read_const_integer();
    std::shared_ptr<cp_info> read_const_float();
    std::shared_ptr<cp_info> read_const_long();
    std::shared_ptr<cp_info> read_const_double();
    std::shared_ptr<cp_info> read_const_name_and_type();
    std::shared_ptr<cp_info> read_const_utf8();
    void read_const_method_handle();
    void read_const_method_type();
    void read_const_invoke_dynamic();

    std::shared_ptr<field> read_field_info(klass* klass, constant_pool &constant_pool);
    std::shared_ptr<method> read_method_info(klass* klass, constant_pool &constant_pool);
    std::unique_ptr<attr_info> read_attr_info(constant_pool &constant_pool);
    std::unique_ptr<code_attr> read_code_attribute(constant_pool &constant_pool);

    uint8_t  read_u1();
    uint16_t read_u2();
    uint32_t read_u4();
    uint64_t read_u8();

    size_t _offset;
    size_t _size;
    char *_data;
};

std::shared_ptr<klass> prim_sig_to_klass(char sig);
std::shared_ptr<klass> atype_to_klass(uint8_t atype);

extern bool verbose_compiler;

extern bool verbose_class;

extern bool verbose_verifier;

bool verify_method(std::shared_ptr<method> method);
void verifier_stats();

class classpath_entry {
public:
    classpath_entry() { }
    virtual ~classpath_entry() { }

    classpath_entry(const classpath_entry&) = delete;
    classpath_entry& operator=(const classpath_entry&) = delete;

    virtual std::shared_ptr<klass> load_class(std::string class_name) = 0;
};

class classpath_dir : public classpath_entry {
public:
    classpath_dir(std::string filename);
    ~classpath_dir();

    classpath_dir(const classpath_dir&) = delete;
    classpath_dir& operator=(const classpath_dir&) = delete;

    std::shared_ptr<klass> load_class(std::string class_name) override;

private:
    std::shared_ptr<klass> load_file(const char *file_name);

    std::string _path;
};

class jar : public classpath_entry {
public:
    jar(std::string filename);
    ~jar();

    jar(const jar&) = delete;
    jar& operator=(const jar&) = delete;

    std::shared_ptr<klass> load_class(std::string class_name) override;

private:
    std::string  _filename;
    hornet::zip* _zip;
};

class loader {
public:
    void register_entry(std::string path);
    std::shared_ptr<klass> load_class(std::string class_name);
private:
    std::shared_ptr<klass> try_to_load_class(std::string class_name);
    std::vector<std::shared_ptr<classpath_entry>> _entries;
};

class system_loader {
public:
    static void init() {
        const char *java_home = getenv("JAVA_HOME");
        if (!java_home)
            java_home = "/usr/lib/jvm/java";

        get()->register_entry(std::string(java_home) + "/jre/lib/rt.jar");
    }
    static loader *get() {
        static loader loader;
        return &loader;
    }
};

inline loader *system_loader()
{
    return system_loader::get();
}

// JVM interpreter frame that has local variables, operand stack, and a program
// counter. It is constructed at each method invocation and destroyed when an
// invocation completes.
struct frame {
    std::vector<value_t> locals;
    std::vector<value_t> ostack;
    uint16_t             pc;

    frame(size_t size)
       : locals(size), pc(0)
    { }

    ~frame()
    { }

    void reserve_more(size_t size) {
        locals.reserve(locals.size() + size);
    }

    void ostack_push(value_t value) {
        ostack.push_back(value);
    }

    void ostack_pop() {
        ostack.pop_back();
    }

    value_t ostack_top() const {
        return ostack.back();
    }
};

enum class backend_type {
    interp,
    dynasm,
    llvm,
};

class backend {
public:
    virtual ~backend() { };
    virtual value_t execute(method* method, frame& frame) = 0;
};

class interp_backend : public backend {
public:
    virtual value_t execute(method* method, frame& frame) override;
};

struct dasm_State;
class dynasm_translator;

class dynasm_backend : public backend {
public:
    dynasm_backend();
    ~dynasm_backend();
    virtual value_t execute(method* method, frame& frame) override;

    dasm_State* D;
private:
    size_t _offset;
    void* _code;

    friend dynasm_translator;
};

class llvm_backend : public backend {
public:
    llvm_backend();
    ~llvm_backend();
    virtual value_t execute(method* method, frame& frame) override;

    static bool debug;
};

extern backend* _backend;

class thread {
public:
    thread();
    ~thread();

    object *exception;

    static thread *current() {
        static thread thread;

        return &thread;
    }

    template<typename... Args>
    frame* make_frame(Args&&... args) {
        char* raw_frame = _stack + _stack_pos;
        _stack_pos += sizeof(struct frame);
        assert(_stack_pos < _stack_max);
        return new (raw_frame) frame(std::forward<Args>(args)...);
    }

    void free_frame(frame* frame) {
        frame->~frame();
        _stack_pos -= sizeof(struct frame);
    }

private:
    static char* mmap_stack(size_t size);
    static void munmap_stack(char* p, size_t size);

    static constexpr size_t _stack_max = 1024*1024; /* 1 MB */
    size_t _stack_pos;
    char* _stack;
};

inline void throw_exception(struct object *exception)
{
    thread *current = thread::current();

    current->exception = exception;
}

}

#endif
