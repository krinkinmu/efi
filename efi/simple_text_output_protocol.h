#ifndef __EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_H__
#define __EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_H__

#include "types.h"

struct efi_simple_text_output_protocol
{
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

#endif // __EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_H__
