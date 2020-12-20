#ifndef __LOG_H__
#define __LOG_H__

struct efi_system_table;

void info(struct efi_system_table *system, const char *fmt, ...);
void err(struct efi_system_table *system, const char *fmt, ...);

#endif  // __LOG_H__
