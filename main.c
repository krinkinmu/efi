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

struct elf_app {
	struct efi_system_table *system;
	struct efi_file_protocol *kernel;
	struct elf64_ehdr header;
	struct elf64_phdr *program_headers;
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
	if (status != EFI_SUCCESS)
		return status;

	while (read < size) {
		efi_uint_t remains = size - read;

		status = file->read(file, &remains, (void *)(buf + read));
		if (status != EFI_SUCCESS)
			return status;

		read += remains;
	}

	return EFI_SUCCESS;
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

static efi_status_t setup_efi_app(
	efi_handle_t handle,
	struct efi_system_table *system,
	struct efi_app *app)
{
	efi_status_t status;

	memset(app, 0, sizeof(*app));
	app->handle = handle;
	app->system = system;

	status = get_image(handle, system, &app->image);
	if (status != EFI_SUCCESS)
		return status;

	app->root_device = app->image->device;

	status = get_rootfs(handle, system, app->root_device, &app->rootfs);
	if (status != EFI_SUCCESS)
		return status;

	status = get_rootdir(app->rootfs, &app->rootdir);
	if (status != EFI_SUCCESS)
		return status;

	return system->out->clear_screen(system->out);
}

static efi_status_t cleanup_efi_app(
	struct efi_app *app)
{
	struct efi_guid image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
	struct efi_guid rootfs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
	efi_status_t status = EFI_SUCCESS;
	efi_status_t other;

	if (app->rootdir) {
		other = app->rootdir->close(app->rootdir);
		if (other == EFI_SUCCESS)
			status = other;
	}

	if (app->rootfs) {
		other = app->system->boot->close_protocol(
			app->root_device, &rootfs_guid, app->handle, NULL);
		if (status == EFI_SUCCESS)
			status = other;
	}

	if (app->image) {
		other = app->system->boot->close_protocol(
			app->handle, &image_guid, app->handle, NULL);
		if (status == EFI_SUCCESS)
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

static efi_status_t read_efl64_program_headers(
	struct efi_boot_table *boot,
	struct efi_file_protocol *file,
	const struct elf64_ehdr *hdr,
	struct elf64_phdr **phdrs)
{
	efi_status_t status;

	status = boot->allocate_pool(
		EFI_LOADER_DATA,
		hdr->e_phnum * hdr->e_phentsize,
		(void **)phdrs);
	if (status != EFI_SUCCESS)
		return status;

	status = efi_read_fixed(
		file,
		hdr->e_phoff,
		hdr->e_phentsize * hdr->e_phnum,
		(void *)*phdrs);
	if (status != EFI_SUCCESS) {
		boot->free_pool((void *)*phdrs);
		return status;
	}

	return EFI_SUCCESS;
}

static efi_status_t verify_elf64_header(
	struct efi_simple_text_output_protocol *out,
	const struct elf64_ehdr *hdr)
{
	uint16_t msg[128];

	if (hdr->e_ident[EI_MAG0] != 0x7f
		|| hdr->e_ident[EI_MAG1] != 'E'
		|| hdr->e_ident[EI_MAG2] != 'L'
		|| hdr->e_ident[EI_MAG3] != 'F') {
		u16snprintf(msg, sizeof(msg),
			"No ELF magic sequence in the ELF header\r\n");
		out->output_string(out, msg);
		return EFI_UNSUPPORTED;
	}

	if (hdr->e_type != ET_EXEC) {
		u16snprintf(msg, sizeof(msg),
			"Unsupported ELF file type 0x%x, the only type 0x%x is supported\r\n",
			(unsigned)hdr->e_type,
			(unsigned)ET_EXEC);
		out->output_string(out, msg);
		return EFI_UNSUPPORTED;
	}

	if (hdr->e_ident[EI_CLASS] != ELFCLASS64) {
		u16snprintf(msg, sizeof(msg),
			"Unsupported ELF class 0x%x, the only class 0x%x is supported\r\n",
			(unsigned)hdr->e_ident[EI_CLASS],
			(unsigned)ELFCLASS64);
		out->output_string(out, msg);
		return EFI_UNSUPPORTED;
	}

	if (hdr->e_phnum == 0) {
		u16snprintf(msg, sizeof(msg),
			"ELF file doesn't contain any program headers\r\n");
		out->output_string(out, msg);
		return EFI_UNSUPPORTED;
	}

	if (hdr->e_phentsize != sizeof(struct elf64_phdr)) {
		u16snprintf(msg, sizeof(msg),
			"Unexpected ELF program header size %u, only size %u is supported\r\n",
			(unsigned)hdr->e_phentsize,
			(unsigned)sizeof(struct elf64_phdr));
		out->output_string(out, msg);
		return EFI_UNSUPPORTED;
	}

	return EFI_SUCCESS;
}

static efi_status_t setup_elf_app(
	struct efi_app *efi, const uint16_t *path, struct elf_app *elf)
{
	efi_status_t status;

	memset(elf, 0, sizeof(*elf));
	elf->system = efi->system;

	status = efi->rootdir->open(
		efi->rootdir,
		&elf->kernel,
		(uint16_t *)path,
		EFI_FILE_MODE_READ,
		EFI_FILE_READ_ONLY);
	if (status != EFI_SUCCESS)
		return status;

	status = read_elf64_header(elf->kernel, &elf->header);
	if (status != EFI_SUCCESS)
		return status;

	status = verify_elf64_header(elf->system->err, &elf->header);
	if (status != EFI_SUCCESS)
		return status;

	return read_efl64_program_headers(
		elf->system->boot, elf->kernel, &elf->header, &elf->program_headers);
}

static efi_status_t cleanup_elf_app(struct elf_app *elf)
{
	efi_status_t status = EFI_SUCCESS;
	efi_status_t other;

	if (elf->program_headers) {
		other = elf->system->boot->free_pool(elf->program_headers);
		if (status == EFI_SUCCESS && other != EFI_SUCCESS)
			status = other;
	}

	if (elf->kernel) {
		other = elf->kernel->close(elf->kernel);
		if (status == EFI_SUCCESS && other != EFI_SUCCESS)
			status = other;
	}

	return status;
}

static efi_status_t dump_elf64_program_header(
	struct efi_simple_text_output_protocol *out,
	const struct elf64_phdr *phdr)
{
	const char *type[] = {
		"PT_NULL",
		"PT_LOAD",
		"PT_DYNAMIC",
		"PT_INTERP",
		"PT_NOTE",
		"PT_SHLIB",
		"PT_PHDR",
		"PT_TLS"
	};
	const char *flag[] = {
		"empty",
		"PF_X",
		"PF_W",
		"PF_X | PF_W",
		"PF_R",
		"PF_X | PF_R",
		"PF_W | PF_R",
		"PF_X | PF_W | PF_R"
	};

	efi_status_t status;
	uint16_t msg[512];

	if (phdr->p_type < sizeof(type) / sizeof(type[0]))
		u16snprintf(msg, sizeof(msg),
			"p_type: %s, ", type[phdr->p_type]);
	else
		u16snprintf(msg, sizeof(msg),
			"p_type: 0x%lx, ", (unsigned long)phdr->p_type);

	status = out->output_string(out, msg);
	if (status != EFI_SUCCESS)
		return status;

	u16snprintf(msg, sizeof(msg),
		"p_flags: %s, "
		"p_offset: 0x%llx, "
		"p_vaddr: 0x%llx, "
		"p_paddr: 0x%llx, "
		"p_filesz: 0x%llx, "
		"p_memsz: 0x%llx, "
		"p_align: 0x%llx\r\n",
		flag[phdr->p_flags & 7],
		(unsigned long long)phdr->p_offset,
		(unsigned long long)phdr->p_vaddr,
		(unsigned long long)phdr->p_paddr,
		(unsigned long long)phdr->p_filesz,
		(unsigned long long)phdr->p_memsz,
		(unsigned long long)phdr->p_align);
	return out->output_string(out, msg);
}

static efi_status_t dump_elf_headers(struct elf_app *elf)
{
	uint16_t msg[] = u"Program headers:\r\n";
	efi_status_t status;

	status = elf->system->out->output_string(elf->system->out, msg);
	if (status != EFI_SUCCESS)
		return status;

	for (size_t i = 0; i < elf->header.e_phnum; ++i) {
		status = dump_elf64_program_header(
			elf->system->out, &elf->program_headers[i]);
		if (status != EFI_SUCCESS)
			return status;
	}

	return EFI_SUCCESS;
}

efi_status_t efi_main(
	efi_handle_t handle,
	struct efi_system_table *system)
{
	uint16_t path[] = u"efi\\boot\\kernel";
	struct efi_app app;
	struct elf_app kernel;
	efi_status_t status;

	status = setup_efi_app(handle, system, &app);
	if (status != EFI_SUCCESS)
		goto out;

	status = setup_elf_app(&app, path, &kernel);
	if (status != EFI_SUCCESS)
		goto out;

	status = dump_elf_headers(&kernel);

out:
	cleanup_elf_app(&kernel);
	cleanup_efi_app(&app);
	return status;
}
