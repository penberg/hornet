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

const_utf8_info *constant_pool::get_utf8(uint16_t idx)
{
    assert(idx < _entries.size());

    auto entry = _entries[idx - 1];

    return reinterpret_cast<const_utf8_info*>(entry.get());
}

}
