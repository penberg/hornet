#include "hornet/java.hh"

#include "hornet/translator.hh"
#include "hornet/vm.hh"

#include <sys/mman.h>
#include <cassert>

#include <classfile_constants.h>
#include <jni.h>

using namespace std;

namespace hornet {

static const int mmap_size = 4096;

class dynasm_translator : public translator {
public:
    dynasm_translator(method* method, dynasm_backend* backend);
    ~dynasm_translator();

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
    virtual void op_pop() override;
    virtual void op_dup() override;
    virtual void op_dup_x1() override;
    virtual void op_swap() override;
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
    dynasm_backend* ctx;
};

#define Dst             ctx
#define Dst_DECL        dynasm_backend *Dst
#define Dst_REF         (ctx->D)

#include <dasm_proto.h>
#include <dasm_x86.h>

#include "dynasm_x64.h"

dynasm_translator::dynasm_translator(method* method, dynasm_backend* backend)
    : translator(method)
    , ctx(backend)
{
}

dynasm_translator::~dynasm_translator()
{
}

template<typename T> T dynasm_translator::trampoline()
{
    size_t size;

    if (dasm_link(ctx, &size) != DASM_S_OK) {
        assert(0);
    }

    if (ctx->_offset + size >= mmap_size) {
        assert(0);
    }

    //
    // XXX: not thread safe
    //
    unsigned char* code = static_cast<unsigned char*>(ctx->_code) + ctx->_offset;
    ctx->_offset += size;

    dasm_encode(ctx, code);

    return reinterpret_cast<T>(code);
}

dynasm_backend::dynasm_backend()
    : _offset(0)
{
    dasm_init(this, DASM_MAXSECTION);

    dasm_setupglobal(this, nullptr, 0);

    dasm_setup(this, actions);

    _code = mmap(NULL, mmap_size, PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (_code == MAP_FAILED) {
        assert(0);
    }
}

dynasm_backend::~dynasm_backend()
{
    munmap(_code, mmap_size);

    dasm_free(this);
}

value_t dynasm_backend::execute(method* method, frame& frame)
{
    dynasm_translator translator(method, this);

    translator.translate();

    auto fp = translator.trampoline<value_t (*)()>();

    return fp();
}

}
