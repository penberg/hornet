#include "hornet/java.hh"

#include "hornet/opcode.hh"
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

        assert(opc < JVM_OPC_MAX);

        if (is_switch(opc)) {
            auto len = switch_opc_len(method->code, pc);
            pc += len;
            continue;
        }

        auto wide = false;

        if (opc == JVM_OPC_wide) {
            opc = method->code[++pc];

            assert(opc < JVM_OPC_MAX);

            wide = true;
        }

        switch (opc) {
        case JVM_OPC_iconst_m1:
        case JVM_OPC_iconst_0:
        case JVM_OPC_iconst_1:
        case JVM_OPC_iconst_2:
        case JVM_OPC_iconst_3:
        case JVM_OPC_iconst_4:
        case JVM_OPC_iconst_5: {
            break;
        }
        case JVM_OPC_lconst_0:
        case JVM_OPC_lconst_1: {
            break;
        }
        case JVM_OPC_fconst_0:
        case JVM_OPC_fconst_1: {
            break;
        }
        case JVM_OPC_dconst_0:
        case JVM_OPC_dconst_1: {
            break;
        }
        case JVM_OPC_iload_0:
        case JVM_OPC_iload_1:
        case JVM_OPC_iload_2:
        case JVM_OPC_iload_3: {
            uint16_t idx = opc - JVM_OPC_iload_0;
            if (idx >= method->max_locals)
                return false;
            break;
        }
        case JVM_OPC_aload_0:
        case JVM_OPC_aload_1:
        case JVM_OPC_aload_2:
        case JVM_OPC_aload_3: {
            uint16_t idx = opc - JVM_OPC_aload_0;
            if (idx >= method->max_locals)
                return false;
            break;
        }
        case JVM_OPC_istore: {
            auto idx = read_opc_u1(method->code + pc);
            if (idx >= method->max_locals)
                return false;
            break;
        }
        case JVM_OPC_lstore: {
            auto idx = read_opc_u1(method->code + pc);
            if (idx >= method->max_locals)
                return false;
            break;
        }
        case JVM_OPC_istore_0:
        case JVM_OPC_istore_1:
        case JVM_OPC_istore_2:
        case JVM_OPC_istore_3: {
            uint16_t idx = opc - JVM_OPC_istore_0;
            if (idx >= method->max_locals)
                return false;
            break;
        }
        case JVM_OPC_lstore_0:
        case JVM_OPC_lstore_1:
        case JVM_OPC_lstore_2:
        case JVM_OPC_lstore_3: {
            uint16_t idx = opc - JVM_OPC_lstore_0;
            if (idx >= method->max_locals)
                return false;
            break;
        }
        case JVM_OPC_iadd: {
            break;
        }
        case JVM_OPC_ladd: {
            break;
        }
        case JVM_OPC_isub: {
            break;
        }
        case JVM_OPC_lsub: {
            break;
        }
        case JVM_OPC_imul: {
            break;
        }
        case JVM_OPC_lmul: {
            break;
        }
        case JVM_OPC_idiv: {
            break;
        }
        case JVM_OPC_ldiv: {
            break;
        }
        case JVM_OPC_irem: {
            break;
        }
        case JVM_OPC_lrem: {
            break;
        }
        case JVM_OPC_return: {
            break;
        }
        default:
            unsupported_opcode[opc]++;
            break;
        }
        if (opcode_length[opc] <= 0) {
            fprintf(stderr, "opcode %u length is %d\n", opc, opcode_length[opc]);
            assert(0);
        }
        pc += opcode_length[opc];
        if (wide) {
            if (opc == JVM_OPC_iinc) {
                pc += 2;
            } else {
                pc += 1;
            }
        }
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
