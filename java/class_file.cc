#include "hornet/java.hh"
#include "hornet/vm.hh"

#include <classfile_constants.h>
#include <functional>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <string>

namespace hornet {

class_file::class_file(void *data, size_t size)
    : _offset(0)
    , _size(size)
    , _data(reinterpret_cast<char *>(data))
{
}

class_file::~class_file()
{
}

std::shared_ptr<klass> class_file::parse()
{
    if (!_size)
        assert(0);

    auto magic = read_u4();

    if (magic != 0xcafebabe)
        assert(0);

    /*auto minor_version = */read_u2();

    auto major_version = read_u2();

    if (major_version > JVM_CLASSFILE_MAJOR_VERSION)
        assert(0);

    auto const_pool = read_constant_pool();

    auto access_flags = read_u2();

    auto this_class = read_u2();

    auto super_class = read_u2();

    auto& klassref = const_pool->get_class(this_class);

    auto& klass_name = const_pool->get_utf8(klassref.name_index);

    auto klass = std::make_shared<hornet::klass>(klass_name.bytes, hornet::system_loader(), const_pool);

    hornet::_jvm->register_class(klass);

    auto interfaces_count = read_u2();

    for (auto i = 0; i < interfaces_count; i++) {
        auto idx = read_u2();

        auto iface = klass->resolve_class(idx);

        klass->add(iface);
    }

    auto fields_count = read_u2();

    for (auto i = 0; i < fields_count; i++) {
        auto field = read_field_info(klass.get(), *const_pool);

        klass->add(field);
    }

    auto methods_count = read_u2();

    for (auto i = 0; i < methods_count; i++) {
        auto method = read_method_info(klass.get(), *const_pool);

        klass->add(method);
    }

    auto attr_count = read_u2();

    for (auto i = 0; i < attr_count; i++) {
        read_attr_info(*const_pool);
    }

    klass->access_flags = access_flags;

    if (super_class) {
        auto super = klass->resolve_class(super_class);
        klass->super = super.get();
    } else {
        klass->super = nullptr;
    }

    return klass;
}

std::shared_ptr<constant_pool> class_file::read_constant_pool()
{
    auto constant_pool_count = read_u2();

    assert(constant_pool_count > 0);

    auto const_pool = std::make_shared<constant_pool>(constant_pool_count);

    for (auto idx = 0; idx < constant_pool_count-1; idx++) {
        auto tag = read_u1();
        std::shared_ptr<cp_info> cp_info;
        switch (tag) {
        case JVM_CONSTANT_Class:
            cp_info = read_const_class();
            break;
        case JVM_CONSTANT_Fieldref:
            cp_info = read_const_fieldref();
            break;
        case JVM_CONSTANT_Methodref:
            cp_info = read_const_methodref();
            break;
        case JVM_CONSTANT_InterfaceMethodref:
            cp_info = read_const_interface_methodref();
            break;
        case JVM_CONSTANT_String:
            cp_info = read_const_string();
            break;
        case JVM_CONSTANT_Integer:
            cp_info = read_const_integer();
            break;
        case JVM_CONSTANT_Float:
            cp_info = read_const_float();
            break;
        case JVM_CONSTANT_Long:
            cp_info = read_const_long();
            break;
        case JVM_CONSTANT_Double:
            cp_info = read_const_double();
            break;
        case JVM_CONSTANT_NameAndType:
            cp_info = read_const_name_and_type();
            break;
        case JVM_CONSTANT_Utf8:
            cp_info = read_const_utf8();
            break;
        case JVM_CONSTANT_MethodHandle:
            read_const_method_handle();
            break;
        case JVM_CONSTANT_MethodType:
            read_const_method_type();
            break;
        case JVM_CONSTANT_InvokeDynamic:
            read_const_invoke_dynamic();
            break;
        default:
            fprintf(stderr, "error: tag %u not supported.\n", tag);
            assert(0);
        }
        const_pool->set(idx, cp_info);
        if (tag == JVM_CONSTANT_Long || tag == JVM_CONSTANT_Double) {
            // 8-byte constants take up two entries in the constant pool.
            idx++;
        }
    }

    return const_pool;
}

std::shared_ptr<cp_info> class_file::read_const_class()
{
    auto name_index = read_u2();

    return cp_info::make_class(name_index);
}

std::shared_ptr<cp_info> class_file::read_const_fieldref()
{
    auto class_index = read_u2();
    auto name_and_type_index = read_u2();

    return cp_info::make_fieldref(class_index, name_and_type_index);
}

std::shared_ptr<cp_info> class_file::read_const_methodref()
{
    auto class_index = read_u2();
    auto name_and_type_index = read_u2();

    return cp_info::make_methodref(class_index, name_and_type_index);
}

std::shared_ptr<cp_info> class_file::read_const_interface_methodref()
{
    auto class_index = read_u2();
    auto name_and_type_index = read_u2();

    return cp_info::make_interface_methodref(class_index, name_and_type_index);
}

std::shared_ptr<cp_info> class_file::read_const_string()
{
    auto string_index = read_u2();

    return cp_info::make_string(string_index);
}

std::shared_ptr<cp_info> class_file::read_const_integer()
{
    auto value = read_u4();
    
    return cp_info::make_integer(value);
}

std::shared_ptr<cp_info> class_file::read_const_float()
{
    auto bytes = read_u4();

    jfloat value;

    memcpy(&value, &bytes, sizeof(value));

    return cp_info::make_float(value);
}

std::shared_ptr<cp_info> class_file::read_const_long()
{
    auto bytes = read_u8();

    return cp_info::make_long(bytes);
}

std::shared_ptr<cp_info> class_file::read_const_double()
{
    auto bytes = read_u8();

    jdouble value;

    memcpy(&value, &bytes, sizeof(value));

    return cp_info::make_double(value);
}

std::shared_ptr<cp_info> class_file::read_const_name_and_type()
{
    auto name_index = read_u2();
    auto descriptor_index = read_u2();

    return cp_info::make_name_and_type(name_index, descriptor_index);
}

std::shared_ptr<cp_info> class_file::read_const_utf8()
{
    auto length = read_u2();

    auto bytes = new char[length + 1];

    for (auto i = 0; i < length; i++)
        bytes[i] = read_u1();

    bytes[length] = '\0';

    return cp_info::make_utf8_info(bytes);
}

void class_file::read_const_method_handle()
{
    /*auto reference_kind = */read_u1();
    /*auto reference_index = */read_u2();
}

void class_file::read_const_method_type()
{
    /*auto descriptor_index = */read_u2();
}

void class_file::read_const_invoke_dynamic()
{
    /*auto bootstrap_method_attr_index = */read_u2();
    /*auto name_and_type_index = */read_u2();
}

std::shared_ptr<field> class_file::read_field_info(klass* klass, constant_pool &constant_pool)
{
    auto access_flags = read_u2();
    auto name_index = read_u2();

    auto& cp_name = constant_pool.get_utf8(name_index);

    auto descriptor_index = read_u2();

    auto& cp_descriptor = constant_pool.get_utf8(descriptor_index);

    auto f = std::make_shared<field>(klass);

    f->name         = cp_name.bytes;
    f->descriptor   = cp_descriptor.bytes;
    f->access_flags = access_flags;

    auto attr_count = read_u2();

    for (auto i = 0; i < attr_count; i++) {
        auto attr = read_attr_info(constant_pool);
    }

    return f;
}

static std::shared_ptr<klass> parse_type(klass* klass, std::string descriptor, int& pos)
{
    auto ch = descriptor[pos++];
    switch (ch) {
    case 'B':
    case 'C':
    case 'D':
    case 'F':
    case 'I':
    case 'J':
    case 'S':
    case 'Z':
    case 'V': {
        return prim_sig_to_klass(ch);
    }
    case 'L': {
        auto start = pos;
        while (descriptor[pos++] != ';')
            ;;
        auto len = pos-start-1;
        assert(len > 0);
        auto name = descriptor.substr(start, len);
        return klass->load_class(name);
    }
    case '[': {
        auto elem_type = parse_type(klass, descriptor, pos);
        if (!elem_type) {
            return nullptr;
        }
        std::string class_name = "[" + elem_type->name;
        return klass->load_class(class_name);
    }
    default:
        fprintf(stderr, "%s '%c'\n", descriptor.c_str(), ch);
        assert(0);
        break;
    }
    return nullptr;
}

static void parse_method_descriptor(std::shared_ptr<method> m)
{
    int pos = 0;

    assert(m->descriptor[pos++] == '(');

    while (m->descriptor[pos] != ')') {
        auto arg_type = parse_type(m->klass, m->descriptor, pos);
        m->arg_types.emplace_back(arg_type.get());
    }
    m->args_count = m->arg_types.size();

    auto return_type = parse_type(m->klass, m->descriptor, ++pos);
    m->return_type = return_type.get();
}

std::shared_ptr<method> class_file::read_method_info(klass* klass, constant_pool &constant_pool)
{
    auto access_flags = read_u2();

    auto name_index = read_u2();

    auto& cp_name = constant_pool.get_utf8(name_index);

    auto descriptor_index = read_u2();

    auto& cp_descriptor = constant_pool.get_utf8(descriptor_index);

    auto attr_count = read_u2();

    auto m = std::make_shared<method>();

    m->klass        = klass;
    m->access_flags = access_flags;
    m->name         = cp_name.bytes;
    m->descriptor   = cp_descriptor.bytes;
    m->code         = nullptr;
    m->code_length  = 0;

    parse_method_descriptor(m);

    for (auto i = 0; i < attr_count; i++) {
        auto attr = read_attr_info(constant_pool);

        switch (attr->type) {
        case attr_type::code: {
            code_attr* c = static_cast<code_attr*>(attr.get());
            m->max_locals  = c->max_locals;
            m->code        = c->code;
            m->code_length = c->code_length;
            break;
        }
        default:
            /* ignore */
            break;
        }
    }

    return m;
}

std::unique_ptr<attr_info>
class_file::read_attr_info(constant_pool& constant_pool)
{
    auto attribute_name_index = read_u2();

    auto attribute_length = read_u4();

    auto& cp_name = constant_pool.get_utf8(attribute_name_index);

    if (!strcmp(cp_name.bytes, "Code")) {
        return read_code_attribute(constant_pool);
    }

    for (uint32_t i = 0; i < attribute_length; i++)
        read_u1();

    return std::unique_ptr<attr_info>(new unknown_attr);
}

std::unique_ptr<code_attr>
class_file::read_code_attribute(constant_pool& constant_pool)
{
    auto* attr = new code_attr();
    /*auto max_stack = */read_u2();
    attr->max_locals = read_u2();
    attr->code_length = read_u4();
    attr->code = new char[attr->code_length];
    for (uint32_t i = 0; i < attr->code_length; i++)
        attr->code[i] = read_u1();
    auto exception_table_length = read_u2();
    for (uint16_t i = 0; i < exception_table_length; i++) {
        /*auto start_pc = */read_u2();
        /*auto end_pc = */read_u2();
        /*auto handler_pc = */read_u2();
        /*auto catch_type = */read_u2();
    }
    auto attr_count = read_u2();
    for (auto i = 0; i < attr_count; i++) {
        read_attr_info(constant_pool);
    }
    return std::unique_ptr<code_attr>(attr);
}

uint8_t class_file::read_u1()
{
    return _data[_offset++];
}

uint16_t class_file::read_u2()
{
    return static_cast<uint16_t>(read_u1()) << 8
         | static_cast<uint16_t>(read_u1());
}

uint32_t class_file::read_u4()
{
    return static_cast<uint32_t>(read_u1()) << 24
         | static_cast<uint32_t>(read_u1()) << 16
         | static_cast<uint32_t>(read_u1()) << 8
         | static_cast<uint32_t>(read_u1());
}

uint64_t class_file::read_u8()
{
    return static_cast<uint64_t>(read_u4()) << 32 | read_u4();
}

}
