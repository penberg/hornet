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

cp_info *constant_pool::get_fieldref(uint16_t idx)
{
    assert(idx < _entries.size());

    auto entry = _entries[idx - 1];

    assert(entry->tag == cp_tag::const_fieldref);

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

    auto value = reinterpret_cast<cp_info*>(entry.get());

    return value->int_value;
}

jlong constant_pool::get_long(uint16_t idx)
{
    assert(idx < _entries.size());

    auto entry = _entries[idx - 1];

    assert(entry->tag == cp_tag::const_long);

    auto value = reinterpret_cast<cp_info*>(entry.get());

    return value->long_value;
}

jdouble constant_pool::get_double(uint16_t idx)
{
    assert(idx < _entries.size());

    auto entry = _entries[idx - 1];

    assert(entry->tag == cp_tag::const_double);

    auto value = reinterpret_cast<cp_info*>(entry.get());

    return value->double_value;
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

std::mutex _intern_mutex;
std::unordered_map<std::string, std::shared_ptr<string>> _intern;

string* intern_string(std::string str)
{
    std::lock_guard<std::mutex> lock(_intern_mutex);
    auto it = _intern.find(str);
    if (it == _intern.end()) {
        auto intern = std::make_shared<string>(str.c_str());
        _intern.insert({str, intern});
        return intern.get();
    }
    return it->second.get();
}

string *constant_pool::get_string(uint16_t idx)
{
    assert(idx < _entries.size());

    auto entry = _entries[idx - 1];

    assert(entry->tag == cp_tag::const_string);

    auto utf8 = get_utf8(entry->string_index);

    return intern_string(std::string(utf8->bytes));
}

}
