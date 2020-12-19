#ifndef __LOADER_H__
#define __LOADER_H__

#include <stddef.h>
#include <stdint.h>

#include "efi/efi.h"


/* Each module describes a file that should be loaded in memory. Some files
 * might require a somewhat involved loading process, like for example ELF
 * binaries, but most of them will be opaque data blobs for the loader. The
 * only goal for the loader is to just read it in memory and pass on to the
 * ELF binary. */
struct module {
	const uint16_t *path;
	const char *name;
};

struct loader {
	struct efi_system_table *system;
	efi_handle_t handle;
	struct efi_loaded_image_protocol *image;
	efi_handle_t root_device;
	struct efi_simple_file_system_protocol *rootfs;
	struct efi_file_protocol *rootdir;

	struct efi_file_protocol *config;
	const char *config_data;

	/* The list of modules that have to be loaded according to the config
	 * file. One of them is a dedicated ELF kernel binary that we will
	 * pass control to after loading. */
	struct module *module;
	size_t module_capacity;
	size_t modules;
	size_t kernel;
};

efi_status_t setup_loader(
	efi_handle_t handle,
	struct efi_system_table *system,
	struct loader *loader);

/* Load the configuration from the specified path into memory. The path is
 * expected to point to a file in the same volume as the loader binary
 * itself. No attempts to verify/parse the config are made in this function. */
efi_status_t load_config(struct loader *loader, const uint16_t *config_path);

/* Parse the config loaded in memory and populate the list of modules to load
 * in memory in the loader structure.
 *
 * The format of the config is rather simplistic - it's just a combination of
 * keys and values. Values are separated from keys by ':' and the key:value
 * paris are separated from each other by whitespace characters. */
efi_status_t parse_config(struct loader *loader);

#endif  // __LOADER_H__
