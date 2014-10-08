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

struct cp_info {
    cp_info(cp_tag tag) : tag(tag) { }

    virtual ~cp_info() { }

    cp_tag tag;

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
    };

    static inline
    std::shared_ptr<cp_info> const_class(uint16_t name_index) {
        auto ret = std::make_shared<cp_info>(cp_tag::const_class);
        ret->name_index = name_index;
        return ret;
    }

    static inline
    std::shared_ptr<cp_info> const_fieldref(uint16_t class_index, uint16_t name_and_type_index) {
        auto ret = std::make_shared<cp_info>(cp_tag::const_fieldref);
        ret->class_index = class_index;
        ret->name_and_type_index = name_and_type_index;
        return ret;
    }

    static inline
    std::shared_ptr<cp_info> const_methodref(uint16_t class_index, uint16_t name_and_type_index) {
        auto ret = std::make_shared<cp_info>(cp_tag::const_methodref);
        ret->class_index = class_index;
        ret->name_and_type_index = name_and_type_index;
        return ret;
    }

    static inline
    std::shared_ptr<cp_info> const_interface_methodref(uint16_t class_index, uint16_t name_and_type_index) {
        auto ret = std::make_shared<cp_info>(cp_tag::const_interface_methodref);
        ret->class_index = class_index;
        ret->name_and_type_index = name_and_type_index;
        return ret;
    }

    static inline
    std::shared_ptr<cp_info> const_string(uint16_t string_index) {
        auto ret = std::make_shared<cp_info>(cp_tag::const_string);
        ret->string_index = string_index;
        return ret;
    }

    static inline
    std::shared_ptr<cp_info> const_integer(jint value) {
        auto ret = std::make_shared<cp_info>(cp_tag::const_integer);
        ret->int_value = value;
        return ret;
    }

    static inline
    std::shared_ptr<cp_info> const_long(jlong value) {
        auto ret = std::make_shared<cp_info>(cp_tag::const_long);
        ret->long_value = value;
        return ret;
    }

    static inline
    std::shared_ptr<cp_info> const_float(jfloat value) {
        auto ret = std::make_shared<cp_info>(cp_tag::const_float);
        ret->float_value = value;
        return ret;
    }

    static inline
    std::shared_ptr<cp_info> const_double(jdouble value) {
        auto ret = std::make_shared<cp_info>(cp_tag::const_double);
        ret->double_value = value;
        return ret;
    }

    static inline
    std::shared_ptr<cp_info> const_name_and_type(uint16_t name_index, uint16_t descriptor_index) {
        auto ret = std::make_shared<cp_info>(cp_tag::const_name_and_type);
        ret->name_index = name_index;
        ret->descriptor_index = descriptor_index;
        return ret;
    }
};

struct const_utf8_info : cp_info {
    const_utf8_info() : cp_info(cp_tag::const_utf8) { }

    ~const_utf8_info() {
        delete[] bytes;
    }

    char *bytes;
};

class constant_pool {
public:
    explicit constant_pool(uint16_t size);
    ~constant_pool();

    void set(uint16_t idx, std::shared_ptr<cp_info> entry);

    cp_info *get(uint16_t idx);
    cp_info *get_class(uint16_t idx);
    cp_info *get_fieldref(uint16_t idx);
    cp_info *get_methodref(uint16_t idx);
    cp_info *get_interface_methodref(uint16_t idx);
    cp_info *get_name_and_type(uint16_t idx);
    jint get_integer(uint16_t idx);
    jlong get_long(uint16_t idx);
    jfloat get_float(uint16_t idx);
    jdouble get_double(uint16_t idx);
    const_utf8_info *get_utf8(uint16_t idx);
    string *get_string(uint16_t idx);

private:
    std::vector<std::shared_ptr<cp_info>> _entries;
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

extern klass jvm_void_klass;

extern bool verbose_compiler;

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
    hornet::zip* _zip;
};

class loader {
public:
    void register_entry(std::string path);
    std::shared_ptr<klass> load_class(const char *class_name);
private:
    std::shared_ptr<klass> try_to_load_class(const char *class_name);
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

extern unsigned char opcode_length[];

static inline uint8_t read_opc_u1(char *p)
{
    return p[1];
}

static inline uint16_t read_opc_u2(char *p)
{
    return read_opc_u1(p) << 8 | read_opc_u1(p+1);
}

struct frame {
    frame(uint16_t max_locals)
       : locals(max_locals), pc(0) {}

    std::vector<value_t> locals;
    std::stack<value_t>    ostack;
    uint16_t               pc;
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
        auto f = new frame(std::forward<Args>(args)...);
        _frames.push_back(std::unique_ptr<frame>(f));
        return f;
    }

    void free_frame(frame* frame) {
        assert(frame == _frames.back().get());
        _frames.pop_back();
    }

private:
    std::vector<std::unique_ptr<frame>> _frames;
};

inline void throw_exception(struct object *exception)
{
    thread *current = thread::current();

    current->exception = exception;
}

}

#endif
