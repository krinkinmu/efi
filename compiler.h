#ifndef __COMPILER_H__
#define __COMPILER_H__

#ifdef __x86_64__
#define ELFABI __attribute__((sysv_abi))
#else
#define ELFABI
#endif

#endif  // __COMPILER_H__
