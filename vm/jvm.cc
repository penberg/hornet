#include <hornet/vm.hh>

#include <classfile_constants.h>
#include <cassert>

namespace hornet {

jvm *_jvm;

void jvm::register_klass(std::shared_ptr<klass> klass)
{
    std::lock_guard<std::mutex> lock(_mutex);

    _klasses.push_back(klass);
}

void jvm::invoke(method* method)
{
    uint8_t opc = method->code[0];

    switch (opc) {
    case JVM_OPC_return: {
        return;
    }
    default:
        fprintf(stderr, "unsupported bytecode: %u\n", opc);
        assert(0);
    }
}

}
