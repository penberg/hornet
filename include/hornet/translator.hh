#ifndef HORNET_TRANSLATOR_HH
#define HORNET_TRANSLATOR_HH

#include <jni.h>

#include <cstdint>
#include <memory>
#include <vector>
#include <map>

namespace hornet {

struct method;
struct field;

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

struct basic_block {
    uint16_t start;
    uint16_t end;

    basic_block(uint16_t start_, uint16_t end_)
        : start(start_), end(end_)
    { }
    basic_block(basic_block const&) = delete;
    basic_block& operator=(basic_block const&) = delete;

    std::shared_ptr<basic_block> split_at(uint16_t pos) {
        auto next = std::make_shared<basic_block>(pos, end);

        end = pos;

        return next;
    }
};

class translator {
public:
    translator(method* method)
        : _method(method)
    { }

    virtual ~translator() { }

    void translate();

protected:
    void scan();

    void translate(std::shared_ptr<basic_block> bblock);

    std::shared_ptr<basic_block> lookup(uint16_t offset);
    std::shared_ptr<basic_block> lookup_contains(uint16_t offset);

    virtual void prologue () = 0;
    virtual void epilogue () = 0;
    virtual void begin(std::shared_ptr<basic_block> bblock) = 0;
    virtual void op_const (type t, int64_t value) = 0;
    virtual void op_load  (type t, uint16_t idx) = 0;
    virtual void op_store (type t, uint16_t idx) = 0;
    virtual void op_arrayload(type t) = 0;
    virtual void op_arraystore(type t, uint16_t ix) = 0;
    virtual void op_pop() = 0;
    virtual void op_dup() = 0;
    virtual void op_dup_x1() = 0;
    virtual void op_swap() = 0;
    virtual void op_binary(type t, binop op) = 0;
    virtual void op_iinc(uint8_t idx, jint value) = 0;
    virtual void op_if_cmp(type t, cmpop op, std::shared_ptr<basic_block> target) = 0;
    virtual void op_goto(std::shared_ptr<basic_block> target) = 0;
    virtual void op_ret() = 0;
    virtual void op_ret_void() = 0;
    virtual void op_invokestatic(method* target) = 0;
    virtual void op_invokevirtual(method* target) = 0;
    virtual void op_getstatic(field* target) = 0;
    virtual void op_putstatic(field* target) = 0;
    virtual void op_new() = 0;
    virtual void op_anewarray(uint16_t idx) = 0;
    virtual void op_arraylength() = 0;

    method* _method;
    std::map<uint16_t, std::shared_ptr<basic_block>> _bblock_map;
    std::vector<std::shared_ptr<basic_block>> _bblock_list;
};

} // namespace hornet

#endif
