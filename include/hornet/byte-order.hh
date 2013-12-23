#ifndef HORNET_BYTE_ORDER_HH
#define HORNET_BYTE_ORDER_HH

namespace hornet {

#ifdef __linux__
#include <endian.h>
#endif

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>

#define le16toh(x) OSSwapLittleToHostInt16(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)
#endif

static inline uint16_t le16_to_cpu(uint16_t x)
{
    return le16toh(x);
}

static inline uint32_t le32_to_cpu(uint32_t x)
{
    return le32toh(x);
}

static inline uint64_t le64_to_cpu(uint64_t x)
{
    return le64toh(x);
}

}

#endif
