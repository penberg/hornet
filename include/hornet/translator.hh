#ifndef HORNET_TRANSLATOR_HH
#define HORNET_TRANSLATOR_HH

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
    virtual void op_binary(type t, binop op) = 0;
    virtual void op_returnvoid() = 0;

protected:
    method* _method;
};

} // namespace hornet

#endif
