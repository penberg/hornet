#include "hornet/java.hh"

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

bool llvm_enable;

Module* module;
ExecutionEngine* engine;

void llvm_init()
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

void llvm_exit()
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

void llvm_interp(method* method, frame& frame)
{
    IRBuilder<> builder(module->getContext());

    std::stack<llvm::Value*> mimic_stack;

    auto func = function(builder, method);

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
        auto c = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), value, 0);
        mimic_stack.push(c);
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

    auto fp = trampoline<void (*)()>(func);
    fp();
}

}
