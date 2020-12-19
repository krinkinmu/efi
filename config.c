#include "loader.h"

#include "io.h"


efi_status_t load_config(struct loader *loader, const uint16_t *config_path)
{
	efi_status_t status = EFI_SUCCESS;
	struct efi_guid guid = EFI_FILE_INFO_GUID;
	struct efi_file_info file_info;
	efi_uint_t size;

	status = loader->rootdir->open(
		loader->rootdir,
		&loader->config,
		(uint16_t *)config_path,
		EFI_FILE_MODE_READ,
		EFI_FILE_READ_ONLY);
	if (status != EFI_SUCCESS)
		return status;

	size = sizeof(file_info);
	status = loader->config->get_info(
		loader->config,
		&guid,
		&size,
		(void *)&file_info);
	if (status != EFI_SUCCESS)
		return status;

	status = loader->system->boot->allocate_pool(
		EFI_LOADER_DATA,
		file_info.file_size + 1,
		(void **)&loader->config_data);
	if (status != EFI_SUCCESS)
		return status;

	memset(loader->config_data, 0, file_info.file_size + 1);
	loader->config->size = file_info.file_size;

	return efi_read_fixed(
		loader->config,
		/* offset */0,
		/* size */file_info.file_size,
		(void *)loader->config_data);
}
