#ifndef __CLIB_H__
#define __CLIB_H__

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

size_t strlen(const char *str);
int strcmp(const char *l, const char *r);

char *strncpy(char *dst, const char *src, size_t size);
void *memcpy(void *dst, const void *src, size_t size);
void *memset(void *ptr, int value, size_t size);

int isdigit(int code);
int isalpha(int code);
int isalnum(int code);
int isspace(int code);

int vsnprintf(uint16_t *buffer, size_t size, const char *fmt, va_list args);
int u16snprintf(uint16_t *buffer, size_t size, const char *fmt, ...);
size_t u16strlen(const uint16_t *str);
uint16_t *to_u16strncpy(uint16_t *dst, const char *src, size_t size);

#endif  // __CLIB_H__
