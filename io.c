#include "io.h"

#include "log.h"


efi_status_t efi_read_fixed(
	struct efi_system_table *system,
	struct efi_file_protocol *file,
	uint64_t offset,
	size_t size,
	void *dst)
{
	efi_status_t status = EFI_SUCCESS;
	unsigned char *buf = dst;
	size_t read = 0;

	status = file->set_position(file, offset);
	if (status != EFI_SUCCESS) {
		err(
			system,
			"failed to set read position: %llu\r\n",
			(unsigned long long)status);
		return status;
	}

	while (read < size) {
		efi_uint_t remains = size - read;

		status = file->read(file, &remains, (void *)(buf + read));
		if (status != EFI_SUCCESS) {
			err(
				system,
				"read failed: %llu\r\n",
				(unsigned long long)status);
			return status;
		}

		read += remains;
	}

	return status;
}
