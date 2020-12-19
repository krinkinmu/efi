#ifndef __IO_H__
#define __IO_H__

#include <stddef.h>
#include <stdint.h>

#include "efi/efi.h"

efi_status_t efi_read_fixed(
	struct efi_file_protocol *file,
	uint64_t offset,
	size_t size,
	void *dst);

#endif  // __IO_H__
