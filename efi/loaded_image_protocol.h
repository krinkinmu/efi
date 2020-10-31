#ifndef __EFI_LOADED_IMAGE_PROTOCOL_H__
#define __EFI_LOADED_IMAGE_PROTOCOL_H__

#include "device_path_protocol.h"
#include "types.h"

#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
	{ 0x5b1b31a1, 0x9562, 0x11d2, \
	  { 0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } }

struct efi_loaded_image_protocol {
	uint32_t revision;
	efi_handle_t parent;
	struct efi_system_table *system;

	// Source location of the image
	efi_handle_t device;
	struct efi_device_path_protocol *filepath;
	void *reserved;

	// Image's load options
	uint32_t load_options_size;
	void *load_options;

	// Location of the image in memory
	void *image_base;
	uint64_t image_size;
	enum efi_memory_type image_code_type;
	enum efi_memory_type image_data_type;
	void (*unused)();
};

#endif // __EFI_LOADED_IMAGE_PROTOCOL_H__
