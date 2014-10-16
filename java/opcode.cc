#include "hornet/opcode.hh"

namespace hornet {

uint16_t switch_opc_len(char* code, uint16_t pc)
{
    uint16_t end;

    uint8_t opc = code[pc];
    switch (opc) {
    case JVM_OPC_tableswitch: {
        auto aligned_pc = (pc + 4) & ~0x03;
        auto low   = read_opc_u4(code + aligned_pc + 4);
        auto high  = read_opc_u4(code + aligned_pc + 8);
        auto count = high - low + 1;
        end = aligned_pc + (3 + count) * sizeof(uint32_t);
        break;
    }
    case JVM_OPC_lookupswitch: {
        auto aligned_pc = (pc + 4) & ~0x03;
        auto npairs = read_opc_u4(code + aligned_pc + 4);
        end = aligned_pc + (2 + npairs) * sizeof(uint32_t);
        break;
    }
    default:
        assert(0);
    }
    return end - pc;
}

}
