#include "hornet/java.hh"

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
void const_interp(frame& frame, T value)
{
    frame.ostack.push(to_value(value));
}

void load_interp(frame& frame, uint16_t idx)
{
    frame.ostack.push(frame.locals[idx]);
}

void store_interp(frame& frame, uint16_t idx)
{
    frame.locals[idx] = frame.ostack.top();
    frame.ostack.pop();
}

template<typename T>
void unop_interp(frame& frame, std::function<T (T)> op)
{
    auto value = from_value<T>(frame.ostack.top());
    frame.ostack.pop();
    auto result = op(value);
    frame.ostack.push(to_value<T>(result));
}

template<typename T>
void binop_interp(frame& frame, std::function<T (T, T)> op)
{
    auto value2 = from_value<T>(frame.ostack.top());
    frame.ostack.pop();
    auto value1 = from_value<T>(frame.ostack.top());
    frame.ostack.pop();
    auto result = op(value1, value2);
    frame.ostack.push(to_value<T>(result));
}

template<typename T>
void shift_interp(frame& frame, std::function<T (T, jint)> op, jint mask)
{
    auto value2 = from_value<jint>(frame.ostack.top());
    frame.ostack.pop();
    auto value1 = from_value<T>(frame.ostack.top());
    frame.ostack.pop();
    auto result = op(value1, value2 & mask);
    frame.ostack.push(to_value<T>(result));
}

template<typename T>
void if_cmp_interp(method* method, frame& frame, std::function<bool (T, T)> op)
{
    uint8_t opc = method->code[frame.pc];
    int16_t offset = read_opc_u2(method->code + frame.pc);
    auto value2 = from_value<T>(frame.ostack.top());
    frame.ostack.pop();
    auto value1 = from_value<T>(frame.ostack.top());
    frame.ostack.pop();
    if (op(value1, value2)) {
        frame.pc += offset;
    } else {
        frame.pc += opcode_length[opc];
    }
}

std::shared_ptr<field> resolve_fieldref(klass* klass, uint16_t idx)
{
    auto const_pool = klass->const_pool();
    auto fieldref = const_pool->get_fieldref(idx);
    auto target_klass = klass->load_class(fieldref->class_index);
    if (!target_klass) {
        return nullptr;
    }
    auto field_name_and_type = const_pool->get_name_and_type(fieldref->name_and_type_index);
    auto field_name = const_pool->get_utf8(field_name_and_type->name_index);
    auto field_type = const_pool->get_utf8(field_name_and_type->descriptor_index);
    return target_klass->lookup_field(field_name->bytes, field_type->bytes);
}

std::shared_ptr<method> resolve_methodref(klass* klass, uint16_t idx)
{
    auto const_pool = klass->const_pool();
    auto methodref = const_pool->get_methodref(idx);
    auto target_klass = klass->load_class(methodref->class_index);
    if (!target_klass) {
        return nullptr;
    }
    auto method_name_and_type = const_pool->get_name_and_type(methodref->name_and_type_index);
    auto method_name = const_pool->get_utf8(method_name_and_type->name_index);
    auto method_type = const_pool->get_utf8(method_name_and_type->descriptor_index);
    return target_klass->lookup_method(method_name->bytes, method_type->bytes);
}

template<typename T>
T op_add(T a, T b)
{
    return a + b;
}

template<typename T>
T op_sub(T a, T b)
{
    return a - b;
}

template<typename T>
T op_mul(T a, T b)
{
    return a * b;
}

template<typename T>
T op_div(T a, T b)
{
    return a / b;
}

template<typename T>
T op_rem(T a, T b)
{
    return a % b;
}

template<typename T>
T op_and(T a, T b)
{
    return a & b;
}

template<typename T>
T op_or(T a, T b)
{
    return a | b;
}

template<typename T>
T op_xor(T a, T b)
{
    return a ^ b;
}

template<typename T>
T op_neg(T a)
{
    return -a;
}

template<typename T>
T op_shl(T a, T b)
{
    return a << b;
}

template<typename T>
T op_shr(T a, T b)
{
    return a >> b;
}

template<typename T>
bool op_cmpeq(T a, T b)
{
    return a == b;
}

template<typename T>
bool op_cmpne(T a, T b)
{
    return a != b;
}

template<typename T>
bool op_cmplt(T a, T b)
{
    return a < b;
}

template<typename T>
bool op_cmpge(T a, T b)
{
    return a >= b;
}

template<typename T>
bool op_cmpgt(T a, T b)
{
    return a > b;
}

template<typename T>
bool op_cmple(T a, T b)
{
    return a <= b;
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
        frame.ostack.push(to_value<jobject>(nullptr));
        break;
    case JVM_OPC_iconst_m1:
    case JVM_OPC_iconst_0:
    case JVM_OPC_iconst_1:
    case JVM_OPC_iconst_2:
    case JVM_OPC_iconst_3:
    case JVM_OPC_iconst_4:
    case JVM_OPC_iconst_5: {
        jint value = opc - JVM_OPC_iconst_0;
        const_interp(frame, value);
        break;
    }
    case JVM_OPC_lconst_0:
    case JVM_OPC_lconst_1: {
        jlong value = opc - JVM_OPC_lconst_0;
        const_interp(frame, value);
        break;
    }
    case JVM_OPC_fconst_0:
    case JVM_OPC_fconst_1: {
        jfloat value = opc - JVM_OPC_fconst_0;
        const_interp(frame, value);
        break;
    }
    case JVM_OPC_dconst_0:
    case JVM_OPC_dconst_1: {
        jdouble value = opc - JVM_OPC_dconst_0;
        const_interp(frame, value);
        break;
    }
    case JVM_OPC_bipush: {
        int8_t value = read_opc_u1(method->code + frame.pc);
        const_interp<jint>(frame, value);
        break;
    }
    case JVM_OPC_sipush: {
        int16_t value = read_opc_u2(method->code + frame.pc);
        const_interp<jint>(frame, value);
        break;
    }
    case JVM_OPC_ldc: {
        auto idx = read_opc_u1(method->code + frame.pc);
        auto const_pool = method->klass->const_pool();
        auto cp_info = const_pool->get(idx);
        switch (cp_info->tag) {
        case cp_tag::const_integer: {
            auto value = const_pool->get_integer(idx);
            frame.ostack.push(to_value<jint>(value));
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
        load_interp(frame, idx);
        break;
    }
    case JVM_OPC_lload: {
        auto idx = read_opc_u1(method->code + frame.pc);
        load_interp(frame, idx);
        break;
    }
    case JVM_OPC_aload: {
        auto idx = read_opc_u1(method->code + frame.pc);
        load_interp(frame, idx);
        break;
    }
    case JVM_OPC_iload_0:
    case JVM_OPC_iload_1:
    case JVM_OPC_iload_2:
    case JVM_OPC_iload_3: {
        uint16_t idx = opc - JVM_OPC_iload_0;
        load_interp(frame, idx);
        break;
    }
    case JVM_OPC_lload_0:
    case JVM_OPC_lload_1:
    case JVM_OPC_lload_2:
    case JVM_OPC_lload_3: {
        uint16_t idx = opc - JVM_OPC_lload_0;
        load_interp(frame, idx);
        break;
    }
    case JVM_OPC_aload_0:
    case JVM_OPC_aload_1:
    case JVM_OPC_aload_2:
    case JVM_OPC_aload_3: {
        uint16_t idx = opc - JVM_OPC_aload_0;
        load_interp(frame, idx);
        break;
    }
    case JVM_OPC_istore: {
        auto idx = read_opc_u1(method->code + frame.pc);
        store_interp(frame, idx);
        break;
    }
    case JVM_OPC_lstore: {
        auto idx = read_opc_u1(method->code + frame.pc);
        store_interp(frame, idx);
        break;
    }
    case JVM_OPC_istore_0:
    case JVM_OPC_istore_1:
    case JVM_OPC_istore_2:
    case JVM_OPC_istore_3: {
        uint16_t idx = opc - JVM_OPC_istore_0;
        store_interp(frame, idx);
        break;
    }
    case JVM_OPC_lstore_0:
    case JVM_OPC_lstore_1:
    case JVM_OPC_lstore_2:
    case JVM_OPC_lstore_3: {
        uint16_t idx = opc - JVM_OPC_lstore_0;
        store_interp(frame, idx);
        break;
    }
    case JVM_OPC_pop: {
        frame.ostack.pop();
        break;
    }
    case JVM_OPC_dup: {
        auto value = frame.ostack.top();
        frame.ostack.push(value);
        break;
    }
    case JVM_OPC_dup_x1: {
        auto value1 = frame.ostack.top();
        frame.ostack.pop();
        auto value2 = frame.ostack.top();
        frame.ostack.pop();

        frame.ostack.push(value1);
        frame.ostack.push(value2);
        frame.ostack.push(value1);
        break;
    }
    case JVM_OPC_swap: {
        auto value1 = frame.ostack.top();
        frame.ostack.pop();
        auto value2 = frame.ostack.top();
        frame.ostack.pop();

        frame.ostack.push(value1);
        frame.ostack.push(value2);
        break;
    }
    case JVM_OPC_iadd: {
        binop_interp<jint>(frame, op_add<jint>);
        break;
    }
    case JVM_OPC_ladd: {
        binop_interp<jlong>(frame, op_add<jlong>);
        break;
    }
    case JVM_OPC_fadd: {
        binop_interp<jfloat>(frame, op_add<jfloat>);
        break;
    }
    case JVM_OPC_dadd: {
        binop_interp<jdouble>(frame, op_add<jdouble>);
        break;
    }
    case JVM_OPC_isub: {
        binop_interp<jint>(frame, op_sub<jint>);
        break;
    }
    case JVM_OPC_lsub: {
        binop_interp<jlong>(frame, op_sub<jlong>);
        break;
    }
    case JVM_OPC_fsub: {
        binop_interp<jfloat>(frame, op_sub<jfloat>);
        break;
    }
    case JVM_OPC_dsub: {
        binop_interp<jdouble>(frame, op_sub<jdouble>);
        break;
    }
    case JVM_OPC_imul: {
        binop_interp<jint>(frame, op_mul<jint>);
        break;
    }
    case JVM_OPC_lmul: {
        binop_interp<jlong>(frame, op_mul<jlong>);
        break;
    }
    case JVM_OPC_fmul: {
        binop_interp<jfloat>(frame, op_mul<jfloat>);
        break;
    }
    case JVM_OPC_dmul: {
        binop_interp<jdouble>(frame, op_mul<jdouble>);
        break;
    }
    case JVM_OPC_idiv: {
        binop_interp<jint>(frame, op_div<jint>);
        break;
    }
    case JVM_OPC_ldiv: {
        binop_interp<jlong>(frame, op_div<jlong>);
        break;
    }
    case JVM_OPC_fdiv: {
        binop_interp<jfloat>(frame, op_div<jfloat>);
        break;
    }
    case JVM_OPC_ddiv: {
        binop_interp<jdouble>(frame, op_div<jdouble>);
        break;
    }
    case JVM_OPC_irem: {
        binop_interp<jint>(frame, op_rem<jint>);
        break;
    }
    case JVM_OPC_lrem: {
        binop_interp<jlong>(frame, op_rem<jlong>);
        break;
    }
    case JVM_OPC_ineg: {
        unop_interp<jint>(frame, op_neg<jint>);
        break;
    }
    case JVM_OPC_lneg: {
        unop_interp<jlong>(frame, op_neg<jlong>);
        break;
    }
    case JVM_OPC_fneg: {
        unop_interp<jfloat>(frame, op_neg<jfloat>);
        break;
    }
    case JVM_OPC_dneg: {
        unop_interp<jdouble>(frame, op_neg<jdouble>);
        break;
    }
    case JVM_OPC_ishl: {
        shift_interp<jint>(frame, op_shl<jint>, 0x1f);
        break;
    }
    case JVM_OPC_lshl: {
        shift_interp<jlong>(frame, op_shl<jlong>, 0x3f);
        break;
    }
    case JVM_OPC_ishr: {
        shift_interp<jint>(frame, op_shr<jint>, 0x1f);
        break;
    }
    case JVM_OPC_lshr: {
        shift_interp<jlong>(frame, op_shr<jlong>, 0x3f);
        break;
    }
    case JVM_OPC_iand: {
        binop_interp<jint>(frame, op_and<jint>);
        break;
    }
    case JVM_OPC_land: {
        binop_interp<jlong>(frame, op_and<jlong>);
        break;
    }
    case JVM_OPC_ior: {
        binop_interp<jint>(frame, op_or<jint>);
        break;
    }
    case JVM_OPC_lor: {
        binop_interp<jlong>(frame, op_or<jlong>);
        break;
    }
    case JVM_OPC_ixor: {
        binop_interp<jint>(frame, op_xor<jint>);
        break;
    }
    case JVM_OPC_lxor: {
        binop_interp<jlong>(frame, op_xor<jlong>);
        break;
    }
    case JVM_OPC_iinc: {
        auto idx = read_opc_u1(method->code + frame.pc);
        auto c   = read_opc_u1(method->code + frame.pc + 1);
        frame.locals[idx] += c;
        break;
    }
    case JVM_OPC_if_icmpeq: {
        if_cmp_interp<jint>(method, frame, op_cmpeq<jint>);
        goto next_insn;
    }
    case JVM_OPC_if_icmpne: {
        if_cmp_interp<jint>(method, frame, op_cmpne<jint>);
        goto next_insn;
    }
    case JVM_OPC_if_icmplt: {
        if_cmp_interp<jint>(method, frame, op_cmplt<jint>);
        goto next_insn;
    }
    case JVM_OPC_if_icmpge: {
        if_cmp_interp<jint>(method, frame, op_cmpge<jint>);
        goto next_insn;
    }
    case JVM_OPC_if_icmpgt: {
        if_cmp_interp<jint>(method, frame, op_cmpgt<jint>);
        goto next_insn;
    }
    case JVM_OPC_if_icmple: {
        if_cmp_interp<jint>(method, frame, op_cmple<jint>);
        goto next_insn;
    }
    case JVM_OPC_goto: {
        int16_t offset = read_opc_u2(method->code + frame.pc);
        frame.pc += offset;
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
        auto field = resolve_fieldref(method->klass, idx);
        assert(field != nullptr);
        frame.ostack.push(field->value);
    }
    case JVM_OPC_invokespecial: {
        break;
    }
    case JVM_OPC_invokestatic: {
        uint16_t idx = read_opc_u2(method->code + frame.pc);
        auto target = resolve_methodref(method->klass, idx);
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
        auto obj = gc_new_object(nullptr);
        frame.ostack.push(to_value<object*>(obj));
        break;
    }
    case JVM_OPC_arraylength: {
        auto* arrayref = from_value<array*>(frame.ostack.top());
        frame.ostack.pop();
        assert(arrayref != nullptr);
        frame.ostack.push(arrayref->length);
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
