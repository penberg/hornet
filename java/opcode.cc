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

class tableswitch_insn {
public:
    int32_t off_default;
    int32_t off_high;
    int32_t off_low;

    tableswitch_insn(size_t count, size_t padding)
        : _offsets(count)
        , _padding(padding)
    { }

    static std::shared_ptr<tableswitch_insn> decode(char* code, uint16_t pc);

    size_t length() const {
        return _padding + (3 + _offsets.size()) * sizeof(uint32_t);
    }

    std::vector<uint16_t> targets(uint16_t pc) {
        auto result = std::vector<uint16_t>();

        result.push_back(pc + off_default);

        for (uint16_t idx = 0; idx < _offsets.size(); idx++) {
            result.push_back(pc + _offsets[idx]);
        }
        return result;
    }

private:
    std::vector<int32_t> _offsets;
    size_t _padding;
};

struct lookupswitch_insn {
public:
    struct pair {
        int32_t match;
        int32_t offset;
    };

    int32_t off_default;

    lookupswitch_insn(size_t count, size_t padding)
        : _pairs(count)
        , _padding(padding)
    { }

    static std::shared_ptr<lookupswitch_insn> decode(char* code, uint16_t pc);

    size_t length() const {
        return _padding + 8 + _pairs.size() * sizeof(uint64_t);
    }

    std::vector<uint16_t> targets(uint16_t pc) {
        auto result = std::vector<uint16_t>();

        result.push_back(pc + off_default);

        for (uint16_t idx = 0; idx < _pairs.size(); idx++) {
            result.push_back(pc + _pairs[idx].offset);
        }
        return result;
    }

private:
    std::vector<pair> _pairs;
    size_t _padding;
};

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
