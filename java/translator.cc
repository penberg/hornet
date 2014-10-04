#include "hornet/translator.hh"

#include "hornet/java.hh"
#include "hornet/vm.hh"

#include <classfile_constants.h>
#include <jni.h>

#include <algorithm>
#include <cstdio>

using namespace std;

namespace hornet {

void translator::translate()
{
    scan();

    prologue();

    for (auto bblock : _bblock_list) {
        translate(bblock);
    }

    epilogue();
}

void translator::translate(std::shared_ptr<basic_block> bblock)
{
    uint16_t pc = bblock->start;

    begin(bblock);

next_insn:
    if (pc >= bblock->end) {
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
        case cp_tag::const_string: {
            auto value = const_pool->get_string(idx);
            op_const(type::t_ref, reinterpret_cast<int64_t>(value));
            break;
        }
        default:
            assert(0);
            break;
        }
        break;
    }
    case JVM_OPC_ldc2_w: {
        auto idx = read_opc_u2(_method->code + pc);
        auto const_pool = _method->klass->const_pool();
        auto cp_info = const_pool->get(idx);
        switch (cp_info->tag) {
        case cp_tag::const_long: {
            auto value = const_pool->get_long(idx);
            op_const(type::t_long, value);
            break;
        }
        case cp_tag::const_double: {
            auto value = const_pool->get_double(idx);
            op_const(type::t_double, value);
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
    case JVM_OPC_aaload: {
        op_arrayload(type::t_ref);
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
    case JVM_OPC_aastore: {
        uint16_t idx = read_opc_u2(_method->code + pc);
        op_arraystore(type::t_ref, idx);
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
        auto target = lookup(pc + offset);
        op_if_cmp(type::t_int, cmpop::op_cmpeq, target);
        break;
    }
    case JVM_OPC_if_icmpne: {
        int16_t offset = read_opc_u2(_method->code + pc);
        auto target = lookup(pc + offset);
        op_if_cmp(type::t_int, cmpop::op_cmpne, target);
        break;
    }
    case JVM_OPC_if_icmplt: {
        int16_t offset = read_opc_u2(_method->code + pc);
        auto target = lookup(pc + offset);
        op_if_cmp(type::t_int, cmpop::op_cmplt, target);
        break;
    }
    case JVM_OPC_if_icmpge: {
        int16_t offset = read_opc_u2(_method->code + pc);
        auto target = lookup(pc + offset);
        op_if_cmp(type::t_int, cmpop::op_cmpge, target);
        break;
    }
    case JVM_OPC_if_icmpgt: {
        int16_t offset = read_opc_u2(_method->code + pc);
        auto target = lookup(pc + offset);
        op_if_cmp(type::t_int, cmpop::op_cmpgt, target);
        break;
    }
    case JVM_OPC_if_icmple: {
        int16_t offset = read_opc_u2(_method->code + pc);
        auto target = lookup(pc + offset);
        op_if_cmp(type::t_int, cmpop::op_cmple, target);
        break;
    }
    case JVM_OPC_if_acmpeq: {
        int16_t offset = read_opc_u2(_method->code + pc);
        auto target = lookup(pc + offset);
        op_if_cmp(type::t_ref, cmpop::op_cmpeq, target);
        break;
    }
    case JVM_OPC_if_acmpne: {
        int16_t offset = read_opc_u2(_method->code + pc);
        auto target = lookup(pc + offset);
        op_if_cmp(type::t_ref, cmpop::op_cmpne, target);
        break;
    }
    case JVM_OPC_goto: {
        int16_t offset = read_opc_u2(_method->code + pc);
        auto target = lookup(pc + offset);
        op_goto(target);
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
    case JVM_OPC_invokevirtual: {
        uint16_t idx = read_opc_u2(_method->code + pc);
        auto target = _method->klass->resolve_method(idx);
        assert(target != nullptr);
        assert(!(target->access_flags & JVM_ACC_STATIC));
        op_invokevirtual(target.get());
        break;
    }
    case JVM_OPC_return: {
        op_ret_void();
        break;
    }
    case JVM_OPC_new: {
        uint16_t idx = read_opc_u2(_method->code + pc);
        auto klass = _method->klass->resolve_class(idx);
        assert(klass != nullptr);
        op_new(klass.get());
        break;
    }
    case JVM_OPC_anewarray: {
        uint16_t idx = read_opc_u2(_method->code + pc);
        op_anewarray(idx);
        break;
    }
    case JVM_OPC_arraylength: {
        op_arraylength();
        break;
    }
    case JVM_OPC_getstatic: {
        uint16_t idx = read_opc_u2(_method->code + pc);
        auto field = _method->klass->resolve_field(idx);
        op_getstatic(field.get());
        break;
    }
    case JVM_OPC_putstatic: {
        uint16_t idx = read_opc_u2(_method->code + pc);
        auto field = _method->klass->resolve_field(idx);
        op_putstatic(field.get());
        break;
    }
    default:
        fprintf(stderr, "error: unsupported bytecode: %u\n", opc);
        abort();
    }

    pc += opcode_length[opc];

    goto next_insn;
}

std::shared_ptr<basic_block> translator::lookup(uint16_t offset)
{
    auto it = _bblock_map.find(offset);

    assert(it != _bblock_map.end());

    return it->second;
}

std::shared_ptr<basic_block> translator::lookup_contains(uint16_t offset)
{
    for (auto bblock : _bblock_list) {
        if (bblock->start <= offset && offset < bblock->end) {
            return bblock;
        }
    }
    assert(0);
}

static bool is_branch(uint8_t opc)
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

static bool is_return(uint8_t opc)
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

static bool is_throw(uint8_t opc)
{
    switch (opc) {
    case JVM_OPC_athrow:
        return true;
    default:
        return false;
    }
}

static bool is_bblock_end(uint8_t opc)
{
    return is_branch(opc) || is_return(opc) || is_throw(opc);
}

static uint16_t branch_target(char* code, uint16_t pos)
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

// Functor for sorting basic blocks wrapped in shared pointers by start
// address.
//
// XXX: Use boost indirect iterators here?
struct SortFunctor {
    bool operator()(shared_ptr<basic_block> i, shared_ptr<basic_block> j) {
        return i->start < j->start;
    }
};

void translator::scan()
{
    auto bblock = std::make_shared<basic_block>(0, _method->code_length);

    _bblock_map.insert({0, bblock});
    _bblock_list.push_back(bblock);

    uint16_t pos = 0;
    while (pos < bblock->end) {
        uint8_t opc = _method->code[pos];
        pos += opcode_length[opc];
        if (is_bblock_end(opc)) {
            bblock = bblock->split_at(pos);
            _bblock_map.insert({pos, bblock});
            _bblock_list.push_back(bblock);
        }
    }
    pos = 0;
    while (pos < _method->code_length) {
        uint8_t opc = _method->code[pos];
        if (is_branch(opc)) {
            auto target_pc = branch_target(_method->code, pos);
            auto bblock = lookup_contains(target_pc);
            if (bblock->start != target_pc) {
                auto target = bblock->split_at(target_pc);
                _bblock_map.insert({target_pc, target});
                _bblock_list.push_back(target);
            }
        }
        pos += opcode_length[opc];
    }

    sort(_bblock_list.begin(), _bblock_list.end(), SortFunctor());
}

}
