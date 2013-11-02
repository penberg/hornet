#include "hornet/java.hh"

#include "hornet/vm.hh"

#include <classfile_constants.h>
#include <cassert>
#include <stack>

namespace hornet {

static inline uint16_t read_opc_u2(char *p)
{
    return p[1] << 8 | p[2];
}

typedef uint64_t value_t;

value_t object_to_value(object* obj)
{
    return reinterpret_cast<value_t>(obj);
}

value_t int32_to_value(int32_t n)
{
    return static_cast<value_t>(n);
}

void interp(method* method)
{
    std::valarray<value_t> locals(method->max_locals);

    std::stack<value_t> ostack;

    uint16_t pc = 0;

next_insn:
    assert(pc < method->code_length);

    uint8_t opc = method->code[pc];

    switch (opc) {
    case JVM_OPC_iconst_m1:
    case JVM_OPC_iconst_0:
    case JVM_OPC_iconst_1:
    case JVM_OPC_iconst_2:
    case JVM_OPC_iconst_3:
    case JVM_OPC_iconst_4:
    case JVM_OPC_iconst_5: {
        int32_t value = opc - JVM_OPC_iconst_0;
        ostack.push(int32_to_value(value));
        break;
    }
    case JVM_OPC_iload_0:
    case JVM_OPC_iload_1:
    case JVM_OPC_iload_2:
    case JVM_OPC_iload_3: {
        uint16_t idx = opc - JVM_OPC_iload_0;
        ostack.push(locals[idx]);
        break;
    }
    case JVM_OPC_aload_0:
    case JVM_OPC_aload_1:
    case JVM_OPC_aload_2:
    case JVM_OPC_aload_3: {
        uint16_t idx = opc - JVM_OPC_aload_0;
        ostack.push(locals[idx]);
        break;
    }
    case JVM_OPC_istore_0:
    case JVM_OPC_istore_1:
    case JVM_OPC_istore_2:
    case JVM_OPC_istore_3: {
        uint16_t idx = opc - JVM_OPC_istore_0;
        locals[idx] = ostack.top();
        ostack.pop();
        break;
    }
    case JVM_OPC_pop: {
        ostack.pop();
        break;
    }
    case JVM_OPC_dup: {
        auto value = ostack.top();
        ostack.push(value);
        break;
    }
    case JVM_OPC_goto: {
        int16_t offset = read_opc_u2(method->code + pc);
        pc += offset;
        goto next_insn;
    }
    case JVM_OPC_return: {
        ostack.empty();
        return;
    }
    case JVM_OPC_invokespecial: {
        break;
    }
    case JVM_OPC_new: {
        auto obj = gc_new_object(nullptr);
        ostack.push(object_to_value(obj));
        break;
    }
    default:
        fprintf(stderr, "error: unsupported bytecode: %u\n", opc);
        abort();
    }

    pc += opcode_length[opc];

    goto next_insn;
}

}
