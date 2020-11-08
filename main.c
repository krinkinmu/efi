#include "clib.h"
#include "efi/efi.h"
#include "elf.h"

struct efi_app {
	struct efi_system_table *system;
	efi_handle_t handle;
	struct efi_loaded_image_protocol *image;
	efi_handle_t root_device;
	struct efi_simple_file_system_protocol *rootfs;
	struct efi_file_protocol *rootdir;
};

static efi_status_t efi_read_fixed(
	struct efi_file_protocol *file,
	uint64_t offset,
	size_t size,
	void *dst)
{
	efi_status_t status;
	unsigned char *buf = dst;
	size_t read = 0;

	status = file->set_position(file, offset);
	if (status != 0)
		return status;

	while (read < size) {
		efi_uint_t remains = size - read;

		status = file->read(file, &remains, (void *)(buf + read));
		if (status != 0)
			return status;

		read += remains;
	}

	return 0;
}

static efi_status_t get_image(
	efi_handle_t app,
	struct efi_system_table *system,
	struct efi_loaded_image_protocol **image)
{
	struct efi_guid guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;

	return system->boot->open_protocol(
		app,
		&guid,
		(void **)image,
		app,
		NULL,
		EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
}

static efi_status_t get_rootfs(
	efi_handle_t app,
	struct efi_system_table *system,
	efi_handle_t device,
	struct efi_simple_file_system_protocol **rootfs)
{
	struct efi_guid guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

	return system->boot->open_protocol(
		device,
		&guid,
		(void **)rootfs,
		app,
		NULL,
		EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
}

static efi_status_t get_rootdir(
	struct efi_simple_file_system_protocol *rootfs,
	struct efi_file_protocol **rootdir)
{
	return rootfs->open_volume(rootfs, rootdir);
}

static efi_status_t setup(
	efi_handle_t handle,
	struct efi_system_table *system,
	struct efi_app *app)
{
	efi_status_t status;

	memset(app, 0, sizeof(*app));
	app->handle = handle;
	app->system = system;

	status = get_image(handle, system, &app->image);
	if (status != 0)
		return status;

	app->root_device = app->image->device;

	status = get_rootfs(handle, system, app->root_device, &app->rootfs);
	if (status != 0)
		return status;

	status = get_rootdir(app->rootfs, &app->rootdir);
	if (status != 0)
		return status;

	return system->out->clear_screen(system->out);
}

static efi_status_t cleanup(
	struct efi_app *app)
{
	struct efi_guid image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
	struct efi_guid rootfs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
	efi_status_t status = 0;
	efi_status_t other;

	if (app->rootdir) {
		other = app->rootdir->close(app->rootdir);
		if (other == 0)
			status = other;
	}

	if (app->rootfs)
	{
		other = app->system->boot->close_protocol(
			app->root_device, &rootfs_guid, app->handle, NULL);
		if (status == 0)
			status = other;
	}

	if (app->image)
	{
		other = app->system->boot->close_protocol(
			app->handle, &image_guid, app->handle, NULL);
		if (status == 0)
			status = other;
	}

	return status;
}

static efi_status_t read_elf64_header(
	struct efi_file_protocol *file,
	struct elf64_ehdr *hdr)
{
	return efi_read_fixed(file, /*offset*/0, sizeof(*hdr), hdr);
}

static efi_status_t dump_elf64_header(
	struct efi_simple_text_output_protocol *out,
	const struct elf64_ehdr *hdr)
{
	uint16_t buffer[512];

	u16snprintf(buffer, sizeof(buffer),
		"e_ident:     0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\r\n"
		"e_type:      0x%x\r\n"
		"e_machine:   0x%x\r\n"
		"e_version:   0x%lx\r\n"
		"e_entry:     0x%llx\r\n"
		"e_phoff:     0x%llx\r\n"
		"e_shoff:     0x%llx\r\n"
		"e_flags:     0x%lx\r\n"
		"e_ehsize:    0x%x\r\n"
		"e_phentsize: 0x%x\r\n"
		"e_phnum:     0x%x\r\n"
		"e_shentsize: 0x%x\r\n"
		"e_shnum:     0x%x\r\n"
		"e_shstrndx:  0x%x\r\n",
		(unsigned)hdr->e_ident[0], (unsigned)hdr->e_ident[1], (unsigned)hdr->e_ident[2], (unsigned)hdr->e_ident[3],
		(unsigned)hdr->e_ident[4], (unsigned)hdr->e_ident[5], (unsigned)hdr->e_ident[6], (unsigned)hdr->e_ident[7],
		(unsigned)hdr->e_ident[8], (unsigned)hdr->e_ident[9], (unsigned)hdr->e_ident[10], (unsigned)hdr->e_ident[11],
		(unsigned)hdr->e_ident[12], (unsigned)hdr->e_ident[13], (unsigned)hdr->e_ident[14], (unsigned)hdr->e_ident[15],
		(unsigned)hdr->e_type,
		(unsigned)hdr->e_machine,
		(unsigned long)hdr->e_version,
		(unsigned long long)hdr->e_entry,
		(unsigned long long)hdr->e_phoff,
		(unsigned long long)hdr->e_shoff,
		(unsigned long)hdr->e_flags,
		(unsigned)hdr->e_ehsize,
		(unsigned)hdr->e_phentsize,
		(unsigned)hdr->e_phnum,
		(unsigned)hdr->e_shentsize,
		(unsigned)hdr->e_shnum,
		(unsigned)hdr->e_shstrndx);
	return out->output_string(out, buffer);
}

efi_status_t efi_main(
	efi_handle_t handle,
	struct efi_system_table *system)
{
	uint16_t path[] = u"efi\\boot\\kernel";
	struct elf64_ehdr hdr;
	struct efi_file_protocol *kernel;
	struct efi_app app;
	efi_status_t status, other;

	status = setup(handle, system, &app);
	if (status != 0)
		goto out;

	status = app.rootdir->open(
		app.rootdir, &kernel, path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
	if (status != 0)
		goto out;

	status = read_elf64_header(kernel, &hdr);
	if (status != 0)
		goto close;

	status = dump_elf64_header(app.system->out, &hdr);
	if (status != 0)
		goto close;

close:
	other = kernel->close(kernel);
	if (status == 0 && other != 0)
		status = other;

out:
	cleanup(&app);
	return status;
}
