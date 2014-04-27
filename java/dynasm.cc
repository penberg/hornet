#include "hornet/java.hh"

#include "hornet/vm.hh"

#include <sys/mman.h>
#include <cassert>

#include <classfile_constants.h>
#include <jni.h>

using namespace std;

namespace hornet {

#define Dst             ctx
#define Dst_DECL        dynasm_backend *Dst
#define Dst_REF         (ctx->D)

#include <dasm_proto.h>
#include <dasm_x86.h>

#include "dynasm_x64.h"

static const int mmap_size = 4096;

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
        op_iconst(this, value);
        break;
    }
    case JVM_OPC_return:
        op_ret(this);
        goto exit;
    default:
        fprintf(stderr, "error: unsupported bytecode: %u\n", opc);
        abort();
    }

    frame.pc += opcode_length[opc];

    goto next_insn;
exit:
    size_t size;

    if (dasm_link(this, &size) != DASM_S_OK) {
        assert(0);
    }

    if (_offset + size >= mmap_size) {
        assert(0);
    }

    //
    // XXX: not thread safe
    //
    unsigned char* code = static_cast<unsigned char*>(_code) + _offset;
    _offset += size;

    dasm_encode(this, code);

    auto fp = reinterpret_cast<value_t (*)()>(code);

    return fp();
}

}
