#include "hornet/translator.hh"

#include "hornet/java.hh"
#include "hornet/vm.hh"

#include <classfile_constants.h>
#include <jni.h>

#include <cstdio>

using namespace std;

namespace hornet {

void translator::translate()
{
    prologue();

    uint16_t pc = 0;

next_insn:
    if (pc >= _method->code_length) {
        return;
    }

    uint8_t opc = _method->code[pc];

    switch (opc) {
    case JVM_OPC_nop:
        break;
    case JVM_OPC_aconst_null:
        op_const(type::t_ref, 0);
        break;
    case JVM_OPC_iconst_m1:
    case JVM_OPC_iconst_0:
    case JVM_OPC_iconst_1:
    case JVM_OPC_iconst_2:
    case JVM_OPC_iconst_3:
    case JVM_OPC_iconst_4:
    case JVM_OPC_iconst_5: {
        jint value = opc - JVM_OPC_iconst_0;
        op_const(type::t_int, value);
        break;
    }
    case JVM_OPC_lconst_0:
    case JVM_OPC_lconst_1: {
        jint value = opc - JVM_OPC_lconst_0;
        op_const(type::t_long, value);
        break;
    }
    case JVM_OPC_bipush: {
        int8_t value = read_opc_u1(_method->code + pc);
        op_const(type::t_int, value);
        break;
    }
    case JVM_OPC_sipush: {
        int16_t value = read_opc_u2(_method->code + pc);
        op_const(type::t_int, value);
        break;
    }
    case JVM_OPC_ldc: {
        auto idx = read_opc_u1(_method->code + pc);
        auto const_pool = _method->klass->const_pool();
        auto cp_info = const_pool->get(idx);
        switch (cp_info->tag) {
        case cp_tag::const_integer: {
            auto value = const_pool->get_integer(idx);
            op_const(type::t_int, value);
            break;
        }
        default:
            assert(0);
            break;
        }
        break;
    }
    case JVM_OPC_iload: {
        auto idx = read_opc_u1(_method->code + pc);
        op_load(type::t_int, idx);
        break;
    }
    case JVM_OPC_lload: {
        auto idx = read_opc_u1(_method->code + pc);
        op_load(type::t_long, idx);
        break;
    }
    case JVM_OPC_aload: {
        auto idx = read_opc_u1(_method->code + pc);
        op_load(type::t_ref, idx);
        break;
    }
    case JVM_OPC_iload_0:
    case JVM_OPC_iload_1:
    case JVM_OPC_iload_2:
    case JVM_OPC_iload_3: {
        uint16_t idx = opc - JVM_OPC_iload_0;
        op_load(type::t_int, idx);
        break;
    }
    case JVM_OPC_lload_0:
    case JVM_OPC_lload_1:
    case JVM_OPC_lload_2:
    case JVM_OPC_lload_3: {
        uint16_t idx = opc - JVM_OPC_lload_0;
        op_load(type::t_long, idx);
        break;
    }
    case JVM_OPC_aload_0:
    case JVM_OPC_aload_1:
    case JVM_OPC_aload_2:
    case JVM_OPC_aload_3: {
        uint16_t idx = opc - JVM_OPC_aload_0;
        op_load(type::t_ref, idx);
        break;
    }
    case JVM_OPC_istore: {
        auto idx = read_opc_u1(_method->code + pc);
        op_store(type::t_int, idx);
        break;
    }
    case JVM_OPC_lstore: {
        auto idx = read_opc_u1(_method->code + pc);
        op_store(type::t_long, idx);
        break;
    }
    case JVM_OPC_astore: {
        auto idx = read_opc_u1(_method->code + pc);
        op_store(type::t_ref, idx);
        break;
    }
    case JVM_OPC_istore_0:
    case JVM_OPC_istore_1:
    case JVM_OPC_istore_2:
    case JVM_OPC_istore_3: {
        uint16_t idx = opc - JVM_OPC_istore_0;
        op_store(type::t_int, idx);
        break;
    }
    case JVM_OPC_lstore_0:
    case JVM_OPC_lstore_1:
    case JVM_OPC_lstore_2:
    case JVM_OPC_lstore_3: {
        uint16_t idx = opc - JVM_OPC_lstore_0;
        op_store(type::t_long, idx);
        break;
    }
    case JVM_OPC_astore_0:
    case JVM_OPC_astore_1:
    case JVM_OPC_astore_2:
    case JVM_OPC_astore_3: {
        uint16_t idx = opc - JVM_OPC_astore_0;
        op_store(type::t_ref, idx);
        break;
    }
    case JVM_OPC_pop: {
        op_pop();
        break;
    }
    case JVM_OPC_dup: {
        op_dup();
        break;
    }
    case JVM_OPC_dup_x1: {
        op_dup_x1();
        break;
    }
    case JVM_OPC_swap: {
        op_swap();
        break;
    }
    case JVM_OPC_iadd: {
        op_binary(type::t_int, binop::op_add);
        break;
    }
    case JVM_OPC_ladd: {
        op_binary(type::t_long, binop::op_add);
        break;
    }
    case JVM_OPC_isub: {
        op_binary(type::t_int, binop::op_sub);
        break;
    }
    case JVM_OPC_lsub: {
        op_binary(type::t_long, binop::op_sub);
        break;
    }
    case JVM_OPC_imul: {
        op_binary(type::t_int, binop::op_mul);
        break;
    }
    case JVM_OPC_lmul: {
        op_binary(type::t_long, binop::op_mul);
        break;
    }
    case JVM_OPC_idiv: {
        op_binary(type::t_int, binop::op_div);
        break;
    }
    case JVM_OPC_ldiv: {
        op_binary(type::t_long, binop::op_div);
        break;
    }
    case JVM_OPC_irem: {
        op_binary(type::t_int, binop::op_rem);
        break;
    }
    case JVM_OPC_lrem: {
        op_binary(type::t_long, binop::op_rem);
        break;
    }
    case JVM_OPC_iand: {
        op_binary(type::t_int, binop::op_and);
        break;
    }
    case JVM_OPC_land: {
        op_binary(type::t_long, binop::op_and);
        break;
    }
    case JVM_OPC_ior: {
        op_binary(type::t_int, binop::op_or);
        break;
    }
    case JVM_OPC_lor: {
        op_binary(type::t_long, binop::op_or);
        break;
    }
    case JVM_OPC_ixor: {
        op_binary(type::t_int, binop::op_xor);
        break;
    }
    case JVM_OPC_lxor: {
        op_binary(type::t_long, binop::op_xor);
        break;
    }
    case JVM_OPC_iinc: {
        auto idx   = read_opc_u1(_method->code + pc);
        auto value = read_opc_u1(_method->code + pc + 1);
        op_iinc(idx, value);
        break;
    }
    case JVM_OPC_if_icmpeq: {
        int16_t offset = read_opc_u2(_method->code + pc);
        op_if_cmp(type::t_int, cmpop::op_cmpeq, offset);
        break;
    }
    case JVM_OPC_if_icmpne: {
        int16_t offset = read_opc_u2(_method->code + pc);
        op_if_cmp(type::t_int, cmpop::op_cmpne, offset);
        break;
    }
    case JVM_OPC_if_icmplt: {
        int16_t offset = read_opc_u2(_method->code + pc);
        op_if_cmp(type::t_int, cmpop::op_cmplt, offset);
        break;
    }
    case JVM_OPC_if_icmpge: {
        int16_t offset = read_opc_u2(_method->code + pc);
        op_if_cmp(type::t_int, cmpop::op_cmpge, offset);
        break;
    }
    case JVM_OPC_if_icmpgt: {
        int16_t offset = read_opc_u2(_method->code + pc);
        op_if_cmp(type::t_int, cmpop::op_cmpgt, offset);
        break;
    }
    case JVM_OPC_if_icmple: {
        int16_t offset = read_opc_u2(_method->code + pc);
        op_if_cmp(type::t_int, cmpop::op_cmple, offset);
        break;
    }
    case JVM_OPC_goto: {
        int16_t offset = read_opc_u2(_method->code + pc);
        op_goto(offset);
        break;
    }
    case JVM_OPC_ireturn:
    case JVM_OPC_lreturn:
    case JVM_OPC_freturn:
    case JVM_OPC_dreturn:
    case JVM_OPC_areturn: {
        op_ret();
        break;
    }
    case JVM_OPC_invokespecial: {
        uint16_t idx = read_opc_u2(_method->code + pc);
        auto target = _method->klass->resolve_method(idx);
        if (_method->klass->access_flags & JVM_ACC_SUPER
            && target->klass->is_subclass_of(_method->klass->super)
            && target->is_init()) {
           target = target->klass->super->lookup_method(target->name, target->descriptor);
        }
        assert(target != nullptr);
        op_invokestatic(target.get());
        break;
    }
    case JVM_OPC_invokestatic: {
        uint16_t idx = read_opc_u2(_method->code + pc);
        auto target = _method->klass->resolve_method(idx);
        assert(target != nullptr);
        assert(target->access_flags & JVM_ACC_STATIC);
        op_invokestatic(target.get());
        break;
    }
    case JVM_OPC_return: {
        op_ret_void();
        break;
    }
    case JVM_OPC_new: {
        op_new();
        break;
    }
    case JVM_OPC_arraylength: {
        op_arraylength();
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
