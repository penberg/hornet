#include "hornet/java.hh"

#include <hornet/vm.hh>

#include <classfile_constants.h>
#include <cassert>
#include <stack>

namespace hornet {

bool verify_method(std::shared_ptr<method> method)
{
    uint16_t pc = 0;

    for (;;) {
        uint8_t opc = method->code[pc++];

        switch (opc) {
        case JVM_OPC_aload_0:
        case JVM_OPC_aload_1:
        case JVM_OPC_aload_2:
        case JVM_OPC_aload_3: {
            uint16_t idx = opc - JVM_OPC_aload_0;
            if (idx >= method->max_locals)
                return false;
        }
        case JVM_OPC_return: {
            return true;
        }
        default:
            fprintf(stderr, "error: Unsupported bytecode: %u\n", opc);
            return false;
        }
    }
}

}
