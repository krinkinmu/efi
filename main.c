#include "efi/efi.h"

// Clang assumes that there are mem* functions defined somewhere, so it
// occasionally generates code that uses them.
void *memcpy(void *dst, const void *src, size_t size)
{
	const char *from = src;
	char *to = dst;

	for (size_t i = 0; i < size; ++i)
		*to++ = *from++;

	return dst;
}

static uint16_t *u64_to_str(uint64_t x, uint16_t *buf)
{
	uint16_t *pos = buf;
	uint16_t *begin;
	uint16_t *end;

	*pos++ = u'0';
	*pos++ = u'x';
	begin = pos;
	do
	{
		if ((x & 0xf) < 10)
			*pos++ = u'0' + (x & 0xf);
		else
			*pos++ = u'A' + (x & 0xf) - 10;
		x >>= 4;
	} while (x);
	end = pos;

	while (begin < end)
	{
		const uint16_t tmp = *begin;

		*begin = *(end - 1);
		*(end - 1) = tmp;
		++begin;
		--end;
	}

	return pos;
}

static uint16_t *efi_guid_to_str(const struct efi_guid *guid, uint16_t *buf)
{
	uint16_t *pos = buf;
	size_t i;

	*pos++ = u'{';
	*pos++ = u' ';
	pos = u64_to_str(guid->data1, pos);
	*pos++ = u',';
	*pos++ = u' ';
	pos = u64_to_str(guid->data2, pos);
	*pos++ = u',';
	*pos++ = u' ';
	pos = u64_to_str(guid->data3, pos);
	*pos++ = u',';
	*pos++ = u' ';
	*pos++ = u'{';
	*pos++ = u' ';
	pos = u64_to_str(guid->data4[0], pos);
	for (i = 1; i < 8; ++i)
	{
		*pos++ = u',';
		*pos++ = u' ';
		pos = u64_to_str(guid->data4[i], pos);
	}
	*pos++ = u' ';
	*pos++ = u'}';
	*pos++ = u' ';
	*pos++ = u'}';

	return pos;
}

static uint16_t *efi_device_path_to_str(
	const struct efi_device_path_protocol *path, uint16_t *buf)
{
	uint16_t *pos = buf;

	*pos++ = u'{';
	*pos++ = u' ';
	pos = u64_to_str(path->type, pos);
	*pos++ = u',';
	*pos++ = u' ';
	pos = u64_to_str(path->subtype, pos);
	*pos++ = u',';
	*pos++ = u' ';
	pos = u64_to_str(path->length, pos);
	*pos++ = u' ';
	*pos++ = u'}';
	return pos;
}

static efi_status_t efi_guid_print(
	struct efi_simple_text_output_protocol *out,
	const struct efi_guid *guid)
{
	uint16_t buf[128];

	*efi_guid_to_str(guid, buf) = 0;
	return out->output_string(out, buf);
}

static efi_status_t efi_device_path_print(
	struct efi_simple_text_output_protocol *out,
	const struct efi_device_path_protocol *path)
{
	efi_status_t status;
	uint16_t buf[128];

	*efi_device_path_to_str(path, buf) = 0;
	status = out->output_string(out, buf);
	if (status != 0)
		return status;

	if (path->type == 0x4 && path->subtype == 0x4) {
		uint16_t header[] = u": path = ";
		uint16_t *str = (uint16_t *)(path + 1);

		status = out->output_string(out, header);
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
