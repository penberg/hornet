#include "hornet/java.hh"
#include "hornet/vm.hh"

#include <unordered_map>
#include <cassert>

namespace hornet {

constant_pool::constant_pool(uint16_t size)
    : _entries(size)
{
}

constant_pool::~constant_pool()
{
}

void constant_pool::set(uint16_t idx, std::shared_ptr<cp_info> entry)
{
    assert(idx < _entries.size());

    _entries[idx] = entry;
}

const cp_info& constant_pool::get(uint16_t idx) const
{
    assert(idx < _entries.size());

    auto entry = _entries[idx - 1];

    return *entry.get();
}

const cp_info& constant_pool::get_class(uint16_t idx) const
{
    return get_ty<cp_info, cp_tag::const_class>(idx);
}

const cp_info& constant_pool::get_fieldref(uint16_t idx) const
{
    return get_ty<cp_info, cp_tag::const_fieldref>(idx);
}

const cp_info& constant_pool::get_methodref(uint16_t idx) const
{
    return get_ty<cp_info, cp_tag::const_methodref>(idx);
}

const cp_info& constant_pool::get_interface_methodref(uint16_t idx) const
{
    return get_ty<cp_info, cp_tag::const_interface_methodref>(idx);
}

jint constant_pool::get_integer(uint16_t idx) const
{
    auto& value = get_ty<cp_info, cp_tag::const_integer>(idx);

    return value.int_value;
}

jlong constant_pool::get_long(uint16_t idx) const
{
    auto& value = get_ty<cp_info, cp_tag::const_long>(idx);

    return value.long_value;
}

jfloat constant_pool::get_float(uint16_t idx) const
{
    auto& value = get_ty<cp_info, cp_tag::const_float>(idx);

    return value.float_value;
}

jdouble constant_pool::get_double(uint16_t idx) const
{
    auto& value = get_ty<cp_info, cp_tag::const_double>(idx);

    return value.double_value;
}

const cp_info& constant_pool::get_name_and_type(uint16_t idx) const
{
    return get_ty<cp_info, cp_tag::const_name_and_type>(idx);
}

const cp_info& constant_pool::get_utf8(uint16_t idx) const
{
    return get_ty<cp_info, cp_tag::const_utf8>(idx);
}

string *constant_pool::get_string(uint16_t idx) const
{
    auto& entry = get_ty<cp_info, cp_tag::const_string>(idx);

    auto& utf8 = get_utf8(entry.string_index);

    return hornet::_jvm->intern_string(std::string(utf8.bytes));
}

template<typename Type, cp_tag Tag>
const Type& constant_pool::get_ty(uint16_t idx) const
{
    assert(idx < _entries.size());

    auto entry = _entries[idx - 1];

    assert(entry->tag == Tag);

    return *reinterpret_cast<Type*>(entry.get());
}

}
