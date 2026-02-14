#pragma once

#include <stdint.h>

typedef unsigned long long size_t;

size_t strlen(const char *a);

char* strcpy(char *dst, const char *src);

/* strcmp: compare two NUL-terminated strings */
int strcmp(const char *a, const char *b);

/* strncmp: compare up to n bytes of two NUL-terminated strings */
int strncmp(const char *a, const char *b, size_t n);

/* memcpy: copy n bytes from src to dst. Behavior for overlapping regions
   is undefined (like the standard memcpy). */
void *memcpy(void *dst, const void *src, size_t n);

/* memset: set n bytes of s to byte value c */
void *memset(void *s, int c, size_t n);

/* strchr: locate first occurrence of character c in string s */
char *strchr(const char *s, int c);

unsigned long strtoul(const char *nptr, char **endptr, int base);
