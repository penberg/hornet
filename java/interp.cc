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

template<>
value_t to_value(object* obj)
{
    return reinterpret_cast<value_t>(obj);
}

template<>
value_t to_value(array* arrayref)
{
    return reinterpret_cast<value_t>(arrayref);
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

template<>
object* from_value<object*>(value_t value)
{
    return reinterpret_cast<object*>(value);
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

void op_arrayload(frame& frame)
{
    assert(0);
}

void op_arraystore(uint16_t idx, frame& frame)
{
    assert(0);
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

void op_lcmp(frame& frame)
{
    auto value2 = from_value<jlong>(frame.ostack.top());
    frame.ostack.pop();
    auto value1 = from_value<jlong>(frame.ostack.top());
    frame.ostack.pop();
    jint result;
    if (value1 < value2) {
        result = -1;
    } else if (value1 > value2) {
        result = 1;
    } else {
        result = 0;
    }
    frame.ostack.push(to_value(result));
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

void op_if(frame& frame, cmpop op, uint16_t offset)
{
    auto value = from_value<jint>(frame.ostack.top());
    frame.ostack.pop();
    if (eval(op, value, 0)) {
        frame.pc = offset;
    }
}

template<typename T>
void op_if_cmp(frame& frame, cmpop op, uint16_t offset)
{
    auto value2 = from_value<T>(frame.ostack.top());
    frame.ostack.pop();
    auto value1 = from_value<T>(frame.ostack.top());
    frame.ostack.pop();
    if (eval(op, value1, value2)) {
        frame.pc = offset;
    }
}

void op_goto(frame& frame, int16_t offset)
{
    frame.pc += offset;
}

void op_getstatic(field* field, frame& frame)
{
    assert(field != nullptr);
    auto klass = field->klass;
    klass->init();
    frame.ostack.push(klass->static_values[field->offset]);
}

void op_putstatic(field* field, frame& frame)
{
    assert(field != nullptr);
    auto klass = field->klass;
    klass->init();
    klass->static_values[field->offset] = frame.ostack.top();
    frame.ostack.pop();
}

void op_getfield(field* field, frame& frame)
{
    assert(field != nullptr);
    field->klass->init();
    auto objectref = from_value<object*>(frame.ostack.top());
    frame.ostack.pop();
    auto value = objectref->get_field(field->offset);
    frame.ostack.push(value);
}

void op_putfield(field* field, frame& frame)
{
    assert(field != nullptr);
    field->klass->init();
    auto value = frame.ostack.top();
    frame.ostack.pop();
    auto objectref = from_value<object*>(frame.ostack.top());
    frame.ostack.pop();
    objectref->set_field(field->offset, value);
}

void op_invokevirtual(method* desc, frame& frame)
{
    auto thread = hornet::thread::current();
    auto args_count = desc->args_count+1;
    auto new_frame = thread->make_frame(args_count);
    for (int i = 1; i < args_count; i++) {
        auto arg_idx = args_count - i - 1;
        new_frame->locals[arg_idx] = frame.ostack.top();
        frame.ostack.pop();
    }
    auto objectref = from_value<object*>(frame.ostack.top());
    frame.ostack.pop();
    assert(objectref != nullptr);
    new_frame->locals[0] = to_value(objectref);
    auto klass = objectref->klass;
    assert(klass != nullptr);
    auto target = klass->lookup_method(desc->name, desc->descriptor);
    assert(target != nullptr);
    assert(!target->is_native());
    new_frame->reserve_more(target->max_locals);
    auto result = hornet::_backend->execute(target.get(), *new_frame);
    if (target->return_type != &jvm_void_klass) {
        frame.ostack.push(result);
    }
    thread->free_frame(new_frame);
}

void op_invokespecial(method* target, frame& frame)
{
    assert(!target->is_native());
    auto thread = hornet::thread::current();
    auto new_frame = thread->make_frame(target->max_locals+1);
    auto args_count = target->args_count+1;
    for (int i = 1; i < args_count; i++) {
        auto arg_idx = args_count - i - 1;
        new_frame->locals[arg_idx] = frame.ostack.top();
        frame.ostack.pop();
    }
    auto objectref = from_value<object*>(frame.ostack.top());
    frame.ostack.pop();
    assert(objectref != nullptr);
    new_frame->locals[0] = to_value(objectref);
    auto klass = objectref->klass;
    assert(klass != nullptr);
    auto result = hornet::_backend->execute(target, *new_frame);
    if (target->return_type != &jvm_void_klass) {
        frame.ostack.push(result);
    }
    thread->free_frame(new_frame);
}

void op_invokestatic(method* target, frame& frame)
{
    value_t result;
    target->klass->init();
    if (target->access_flags & JVM_ACC_NATIVE) {
        fprintf(stderr, "warning: %s: stubbed\n", target->full_name().c_str());
        for (int i = 0; i < target->args_count; i++) {
            frame.ostack.pop();
        }
        result = to_value<object*>(nullptr);
    } else {
       auto thread = hornet::thread::current();
       auto new_frame = thread->make_frame(target->max_locals);
       for (int i = 0; i < target->args_count; i++) {
           auto arg_idx = target->args_count - i - 1;
           new_frame->locals[arg_idx] = frame.ostack.top();
           frame.ostack.pop();
       }
       result = hornet::_backend->execute(target, *new_frame);
       thread->free_frame(new_frame);
    }
    if (target->return_type != &jvm_void_klass) {
        frame.ostack.push(result);
    }
}

void op_invokeinterface(method* desc, frame& frame)
{
    op_invokevirtual(desc, frame);
}

void op_new(klass* klass, frame& frame)
{
    klass->init();
    object* obj = gc_new_object(klass);
    frame.ostack.push(to_value<object*>(obj));
}

void op_newarray(uint8_t atype, frame& frame)
{
    auto count = from_value<jint>(frame.ostack.top());
    auto klass = klass::primitive_type(atype);
    auto* arrayref = gc_new_object_array(klass, count);
    frame.ostack.push(to_value(arrayref));
}

void op_anewarray(klass* klass, frame& frame)
{
    auto count = from_value<jint>(frame.ostack.top());
    auto* arrayref = gc_new_object_array(klass, count);
    frame.ostack.push(to_value(arrayref));
}

void op_arraylength(frame& frame)
{
    auto* arrayref = from_value<array*>(frame.ostack.top());
    frame.ostack.pop();
    assert(arrayref != nullptr);
    frame.ostack.push(arrayref->length);
}

//
// Instruction opcodes of the interpreter.
//
// This enumeration needs to be in the same order as labels in dispatch_table.
//
enum class opc : uint8_t {
    iconst,
    lconst,
    fconst,
    dconst,
    aconst,

    load,
    store,

    arrayload,
    arraystore,

    pop,
    dup,
    dup_x1,
    swap,

    iadd,
    isub,
    imul,
    idiv,
    irem,
    ineg,
    ishl,
    ishr,
    iushr,
    iand,
    ior,
    ixor,

    ladd,
    lsub,
    lmul,
    ldiv,
    lrem,
    lneg,
    lshl,
    lshr,
    lushr,
    land,
    lor,
    lxor,

    fadd,
    fsub,
    fmul,
    fdiv,
    frem,

    dadd,
    dsub,
    dmul,
    ddiv,
    drem,

    iinc,

    lcmp,

    ifeq,
    ifne,
    iflt,
    ifge,
    ifgt,
    ifle,

    if_icmpeq,
    if_icmpne,
    if_icmplt,
    if_icmpge,
    if_icmpgt,
    if_icmple,
    if_acmpeq,
    if_acmpne,

    goto_,

    ret,
    ret_void,

    getstatic,
    putstatic,
    getfield,
    putfield,

    invokevirtual,
    invokespecial,
    invokestatic,
    invokeinterface,

    new_,
    newarray,
    anewarray,

    arraylength,
};

template<typename T>
T read_const(const char* code, uint16_t& pc)
{
    auto* src = reinterpret_cast<const T*>(code + pc);
    pc += sizeof(T);
    return *src;
}

uint16_t read_label(const char* code, uint16_t& pc)
{
    return read_const<uint16_t>(code, pc);
}

value_t interp(frame& frame, const char *code)
{
    static void* dispatch_table[] = {
        &&op_iconst,
        &&op_lconst,
        &&op_fconst,
        &&op_dconst,
        &&op_aconst,

        &&op_load,
        &&op_store,

        &&op_arrayload,
        &&op_arraystore,

        &&op_pop,
        &&op_dup,
        &&op_dup_x1,
        &&op_swap,

        &&op_iadd,
        &&op_isub,
        &&op_imul,
        &&op_idiv,
        &&op_irem,
        &&op_ineg,
        &&op_ishl,
        &&op_ishr,
        &&op_iushr,
        &&op_iand,
        &&op_ior,
        &&op_ixor,

        &&op_ladd,
        &&op_lsub,
        &&op_lmul,
        &&op_ldiv,
        &&op_lrem,
        &&op_lneg,
        &&op_lshl,
        &&op_lshr,
        &&op_lushr,
        &&op_land,
        &&op_lor,
        &&op_lxor,

        &&op_fadd,
        &&op_fsub,
        &&op_fmul,
        &&op_fdiv,
        &&op_frem,

        &&op_dadd,
        &&op_dsub,
        &&op_dmul,
        &&op_ddiv,
        &&op_drem,

        &&op_iinc,

        &&op_lcmp,

        &&op_ifeq,
        &&op_ifne,
        &&op_iflt,
        &&op_ifge,
        &&op_ifgt,
        &&op_ifle,

        &&op_if_icmpeq,
        &&op_if_icmpne,
        &&op_if_icmplt,
        &&op_if_icmpge,
        &&op_if_icmpgt,
        &&op_if_icmple,
        &&op_if_acmpeq,
        &&op_if_acmpne,

        &&op_goto,

        &&op_ret,
        &&op_ret_void,

        &&op_getstatic,
        &&op_putstatic,
        &&op_getfield,
        &&op_putfield,

        &&op_invokevirtual,
        &&op_invokespecial,
        &&op_invokestatic,
        &&op_invokeinterface,

        &&op_new,
        &&op_newarray,
        &&op_anewarray,

        &&op_arraylength,
    };

    #define dispatch() goto *dispatch_table[(int)code[frame.pc++]]

    frame.pc = 0;

    dispatch();

    while (1) {
        op_iconst: {
            auto value = read_const<jint>(code, frame.pc);
            op_const(frame, value);
            dispatch();
        }
        op_lconst: {
            auto value = read_const<jlong>(code, frame.pc);
            op_const(frame, value);
            dispatch();
        }
        op_fconst: {
            auto value = read_const<jfloat>(code, frame.pc);
            op_const(frame, value);
            dispatch();
        }
        op_dconst: {
            auto value = read_const<jdouble>(code, frame.pc);
            op_const(frame, value);
            dispatch();
        }
        op_aconst: {
            auto value = read_const<object*>(code, frame.pc);
            op_const(frame, value);
            dispatch();
        }
        op_load: {
            auto idx = read_const<uint16_t>(code, frame.pc);
            op_load(frame, idx);
            dispatch();
        }
        op_store: {
            auto idx = read_const<uint16_t>(code, frame.pc);
            op_store(frame, idx);
            dispatch();
        }
        op_arrayload: {
            op_arrayload(frame);
            dispatch();
        }
        op_arraystore: {
            auto idx = read_const<uint16_t>(code, frame.pc);
            op_arraystore(idx, frame);
            dispatch();
        }
        op_pop: {
            op_pop(frame);
            dispatch();
        }
        op_dup: {
            op_dup(frame);
            dispatch();
        }
        op_dup_x1: {
            op_dup_x1(frame);
            dispatch();
        }
        op_swap: {
            op_swap(frame);
            dispatch();
        }

        op_iadd: op_binary<jint>   (frame, binop::op_add); dispatch();
        op_isub: op_binary<jint>   (frame, binop::op_sub); dispatch();
        op_imul: op_binary<jint>   (frame, binop::op_mul); dispatch();
        op_idiv: op_binary<jint>   (frame, binop::op_div); dispatch();
        op_irem: op_binary<jint>   (frame, binop::op_rem); dispatch();
        op_ineg: op_unary<jint>    (frame, unop::op_neg); dispatch();
        op_ishl: op_shift<jint>    (frame, shiftop::op_shl, 0x1f); dispatch();
        op_ishr: op_shift<jint>    (frame, shiftop::op_shr, 0x1f); dispatch();
        op_iushr: op_shift<uint32_t>(frame, shiftop::op_shr, 0x1f); dispatch();
        op_iand: op_binary<jint>   (frame, binop::op_and); dispatch();
        op_ior:  op_binary<jint>   (frame, binop::op_or);  dispatch();
        op_ixor: op_binary<jint>   (frame, binop::op_xor); dispatch();

        op_ladd: op_binary<jlong>  (frame, binop::op_add); dispatch();
        op_lsub: op_binary<jlong>  (frame, binop::op_sub); dispatch();
        op_lmul: op_binary<jlong>  (frame, binop::op_mul); dispatch();
        op_ldiv: op_binary<jlong>  (frame, binop::op_div); dispatch();
        op_lrem: op_binary<jlong>  (frame, binop::op_rem); dispatch();
        op_lneg: op_unary<jlong>   (frame, unop::op_neg);  dispatch();
        op_lshl: op_shift<jlong>   (frame, shiftop::op_shl, 0x3f); dispatch();
        op_lshr: op_shift<jlong>   (frame, shiftop::op_shr, 0x3f); dispatch();
        op_lushr: op_shift<uint64_t>(frame, shiftop::op_shr, 0x3f); dispatch();
        op_land: op_binary<jlong>  (frame, binop::op_and); dispatch();
        op_lor:  op_binary<jlong>  (frame, binop::op_or);  dispatch();
        op_lxor: op_binary<jlong>  (frame, binop::op_xor); dispatch();

        op_fadd: op_binary<jfloat> (frame, binop::op_add); dispatch();
        op_fsub: op_binary<jfloat> (frame, binop::op_sub); dispatch();
        op_fmul: op_binary<jfloat> (frame, binop::op_mul); dispatch();
        op_fdiv: op_binary<jfloat> (frame, binop::op_div); dispatch();
        op_frem: op_binary<jfloat> (frame, binop::op_rem); dispatch();

        op_dadd: op_binary<jdouble>(frame, binop::op_add); dispatch();
        op_dsub: op_binary<jdouble>(frame, binop::op_sub); dispatch();
        op_dmul: op_binary<jdouble>(frame, binop::op_mul); dispatch();
        op_ddiv: op_binary<jdouble>(frame, binop::op_div); dispatch();
        op_drem: op_binary<jdouble>(frame, binop::op_rem); dispatch();

        op_ret_void: {
            return to_value<object*>(nullptr);
        }
        op_iinc: {
            auto idx = read_const<uint8_t>(code, frame.pc);
            auto value = read_const<jint>(code, frame.pc);
            op_iinc(frame, idx, value);
            dispatch();
        }
        op_lcmp: {
            op_lcmp(frame);
            dispatch();
        }
        op_ifeq: {
            auto offset = read_label(code, frame.pc);
            op_if(frame, cmpop::op_cmpeq, offset);
            dispatch();
        }
        op_ifne: {
            auto offset = read_label(code, frame.pc);
            op_if(frame, cmpop::op_cmpne, offset);
            dispatch();
        }
        op_iflt: {
            auto offset = read_label(code, frame.pc);
            op_if(frame, cmpop::op_cmplt, offset);
            dispatch();
        }
        op_ifge: {
            auto offset = read_label(code, frame.pc);
            op_if(frame, cmpop::op_cmpge, offset);
            dispatch();
        }
        op_ifgt: {
            auto offset = read_label(code, frame.pc);
            op_if(frame, cmpop::op_cmpgt, offset);
            dispatch();
        }
        op_ifle: {
            auto offset = read_label(code, frame.pc);
            op_if(frame, cmpop::op_cmple, offset);
            dispatch();
        }
        op_if_icmpeq: {
            auto offset = read_label(code, frame.pc);
            op_if_cmp<jint>(frame, cmpop::op_cmpeq, offset);
            dispatch();
        }
        op_if_icmpne: {
            auto offset = read_label(code, frame.pc);
            op_if_cmp<jint>(frame, cmpop::op_cmpne, offset);
            dispatch();
        }
        op_if_icmplt: {
            auto offset = read_label(code, frame.pc);
            op_if_cmp<jint>(frame, cmpop::op_cmplt, offset);
            dispatch();
        }
        op_if_icmpge: {
            auto offset = read_label(code, frame.pc);
            op_if_cmp<jint>(frame, cmpop::op_cmpge, offset);
            dispatch();
        }
        op_if_icmpgt: {
            auto offset = read_label(code, frame.pc);
            op_if_cmp<jint>(frame, cmpop::op_cmpgt, offset);
            dispatch();
        }
        op_if_icmple: {
            auto offset = read_label(code, frame.pc);
            op_if_cmp<jint>(frame, cmpop::op_cmple, offset);
            dispatch();
        }
        op_if_acmpeq: {
            auto offset = read_label(code, frame.pc);
            op_if_cmp<object*>(frame, cmpop::op_cmpeq, offset);
            dispatch();
        }
        op_if_acmpne: {
            auto offset = read_label(code, frame.pc);
            op_if_cmp<object*>(frame, cmpop::op_cmpne, offset);
            dispatch();
        }
        op_goto: {
            auto offset = read_label(code, frame.pc);
            frame.pc = offset;
            dispatch();
        }
        op_ret: {
            auto value = frame.ostack.top();
            frame.ostack.pop();
            return value;
        }

        op_getstatic: {
            auto* target = read_const<field*>(code, frame.pc);
            op_getstatic(target, frame);
            dispatch();
        }
        op_putstatic: {
            auto* target = read_const<field*>(code, frame.pc);
            op_putstatic(target, frame);
            dispatch();
        }
        op_getfield: {
            auto* target = read_const<field*>(code, frame.pc);
            op_getfield(target, frame);
            dispatch();
        }
        op_putfield: {
            auto* target = read_const<field*>(code, frame.pc);
            op_putfield(target, frame);
            dispatch();
        }
        op_invokevirtual: {
            auto* target = read_const<method*>(code, frame.pc);
            op_invokevirtual(target, frame);
            dispatch();
        }
        op_invokespecial: {
            auto* target = read_const<method*>(code, frame.pc);
            op_invokespecial(target, frame);
            dispatch();
        }
        op_invokestatic: {
            auto* target = read_const<method*>(code, frame.pc);
            op_invokestatic(target, frame);
            dispatch();
        }
        op_invokeinterface: {
            auto* target = read_const<method*>(code, frame.pc);
            op_invokeinterface(target, frame);
            dispatch();
        }
        op_new: {
            auto* type = read_const<klass*>(code, frame.pc);
            op_new(type, frame);
            dispatch();
        }
        op_newarray: {
            auto atype = read_const<uint8_t>(code, frame.pc);
            op_newarray(atype, frame);
            dispatch();
        }
        op_anewarray: {
            auto* type = read_const<klass*>(code, frame.pc);
            op_anewarray(type, frame);
            dispatch();
        }
        op_arraylength:
            op_arraylength(frame);
            dispatch();
    }
}

// A branch label that is backpatched to a branch offset after all basic blocks
// are translated.
class label {
public:
    // The location of the branch offset that needs to be backpatched in code.
    uint16_t pc;
    // The target basic block of this branch label.
    std::shared_ptr<basic_block> bblock;

    label(uint16_t pc_, std::shared_ptr<basic_block> bblock_)
        : pc(pc_)
        , bblock(bblock_)
    { }
};

class interp_translator : public translator {
public:
    interp_translator(method* method);
    ~interp_translator();

    template<typename T>
    T trampoline();

    virtual void prologue() override;
    virtual void epilogue() override;
    virtual void begin(std::shared_ptr<basic_block> bblock) override;
    virtual void op_const (type t, int64_t value) override;
    virtual void op_load  (type t, uint16_t idx) override;
    virtual void op_store (type t, uint16_t idx) override;
    virtual void op_arrayload(type t) override;
    virtual void op_arraystore(type t, uint16_t idx) override;
    virtual void op_convert(type from, type to) override;
    virtual void op_pop() override;
    virtual void op_dup() override;
    virtual void op_dup_x1() override;
    virtual void op_dup_x2() override;
    virtual void op_dup2() override;
    virtual void op_dup2_x1() override;
    virtual void op_dup2_x2() override;
    virtual void op_swap() override;
    virtual void op_unary(type t, unaryop op) override;
    virtual void op_binary(type t, binop op) override;
    virtual void op_iinc(uint8_t idx, jint value) override;
    virtual void op_lcmp() override;
    virtual void op_if(cmpop op, std::shared_ptr<basic_block> target) override;
    virtual void op_if_cmp(type t, cmpop op, std::shared_ptr<basic_block> bblock) override;
    virtual void op_goto(std::shared_ptr<basic_block> bblock) override;
    virtual void op_ret() override;
    virtual void op_ret_void() override;
    virtual void op_getstatic(field* field) override;
    virtual void op_putstatic(field* field) override;
    virtual void op_getfield(field* field) override;
    virtual void op_putfield(field* field) override;
    virtual void op_invokevirtual(method* target) override;
    virtual void op_invokespecial(method* target) override;
    virtual void op_invokestatic(method* target) override;
    virtual void op_invokeinterface(method* target) override;
    virtual void op_new(klass* klass) override;
    virtual void op_newarray(uint8_t atype) override;
    virtual void op_anewarray(klass* klass) override;
    virtual void op_arraylength() override;
    virtual void op_checkcast(klass* klass) override;

private:
    void put_opc(opc x) {
      _code.resize(_code.size() + sizeof(opc));
      auto* code = _code.data();
      code[_pc++] = static_cast<uint8_t>(x);
    }
    template<typename T>
    void put_const(T x, uint16_t pc) {
      auto* code = _code.data() + pc;
      auto* dst = reinterpret_cast<T*>(code);
      *dst = x;
    }
    template<typename T>
    void put_const(T x) {
      _code.resize(_code.size() + sizeof(T));
     put_const(x, _pc);
      _pc += sizeof(T);
    }
    // Puts a zero offset to the instruction stream and registers the branch
    // for backpatching.
    void put_label(const std::shared_ptr<basic_block>& bblock) {
      _label_list.push_back(label(_pc, bblock));
      put_const<uint16_t>(0);
    }
    void backpatch() {
      for (auto&& label : _label_list) {
        auto it = _bblock_map.find(label.bblock);
        assert(it != _bblock_map.end());
        uint16_t offset = it->second;
        put_const(offset, label.pc);
      }
    }

    std::map<std::shared_ptr<basic_block>, uint16_t> _bblock_map;
    std::vector<uint8_t> _code;
    std::vector<label> _label_list;
    uint16_t _pc;
};

interp_translator::interp_translator(method* method)
    : translator(method)
    , _pc(0)
{
}

interp_translator::~interp_translator()
{
}

template<typename T> T interp_translator::trampoline()
{
    return reinterpret_cast<T>(_code.data());
}

void interp_translator::prologue()
{
}

void interp_translator::epilogue()
{
    backpatch();
}

void interp_translator::begin(std::shared_ptr<basic_block> bblock)
{
    _bblock_map.insert(std::make_pair(bblock, _pc));
}

void interp_translator::op_const(type t, int64_t value)
{
    switch (t) {
    case type::t_int:
        put_opc(opc::iconst);
        put_const<jint>(value);
        break;
    case type::t_long:
        put_opc(opc::lconst);
        put_const<jlong>(value);
        break;
    case type::t_float:
        put_opc(opc::fconst);
        put_const<jfloat>(value);
        break;
    case type::t_double:
        put_opc(opc::dconst);
        put_const<jdouble>(value);
        break;
    case type::t_ref:
        put_opc(opc::aconst);
        put_const<object*>(reinterpret_cast<object*>(value));
        break;
    default: assert(0);
    }
}

void interp_translator::op_load(type t, uint16_t idx)
{
    put_opc(opc::load);
    put_const(idx);
}

void interp_translator::op_store(type t, uint16_t idx)
{
    put_opc(opc::store);
    put_const(idx);
}

void interp_translator::op_arrayload(type t)
{
    put_opc(opc::arrayload);
}

void interp_translator::op_arraystore(type t, uint16_t idx)
{
    put_opc(opc::arraystore);
    put_const(idx);
}

void interp_translator::op_convert(type from, type to)
{
    assert(0);
}

void interp_translator::op_pop()
{
    put_opc(opc::pop);
}

void interp_translator::op_dup()
{
    put_opc(opc::dup);
}

void interp_translator::op_dup_x1()
{
    put_opc(opc::dup_x1);
}

void interp_translator::op_dup_x2()
{
    assert(0);
}

void interp_translator::op_dup2()
{
    assert(0);
}

void interp_translator::op_dup2_x1()
{
    assert(0);
}

void interp_translator::op_dup2_x2()
{
    assert(0);
}

void interp_translator::op_swap()
{
    put_opc(opc::swap);
}

void interp_translator::op_unary(type t, unaryop op)
{
    switch (t) {
    case type::t_int: {
        switch (op) {
        case unaryop::op_neg: put_opc(opc::ineg); break;
        default: assert(0);
        }
        break;
    }
    case type::t_long: {
        switch (op) {
        case unaryop::op_neg: put_opc(opc::ineg); break;
        default: assert(0);
        }
        break;
    }
    default: assert(0);
    }
}

void interp_translator::op_binary(type t, binop op)
{
    switch (t) {
    case type::t_int: {
        switch (op) {
        case binop::op_add:  put_opc(opc::iadd);  break;
        case binop::op_sub:  put_opc(opc::isub);  break;
        case binop::op_mul:  put_opc(opc::imul);  break;
        case binop::op_div:  put_opc(opc::idiv);  break;
        case binop::op_rem:  put_opc(opc::irem);  break;
        case binop::op_shl:  put_opc(opc::ishl);  break;
        case binop::op_shr:  put_opc(opc::ishr);  break;
        case binop::op_ushr: put_opc(opc::iushr); break;
        case binop::op_and:  put_opc(opc::iand);  break;
        case binop::op_or:   put_opc(opc::ior);   break;
        case binop::op_xor:  put_opc(opc::ixor);  break;
        default: assert(0);
        }
        break;
    }
    case type::t_long: {
        switch (op) {
        case binop::op_add:  put_opc(opc::ladd);  break;
        case binop::op_sub:  put_opc(opc::lsub);  break;
        case binop::op_mul:  put_opc(opc::lmul);  break;
        case binop::op_div:  put_opc(opc::ldiv);  break;
        case binop::op_rem:  put_opc(opc::lrem);  break;
        case binop::op_shl:  put_opc(opc::lshl);  break;
        case binop::op_shr:  put_opc(opc::lshr);  break;
        case binop::op_ushr: put_opc(opc::lushr); break;
        case binop::op_and:  put_opc(opc::land);  break;
        case binop::op_or:   put_opc(opc::lor);   break;
        case binop::op_xor:  put_opc(opc::lxor);  break;
        default: assert(0);
        }
        break;
    }
    case type::t_float: {
        switch (op) {
        case binop::op_add: put_opc(opc::fadd); break;
        case binop::op_sub: put_opc(opc::fsub); break;
        case binop::op_mul: put_opc(opc::fmul); break;
        case binop::op_div: put_opc(opc::fdiv); break;
        case binop::op_rem: put_opc(opc::frem); break;
        default: assert(0);
        }
        break;
    }
    case type::t_double: {
        switch (op) {
        case binop::op_add: put_opc(opc::dadd); break;
        case binop::op_sub: put_opc(opc::dsub); break;
        case binop::op_mul: put_opc(opc::dmul); break;
        case binop::op_div: put_opc(opc::ddiv); break;
        case binop::op_rem: put_opc(opc::drem); break;
        default: assert(0);
        }
        break;
    }
    default: assert(0);
    }
}

void interp_translator::op_iinc(uint8_t idx, jint value)
{
    put_opc(opc::iinc);
    put_const(idx);
    put_const(value);
}

void interp_translator::op_lcmp()
{
    put_opc(opc::lcmp);
}

void interp_translator::op_if(cmpop op, std::shared_ptr<basic_block> target)
{
    switch (op) {
    case cmpop::op_cmpeq: put_opc(opc::ifeq); break;
    case cmpop::op_cmpne: put_opc(opc::ifne); break;
    case cmpop::op_cmplt: put_opc(opc::iflt); break;
    case cmpop::op_cmpge: put_opc(opc::ifge); break;
    case cmpop::op_cmpgt: put_opc(opc::ifgt); break;
    case cmpop::op_cmple: put_opc(opc::ifle); break;
    default:              assert(0);
    }
    put_label(target);
}

void interp_translator::op_if_cmp(type t, cmpop op, std::shared_ptr<basic_block> bblock)
{
    switch (t) {
    case type::t_int: {
        switch (op) {
        case cmpop::op_cmpeq: put_opc(opc::if_icmpeq); break;
        case cmpop::op_cmpne: put_opc(opc::if_icmpne); break;
        case cmpop::op_cmplt: put_opc(opc::if_icmplt); break;
        case cmpop::op_cmpge: put_opc(opc::if_icmpge); break;
        case cmpop::op_cmpgt: put_opc(opc::if_icmpgt); break;
        case cmpop::op_cmple: put_opc(opc::if_icmple); break;
        default:              assert(0);
        }
        break;
    }
    case type::t_ref: {
        switch (op) {
        case cmpop::op_cmpeq: put_opc(opc::if_acmpeq); break;
        case cmpop::op_cmpne: put_opc(opc::if_acmpne); break;
        default:              assert(0);
        }
        break;
    }
    default: assert(0);
    }

    put_label(bblock);
}

void interp_translator::op_goto(std::shared_ptr<basic_block> bblock)
{
    put_opc(opc::goto_);

    put_label(bblock);
}

void interp_translator::op_ret()
{
    put_opc(opc::ret);
}

void interp_translator::op_ret_void()
{
    put_opc(opc::ret_void);
}

void interp_translator::op_getstatic(field* field)
{
    put_opc(opc::getstatic);
    put_const(field);
}

void interp_translator::op_putstatic(field* field)
{
    put_opc(opc::putstatic);
    put_const(field);
}

void interp_translator::op_getfield(field* field)
{
    put_opc(opc::getfield);
    put_const(field);
}

void interp_translator::op_putfield(field* field)
{
    put_opc(opc::putfield);
    put_const(field);
}

void interp_translator::op_invokevirtual(method* target)
{
    put_opc(opc::invokevirtual);
    put_const(target);
}

void interp_translator::op_invokespecial(method* target)
{
    put_opc(opc::invokespecial);
    put_const(target);
}

void interp_translator::op_invokestatic(method* target)
{
    put_opc(opc::invokestatic);
    put_const(target);
}

void interp_translator::op_invokeinterface(method* target)
{
    put_opc(opc::invokeinterface);
    put_const(target);
}

void interp_translator::op_new(klass* klass)
{
    put_opc(opc::new_);
    put_const(klass);
}

void interp_translator::op_newarray(uint8_t atype)
{
    put_opc(opc::newarray);
    put_const(atype);
}

void interp_translator::op_anewarray(klass* klass)
{
    put_opc(opc::anewarray);
    put_const(klass);
}

void interp_translator::op_arraylength()
{
    put_opc(opc::arraylength);
}

void interp_translator::op_checkcast(klass* klass)
{
    assert(0);
}

value_t interp_backend::execute(method* method, frame& frame)
{
    interp_translator translator(method);

    translator.translate();

    const char* code = translator.trampoline<const char*>();

    return interp(frame, code);
}

}
