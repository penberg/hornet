#include "hornet/vm.hh"

#include "hornet/java.hh"

#include <string>

namespace hornet {

klass::klass()
    : object(nullptr)
    , nr_fields(0)
    , _loader(nullptr)
{
}

klass::klass(loader *loader, std::shared_ptr<constant_pool> const_pool)
    : object(nullptr)
    , nr_fields(0)
    , _const_pool(const_pool)
    , _loader(loader)
{
}

klass::~klass()
{
}

void klass::add(std::shared_ptr<klass> iface)
{
    interfaces.push_back(iface);
}

void klass::add(std::shared_ptr<field> field)
{
    if (field->is_static()) {
        auto offset = static_values.size();
        static_values.reserve(offset + 1);
        static_values[offset] = 0;
        field->offset = offset;
    } else {
        auto offset = nr_fields;
        field->offset = offset;
        nr_fields++;
    }
    _fields.push_back(field);
}

void klass::add(std::shared_ptr<method> method)
{
    _methods.push_back(method);
}

std::shared_ptr<field> klass::lookup_field(std::string name, std::string descriptor)
{
    klass* klass = this;
    while (klass) {
        for (auto field : klass->_fields) {
            if (field->matches(name, descriptor))
                return field;
        }
        klass = klass->super;
    }
    return nullptr;
}

std::shared_ptr<method> klass::lookup_method(std::string name, std::string descriptor)
{
    klass* klass = this;
    while (klass) {
        for (auto method : klass->_methods) {
            if (method->matches(name, descriptor))
                return method;
        }
        klass = klass->super;
    }
    return nullptr;
}

std::shared_ptr<klass> klass::resolve_class(uint16_t idx)
{
    auto klassref = _const_pool->get_class(idx);
    auto klass_name = _const_pool->get_utf8(klassref->name_index);
    return _loader->load_class(klass_name->bytes);
}

std::shared_ptr<field> klass::resolve_field(uint16_t idx)
{
    auto fieldref = _const_pool->get_fieldref(idx);
    auto target_klass = resolve_class(fieldref->class_index);
    if (!target_klass) {
        return nullptr;
    }
    auto field_name_and_type = _const_pool->get_name_and_type(fieldref->name_and_type_index);
    auto field_name = _const_pool->get_utf8(field_name_and_type->name_index);
    auto field_type = _const_pool->get_utf8(field_name_and_type->descriptor_index);
    return target_klass->lookup_field(field_name->bytes, field_type->bytes);
}

std::shared_ptr<method> klass::resolve_method(uint16_t idx)
{
    auto methodref = _const_pool->get_methodref(idx);
    auto target_klass = resolve_class(methodref->class_index);
    if (!target_klass) {
        return nullptr;
    }
    auto method_name_and_type = _const_pool->get_name_and_type(methodref->name_and_type_index);
    auto method_name = _const_pool->get_utf8(method_name_and_type->name_index);
    auto method_type = _const_pool->get_utf8(method_name_and_type->descriptor_index);
    return target_klass->lookup_method(method_name->bytes, method_type->bytes);
}

std::shared_ptr<method> klass::resolve_interface_method(uint16_t idx)
{
    auto methodref = _const_pool->get_interface_methodref(idx);
    auto target_klass = resolve_class(methodref->class_index);
    if (!target_klass) {
        return nullptr;
    }
    auto method_name_and_type = _const_pool->get_name_and_type(methodref->name_and_type_index);
    auto method_name = _const_pool->get_utf8(method_name_and_type->name_index);
    auto method_type = _const_pool->get_utf8(method_name_and_type->descriptor_index);
    return target_klass->lookup_method(method_name->bytes, method_type->bytes);
}

void klass::init()
{
    switch (state) {
    case klass_state::loaded: {
        state = klass_state::initialized;
        auto clinit = lookup_method("<clinit>", "()V");
        if (clinit) {
            auto thread = hornet::thread::current();
            auto new_frame = thread->make_frame(0);
            hornet::_backend->execute(clinit.get(), *new_frame);
            thread->free_frame(new_frame);
        }
        break;
    }
    case klass_state::initialized:
        break;
    }
}

bool klass::verify()
{
    for (auto method : _methods) {
        if (!verify_method(method))
            return false;
    }
    return true;
}

template<typename T>
struct primitive_klass : public klass {
    primitive_klass() {
    }

    ~primitive_klass() {
    }

    virtual bool is_primitive() const override {
        return true;
    }

    virtual size_t size() const override {
        return sizeof(T);
    }
};

static primitive_klass<jboolean> jboolean_klass;
static primitive_klass<jchar>    jchar_klass;
static primitive_klass<jfloat>   jfloat_klass;
static primitive_klass<jdouble>  jdouble_klass;
static primitive_klass<jbyte>    jbyte_klass;
static primitive_klass<jshort>   jshort_klass;
static primitive_klass<jint>     jint_klass;
static primitive_klass<jlong>    jlong_klass;

klass* klass::primitive_type(uint8_t atype)
{
    switch (atype) {
    case JVM_T_BOOLEAN: return &jboolean_klass;
    case JVM_T_CHAR:    return &jchar_klass;
    case JVM_T_FLOAT:   return &jfloat_klass;
    case JVM_T_DOUBLE:  return &jdouble_klass;
    case JVM_T_BYTE:    return &jbyte_klass;
    case JVM_T_SHORT:   return &jshort_klass;
    case JVM_T_INT:     return &jint_klass;
    case JVM_T_LONG:    return &jlong_klass;
    default:            assert(0);
    }
}

};
