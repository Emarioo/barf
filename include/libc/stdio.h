#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

// @TODO WHAAAAT if 32-bit architecture!!??
typedef unsigned long long size_t;

int vsnprintf(char *str, size_t size, const char *fmt, va_list ap);

int snprintf(char *str, size_t size, const char *fmt, ...);
