#ifndef __CLIB_H__
#define __CLIB_H__

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

size_t strlen(const char *str);
size_t u16strlen(const uint16_t *str);
void *memcpy(void *dst, const void *src, size_t size);
void *memset(void *ptr, int value, size_t size);
int isdigit(int code);
int u16snprintf(uint16_t *buffer, size_t size, const char *fmt, ...);

#endif  // __CLIB_H__
