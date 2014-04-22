#include "hornet/java.hh"
#include "hornet/vm.hh"

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

cp_info *constant_pool::get(uint16_t idx)
{
    assert(idx < _entries.size());

    auto entry = _entries[idx - 1];

    return entry.get();
}

cp_info *constant_pool::get_class(uint16_t idx)
{
    assert(idx < _entries.size());

    auto entry = _entries[idx - 1];

    assert(entry->tag == cp_tag::const_class);

    return reinterpret_cast<cp_info*>(entry.get());
}

cp_info *constant_pool::get_methodref(uint16_t idx)
{
    assert(idx < _entries.size());

    auto entry = _entries[idx - 1];

    assert(entry->tag == cp_tag::const_methodref);

    return reinterpret_cast<cp_info*>(entry.get());
}

jint constant_pool::get_integer(uint16_t idx)
{
    assert(idx < _entries.size());

    auto entry = _entries[idx - 1];

    assert(entry->tag == cp_tag::const_integer);

    auto integer = reinterpret_cast<cp_info*>(entry.get());

    return integer->value;
}

cp_info *constant_pool::get_name_and_type(uint16_t idx)
{
    assert(idx < _entries.size());

    auto entry = _entries[idx - 1];

    assert(entry->tag == cp_tag::const_name_and_type);

    return reinterpret_cast<cp_info*>(entry.get());
}

const_utf8_info *constant_pool::get_utf8(uint16_t idx)
{
    assert(idx < _entries.size());

    auto entry = _entries[idx - 1];

    assert(entry->tag == cp_tag::const_utf8);

    return reinterpret_cast<const_utf8_info*>(entry.get());
}

}
