#pragma once

#include <stdint.h>
#include <stdbool.h>
// @TODO We use snprintf from stdio, remove header, implement on our own?
#include <libc/stdio.h>
#include <libc/stdlib.h>
#include <libc/string.h>

#include "platform/platform.h"

typedef int64_t int64;
typedef int32_t int32;
typedef int16_t int16;
typedef int8_t int8;

typedef uint32_t uint;
typedef uint64_t u64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t uint8;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;

#ifdef _WIN32
    #define FL "%ll"
#else
    #define FL "%l"
#endif


#define ASSERT(E) ( (E) ? 0 : (log__printf("[ASSERT] %s:%d %s\n", __FILE__, __LINE__, #E), *((int*)0) = 5) )
