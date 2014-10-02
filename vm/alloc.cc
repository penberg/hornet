#include "hornet/vm.hh"

#include "hornet/java.hh"

extern "C" {
#include "../mps/mps.h"
#include "../mps/mpsavm.h"
#include "../mps/mpscamc.h"
}

#include <cassert>
#include <cstdlib>

namespace hornet {

void out_of_memory()
{
    fprintf(stderr, "error: out of memory\n");
    abort();
}

static mps_ap_t obj_ap;

object* gc_new_object(klass* klass)
{
    size_t size = sizeof(object);
    mps_addr_t addr;
    do {
        mps_res_t res = mps_reserve(&addr, obj_ap, size);
        if (res != MPS_RES_OK)
            assert(0);
    } while (!mps_commit(obj_ap, addr, size));
    return new (addr) object{klass};
}

array* gc_new_object_array(klass* klass, size_t length)
{
    size_t size = sizeof(array) + length * sizeof(void*);
    mps_addr_t addr;
    do {
        mps_res_t res = mps_reserve(&addr, obj_ap, size);
        if (res != MPS_RES_OK)
            assert(0);
    } while (!mps_commit(obj_ap, addr, size));
    return new (addr) array{klass, static_cast<uint32_t>(length)};
}

static void obj_pad(mps_addr_t addr, size_t size)
{
    // XXX: padding objects?
    assert(0);
}

static mps_addr_t obj_isfwd(mps_addr_t addr)
{
    auto obj = static_cast<object*>(addr);

    return static_cast<mps_addr_t>(obj->fwd);
}

static void obj_fwd(mps_addr_t old, mps_addr_t new_)
{
    auto obj = static_cast<object*>(old);

    obj->fwd = static_cast<object*>(new_);
}

static mps_res_t obj_scan(mps_ss_t ss, mps_addr_t base, mps_addr_t limit)
{
    // XXX: objects have no fields
    // XXX: arrays need to be scanned
    assert(0);
    return MPS_RES_OK;
}

static mps_addr_t obj_skip(mps_addr_t base)
{
    auto end = static_cast<char*>(base) + sizeof(object);

    return static_cast<mps_addr_t>(end);
}

static mps_res_t globals_scan(mps_ss_t ss, void *p, size_t s)
{
    assert(0);
}

void gc_init()
{
    static mps_arena_t arena;
    mps_res_t res;
    MPS_ARGS_BEGIN(args) {
        MPS_ARGS_ADD(args, MPS_KEY_ARENA_SIZE, 32 * 1024 * 1024);
        res = mps_arena_create_k(&arena, mps_arena_class_vm(), args);
    } MPS_ARGS_END(args);
    if (res != MPS_RES_OK)
        assert(0);

    mps_fmt_t obj_fmt;
    MPS_ARGS_BEGIN(args) {
        MPS_ARGS_ADD(args, MPS_KEY_FMT_ALIGN, alignof(object));
        MPS_ARGS_ADD(args, MPS_KEY_FMT_SCAN,  obj_scan);
        MPS_ARGS_ADD(args, MPS_KEY_FMT_SKIP,  obj_skip);
        MPS_ARGS_ADD(args, MPS_KEY_FMT_FWD,   obj_fwd);
        MPS_ARGS_ADD(args, MPS_KEY_FMT_ISFWD, obj_isfwd);
        MPS_ARGS_ADD(args, MPS_KEY_FMT_PAD,   obj_pad);
        res = mps_fmt_create_k(&obj_fmt, arena, args);
    } MPS_ARGS_END(args);
    if (res != MPS_RES_OK)
        assert(0);

    mps_pool_t obj_pool;
    MPS_ARGS_BEGIN(args) {
        MPS_ARGS_ADD(args, MPS_KEY_FORMAT, obj_fmt);
        res = mps_pool_create_k(&obj_pool, arena, mps_class_amc(), args);
    } MPS_ARGS_END(args);
    if (res != MPS_RES_OK)
        assert(0);

    res = mps_ap_create_k(&obj_ap, obj_pool, mps_args_none);

    if (res != MPS_RES_OK)
        assert(0);

    mps_root_t globals_root;
    res = mps_root_create(&globals_root, arena, mps_rank_exact(), 0, globals_scan, nullptr, 0);
    if (res != MPS_RES_OK)
        assert(0);
}

}
