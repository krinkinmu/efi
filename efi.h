#ifndef __EFI_H__
#define __EFI_H__

#include <stdint.h>
#include <stdbool.h>

typedef void *efi_handle_t;
typedef uint64_t efi_status_t;
typedef uint64_t efi_uint_t;

struct efi_table_header {
	uint64_t signature;
	uint32_t revision;
	uint32_t header_size;
	uint32_t crc32;
	uint32_t reserved;
};

struct efi_simple_text_output_protocol {
	efi_status_t (*unused1)(
			struct efi_simple_text_output_protocol *,
			bool);

	efi_status_t (*output_string)(
			struct efi_simple_text_output_protocol *,
			uint16_t *);

	efi_status_t (*unused2)(
			struct efi_simple_text_output_protocol *,
			uint16_t *);
	efi_status_t (*unused3)(
			struct efi_simple_text_output_protocol *,
			efi_uint_t, efi_uint_t *, efi_uint_t *);
	efi_status_t (*unused4)(
			struct efi_simple_text_output_protocol *,
			efi_uint_t);
	efi_status_t (*unused5)(
			struct efi_simple_text_output_protocol *,
			efi_uint_t);

	efi_status_t (*clear_screen)(
			struct efi_simple_text_output_protocol *);

	efi_status_t (*unused6)(
			struct efi_simple_text_output_protocol *,
			efi_uint_t, efi_uint_t);
	efi_status_t (*unused7)(
			struct efi_simple_text_output_protocol *,
			bool);

	void *unused8;
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
	void *unused9;
	efi_uint_t unused10;
	void *unused11;
};

#endif  // __EFI_H__
