#include "hornet/java.hh"

#include <hornet/vm.hh>

#include <classfile_constants.h>
#include <cassert>
#include <stack>

namespace hornet {

bool verbose_verifier;
unsigned char unsupported_opcode[JVM_OPC_MAX+1];
unsigned char opcode_length[JVM_OPC_MAX+1] = JVM_OPCODE_LENGTH_INITIALIZER;

bool verify_method(std::shared_ptr<method> method)
{
    unsigned int pc = 0;

    for (;;) {
        if (pc >= method->code_length)
            break;

        uint8_t opc = method->code[pc];

        switch (opc) {
        case JVM_OPC_aload_0:
        case JVM_OPC_aload_1:
        case JVM_OPC_aload_2:
        case JVM_OPC_aload_3: {
            uint16_t idx = opc - JVM_OPC_aload_0;
            if (idx >= method->max_locals)
                return false;
            break;
        }
        case JVM_OPC_return: {
            break;
        }
        default:
            unsupported_opcode[opc]++;
            break;
        }

        pc += opcode_length[opc];
    }

    return true;
}

void verifier_stats()
{
    if (!verbose_verifier) {
        return;
    }
    fprintf(stderr, "Unsupported opcodes:\n");
    fprintf(stderr, "    #  opcode\n");
    for (auto opc = 0; opc < JVM_OPC_MAX; opc++) {
        if (unsupported_opcode[opc]) {
            fprintf(stderr, "%5d  %d\n", unsupported_opcode[opc], opc);
        } 
    }
}

}
