#include "efi.h"

efi_status_t efi_main(
	efi_handle_t handle, struct efi_system_table *system_table)
{
	uint16_t msg[] = u"Hello World!";
	efi_status_t status;

	status = system_table->out->clear_screen(system_table->out);
	if (status != 0)
		return status;

	status = system_table->out->output_string(system_table->out, msg);
	if (status != 0)
		return status;

	while (1);

	return 0;
}
