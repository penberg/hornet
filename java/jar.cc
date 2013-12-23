#include "hornet/java.hh"
#include "hornet/zip.hh"

#include <cassert>
#include <climits>

namespace hornet {

jar::jar(std::string filename)
    : _filename(filename)
    , _zip(zip_open(_filename.c_str()))
{
}

jar::~jar()
{
    zip_close(_zip);
}

std::shared_ptr<klass> jar::load_class(std::string class_name)
{
    zip_entry *entry = zip_entry_find_class(_zip, class_name.c_str());
    assert(entry != nullptr);

    char *data = (char*)zip_entry_data(_zip, entry);

    auto file = class_file{data, static_cast<size_t>(entry->uncomp_size)};

    auto klass = file.parse();

    free(data);

    return klass;
}

};
