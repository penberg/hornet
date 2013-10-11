#include "hornet/java.hh"

#include <zip.h>

#include <cassert>
#include <climits>

namespace hornet {

jar::jar(std::string filename)
    : _filename(filename)
{
}

std::shared_ptr<klass> jar::load_class(std::string class_name)
{
    char pathname[PATH_MAX];

    snprintf(pathname, sizeof(pathname), "%s.class", class_name.c_str());

    int err;

    zip* zip = zip_open(_filename.c_str(), 0, &err);
    assert(zip != nullptr);

    struct zip_stat st;
    zip_stat_init(&st);
    zip_stat(zip, pathname, 0, &st);

    char *data = new char[st.size];

    zip_file *f = zip_fopen(zip, pathname, 0);
    assert(f != nullptr);

    if (zip_fread(f, data, st.size) < 0)
        assert(0);

    zip_fclose(f);

    zip_close(zip);

    auto file = class_file{data, static_cast<size_t>(st.size)};

    return file.parse();
}

};
