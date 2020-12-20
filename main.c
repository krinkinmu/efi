#include "efi/efi.h"
#include "loader.h"
#include "log.h"


efi_status_t efi_main(
	efi_handle_t handle,
	struct efi_system_table *system)
{
	uint16_t config_path[] = u"efi\\boot\\config.txt";
	struct loader loader;
	efi_status_t status;

	info(system, "Setting up the loader...\r\n");
	status = setup_loader(handle, system, &loader);
	if (status != EFI_SUCCESS)
		return status;

	info(system, "Loading the config...\r\n");
	status = load_config(&loader, config_path);
	if (status != EFI_SUCCESS)
		return status;

	info(system, "Parsing the config...\r\n");
	status = parse_config(&loader);
	if (status != EFI_SUCCESS)
		return status;

	info(system, "Loading the kernel...\r\n");
	status = load_kernel(&loader);
	if (status != EFI_SUCCESS)
		return status;

	info(system, "Loading the data...\r\n");
	status = load_modules(&loader);
	if (status != EFI_SUCCESS)
		return status;

	info(system, "Starting the kernel...\r\n");
	status = start_kernel(&loader);
	if (status != EFI_SUCCESS)
		return status;

	return EFI_SUCCESS;
}
