#include "hornet/java.hh"
#include "hornet/zip.hh"

#include <cassert>
#include <climits>

namespace hornet {

jar::jar(std::string filename)
    : _filename{filename}
    , _zip{zip_open(filename.c_str())}
{
    if (verbose_class) {
        printf("[Opened %s]\n", _filename.c_str());
    }
}

jar::~jar()
{
    if (verbose_class) {
        printf("[Closed %s]\n", _filename.c_str());
    }
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

    if (verbose_class && klass) {
        printf("[Loaded %s from %s]\n", class_name.c_str(), _filename.c_str());
    }

    return klass;
}

};
