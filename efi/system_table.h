#ifndef __EFI_SYSTEM_TABLE_H__
#define __EFI_SYSTEM_TABLE_H__

#include "boot_table.h"
#include "simple_text_output_protocol.h"
#include "types.h"

struct efi_system_table
{
	struct efi_table_header header;
	uint16_t *unused1;
	uint32_t unused2;
	void *unused3;
	void *unused4;
	void *unused5;
	struct efi_simple_text_output_protocol *out;
	void *unused6;
	struct efi_simple_text_output_protocol *err;
	void *unused8;
	struct efi_boot_table *boot;
	efi_uint_t unused10;
	void *unused11;
};

#endif // __EFI_SYSTEM_TABLE_H__
