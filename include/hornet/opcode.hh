#ifndef HORNET_OPCODE_HH
#define HORNET_OPCODE_HH

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <classfile_constants.h>

namespace hornet {

extern unsigned char opcode_length[];

inline uint8_t read_opc_u1(char *p)
{
    return p[1];
}

inline uint16_t read_opc_u2(char *p)
{
    return read_opc_u1(p) << 8 | read_opc_u1(p+1);
}

inline uint32_t read_opc_u4(char *p)
{
    return static_cast<uint32_t>(read_opc_u1(p+0)) << 24
         | static_cast<uint32_t>(read_opc_u1(p+1)) << 16
         | static_cast<uint32_t>(read_opc_u1(p+2)) << 8
         | static_cast<uint32_t>(read_opc_u1(p+3));
}

inline uint8_t read_u1(char *p)
{
    return *p;
}

inline uint32_t read_u4(char *p)
{
    return static_cast<uint32_t>(read_u1(p+0)) << 24
         | static_cast<uint32_t>(read_u1(p+1)) << 16
         | static_cast<uint32_t>(read_u1(p+2)) << 8
         | static_cast<uint32_t>(read_u1(p+3));
}

inline bool is_branch(uint8_t opc)
{
    switch (opc) {
    case JVM_OPC_goto:
    case JVM_OPC_goto_w:
    case JVM_OPC_ifeq:
    case JVM_OPC_ifge:
    case JVM_OPC_ifgt:
    case JVM_OPC_ifle:
    case JVM_OPC_iflt:
    case JVM_OPC_ifne:
    case JVM_OPC_ifnonnull:
    case JVM_OPC_ifnull:
    case JVM_OPC_if_acmpeq:
    case JVM_OPC_if_acmpne:
    case JVM_OPC_if_icmpeq:
    case JVM_OPC_if_icmpge:
    case JVM_OPC_if_icmpgt:
    case JVM_OPC_if_icmple:
    case JVM_OPC_if_icmplt:
    case JVM_OPC_if_icmpne:
    case JVM_OPC_jsr:
    case JVM_OPC_jsr_w:
    case JVM_OPC_lookupswitch:
    case JVM_OPC_tableswitch:
        return true;
    default:
        return false;
    }
}

inline bool is_switch(uint8_t opc)
{
    switch (opc) {
    case JVM_OPC_lookupswitch:
    case JVM_OPC_tableswitch:
        return true;
    default:
        return false;
    }
}

inline bool is_return(uint8_t opc)
{
    switch (opc) {
    case JVM_OPC_ireturn:
    case JVM_OPC_lreturn:
    case JVM_OPC_freturn:
    case JVM_OPC_dreturn:
    case JVM_OPC_areturn:
    case JVM_OPC_return:
        return true;
    default:
        return false;
    }
}

inline bool is_throw(uint8_t opc)
{
    switch (opc) {
    case JVM_OPC_athrow:
        return true;
    default:
        return false;
    }
}

inline bool is_bblock_end(uint8_t opc)
{
    return is_branch(opc) || is_switch(opc) || is_return(opc) || is_throw(opc);
}

inline uint16_t branch_target(char* code, uint16_t pos)
{
    uint8_t opc = code[pos];
    switch (opc) {
    case JVM_OPC_goto:
    case JVM_OPC_goto_w:
    case JVM_OPC_ifeq:
    case JVM_OPC_ifge:
    case JVM_OPC_ifgt:
    case JVM_OPC_ifle:
    case JVM_OPC_iflt:
    case JVM_OPC_ifne:
    case JVM_OPC_ifnonnull:
    case JVM_OPC_ifnull:
    case JVM_OPC_if_acmpeq:
    case JVM_OPC_if_acmpne:
    case JVM_OPC_if_icmpeq:
    case JVM_OPC_if_icmpge:
    case JVM_OPC_if_icmpgt:
    case JVM_OPC_if_icmple:
    case JVM_OPC_if_icmplt:
    case JVM_OPC_if_icmpne: {
        int16_t offset = read_opc_u2(code + pos);
        return pos + offset;
    }
    default:
        assert(0);
    }
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

uint16_t switch_opc_len(char* code, uint16_t pc);
std::vector<uint16_t> switch_targets(char* code, uint16_t pc);

}

#endif
