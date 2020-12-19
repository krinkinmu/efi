#ifndef __LOADER_H__
#define __LOADER_H__

#include <stddef.h>
#include <stdint.h>

#include "efi/efi.h"

struct loader {
	struct efi_system_table *system;
	efi_handle_t handle;
	struct efi_loaded_image_protocol *image;
	efi_handle_t root_device;
	struct efi_simple_file_system_protocol *rootfs;
	struct efi_file_protocol *rootdir;

	struct efi_file_protocol *config;
	const char *config_data;
	size_t config_size;
};

efi_status_t setup_loader(
	efi_handle_t handle,
	struct efi_system_table *system,
	struct loader *loader);

/* Load the configuration from the specified path into memory. The path is
 * expected to point to a file in the same volume as the loader binary
 * itself. No attempts to verify/parse the config are made in this function. */
efi_status_t load_config(struct loader *loader, const uint16_t *config_path);

#endif  // __LOADER_H__
