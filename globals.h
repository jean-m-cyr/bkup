#pragma once

#include <stdint.h>

typedef struct
{
    uint8_t backup : 1;
    uint8_t compress : 1;
    uint8_t debug : 2;
} flags_t;

extern flags_t flags;

extern uint16_t sector_size;
extern uint32_t device_size;

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__

static inline uint32_t le32(uint32_t v)
{
    return __builtin_bswap32(v);
}

static inline uint16_t le16(uint16_t v)
{
    return __builtin_bswap16(v);
}

#else

static inline uint32_t le32(uint32_t v)
{
    return v;
}

static inline uint16_t le16(uint16_t v)
{
    return v;
}

#endif  // __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
