#pragma once

#include "hornet/types.hh"

#include <ffi.h>

namespace hornet {

class method;
class klass;

void ffi_java_init();

void* ffi_java_sym(method* m);

ffi_type* klass_to_ffi_type(klass*);

}
