#include "clib.h"
#include "efi/efi.h"

static efi_status_t efi_guid_print(
	struct efi_simple_text_output_protocol *out,
	const struct efi_guid *guid)
{
	uint16_t buf[128];

	u16snprintf(
		buf,
		sizeof(buf)/sizeof(buf[0]),
		"{ 0x%x, 0x%x, 0x%x, {"
		" 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x "
		"} }",
		(unsigned)guid->data1,
		(unsigned)guid->data2,
		(unsigned)guid->data3,
		(unsigned)guid->data4[0],
		(unsigned)guid->data4[1],
		(unsigned)guid->data4[2],
		(unsigned)guid->data4[3],
		(unsigned)guid->data4[4],
		(unsigned)guid->data4[5],
		(unsigned)guid->data4[6],
		(unsigned)guid->data4[7],
		(unsigned)guid->data4[8]);
	return out->output_string(out, buf);
}

static efi_status_t efi_device_path_print(
	struct efi_simple_text_output_protocol *out,
	const struct efi_device_path_protocol *path)
{
	efi_status_t status;
	uint16_t buf[128];

	u16snprintf(
		buf,
		sizeof(buf)/sizeof(buf[0]),
		"{ 0x%x, 0x%x, 0x%x }",
		(unsigned)path->type,
		(unsigned)path->subtype,
		(unsigned)path->length);
	status = out->output_string(out, buf);
	if (status != 0)
		return status;

	if (path->type == 0x4 && path->subtype == 0x4) {
		uint16_t *str = (uint16_t *)(path + 1);

		u16snprintf(buf, sizeof(buf)/sizeof(buf[0]), ": path = ");
		status = out->output_string(out, buf);
		if (status == 0)
			status = out->output_string(out, str);
	}

	return status;
}

static efi_status_t efi_handle_explore(
	struct efi_system_table *system_table,
	efi_handle_t handle)
{
	uint16_t rn[] = u"\r\n";
	uint16_t header[] = u"Supported protocols:\r\n";
	efi_status_t status, other;
	struct efi_guid **guids;
	efi_uint_t count;
	size_t i;

	status = system_table->out->output_string(system_table->out, header);
	if (status != 0)
		return status;

	status = system_table->boot->protocols_per_handle(
		handle, &guids, &count);
	if (status != 0)
		return status;

	for (i = 0; i < count; ++i)
	{
		status = efi_guid_print(system_table->out, guids[i]);
		if (status != 0)
			break;
		status = system_table->out->output_string(
			system_table->out, rn);
		if (status != 0)
			break;
	}
	other = system_table->boot->free_pool(guids);
	return status != 0 ? status : other;
}

static efi_status_t efi_image_info(
	struct efi_system_table *system_table, efi_handle_t image_handle)
{
	uint16_t header[] = u"Image path:\r\n";
	struct efi_guid guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
	struct efi_loaded_image_protocol *protocol;
	efi_status_t status, other;

	status = system_table->boot->open_protocol(
		image_handle,
		&guid,
		(void **)&protocol,
		image_handle,
		NULL,
		EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
	if (status != 0)
		return status;

	status = efi_handle_explore(
		system_table, protocol->device_handle);
	if (status != 0)
		goto out;

	status = system_table->out->output_string(
		system_table->out, header);
	if (status != 0)
		goto out;

	status = efi_device_path_print(
		system_table->out, protocol->file_path);

out:
	other = system_table->boot->close_protocol(
		image_handle,
		&guid,
		image_handle,
		NULL);

	return status != 0 ? status : other;
}

efi_status_t efi_main(
	efi_handle_t handle, struct efi_system_table *system_table)
{
	efi_status_t status;

	status = system_table->out->clear_screen(system_table->out);
	if (status != 0)
		return status;

	status = efi_handle_explore(system_table, handle);
	if (status != 0)
		return status;

	status = efi_image_info(system_table, handle);
	if (status != 0)
		return status;

	while (1)
		;

	return 0;
}
