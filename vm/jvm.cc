#include <hornet/vm.hh>

#include "hornet/java.hh"

#include <classfile_constants.h>
#include <cassert>
#include <stack>

namespace hornet {

jvm *_jvm;

void jvm::register_klass(std::shared_ptr<klass> klass)
{
    std::lock_guard<std::mutex> lock(_mutex);

    _klasses.push_back(klass);
}

void jvm::invoke(method* method)
{
    std::valarray<object*> locals(method->max_locals);

    std::stack<object*> ostack;

    uint16_t pc = 0;

    for (;;) {
        assert(pc < method->code_length);

        uint8_t opc = method->code[pc];

        switch (opc) {
        case JVM_OPC_aload_0:
        case JVM_OPC_aload_1:
        case JVM_OPC_aload_2:
        case JVM_OPC_aload_3: {
            uint16_t idx = opc - JVM_OPC_aload_0;
            ostack.push(locals[idx]);
            break;
        }
        case JVM_OPC_return: {
            ostack.empty();
            return;
        }
        default:
            fprintf(stderr, "unsupported bytecode: %u\n", opc);
            assert(0);
        }

        pc += opcode_length[opc];
    }
}

}
