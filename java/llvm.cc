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

enum class type {
    t_int,
};

Module*          module;
ExecutionEngine* engine;

Type* typeof(type t)
{
    switch (t) {
    case type::t_int: return Type::getInt32Ty(getGlobalContext());
    default:          assert(0);
    }
}

Instruction::BinaryOps to_binary_op(binop op)
{
    switch (op) {
    case binop::op_add: return Instruction::BinaryOps::Add;
    default:            assert(0);
    }
}

class llvm_translator {
public:
    llvm_translator(method* method);
    ~llvm_translator();

    void translate();
    template<typename T>
    T trampoline();
    template<typename T>
    void op_const     (type t, T value);
    void op_load      (type t, uint16_t idx);
    void op_store     (type t, uint16_t idx);
    void op_binary    (type t, binop op);
    void op_returnvoid();

private:
    AllocaInst* lookup_local(unsigned int idx, Type* type);

    std::stack<Value*> _mimic_stack;
    std::vector<AllocaInst*> _locals;
    IRBuilder<> _builder;
    Function* _func;
    method* _method;
};

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

llvm_translator::llvm_translator(method* method)
    : _locals(method->max_locals)
    , _builder(module->getContext())
    , _method(method)
{
    _func = function(_builder, _method);
}

llvm_translator::~llvm_translator()
{
}

void llvm_translator::translate()
{
    uint16_t pc = 0;

next_insn:
    assert(pc < _method->code_length);

    uint8_t opc = _method->code[pc];

    switch (opc) {
    case JVM_OPC_iconst_m1:
    case JVM_OPC_iconst_0:
    case JVM_OPC_iconst_1:
    case JVM_OPC_iconst_2:
    case JVM_OPC_iconst_3:
    case JVM_OPC_iconst_4:
    case JVM_OPC_iconst_5: {
        jint value = opc - JVM_OPC_iconst_0;
        op_const<jint>(type::t_int, value);
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
    case JVM_OPC_istore_0:
    case JVM_OPC_istore_1:
    case JVM_OPC_istore_2:
    case JVM_OPC_istore_3: {
        uint16_t idx = opc - JVM_OPC_istore_0;
        op_store(type::t_int, idx);
        break;
    }
    case JVM_OPC_iadd: {
        op_binary(type::t_int, binop::op_add);
        break;
    }
    case JVM_OPC_return:
        op_returnvoid();
        return;
    default:
        fprintf(stderr, "error: unsupported bytecode: %u\n", opc);
        abort();
    }

    pc += opcode_length[opc];

    goto next_insn;
}

template<typename T> T llvm_translator::trampoline()
{
    verifyFunction(*_func, PrintMessageAction);
    return reinterpret_cast<T>(engine->getPointerToFunction(_func));
}

AllocaInst* llvm_translator::lookup_local(unsigned int idx, Type* type)
{
    if (_locals[idx]) {
        return _locals[idx];
    }
    IRBuilder<> builder(&_func->getEntryBlock(), _func->getEntryBlock().begin());
    auto ret = builder.CreateAlloca(type, nullptr, "");
    _locals[idx] = ret;
    return ret;
}

template<typename T>
void llvm_translator::op_const(type t, T value)
{
    auto c = ConstantInt::get(typeof(t), value, 0);

    _mimic_stack.push(c);
}

void llvm_translator::op_load(type t, uint16_t idx)
{
    auto value = _builder.CreateLoad(_locals[idx]);

    _mimic_stack.push(value);
}

void llvm_translator::op_store(type t, uint16_t idx)
{
    auto value = _mimic_stack.top();
    _mimic_stack.pop();
    auto local = lookup_local(idx, typeof(t));
    _builder.CreateStore(value, local);
}

void llvm_translator::op_binary(type t, binop op)
{
    auto value2 = _mimic_stack.top();
    _mimic_stack.pop();
    auto value1 = _mimic_stack.top();
    _mimic_stack.pop();
    auto result = _builder.CreateBinOp(to_binary_op(op), value1, value2);
    _mimic_stack.push(result);
}

void llvm_translator::op_returnvoid()
{
    _builder.CreateRetVoid();
}

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

value_t llvm_backend::execute(method* method, frame& frame)
{
    llvm_translator translator(method);

    translator.translate();

    auto fp = translator.trampoline<value_t (*)()>();

    return fp();
}

}
