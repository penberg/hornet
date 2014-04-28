#include "hornet/java.hh"

#include "hornet/translator.hh"
#include "hornet/vm.hh"

#include <cassert>

#include <classfile_constants.h>
#include <jni.h>

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"

using namespace std;

namespace hornet {

using namespace llvm;

Module* module;
ExecutionEngine* engine;

struct llvm_context {
    llvm_context(Function* func_, IRBuilder<>& builder_,
                 std::stack<Value*>& mimic_stack_, unsigned int max_locals)
        : func(func_)
        , builder(builder_)
        , mimic_stack(mimic_stack_)
        , locals(max_locals)
    { }

    Function*                func;
    IRBuilder<>&             builder;
    std::stack<Value*>&      mimic_stack;
    std::vector<AllocaInst*> locals;
};

llvm_backend::llvm_backend()
{
    InitializeNativeTarget();
    module = new Module("JIT", getGlobalContext());
    auto engine_builder = EngineBuilder(module);
    std::string error_str;
    engine_builder.setErrorStr(&error_str);
    engine = engine_builder.create();
    if (!engine) {
        throw std::runtime_error(error_str);
    }
}

llvm_backend::~llvm_backend()
{
    delete engine;
}

FunctionType* function_type(IRBuilder<>& builder, method* method)
{
    return FunctionType::get(builder.getVoidTy(), false);
}

Function* function(IRBuilder<>& builder, method* method)
{
    auto func_type = function_type(builder, method);
    auto func = Function::Create(func_type, Function::ExternalLinkage, method->name, module);
    auto entry = BasicBlock::Create(builder.getContext(), "entry", func);
    builder.SetInsertPoint(entry);
    return func;
}

template<typename T> T trampoline(Function* func)
{
    return reinterpret_cast<T>(engine->getPointerToFunction(func));
}

static AllocaInst* lookup_local(llvm_context& ctx, unsigned int idx, Type* type)
{
    if (ctx.locals[idx]) {
        return ctx.locals[idx];
    }
    IRBuilder<> builder(&ctx.func->getEntryBlock(), ctx.func->getEntryBlock().begin());
    auto ret = builder.CreateAlloca(type, nullptr, "");
    ctx.locals[idx] = ret;
    return ret;
}

enum class type {
    t_int,
};

Type* typeof(type t)
{
    switch (t) {
    case type::t_int: return Type::getInt32Ty(getGlobalContext());
    default:          assert(0);
    }
}

template<typename T>
void op_const(llvm_context& ctx, type t, T value)
{
    auto c = ConstantInt::get(typeof(t), value, 0);

    ctx.mimic_stack.push(c);
}

void op_load(llvm_context& ctx, type t, uint16_t idx)
{
    auto value = ctx.builder.CreateLoad(ctx.locals[idx]);

    ctx.mimic_stack.push(value);
}

void op_store(llvm_context& ctx, type t, uint16_t idx)
{
    auto value = ctx.mimic_stack.top();
    ctx.mimic_stack.pop();
    auto local = lookup_local(ctx, idx, typeof(t));
    ctx.builder.CreateStore(value, local);
}

Instruction::BinaryOps to_binary_op(binop op)
{
    switch (op) {
    case binop::op_add: return Instruction::BinaryOps::Add;
    default:            assert(0);
    }
}

void op_binary(llvm_context& ctx, type t, binop op)
{
    auto value2 = ctx.mimic_stack.top();
    ctx.mimic_stack.pop();
    auto value1 = ctx.mimic_stack.top();
    ctx.mimic_stack.pop();
    auto result = ctx.builder.CreateBinOp(to_binary_op(op), value1, value2);
    ctx.mimic_stack.push(result);
}

value_t llvm_backend::execute(method* method, frame& frame)
{
    IRBuilder<> builder(module->getContext());

    std::stack<llvm::Value*> mimic_stack;

    auto func = function(builder, method);

    llvm_context ctx(func, builder, mimic_stack, method->max_locals);

next_insn:
    assert(frame.pc < method->code_length);

    uint8_t opc = method->code[frame.pc];

    switch (opc) {
    case JVM_OPC_iconst_m1:
    case JVM_OPC_iconst_0:
    case JVM_OPC_iconst_1:
    case JVM_OPC_iconst_2:
    case JVM_OPC_iconst_3:
    case JVM_OPC_iconst_4:
    case JVM_OPC_iconst_5: {
        jint value = opc - JVM_OPC_iconst_0;
        op_const<jint>(ctx, type::t_int, value);
        break;
    }
    case JVM_OPC_iload_0:
    case JVM_OPC_iload_1:
    case JVM_OPC_iload_2:
    case JVM_OPC_iload_3: {
        uint16_t idx = opc - JVM_OPC_iload_0;
        op_load(ctx, type::t_int, idx);
        break;
    }
    case JVM_OPC_istore_0:
    case JVM_OPC_istore_1:
    case JVM_OPC_istore_2:
    case JVM_OPC_istore_3: {
        uint16_t idx = opc - JVM_OPC_istore_0;
        op_store(ctx, type::t_int, idx);
        break;
    }
    case JVM_OPC_iadd: {
        op_binary(ctx, type::t_int, binop::op_add);
        break;
    }
    case JVM_OPC_return:
        builder.CreateRetVoid();
        goto exit;
    default:
        fprintf(stderr, "error: unsupported bytecode: %u\n", opc);
        abort();
    }

    frame.pc += opcode_length[opc];

    goto next_insn;
exit:
    verifyFunction(*func, PrintMessageAction);

    auto fp = trampoline<value_t (*)()>(func);
    return fp();
}

}
