#ifndef __EFI_BOOT_TABLE_H__
#define __EFI_BOOT_TABLE_H__

#include "types.h"

static const uint32_t EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL = 0x00000001;

struct efi_boot_table
{
	struct efi_table_header header;

	// Task Priority Services
	void (*unused1)();
	void (*unused2)();

	// Memory Services
	void (*unused3)();
	void (*unused4)();
	void (*unused5)();

	efi_status_t (*allocate_pool)(enum efi_memory_type, efi_uint_t, void **);
	efi_status_t (*free_pool)(void *);

	// Event & Timer Services
	void (*unused7)();
	void (*unused8)();
	void (*unused9)();
	void (*unused10)();
	void (*unused11)();
	void (*unused12)();

	// Protocol Handler Services
	void (*unused13)();
	void (*unused14)();
	void (*unused15)();
	void (*unused16)();
	void *reserved;
	void (*unused17)();
	void (*unused18)();
	void (*unused19)();
	void (*unused20)();

	// Image Services
	void (*unused21)();
	void (*unused22)();
	void (*unused23)();
	void (*unused24)();
	void (*unused25)();

	// Miscellaneius Services
	void (*unused26)();
	void (*unused27)();
	void (*unused28)();

	// DriverSupport Services
	void (*unused29)();
	void (*unused30)();

	// Open and Close Protocol Services
	efi_status_t (*open_protocol)(
		efi_handle_t,
		struct efi_guid *,
		void **,
		efi_handle_t,
		efi_handle_t,
		uint32_t);
	efi_status_t (*close_protocol)(
		efi_handle_t,
		struct efi_guid *,
		efi_handle_t,
		efi_handle_t);
	void (*unused33)();

	// Library Services
	efi_status_t (*protocols_per_handle)(
		efi_handle_t, struct efi_guid ***, efi_uint_t *);
	void (*unused35)();
	void (*unused36)();
	void (*unused37)();
	void (*unused38)();

	// 32-bit CRC Services
	void (*unused39)();

	// Miscellaneius Services (cont)
	void (*unused40)();
	void (*unused41)();
	void (*unused42)();
};

#endif // __EFI_BOOT_TABLE_H__
