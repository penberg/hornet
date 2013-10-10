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

    auto minor_version = read_u2();

    auto major_version = read_u2();

    if (major_version != JVM_CLASSFILE_MAJOR_VERSION)
        assert(0);

    if (minor_version != JVM_CLASSFILE_MINOR_VERSION)
        assert(0);

    auto constant_pool_count = read_u2();

    assert(constant_pool_count > 0);

    constant_pool constant_pool(constant_pool_count);

    for (auto i = 0; i < constant_pool_count-1; i++) {
        auto cp_info = read_cp_info();

        constant_pool.set(i, cp_info);
    }

    /*auto access_flags = */read_u2();

    /*auto this_class = */read_u2();

    /*auto super_class = */read_u2();

    auto interfaces_count = read_u2();

    for (auto i = 0; i < interfaces_count; i++)
        read_u2();

    auto fields_count = read_u2();

    for (auto i = 0; i < fields_count; i++)
        read_field_info();

    auto methods_count = read_u2();

    auto methods = std::valarray<std::shared_ptr<method>>(methods_count);

    for (auto i = 0; i < methods_count; i++) {
        methods[i] = read_method_info(constant_pool);
    }

    auto attr_count = read_u2();

    for (auto i = 0; i < attr_count; i++) {
        read_attr_info(constant_pool);
    }

    return std::make_shared<klass>(methods);
}

std::shared_ptr<cp_info> class_file::read_cp_info()
{
    auto tag = read_u1();

    switch (tag) {
    case JVM_CONSTANT_Class:
        return read_const_class();
    case JVM_CONSTANT_Fieldref:
        read_const_fieldref();
        break;
    case JVM_CONSTANT_Methodref:
        read_const_methodref();
        break;
    case JVM_CONSTANT_InterfaceMethodref:
        assert(0);
    case JVM_CONSTANT_String:
        read_const_string();
        break;
    case JVM_CONSTANT_Integer:
        assert(0);
    case JVM_CONSTANT_Float:
        assert(0);
    case JVM_CONSTANT_Long:
        assert(0);
    case JVM_CONSTANT_Double:
        assert(0);
    case JVM_CONSTANT_NameAndType:
        read_const_name_and_type();
        break;
    case JVM_CONSTANT_Utf8:
        return read_const_utf8();
    case JVM_CONSTANT_MethodHandle:
        assert(0);
    case JVM_CONSTANT_MethodType:
        assert(0);
    case JVM_CONSTANT_InvokeDynamic:
        assert(0);
    default:
        fprintf(stderr, "error: tag %u not supported.\n", tag);
        assert(0);
    }

    return std::shared_ptr<cp_info>(nullptr);
}

std::shared_ptr<const_class_info> class_file::read_const_class()
{
    auto *ret = new const_class_info;

    ret->name_index = read_u2();

    return std::shared_ptr<const_class_info>(ret);
}

void class_file::read_const_fieldref()
{
    /*auto class_index = */read_u2();
    /*auto name_and_type_index = */read_u2();
}

void class_file::read_const_methodref()
{
    /*auto class_index = */read_u2();
    /*auto name_and_type_index = */read_u2();
}

void class_file::read_const_string()
{
    /*auto string_index = */read_u2();
}

void class_file::read_const_name_and_type()
{
    /*auto name_index = */read_u2();
    /*auto descriptor_index = */read_u2();
}

std::shared_ptr<const_utf8_info> class_file::read_const_utf8()
{
    auto length = read_u2();

    auto ret = new const_utf8_info();

    ret->bytes = new char[length + 1];

    for (auto i = 0; i < length; i++)
        ret->bytes[i] = read_u1();

    ret->bytes[length] = '\0';

    return std::shared_ptr<const_utf8_info>(ret);
}

std::shared_ptr<field> class_file::read_field_info()
{
    assert(0);
}

std::shared_ptr<method> class_file::read_method_info(constant_pool &constant_pool)
{
    /*auto access_flags = */read_u2();

    auto name_index = read_u2();

    auto *cp_name = constant_pool.get_utf8(name_index);

    assert(cp_name != nullptr);

    auto descriptor_index = read_u2();

    auto *cp_descriptor = constant_pool.get_utf8(descriptor_index);

    assert(cp_descriptor != nullptr);

    auto attr_count = read_u2();

    auto m = std::make_shared<method>();

    m->name        = cp_name->bytes;
    m->descriptor  = cp_descriptor->bytes;
    m->code        = nullptr;
    m->code_length = 0;

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

    auto cp_name = constant_pool.get_utf8(attribute_name_index);

    if (!strcmp(cp_name->bytes, "Code")) {
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
