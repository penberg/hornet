#include "hornet/ffi.hh"

#include "hornet/vm.hh"

#include <cassert>
#include <dlfcn.h>
#include <string>

namespace hornet {

static void* ffi_handle;

std::string ffi_path(const std::string& path)
{
    return std::string(std::getenv("JAVA_HOME")) + path;
}

void ffi_java_init()
{
    auto libjvm = dlopen(ffi_path("/jre/lib/amd64/server/libjvm.so").c_str(), RTLD_LAZY);
    if (!libjvm) {
        throw std::runtime_error("unable to load libjvm.so: " + std::string(dlerror()));
    }
    auto libjava = dlopen(ffi_path("/jre/lib/amd64/libjava.so").c_str(), RTLD_LAZY);
    if (!libjava) {
        throw std::runtime_error("unable to load libjava.so: " + std::string(dlerror()));
    }
    ffi_handle = libjava;
}

void* ffi_java_sym(method* m)
{
    return dlsym(ffi_handle, m->jni_name().c_str());
}

ffi_type* klass_to_ffi_type(klass* klass)
{
    switch (klass->get_type()) {
    case type::t_boolean: return &ffi_type_sint8;
    case type::t_byte:    return &ffi_type_sint8;
    case type::t_char:    return &ffi_type_sint16;
    case type::t_short:   return &ffi_type_sint16;
    case type::t_int:     return &ffi_type_sint32;
    case type::t_long:    return &ffi_type_sint64;
    case type::t_float:   return &ffi_type_float;
    case type::t_double:  return &ffi_type_double;
    case type::t_ref:     return &ffi_type_pointer;
    case type::t_void:    return &ffi_type_void;
    default:              throw std::invalid_argument("invalid class type: " + klass->name);
    }
}

}
