#include "hornet/vm.hh"
#include "hornet/java.hh"
#include "hornet/system_error.hh"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cassert>
#include <climits>
#include <fcntl.h>
#include <cstdio>

namespace hornet {

std::shared_ptr<klass> loader::load_class(const char *class_name)
{
    char pathname[PATH_MAX];

    snprintf(pathname, sizeof(pathname), "%s.class", class_name);

    return load_file(pathname);
}

std::shared_ptr<klass> loader::load_file(const char *pathname)
{
    auto fd = open(pathname, O_RDONLY);
    if (fd < 0) {
        hornet::throw_exception(java_lang_NoClassDefFoundError);
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

    if (!klass->verify()) {
        throw_exception(java_lang_VerifyError);
        return nullptr;
    }

    hornet::_jvm->register_klass(klass);

    return klass;
}

}
