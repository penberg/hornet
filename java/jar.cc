#include "hornet/java.hh"
#include "hornet/zip.hh"

#include <cassert>
#include <climits>

namespace hornet {

jar::jar(std::string filename)
    : _zip(zip_open(filename.c_str()))
{
}

jar::~jar()
{
    zip_close(_zip);
}

std::shared_ptr<klass> jar::load_class(std::string class_name)
{
    if (!_zip) {
        return nullptr;
    }
    zip_entry *entry = zip_entry_find_class(_zip, class_name.c_str());
    if (!entry) {
        return nullptr;
    }
    char *data = (char*)zip_entry_data(_zip, entry);

    auto file = class_file{data, static_cast<size_t>(entry->uncomp_size)};

    auto klass = file.parse();

    free(data);

    return klass;
}

};
