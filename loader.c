#include "loader.h"

#include "clib.h"


static efi_status_t get_loader_image(
	efi_handle_t loader,
	struct efi_system_table *system,
	struct efi_loaded_image_protocol **image)
{
	struct efi_guid guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;

	return system->boot->open_protocol(
		loader,
		&guid,
		(void **)image,
		loader,
		NULL,
		EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
}

static efi_status_t get_rootfs(
	efi_handle_t loader,
	struct efi_system_table *system,
	efi_handle_t device,
	struct efi_simple_file_system_protocol **rootfs)
{
	struct efi_guid guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

	return system->boot->open_protocol(
		device,
		&guid,
		(void **)rootfs,
		loader,
		NULL,
		EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
}

static efi_status_t get_rootdir(
	struct efi_simple_file_system_protocol *rootfs,
	struct efi_file_protocol **rootdir)
{
	return rootfs->open_volume(rootfs, rootdir);
}

efi_status_t setup_loader(
	efi_handle_t handle,
	struct efi_system_table *system,
	struct loader *loader)
{
	efi_status_t status = EFI_SUCCESS;

	memset(loader, 0, sizeof(*loader));
	loader->system = system;
	loader->handle = handle;

	status = get_loader_image(handle, system, &loader->image);
	if (status != EFI_SUCCESS)
		return status;

	loader->root_device = loader->image->device;
	status = get_rootfs(
		handle, system, &loader->root_device, &loader->rootfs);
	if (status != EFI_SUCCESS)
		return status;

	return get_rootdir(loader->rootfs, &loader->rootdir);
}
