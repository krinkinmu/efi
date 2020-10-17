#include "efi.h"

static uint16_t *u64_to_str(uint64_t x, uint16_t *buf)
{
	uint16_t *pos = buf;
	uint16_t *begin;
	uint16_t *end;

	*pos++ = u'0';
	*pos++ = u'x';
	begin = pos;
	do {
		if ((x & 0xf) < 10) *pos++ = u'0' + (x & 0xf);
		else *pos++ = u'A' + (x & 0xf) - 10;
		x >>= 4;
	} while (x);
	end = pos;

	while (begin < end) {
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

	*pos++ = u'{'; *pos++ = u' ';
	pos = u64_to_str(guid->data1, pos);
	*pos++ = u','; *pos++ = u' ';
	pos = u64_to_str(guid->data2, pos);
	*pos++ = u','; *pos++ = u' ';
	pos = u64_to_str(guid->data3, pos);
	*pos++ = u','; *pos++ = u' ';
	*pos++ = u'{'; *pos++ = u' ';
	pos = u64_to_str(guid->data4[0], pos);
	for (i = 1; i < 8; ++i) {
		*pos++ = u','; *pos++ = u' ';
		pos = u64_to_str(guid->data4[i], pos);
	}
	*pos++ = u' '; *pos++ = u'}';
	*pos++ = u' '; *pos++ = u'}';

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

static efi_status_t efi_handle_explore(
	struct efi_system_table *system_table,
	efi_handle_t handle)
{
	uint16_t rn[] = u"\r\n";
	efi_status_t status, other;
	struct efi_guid **guids;
	efi_uint_t count;
	size_t i;

	status = system_table->boot->protocols_per_handle(
		handle, &guids, &count);
	if (status != 0)
		return status;

	for (i = 0; i < count; ++i) {
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

	while (1);

	return 0;
}
