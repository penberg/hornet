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

bool llvm_backend::debug = false;

using namespace llvm;

Module*          module;
ExecutionEngine* engine;

Type* typeof(type t)
{
    switch (t) {
    case type::t_int:  return Type::getInt32Ty(getGlobalContext());
    case type::t_long: return Type::getInt64Ty(getGlobalContext());
    case type::t_ref:  return PointerType::get(Type::getInt8Ty(getGlobalContext()), 0);
    default:           assert(0);
    }
}

Instruction::BinaryOps to_binary_op(binop op)
{
    switch (op) {
    case binop::op_add: return Instruction::BinaryOps::Add;
    case binop::op_sub: return Instruction::BinaryOps::Sub;
    case binop::op_mul: return Instruction::BinaryOps::Mul;
    case binop::op_div: return Instruction::BinaryOps::SDiv;
    case binop::op_rem: return Instruction::BinaryOps::SRem;
    case binop::op_and: return Instruction::BinaryOps::And;
    case binop::op_or:  return Instruction::BinaryOps::Or;
    case binop::op_xor: return Instruction::BinaryOps::Xor;
    default:            assert(0);
    }
}

class llvm_translator : public translator {
public:
    llvm_translator(method* method);
    ~llvm_translator();

    template<typename T>
    T trampoline();

    virtual void prologue() override;
    virtual void epilogue() override;
    virtual void begin(std::shared_ptr<basic_block> bblock) override;
    virtual void op_const (type t, int64_t value) override;
    virtual void op_load  (type t, uint16_t idx) override;
    virtual void op_store (type t, uint16_t idx) override;
    virtual void op_arrayload(type t) override;
    virtual void op_arraystore(type t, uint16_t) override;
    virtual void op_pop() override;
    virtual void op_dup() override;
    virtual void op_dup_x1() override;
    virtual void op_swap() override;
    virtual void op_binary(type t, binop op) override;
    virtual void op_iinc(uint8_t idx, jint value) override;
    virtual void op_if_cmp(type t, cmpop op, std::shared_ptr<basic_block> bblock) override;
    virtual void op_goto(std::shared_ptr<basic_block> bblock) override;
    virtual void op_ret() override;
    virtual void op_ret_void() override;
    virtual void op_new() override;
    virtual void op_anewarray(uint16_t idx) override;
    virtual void op_invokestatic(method* target) override;
    virtual void op_invokevirtual(method* target) override;
    virtual void op_getstatic(field* target) override;
    virtual void op_putstatic(field* target) override;
    virtual void op_arraylength() override;

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
    : translator(method)
    , _locals(method->max_locals)
    , _builder(module->getContext())
    , _method(method)
{
    _func = function(_builder, _method);
}

llvm_translator::~llvm_translator()
{
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

void llvm_translator::prologue()
{
}

void llvm_translator::epilogue()
{
    if (llvm_backend::debug) {
        _func->dump();
    }
}

void llvm_translator::begin(std::shared_ptr<basic_block> bblock)
{
}

void llvm_translator::op_const(type t, int64_t value)
{
    auto c = ConstantInt::get(typeof(t), value, 0);

    _mimic_stack.push(c);
}

void llvm_translator::op_load(type t, uint16_t idx)
{
    auto local = lookup_local(idx, typeof(t));

    auto value = _builder.CreateLoad(local);

    _mimic_stack.push(value);
}

void llvm_translator::op_store(type t, uint16_t idx)
{
    auto value = _mimic_stack.top();
    _mimic_stack.pop();
    auto local = lookup_local(idx, typeof(t));
    _builder.CreateStore(value, local);
}

void llvm_translator::op_arrayload(type t)
{
    assert(0);
}

void llvm_translator::op_arraystore(type t, uint16_t)
{
    assert(0);
}

void llvm_translator::op_pop()
{
    assert(0);
}

void llvm_translator::op_dup()
{
    assert(0);
}

void llvm_translator::op_dup_x1()
{
    assert(0);
}

void llvm_translator::op_swap()
{
    assert(0);
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

void llvm_translator::op_iinc(uint8_t idx, jint value)
{
    assert(0);
}

void llvm_translator::op_if_cmp(type t, cmpop op, std::shared_ptr<basic_block> bblock)
{
    assert(0);
}

void llvm_translator::op_goto(std::shared_ptr<basic_block> bblock)
{
    assert(0);
}

void llvm_translator::op_ret()
{
    assert(0);
}

void llvm_translator::op_ret_void()
{
    _builder.CreateRetVoid();
}

void llvm_translator::op_invokestatic(method* target)
{
    assert(0);
}

void llvm_translator::op_invokevirtual(method* target)
{
    assert(0);
}

void llvm_translator::op_getstatic(field* target)
{
    assert(0);
}

void llvm_translator::op_putstatic(field* target)
{
    assert(0);
}

void llvm_translator::op_new()
{
    assert(0);
}

void llvm_translator::op_anewarray(uint16_t idx)
{
    assert(0);
}

void llvm_translator::op_arraylength()
{
    auto arrayref = _mimic_stack.top();
    _mimic_stack.pop();
    auto idx = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), offsetof(array, length), 0);
    auto gep = _builder.CreateGEP(arrayref, idx);
    auto addr = _builder.CreateBitCast(gep, PointerType::get(Type::getInt32Ty(getGlobalContext()), 0));
    auto len = _builder.CreateLoad(addr);
    _mimic_stack.push(len);
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
