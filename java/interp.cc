#include "hornet/java.hh"

#include "hornet/vm.hh"

#include <cassert>
#include <stack>

#include <classfile_constants.h>
#include <jni.h>

namespace hornet {

value_t object_to_value(object* obj)
{
    return reinterpret_cast<value_t>(obj);
}

jint value_to_jint(value_t value)
{
    return static_cast<jint>(value);
}

jlong value_to_jlong(value_t value)
{
   return static_cast<jlong>(value);
}

jfloat value_to_jfloat(value_t value)
{
    return static_cast<jfloat>(value);
}

jdouble value_to_jdouble(value_t value)
{
    return static_cast<jdouble>(value);
}

array* value_to_array(value_t value)
{
   return reinterpret_cast<array*>(value);
}

value_t jint_to_value(jint n)
{
    return static_cast<value_t>(n);
}

value_t jlong_to_value(jlong n)
{
    return static_cast<value_t>(n);
}

value_t jfloat_to_value(jfloat n)
{
    return static_cast<value_t>(n);
}

value_t jdouble_to_value(jdouble n)
{
    return static_cast<value_t>(n);
}

#define CONST_INTERP(type, value)                               \
    do {                                                        \
        frame.ostack.push(type##_to_value(value));              \
    } while (0)

#define LOAD_INTERP(type, idx)                                  \
    do {                                                        \
        frame.ostack.push(frame.locals[idx]);                   \
    } while (0)

#define STORE_INTERP(type, idx)                                 \
    do {                                                        \
        frame.locals[idx] = frame.ostack.top();                 \
        frame.ostack.pop();                                     \
    } while (0)

#define UNARY_OP_INTERP(type, op)                               \
    do {                                                        \
        auto value = value_to_##type(frame.ostack.top());       \
        frame.ostack.pop();                                     \
        type result = op value;                                 \
        frame.ostack.push(type##_to_value(result));             \
   } while (0)

#define BINOP_INTERP(type, op)                                  \
    do {                                                        \
        auto value2 = value_to_##type(frame.ostack.top());      \
        frame.ostack.pop();                                     \
        auto value1 = value_to_##type(frame.ostack.top());      \
        frame.ostack.pop();                                     \
        type result = value1 op value2;                         \
        frame.ostack.push(type##_to_value(result));             \
   } while (0)

#define SHL_INTERP(type, mask)                                  \
    do {                                                        \
        auto value2 = value_to_jint(frame.ostack.top());        \
        frame.ostack.pop();                                     \
        auto value1 = value_to_##type(frame.ostack.top());      \
        frame.ostack.pop();                                     \
        type result = value1 << (value2 & mask);                \
        frame.ostack.push(type##_to_value(result));             \
    } while (0)

#define SHR_INTERP(type, mask)                                  \
    do {                                                        \
        auto value2 = value_to_jint(frame.ostack.top());        \
        frame.ostack.pop();                                     \
        auto value1 = value_to_##type(frame.ostack.top());      \
        frame.ostack.pop();                                     \
        type result = value1 >> (value2 & mask);                \
        frame.ostack.push(type##_to_value(result));             \
    } while (0)

#define IF_CMP_INTERP(type, cond)                               \
    do {                                                        \
        int16_t offset = read_opc_u2(method->code + frame.pc);  \
        auto value2 = value_to_##type(frame.ostack.top());      \
        frame.ostack.pop();                                     \
        auto value1 = value_to_##type(frame.ostack.top());      \
        frame.ostack.pop();                                     \
        if (value1 cond value2) {                               \
            frame.pc += offset;                                 \
            goto next_insn;                                     \
        }                                                       \
    } while (0)

std::shared_ptr<klass> resolve_klass(klass* klass, uint16_t idx)
{
    auto const_pool = klass->const_pool();
    auto klassref = const_pool->get_class(idx);
    auto klass_name = const_pool->get_utf8(klassref->name_index);
    auto loader = hornet::system_loader();
    return loader->load_class(klass_name->bytes);
}

std::shared_ptr<field> resolve_fieldref(klass* klass, uint16_t idx)
{
    auto const_pool = klass->const_pool();
    auto fieldref = const_pool->get_fieldref(idx);
    auto target_klass = resolve_klass(klass, fieldref->class_index);
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
    auto target_klass = resolve_klass(klass, methodref->class_index);
    if (!target_klass) {
        return nullptr;
    }
    auto method_name_and_type = const_pool->get_name_and_type(methodref->name_and_type_index);
    auto method_name = const_pool->get_utf8(method_name_and_type->name_index);
    auto method_type = const_pool->get_utf8(method_name_and_type->descriptor_index);
    return target_klass->lookup_method(method_name->bytes, method_type->bytes);
}

value_t interp(method* method, frame& frame)
{
next_insn:
    assert(frame.pc < method->code_length);

    uint8_t opc = method->code[frame.pc];

    switch (opc) {
    case JVM_OPC_nop:
        break;
    case JVM_OPC_aconst_null:
        frame.ostack.push(object_to_value(nullptr));
        break;
    case JVM_OPC_iconst_m1:
    case JVM_OPC_iconst_0:
    case JVM_OPC_iconst_1:
    case JVM_OPC_iconst_2:
    case JVM_OPC_iconst_3:
    case JVM_OPC_iconst_4:
    case JVM_OPC_iconst_5: {
        jint value = opc - JVM_OPC_iconst_0;
        CONST_INTERP(jint, value);
        break;
    }
    case JVM_OPC_lconst_0:
    case JVM_OPC_lconst_1: {
        jlong value = opc - JVM_OPC_lconst_0;
        CONST_INTERP(jlong, value);
        break;
    }
    case JVM_OPC_fconst_0:
    case JVM_OPC_fconst_1: {
        jfloat value = opc - JVM_OPC_fconst_0;
        CONST_INTERP(jfloat, value);
        break;
    }
    case JVM_OPC_dconst_0:
    case JVM_OPC_dconst_1: {
        jdouble value = opc - JVM_OPC_dconst_0;
        CONST_INTERP(jdouble, value);
        break;
    }
    case JVM_OPC_bipush: {
        int8_t value = read_opc_u1(method->code + frame.pc);
        CONST_INTERP(jint, value);
        break;
    }
    case JVM_OPC_sipush: {
        int16_t value = read_opc_u2(method->code + frame.pc);
        CONST_INTERP(jint, value);
        break;
    }
    case JVM_OPC_ldc: {
        auto idx = read_opc_u1(method->code + frame.pc);
        auto const_pool = method->klass->const_pool();
        auto cp_info = const_pool->get(idx);
        switch (cp_info->tag) {
        case cp_tag::const_integer: {
            auto value = const_pool->get_integer(idx);
            frame.ostack.push(jint_to_value(value));
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
        LOAD_INTERP(jint, idx);
        break;
    }
    case JVM_OPC_lload: {
        auto idx = read_opc_u1(method->code + frame.pc);
        LOAD_INTERP(jlong, idx);
        break;
    }
    case JVM_OPC_aload: {
        auto idx = read_opc_u1(method->code + frame.pc);
        LOAD_INTERP(jobject, idx);
        break;
    }
    case JVM_OPC_iload_0:
    case JVM_OPC_iload_1:
    case JVM_OPC_iload_2:
    case JVM_OPC_iload_3: {
        uint16_t idx = opc - JVM_OPC_iload_0;
        LOAD_INTERP(jint, idx);
        break;
    }
    case JVM_OPC_lload_0:
    case JVM_OPC_lload_1:
    case JVM_OPC_lload_2:
    case JVM_OPC_lload_3: {
        uint16_t idx = opc - JVM_OPC_lload_0;
        LOAD_INTERP(jlong, idx);
        break;
    }
    case JVM_OPC_aload_0:
    case JVM_OPC_aload_1:
    case JVM_OPC_aload_2:
    case JVM_OPC_aload_3: {
        uint16_t idx = opc - JVM_OPC_aload_0;
        LOAD_INTERP(jobject, idx);
        break;
    }
    case JVM_OPC_istore: {
        auto idx = read_opc_u1(method->code + frame.pc);
        STORE_INTERP(jint, idx);
        break;
    }
    case JVM_OPC_lstore: {
        auto idx = read_opc_u1(method->code + frame.pc);
        STORE_INTERP(jlong, idx);
        break;
    }
    case JVM_OPC_istore_0:
    case JVM_OPC_istore_1:
    case JVM_OPC_istore_2:
    case JVM_OPC_istore_3: {
        uint16_t idx = opc - JVM_OPC_istore_0;
        STORE_INTERP(jint, idx);
        break;
    }
    case JVM_OPC_lstore_0:
    case JVM_OPC_lstore_1:
    case JVM_OPC_lstore_2:
    case JVM_OPC_lstore_3: {
        uint16_t idx = opc - JVM_OPC_lstore_0;
        STORE_INTERP(jlong, idx);
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
        BINOP_INTERP(jint, +);
        break;
    }
    case JVM_OPC_ladd: {
        BINOP_INTERP(jlong, +);
        break;
    }
    case JVM_OPC_fadd: {
        BINOP_INTERP(jfloat, +);
        break;
    }
    case JVM_OPC_dadd: {
        BINOP_INTERP(jdouble, +);
        break;
    }
    case JVM_OPC_isub: {
        BINOP_INTERP(jint, -);
        break;
    }
    case JVM_OPC_lsub: {
        BINOP_INTERP(jlong, -);
        break;
    }
    case JVM_OPC_fsub: {
        BINOP_INTERP(jfloat, -);
        break;
    }
    case JVM_OPC_dsub: {
        BINOP_INTERP(jdouble, -);
        break;
    }
    case JVM_OPC_imul: {
        BINOP_INTERP(jint, *);
        break;
    }
    case JVM_OPC_lmul: {
        BINOP_INTERP(jlong, *);
        break;
    }
    case JVM_OPC_fmul: {
        BINOP_INTERP(jfloat, *);
        break;
    }
    case JVM_OPC_dmul: {
        BINOP_INTERP(jdouble, *);
        break;
    }
    case JVM_OPC_idiv: {
        BINOP_INTERP(jint, /);
        break;
    }
    case JVM_OPC_ldiv: {
        BINOP_INTERP(jlong, /);
        break;
    }
    case JVM_OPC_fdiv: {
        BINOP_INTERP(jfloat, /);
        break;
    }
    case JVM_OPC_ddiv: {
        BINOP_INTERP(jdouble, /);
        break;
    }
    case JVM_OPC_irem: {
        BINOP_INTERP(jint, %);
        break;
    }
    case JVM_OPC_lrem: {
        BINOP_INTERP(jlong, %);
        break;
    }
    case JVM_OPC_ineg: {
        UNARY_OP_INTERP(jint, -);
        break;
    }
    case JVM_OPC_lneg: {
        UNARY_OP_INTERP(jlong, -);
        break;
    }
    case JVM_OPC_fneg: {
        UNARY_OP_INTERP(jfloat, -);
        break;
    }
    case JVM_OPC_dneg: {
        UNARY_OP_INTERP(jdouble, -);
        break;
    }
    case JVM_OPC_ishl: {
        SHL_INTERP(jint, 0x1f);
        break;
    }
    case JVM_OPC_lshl: {
        SHL_INTERP(jlong, 0x3f);
        break;
    }
    case JVM_OPC_ishr: {
        SHR_INTERP(jint, 0x1f);
        break;
    }
    case JVM_OPC_lshr: {
        SHR_INTERP(jlong, 0x3f);
        break;
    }
    case JVM_OPC_iand: {
        BINOP_INTERP(jint, &);
        break;
    }
    case JVM_OPC_land: {
        BINOP_INTERP(jlong, &);
        break;
    }
    case JVM_OPC_ior: {
        BINOP_INTERP(jint, |);
        break;
    }
    case JVM_OPC_lor: {
        BINOP_INTERP(jlong, |);
        break;
    }
    case JVM_OPC_ixor: {
        BINOP_INTERP(jint, ^);
        break;
    }
    case JVM_OPC_lxor: {
        BINOP_INTERP(jlong, ^);
        break;
    }
    case JVM_OPC_iinc: {
        auto idx = read_opc_u1(method->code + frame.pc);
        auto c   = read_opc_u1(method->code + frame.pc + 1);
        frame.locals[idx] += c;
        break;
    }
    case JVM_OPC_if_icmpeq: {
        IF_CMP_INTERP(jint, ==);
        break;
    }
    case JVM_OPC_if_icmpne: {
        IF_CMP_INTERP(jint, !=);
        break;
    }
    case JVM_OPC_if_icmplt: {
        IF_CMP_INTERP(jint, <);
        break;
    }
    case JVM_OPC_if_icmpge: {
        IF_CMP_INTERP(jint, >=);
        break;
    }
    case JVM_OPC_if_icmpgt: {
        IF_CMP_INTERP(jint, >);
        break;
    }
    case JVM_OPC_if_icmple: {
        IF_CMP_INTERP(jint, <=);
        break;
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
        return object_to_value(nullptr);
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
        auto result = interp(target.get(), new_frame);
        if (target->return_type != &jvm_void_klass) {
            frame.ostack.push(result);
        }
        break;
    }
    case JVM_OPC_new: {
        auto obj = gc_new_object(nullptr);
        frame.ostack.push(object_to_value(obj));
        break;
    }
    case JVM_OPC_arraylength: {
        auto* arrayref = value_to_array(frame.ostack.top());
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
