#include "hornet/java.hh"

#include "hornet/translator.hh"
#include "hornet/vm.hh"

#include <cassert>
#include <stack>

#include <classfile_constants.h>
#include <jni.h>

namespace hornet {

template<typename T>
value_t to_value(T x)
{
    return static_cast<value_t>(x);
}

template<typename T>
value_t to_value(object* obj)
{
    return reinterpret_cast<value_t>(obj);
}

template<typename T>
T from_value(value_t value)
{
    return static_cast<T>(value);
}

template<>
array* from_value<array*>(value_t value)
{
    return reinterpret_cast<array*>(value);
}

template<typename T>
void op_const(frame& frame, T value)
{
    frame.ostack.push(to_value<T>(value));
}

void op_load(frame& frame, uint16_t idx)
{
    frame.ostack.push(frame.locals[idx]);
}

void op_store(frame& frame, uint16_t idx)
{
    frame.locals[idx] = frame.ostack.top();
    frame.ostack.pop();
}

void op_pop(frame& frame)
{
    frame.ostack.pop();
}

void op_dup(frame& frame)
{
    auto value = frame.ostack.top();
    frame.ostack.push(value);
}

void op_dup_x1(frame& frame)
{
    auto value1 = frame.ostack.top();
    frame.ostack.pop();
    auto value2 = frame.ostack.top();
    frame.ostack.pop();

    frame.ostack.push(value1);
    frame.ostack.push(value2);
    frame.ostack.push(value1);
}

void op_swap(frame& frame)
{
    auto value1 = frame.ostack.top();
    frame.ostack.pop();
    auto value2 = frame.ostack.top();
    frame.ostack.pop();
    frame.ostack.push(value1);
    frame.ostack.push(value2);
}

enum class unop {
    op_neg,
};

template<typename T>
T eval(unop op, T a)
{
    switch (op) {
    case unop::op_neg: return -a;
    default:           assert(0);
    }
}

template<typename T>
void op_unary(frame& frame, unop op)
{
    auto value = from_value<T>(frame.ostack.top());
    frame.ostack.pop();
    auto result = eval(op, value);
    frame.ostack.push(to_value<T>(result));
}

template<typename T>
T eval(binop op, T a, T b)
{
    switch (op) {
    case binop::op_add: return a + b;
    case binop::op_sub: return a - b;
    case binop::op_mul: return a * b;
    case binop::op_div: return a / b;
    case binop::op_rem: return a % b;
    case binop::op_and: return a & b;
    case binop::op_or:  return a | b;
    case binop::op_xor: return a ^ b;
    default:            assert(0);
    }
}

template<>
jfloat eval(binop op, jfloat a, jfloat b)
{
    switch (op) {
    case binop::op_add: return a + b;
    case binop::op_sub: return a - b;
    case binop::op_mul: return a * b;
    case binop::op_div: return a / b;
    default:            assert(0);
    }
}

template<>
jdouble eval(binop op, jdouble a, jdouble b)
{
    switch (op) {
    case binop::op_add: return a + b;
    case binop::op_sub: return a - b;
    case binop::op_mul: return a * b;
    case binop::op_div: return a / b;
    default:            assert(0);
    }
}

template<typename T>
void op_binary(frame& frame, binop op)
{
    auto value2 = from_value<T>(frame.ostack.top());
    frame.ostack.pop();
    auto value1 = from_value<T>(frame.ostack.top());
    frame.ostack.pop();
    auto result = eval(op, value1, value2);
    frame.ostack.push(to_value<T>(result));
}

void op_iinc(frame& frame, uint16_t idx, jint value)
{
    frame.locals[idx] += value;
}

enum class shiftop {
    op_shl,
    op_shr,
};

template<typename T>
T eval(shiftop op, T a, jint b)
{
    switch (op) {
    case shiftop::op_shl: return a << b;
    case shiftop::op_shr: return a >> b;
    default:              assert(0);
    }
}

template<typename T>
void op_shift(frame& frame, shiftop op, jint mask)
{
    auto value2 = from_value<jint>(frame.ostack.top());
    frame.ostack.pop();
    auto value1 = from_value<T>(frame.ostack.top());
    frame.ostack.pop();
    auto result = eval(op, value1, value2 & mask);
    frame.ostack.push(to_value<T>(result));
}

enum class cmpop {
    op_cmpeq,
    op_cmpne,
    op_cmplt,
    op_cmpge,
    op_cmpgt,
    op_cmple,
};

template<typename T>
bool eval(cmpop op, T a, T b)
{
    switch (op) {
    case cmpop::op_cmpeq: return a == b;
    case cmpop::op_cmpne: return a != b;
    case cmpop::op_cmplt: return a < b;
    case cmpop::op_cmpge: return a >= b;
    case cmpop::op_cmpgt: return a > b;
    case cmpop::op_cmple: return a <= b;
    default:              assert(0);
    }
}

template<typename T>
void op_if_cmp(method* method, frame& frame, cmpop op, int16_t offset)
{
    uint8_t opc = method->code[frame.pc];
    auto value2 = from_value<T>(frame.ostack.top());
    frame.ostack.pop();
    auto value1 = from_value<T>(frame.ostack.top());
    frame.ostack.pop();
    if (eval(op, value1, value2)) {
        frame.pc += offset;
    } else {
        frame.pc += opcode_length[opc];
    }
}

void op_goto(frame& frame, int16_t offset)
{
    frame.pc += offset;
}

void op_getstatic(method* method, frame& frame, uint16_t idx)
{
    auto field = method->klass->resolve_field(idx);
    assert(field != nullptr);
    frame.ostack.push(field->value);
}

void op_new(frame& frame)
{
    auto obj = gc_new_object(nullptr);
    frame.ostack.push(to_value<object*>(obj));
}

void op_arraylength(frame& frame)
{
    auto* arrayref = from_value<array*>(frame.ostack.top());
    frame.ostack.pop();
    assert(arrayref != nullptr);
    frame.ostack.push(arrayref->length);
}

value_t interp_backend::execute(method* method, frame& frame)
{
next_insn:
    assert(frame.pc < method->code_length);

    uint8_t opc = method->code[frame.pc];

    switch (opc) {
    case JVM_OPC_nop:
        break;
    case JVM_OPC_aconst_null:
        op_const<object*>(frame, nullptr);
        break;
    case JVM_OPC_iconst_m1:
    case JVM_OPC_iconst_0:
    case JVM_OPC_iconst_1:
    case JVM_OPC_iconst_2:
    case JVM_OPC_iconst_3:
    case JVM_OPC_iconst_4:
    case JVM_OPC_iconst_5: {
        jint value = opc - JVM_OPC_iconst_0;
        op_const(frame, value);
        break;
    }
    case JVM_OPC_lconst_0:
    case JVM_OPC_lconst_1: {
        jlong value = opc - JVM_OPC_lconst_0;
        op_const(frame, value);
        break;
    }
    case JVM_OPC_fconst_0:
    case JVM_OPC_fconst_1: {
        jfloat value = opc - JVM_OPC_fconst_0;
        op_const(frame, value);
        break;
    }
    case JVM_OPC_dconst_0:
    case JVM_OPC_dconst_1: {
        jdouble value = opc - JVM_OPC_dconst_0;
        op_const(frame, value);
        break;
    }
    case JVM_OPC_bipush: {
        int8_t value = read_opc_u1(method->code + frame.pc);
        op_const<jint>(frame, value);
        break;
    }
    case JVM_OPC_sipush: {
        int16_t value = read_opc_u2(method->code + frame.pc);
        op_const<jint>(frame, value);
        break;
    }
    case JVM_OPC_ldc: {
        auto idx = read_opc_u1(method->code + frame.pc);
        auto const_pool = method->klass->const_pool();
        auto cp_info = const_pool->get(idx);
        switch (cp_info->tag) {
        case cp_tag::const_integer: {
            auto value = const_pool->get_integer(idx);
            op_const<jint>(frame, value);
            break;
        }
        default:
            assert(0);
            break;
        }
        break;
    }
    case JVM_OPC_iload: {
        auto idx = read_opc_u1(method->code + frame.pc);
        op_load(frame, idx);
        break;
    }
    case JVM_OPC_lload: {
        auto idx = read_opc_u1(method->code + frame.pc);
        op_load(frame, idx);
        break;
    }
    case JVM_OPC_aload: {
        auto idx = read_opc_u1(method->code + frame.pc);
        op_load(frame, idx);
        break;
    }
    case JVM_OPC_iload_0:
    case JVM_OPC_iload_1:
    case JVM_OPC_iload_2:
    case JVM_OPC_iload_3: {
        uint16_t idx = opc - JVM_OPC_iload_0;
        op_load(frame, idx);
        break;
    }
    case JVM_OPC_lload_0:
    case JVM_OPC_lload_1:
    case JVM_OPC_lload_2:
    case JVM_OPC_lload_3: {
        uint16_t idx = opc - JVM_OPC_lload_0;
        op_load(frame, idx);
        break;
    }
    case JVM_OPC_aload_0:
    case JVM_OPC_aload_1:
    case JVM_OPC_aload_2:
    case JVM_OPC_aload_3: {
        uint16_t idx = opc - JVM_OPC_aload_0;
        op_load(frame, idx);
        break;
    }
    case JVM_OPC_istore: {
        auto idx = read_opc_u1(method->code + frame.pc);
        op_store(frame, idx);
        break;
    }
    case JVM_OPC_lstore: {
        auto idx = read_opc_u1(method->code + frame.pc);
        op_store(frame, idx);
        break;
    }
    case JVM_OPC_istore_0:
    case JVM_OPC_istore_1:
    case JVM_OPC_istore_2:
    case JVM_OPC_istore_3: {
        uint16_t idx = opc - JVM_OPC_istore_0;
        op_store(frame, idx);
        break;
    }
    case JVM_OPC_lstore_0:
    case JVM_OPC_lstore_1:
    case JVM_OPC_lstore_2:
    case JVM_OPC_lstore_3: {
        uint16_t idx = opc - JVM_OPC_lstore_0;
        op_store(frame, idx);
        break;
    }
    case JVM_OPC_pop: {
        op_pop(frame);
        break;
    }
    case JVM_OPC_dup: {
        op_dup(frame);
        break;
    }
    case JVM_OPC_dup_x1: {
        op_dup_x1(frame);
        break;
    }
    case JVM_OPC_swap: {
        op_swap(frame);
        break;
    }
    case JVM_OPC_iadd: {
        op_binary<jint>(frame, binop::op_add);
        break;
    }
    case JVM_OPC_ladd: {
        op_binary<jlong>(frame, binop::op_add);
        break;
    }
    case JVM_OPC_fadd: {
        op_binary<jfloat>(frame, binop::op_add);
        break;
    }
    case JVM_OPC_dadd: {
        op_binary<jdouble>(frame, binop::op_add);
        break;
    }
    case JVM_OPC_isub: {
        op_binary<jint>(frame, binop::op_sub);
        break;
    }
    case JVM_OPC_lsub: {
        op_binary<jlong>(frame, binop::op_sub);
        break;
    }
    case JVM_OPC_fsub: {
        op_binary<jfloat>(frame, binop::op_sub);
        break;
    }
    case JVM_OPC_dsub: {
        op_binary<jdouble>(frame, binop::op_sub);
        break;
    }
    case JVM_OPC_imul: {
        op_binary<jint>(frame, binop::op_mul);
        break;
    }
    case JVM_OPC_lmul: {
        op_binary<jlong>(frame, binop::op_mul);
        break;
    }
    case JVM_OPC_fmul: {
        op_binary<jfloat>(frame, binop::op_mul);
        break;
    }
    case JVM_OPC_dmul: {
        op_binary<jdouble>(frame, binop::op_mul);
        break;
    }
    case JVM_OPC_idiv: {
        op_binary<jint>(frame, binop::op_div);
        break;
    }
    case JVM_OPC_ldiv: {
        op_binary<jlong>(frame, binop::op_div);
        break;
    }
    case JVM_OPC_fdiv: {
        op_binary<jfloat>(frame, binop::op_div);
        break;
    }
    case JVM_OPC_ddiv: {
        op_binary<jdouble>(frame, binop::op_div);
        break;
    }
    case JVM_OPC_irem: {
        op_binary<jint>(frame, binop::op_rem);
        break;
    }
    case JVM_OPC_lrem: {
        op_binary<jlong>(frame, binop::op_rem);
        break;
    }
    case JVM_OPC_ineg: {
        op_unary<jint>(frame, unop::op_neg);
        break;
    }
    case JVM_OPC_lneg: {
        op_unary<jlong>(frame, unop::op_neg);
        break;
    }
    case JVM_OPC_fneg: {
        op_unary<jfloat>(frame, unop::op_neg);
        break;
    }
    case JVM_OPC_dneg: {
        op_unary<jdouble>(frame, unop::op_neg);
        break;
    }
    case JVM_OPC_ishl: {
        op_shift<jint>(frame, shiftop::op_shl, 0x1f);
        break;
    }
    case JVM_OPC_lshl: {
        op_shift<jlong>(frame, shiftop::op_shl, 0x3f);
        break;
    }
    case JVM_OPC_ishr: {
        op_shift<jint>(frame, shiftop::op_shr, 0x1f);
        break;
    }
    case JVM_OPC_lshr: {
        op_shift<jlong>(frame, shiftop::op_shr, 0x3f);
        break;
    }
    case JVM_OPC_iand: {
        op_binary<jint>(frame, binop::op_and);
        break;
    }
    case JVM_OPC_land: {
        op_binary<jlong>(frame, binop::op_and);
        break;
    }
    case JVM_OPC_ior: {
        op_binary<jint>(frame, binop::op_or);
        break;
    }
    case JVM_OPC_lor: {
        op_binary<jlong>(frame, binop::op_or);
        break;
    }
    case JVM_OPC_ixor: {
        op_binary<jint>(frame, binop::op_xor);
        break;
    }
    case JVM_OPC_lxor: {
        op_binary<jlong>(frame, binop::op_xor);
        break;
    }
    case JVM_OPC_iinc: {
        auto idx   = read_opc_u1(method->code + frame.pc);
        auto value = read_opc_u1(method->code + frame.pc + 1);
        op_iinc(frame, idx, value);
        break;
    }
    case JVM_OPC_if_icmpeq: {
        int16_t offset = read_opc_u2(method->code + frame.pc);
        op_if_cmp<jint>(method, frame, cmpop::op_cmpeq, offset);
        goto next_insn;
    }
    case JVM_OPC_if_icmpne: {
        int16_t offset = read_opc_u2(method->code + frame.pc);
        op_if_cmp<jint>(method, frame, cmpop::op_cmpne, offset);
        goto next_insn;
    }
    case JVM_OPC_if_icmplt: {
        int16_t offset = read_opc_u2(method->code + frame.pc);
        op_if_cmp<jint>(method, frame, cmpop::op_cmplt, offset);
        goto next_insn;
    }
    case JVM_OPC_if_icmpge: {
        int16_t offset = read_opc_u2(method->code + frame.pc);
        op_if_cmp<jint>(method, frame, cmpop::op_cmpge, offset);
        goto next_insn;
    }
    case JVM_OPC_if_icmpgt: {
        int16_t offset = read_opc_u2(method->code + frame.pc);
        op_if_cmp<jint>(method, frame, cmpop::op_cmpgt, offset);
        goto next_insn;
    }
    case JVM_OPC_if_icmple: {
        int16_t offset = read_opc_u2(method->code + frame.pc);
        op_if_cmp<jint>(method, frame, cmpop::op_cmple, offset);
        goto next_insn;
    }
    case JVM_OPC_goto: {
        int16_t offset = read_opc_u2(method->code + frame.pc);
        op_goto(frame, offset);
        goto next_insn;
    }
    case JVM_OPC_ireturn:
    case JVM_OPC_lreturn:
    case JVM_OPC_freturn:
    case JVM_OPC_dreturn:
    case JVM_OPC_areturn: {
        auto value = frame.ostack.top();
        frame.ostack.empty();
        return value;
    }
    case JVM_OPC_return: {
        frame.ostack.empty();
        return to_value<jobject>(nullptr);
    }
    case JVM_OPC_getstatic: {
        uint16_t idx = read_opc_u2(method->code + frame.pc);
        op_getstatic(method, frame, idx);
    }
    case JVM_OPC_invokespecial: {
        break;
    }
    case JVM_OPC_invokestatic: {
        uint16_t idx = read_opc_u2(method->code + frame.pc);
        auto target = method->klass->resolve_method(idx);
        assert(target != nullptr);
        assert(target->access_flags & JVM_ACC_STATIC);
        hornet::frame new_frame(target->max_locals);
        for (int i = 0; i < target->args_count; i++) {
            auto arg_idx = target->args_count - i - 1;
            new_frame.locals[arg_idx] = frame.ostack.top();
            frame.ostack.pop();
        }
        auto result = execute(target.get(), new_frame);
        if (target->return_type != &jvm_void_klass) {
            frame.ostack.push(result);
        }
        break;
    }
    case JVM_OPC_new: {
        op_new(frame);
        break;
    }
    case JVM_OPC_arraylength: {
        op_arraylength(frame);
        break;
    }
    default:
        fprintf(stderr, "error: unsupported bytecode: %u\n", opc);
        abort();
    }

    frame.pc += opcode_length[opc];

    goto next_insn;
}

}
