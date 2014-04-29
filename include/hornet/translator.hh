#ifndef HORNET_TRANSLATOR_HH
#define HORNET_TRANSLATOR_HH

#include <jni.h>

#include <cstdint>

namespace hornet {

struct method;

enum class type {
    t_int,
    t_long,
    t_ref,
};

enum class binop {
    op_add,
    op_sub,
    op_mul,
    op_div,
    op_rem,
    op_and,
    op_or,
    op_xor,
};

enum class cmpop {
    op_cmpeq,
    op_cmpne,
    op_cmplt,
    op_cmpge,
    op_cmpgt,
    op_cmple,
};

class translator {
public:
    translator(method* method)
        : _method(method)
    { }

    virtual ~translator() { }

    void translate();

    virtual void prologue () = 0;
    virtual void op_const (type t, int64_t value) = 0;
    virtual void op_load  (type t, uint16_t idx) = 0;
    virtual void op_store (type t, uint16_t idx) = 0;
    virtual void op_pop() = 0;
    virtual void op_dup() = 0;
    virtual void op_dup_x1() = 0;
    virtual void op_swap() = 0;
    virtual void op_binary(type t, binop op) = 0;
    virtual void op_iinc(uint8_t idx, jint value) = 0;
    virtual void op_if_cmp(type t, cmpop op, int16_t offset) = 0;
    virtual void op_goto(int16_t offset) = 0;
    virtual void op_ret() = 0;
    virtual void op_ret_void() = 0;
    virtual void op_invokestatic(method* target) = 0;
    virtual void op_new() = 0;
    virtual void op_arraylength() = 0;

protected:
    method* _method;
};

} // namespace hornet

#endif
