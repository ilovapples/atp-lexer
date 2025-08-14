#ifndef TYPES_H
#define TYPES_H

#include <inttypes.h>


typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;


typedef s16 sc16;
typedef u16 uc16;
typedef s32 sc32;
typedef u32 uc32;

typedef s32 fd_t;

typedef s32 err32_t;
typedef s64 err64_t;

#ifndef DISCLUDE_VOLATILE_TYPES
typedef volatile int8_t vs8;
typedef volatile int16_t vs16;
typedef volatile int32_t vs32;
typedef volatile int64_t vs64;

typedef volatile uint8_t vu8;
typedef volatile uint16_t vu16;
typedef volatile uint32_t vu32;
typedef volatile uint64_t vu64;


typedef volatile s16 vsc16;
typedef volatile u16 vuc16;
typedef volatile s32 vsc32;
typedef volatile u32 vuc32;

typedef volatile s32 vfd_t;

typedef volatile s32 verr32_t;
typedef volatile s64 verr64_t;

#endif

#endif /* TYPES_H */
