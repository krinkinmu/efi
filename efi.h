#ifndef __EFI_H__
#define __EFI_H__

#include <stdint.h>
#include <stdbool.h>

typedef void *efi_handle_t;
typedef uint64_t efi_status_t;
typedef uint64_t efi_uint_t;

struct efi_guid {
	uint32_t data1;
	uint16_t data2;
	uint16_t data3;
	uint8_t data4[8];
};

struct efi_table_header {
	uint64_t signature;
	uint32_t revision;
	uint32_t header_size;
	uint32_t crc32;
	uint32_t reserved;
};

struct efi_simple_text_output_protocol {
	void (*unused1)();

	efi_status_t (*output_string)(
			struct efi_simple_text_output_protocol *,
			uint16_t *);

	void (*unused2)();
	void (*unused3)();
	void (*unused4)();
	void (*unused5)();

	efi_status_t (*clear_screen)(
			struct efi_simple_text_output_protocol *);

	void (*unused6)();
	void (*unused7)();

	void *unused8;
};

struct efi_boot_services {
	struct efi_table_header header;

	// Task Priority Services
	void (*unused1)();
	void (*unused2)();

	// Memory Services
	void (*unused3)();
	void (*unused4)();
	void (*unused5)();
	void (*unused6)();
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
	void (*unused31)();
	void (*unused32)();
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

struct efi_system_table {
	struct efi_table_header header;
	uint16_t *unused1;
	uint32_t unused2;
	void *unused3;
	void *unused4;
	void *unused5;
	struct efi_simple_text_output_protocol *out;
	void *unused6;
	void *unused7;
	void *unused8;
	struct efi_boot_services *boot;
	efi_uint_t unused10;
	void *unused11;
};

#endif  // __EFI_H__
