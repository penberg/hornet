#include "hornet/java.hh"

#include "hornet/system_error.hh"
#include "hornet/java.hh"
#include "hornet/zip.hh"
#include "hornet/vm.hh"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <climits>
#include <cstring>
#include <fcntl.h>
#include <cstdio>

namespace hornet {

bool verbose_class;

void loader::register_entry(std::string path)
{
    struct stat st;

    if (stat(path.c_str(), &st) < 0) {
        fprintf(stderr, "warning: %s: %s\n", path.c_str(), strerror(errno));
        return;
    }
    if (S_ISDIR(st.st_mode)) {
        _entries.push_back(std::make_shared<classpath_dir>(path));
    } else {
        _entries.push_back(std::make_shared<jar>(path));
    }
}

std::shared_ptr<klass> loader::load_class(std::string class_name)
{
    std::replace(class_name.begin(), class_name.end(), '.', '/');

    if (is_array_type_name(class_name)) {
        auto elem_type_name = class_name.substr(1, std::string::npos);
        auto elem_type = load_class(elem_type_name);
        if (!elem_type) {
            hornet::throw_exception(java_lang_NoClassDefFoundError);
            return nullptr;
        }
        auto klass = std::make_shared<array_klass>(class_name, elem_type.get());
        hornet::_jvm->register_class(klass);
        return klass;
    }

    auto klass = hornet::_jvm->lookup_class(class_name);

    if (klass) {
        return klass;
    }

    klass = try_to_load_class(class_name);

    if (!klass) {
        hornet::throw_exception(java_lang_NoClassDefFoundError);
        return nullptr;
    }

    if (!klass->verify()) {
        throw_exception(java_lang_VerifyError);
        return nullptr;
    }

    return klass;
}

std::shared_ptr<klass> loader::try_to_load_class(std::string class_name)
{
    for (auto entry : _entries) {
        auto klass = entry->load_class(class_name);
        if (klass) {
            return klass;
        }
    }
    return nullptr;
}

classpath_dir::classpath_dir(std::string path)
    : _path(path)
{
}

classpath_dir::~classpath_dir()
{
}

std::shared_ptr<klass> classpath_dir::load_class(std::string class_name)
{
    char pathname[PATH_MAX];

    snprintf(pathname, sizeof(pathname), "%s/%s.class", _path.c_str(), class_name.c_str());

    auto klass = load_file(pathname);

    if (verbose_class && klass) {
        printf("[Loaded %s from file:%s]\n", class_name.c_str(), _path.c_str());
    }
    return klass;
}

std::shared_ptr<klass> classpath_dir::load_file(const char *pathname)
{
    auto fd = open(pathname, O_RDONLY);
    if (fd < 0) {
        return nullptr;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        throw_system_error("fstat");
    }

    auto data = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        throw_system_error("mmap");
    }

    auto file = class_file{data, static_cast<size_t>(st.st_size)};

    auto klass = file.parse();

    if (munmap(data, st.st_size) < 0) {
        throw_system_error("munmap");
    }

    if (close(fd) < 0) {
        throw_system_error("close");
    }

    return klass;
}

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

}
