#include "hornet/java.hh"

#include "hornet/system_error.hh"
#include "hornet/vm.hh"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cassert>
#include <climits>
#include <cstring>
#include <fcntl.h>
#include <cstdio>

namespace hornet {

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

std::shared_ptr<klass> loader::load_class(const char *class_name)
{
    auto klass = try_to_load_class(class_name);

    if (!klass) {
        hornet::throw_exception(java_lang_NoClassDefFoundError);
        return nullptr;
    }

    if (!klass->verify()) {
        throw_exception(java_lang_VerifyError);
        return nullptr;
    }

    hornet::_jvm->register_klass(klass);

    return klass;
}

std::shared_ptr<klass> loader::try_to_load_class(const char *class_name)
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

    return load_file(pathname);
}

std::shared_ptr<klass> classpath_dir::load_file(const char *pathname)
{
    auto fd = open(pathname, O_RDONLY);
    if (fd < 0) {
        return nullptr;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        THROW_ERRNO("fstat");
    }

    auto data = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        THROW_ERRNO("mmap");
    }

    auto file = class_file{data, static_cast<size_t>(st.st_size)};

    auto klass = file.parse();

    if (munmap(data, st.st_size) < 0) {
        THROW_ERRNO("munmap");
    }

    if (close(fd) < 0) {
        THROW_ERRNO("close");
    }

    return klass;
}

}
