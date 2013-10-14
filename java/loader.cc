#include "hornet/java.hh"

#include "hornet/system_error.hh"
#include "hornet/vm.hh"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cassert>
#include <climits>
#include <fcntl.h>
#include <cstdio>

namespace hornet {

void loader::register_jar(jar jar)
{
    _jars.push_back(jar);
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
    char pathname[PATH_MAX];

    snprintf(pathname, sizeof(pathname), "%s.class", class_name);

    auto klass = load_file(pathname);
    if (klass)
        return klass;

    for (auto jar : _jars) {
        auto klass = jar.load_class(class_name);
        if (klass) {
            return klass;
        }
    }

    return nullptr;
}

std::shared_ptr<klass> loader::load_file(const char *pathname)
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
