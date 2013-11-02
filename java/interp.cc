#include "hornet/java.hh"

#include "hornet/vm.hh"

#include <cassert>
#include <stack>

#include <classfile_constants.h>
#include <jni.h>

namespace hornet {

typedef uint64_t value_t;

value_t object_to_value(object* obj)
{
    return reinterpret_cast<value_t>(obj);
}

jint value_to_jint(value_t value)
{
    return static_cast<jint>(value);
}

value_t jint_to_value(jint n)
{
    return static_cast<value_t>(n);
}

#define BINOP_INTERP(type, op)				\
    do {						\
        auto value2 = value_to_##type(ostack.top());	\
        ostack.pop();					\
        auto value1 = value_to_##type(ostack.top());	\
        ostack.pop();					\
        type result = value1 op value2;			\
        ostack.push(type##_to_value(result));		\
   } while (0)

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
        jint value = opc - JVM_OPC_iconst_0;
        ostack.push(jint_to_value(value));
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
    case JVM_OPC_istore: {
        auto idx = read_opc_u1(method->code + pc);
        locals[idx] = ostack.top();
        ostack.pop();
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
    case JVM_OPC_iadd: {
        BINOP_INTERP(jint, +);
        break;
    }
    case JVM_OPC_isub: {
        BINOP_INTERP(jint, -);
        break;
    }
    case JVM_OPC_imul: {
        BINOP_INTERP(jint, *);
        break;
    }
    case JVM_OPC_idiv: {
        BINOP_INTERP(jint, /);
        break;
    }
    case JVM_OPC_irem: {
        BINOP_INTERP(jint, %);
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
