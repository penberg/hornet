#include "hornet/opcode.hh"

#include <cstddef>
#include <memory>
#include <vector>

#include <jni.h>

namespace hornet {

inline uint16_t switch_align_pc(uint16_t pc)
{
    return (pc + 4) & ~0x03;
}

std::shared_ptr<tableswitch_insn> tableswitch_insn::decode(char* code, uint16_t pc)
{
    auto aligned_pc = switch_align_pc(pc);

    auto off_default = read_u4(code + aligned_pc + 0);
    auto off_low     = read_u4(code + aligned_pc + 4);
    auto off_high    = read_u4(code + aligned_pc + 8);

    auto count = off_high - off_low + 1;

    auto insn = std::make_shared<tableswitch_insn>(count, aligned_pc - pc);
    insn->off_default = off_default;
    insn->off_high    = off_high;
    insn->off_low     = off_low;

    auto offsets_start = code + aligned_pc + 3 * sizeof(uint32_t);
    for (size_t idx = 0; idx < count; idx++) {
        auto offset = idx*sizeof(uint32_t);
        insn->_offsets[idx] = read_u4(offsets_start + offset);
    }
    return insn;
}

std::shared_ptr<lookupswitch_insn> lookupswitch_insn::decode(char* code, uint16_t pc)
{
    auto aligned_pc = switch_align_pc(pc);

    auto off_default = read_u4(code + aligned_pc + 0);
    auto npairs      = read_u4(code + aligned_pc + 4);

    auto insn = std::make_shared<lookupswitch_insn>(npairs, aligned_pc - pc);
    insn->off_default = off_default;

    auto pairs_start = code + aligned_pc + 8;
    for (size_t idx = 0; idx < npairs; idx++) {
        auto offset = idx * sizeof(pair);
        insn->_pairs[idx].match  = read_u4(pairs_start + offset);
        insn->_pairs[idx].offset = read_u4(pairs_start + offset + 4);
    }
    return insn;
}

uint16_t switch_opc_len(char* code, uint16_t pc)
{
    uint8_t opc = code[pc];
    switch (opc) {
    case JVM_OPC_tableswitch: {
        auto insn = tableswitch_insn::decode(code, pc);

        return insn->length();
    }
    case JVM_OPC_lookupswitch: {
        auto insn = lookupswitch_insn::decode(code, pc);

        return insn->length();
    }
    default:
        assert(0);
    }
}

std::vector<uint16_t> switch_targets(char* code, uint16_t pc)
{
    uint8_t opc = code[pc];
    switch (opc) {
    case JVM_OPC_tableswitch: {
        auto insn = tableswitch_insn::decode(code, pc);

        return insn->targets(pc);
    }
    case JVM_OPC_lookupswitch: {
        auto insn = lookupswitch_insn::decode(code, pc);

        return insn->targets(pc);
    }
    default:
        assert(0);
    }
}

}
