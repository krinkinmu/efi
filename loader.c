#include "loader.h"

#include "clib.h"
#include "compiler.h"
#include "io.h"
#include "log.h"


static efi_status_t reserve(
	struct loader *loader,
	const char *name,
	uint64_t begin,
	uint64_t end)
{
	if (loader->reserves == loader->reserve_capacity) {
		efi_status_t status = EFI_SUCCESS;
		size_t new_size = 2 * loader->reserves;
		struct reserve *new_reserve = NULL;
		struct reserve *old_reserve = loader->reserve;

		if (new_size == 0)
			new_size = 16;

		status = loader->system->boot->allocate_pool(
			EFI_LOADER_DATA,
			new_size * sizeof(struct reserve),
			(void **)&new_reserve);
		if (status != EFI_SUCCESS) {
			err(
				loader->system,
				"failed to allocate buffer for memory reserve\r\n");
			return status;
		}

		memcpy(
			new_reserve,
			old_reserve,
			loader->reserves * sizeof(struct reserve));
		loader->reserve = new_reserve;
		loader->reserve_capacity = new_size;

		if (old_reserve != NULL) {
			status = loader->system->boot->free_pool(
				(void *)old_reserve);
			if (status != EFI_SUCCESS) {
				err(
					loader->system,
					"failed to free buffer for memory reserve\r\n");
				return status;
			}
		}
	}

	memset(&loader->reserve[loader->reserves], 0, sizeof(struct reserve));
	loader->reserve[loader->reserves].name = name;
	loader->reserve[loader->reserves].begin = begin;
	loader->reserve[loader->reserves].end = end;
	++loader->reserves;
	return EFI_SUCCESS;
}

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
	if (status != EFI_SUCCESS) {
		err(
			system,
			"failed to get loader image protocol\r\n");
		return status;
	}

	loader->root_device = loader->image->device;
	status = get_rootfs(
		handle, system, loader->root_device, &loader->rootfs);
	if (status != EFI_SUCCESS) {
		err(
			system,
			"failed to get root volume\r\n");
		return status;
	}

	status = get_rootdir(loader->rootfs, &loader->rootdir);
	if (status != EFI_SUCCESS) {
		err(
			system,
			"failed to get root filesystem directory\r\n");
		return status;
	}

	return EFI_SUCCESS;
}

static efi_status_t read_elf64_header(
	struct efi_system_table *system,
	struct efi_file_protocol *file,
	struct elf64_ehdr *hdr)
{
	return efi_read_fixed(system, file, /*offset*/0, sizeof(*hdr), hdr);
}

static efi_status_t verify_elf64_header(
	struct efi_system_table *system,
	const struct elf64_ehdr *hdr)
{
	if (hdr->e_ident[EI_MAG0] != 0x7f
		|| hdr->e_ident[EI_MAG1] != 'E'
		|| hdr->e_ident[EI_MAG2] != 'L'
		|| hdr->e_ident[EI_MAG3] != 'F') {
		err(system, "No ELF magic sequence in the ELF header\r\n");
		return EFI_UNSUPPORTED;
	}

	if (hdr->e_type != ET_EXEC && hdr->e_type != ET_DYN) {
		err(
			system,
			"Unsupported ELF file type 0x%x, the only type 0x%x is supported\r\n",
			(unsigned)hdr->e_type,
			(unsigned)ET_EXEC);
		return EFI_UNSUPPORTED;
	}

	if (hdr->e_ident[EI_CLASS] != ELFCLASS64) {
		err(
			system,
			"Unsupported ELF class 0x%x, the only class 0x%x is supported\r\n",
			(unsigned)hdr->e_ident[EI_CLASS],
			(unsigned)ELFCLASS64);
		return EFI_UNSUPPORTED;
	}

	if (hdr->e_phnum == 0) {
		err(
			system,
			"ELF file doesn't contain any program headers\r\n");
		return EFI_UNSUPPORTED;
	}

	if (hdr->e_phentsize != sizeof(struct elf64_phdr)) {
		err(
			system,
			"Unexpected ELF program header size %u, only size %u is supported\r\n",
			(unsigned)hdr->e_phentsize,
			(unsigned)sizeof(struct elf64_phdr));
		return EFI_UNSUPPORTED;
	}

	return EFI_SUCCESS;
}

static efi_status_t read_elf64_program_headers(
	struct efi_system_table *system,
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
	if (status != EFI_SUCCESS) {
		err(
			system,
			"failed to allocate buffer for program headers\r\n");
		return status;
	}

	status = efi_read_fixed(
		system,
		file,
		hdr->e_phoff,
		hdr->e_phentsize * hdr->e_phnum,
		(void *)*phdrs);
	if (status != EFI_SUCCESS) {
		err(
			system,
			"failed to read program headers\r\n");
		boot->free_pool((void *)*phdrs);
		return status;
	}

	return EFI_SUCCESS;
}

static void elf64_image_size(
	struct loader *loader,
	uint64_t alignment,
	uint64_t *begin,
	uint64_t *end)
{
	*begin = UINT64_MAX;
	*end = 0;

	for (size_t i = 0; i < loader->kernel_header.e_phnum; ++i) {
		struct elf64_phdr *phdr = &loader->program_headers[i];
		uint64_t phdr_begin, phdr_end;
		uint64_t align = alignment;

		if (phdr->p_type != PT_LOAD)
			continue;

		if (phdr->p_align > align)
			align = phdr->p_align;

		phdr_begin = phdr->p_vaddr;
		phdr_begin &= ~(align - 1);
		if (*begin > phdr_begin)
			*begin = phdr_begin;

		phdr_end = phdr->p_vaddr + phdr->p_memsz + align - 1;
		phdr_end &= ~(align - 1);
		if (*end < phdr_end)
			*end = phdr_end;
	}
}

efi_status_t load_kernel(struct loader *loader)
{
	efi_status_t status = EFI_SUCCESS;
	uint64_t page_size = 4096;
	uint64_t image_begin;
	uint64_t image_end;
	uint64_t image_size;
	uint64_t image_addr;

	status = loader->rootdir->open(
		loader->rootdir,
		&loader->kernel_image,
		(uint16_t *)loader->module[loader->kernel].path,
		EFI_FILE_MODE_READ,
		EFI_FILE_READ_ONLY);
	if (status != EFI_SUCCESS) {
		err(
			loader->system,
			"failed to open kernel file\r\n");
		return status;
	}

	status = read_elf64_header(
		loader->system, loader->kernel_image, &loader->kernel_header);
	if (status != EFI_SUCCESS) {
		err(
			loader->system,
			"failed to read ELF header\r\n");
		return status;
	}

	status = verify_elf64_header(
		loader->system, &loader->kernel_header);
	if (status != EFI_SUCCESS) {
		err(
			loader->system,
			"ELF header didn't pass verifications\r\n");
		return status;
	}

	status = read_elf64_program_headers(
		loader->system,
		loader->system->boot,
		loader->kernel_image,
		&loader->kernel_header,
		&loader->program_headers);
	if (status != EFI_SUCCESS) {
		err(
			loader->system,
			"failed to read ELF program headers\r\n");
		return status;
	}

	elf64_image_size(
		loader,
		page_size,
		&image_begin,
		&image_end);
	image_size = image_end - image_begin;
	status = loader->system->boot->allocate_pages(
		EFI_ALLOCATE_ANY_PAGES,
		EFI_LOADER_DATA,
		image_size / page_size,
		&image_addr);
	if (status != EFI_SUCCESS) {
		err(
			loader->system,
			"failed to allocate buffer for the kernel\r\n");
		return status;
	}

	memset((void *)image_addr, 0, image_size);
	for (size_t i = 0; i < loader->kernel_header.e_phnum; ++i) {
		struct elf64_phdr *phdr = &loader->program_headers[i];
		uint64_t phdr_addr;

		if (phdr->p_type != PT_LOAD)
			continue;

		phdr_addr = image_addr + phdr->p_vaddr - image_begin;
		status = efi_read_fixed(
			loader->system,
			loader->kernel_image,
			phdr->p_offset,
			phdr->p_filesz,
			(void *)phdr_addr);
		if (status != EFI_SUCCESS) {
			err(
				loader->system,
				"failed to read the kernel segment in memory\r\n");
			return status;
		}

		status = reserve(
			loader,
			"kernel",
			phdr_addr,
			phdr_addr + phdr->p_memsz);
		if (status != EFI_SUCCESS) {
			err(
				loader->system,
				"failed to mark kernel segment as reserved\r\n");
			return status;
		}
	}

	loader->kernel_image_entry =
		image_addr + loader->kernel_header.e_entry - image_begin;
	return EFI_SUCCESS;
}

static efi_status_t load_module(
	struct loader *loader,
	struct efi_file_protocol *file,
	const char *name)
{
	efi_status_t status = EFI_SUCCESS;
	struct efi_guid guid = EFI_FILE_INFO_GUID;
	void *addr = NULL;
	struct efi_file_info file_info;
	efi_uint_t size;

	size = sizeof(file_info);
	status = file->get_info(
		file,
		&guid,
		&size,
		(void *)&file_info);
	if (status != EFI_SUCCESS) {
		err(
			loader->system,
			"failed to find module file size\r\n");
		return status;
	}

	status = loader->system->boot->allocate_pool(
		EFI_LOADER_DATA,
		file_info.file_size,
		&addr);
	if (status != EFI_SUCCESS) {
		err(
			loader->system,
			"failed to allocate memory for module\r\n");
		return status;
	}

	status = efi_read_fixed(
		loader->system,
		file,
		/* offset */0,
		/* size */file_info.file_size,
		addr);
	if (status != EFI_SUCCESS) {
		err(
			loader->system,
			"failed to read module in memory\r\n");
		return status;
	}

	status = reserve(
		loader,
		name,
		(uint64_t)addr,
		(uint64_t)addr + file_info.file_size);
	if (status != EFI_SUCCESS) {
		err(
			loader->system,
			"failed to mark module memory as reserved\r\n");
		return status;
	}
	return EFI_SUCCESS;
}

efi_status_t load_modules(struct loader *loader)
{
	for (size_t i = 0; i < loader->modules; ++i) {
		efi_status_t status = EFI_SUCCESS;
		struct efi_file_protocol *file = NULL;

		if (i == loader->kernel)
			continue;

		status = loader->rootdir->open(
			loader->rootdir,
			&file,
			(uint16_t *)loader->module[i].path,
			EFI_FILE_MODE_READ,
			EFI_FILE_READ_ONLY);
		if (status != EFI_SUCCESS) {
			err(
				loader->system,
				"failed to open module file\r\n");
			return status;
		}

		status = load_module(loader, file, loader->module[i].name);
		if (status != EFI_SUCCESS) {
			err(
				loader->system,
				"failed to load module\r\n");
			return status;
		}

		status = file->close(file);
		if (status != EFI_SUCCESS) {
			err(
				loader->system,
				"failed to close module file\r\n");
			return status;
		}
	}

	return EFI_SUCCESS;
}

static efi_status_t exit_efi_boot_services(struct loader *loader)
{
	struct efi_memory_descriptor *mmap;
	efi_uint_t mmap_size = 4096;
	efi_uint_t mmap_key;
	efi_uint_t desc_size;
	uint32_t desc_version;
	efi_status_t status;

	while (1) {
		status = loader->system->boot->allocate_pool(
			EFI_LOADER_DATA,
			mmap_size,
			(void **)&mmap);
		if (status != EFI_SUCCESS)
			return status;

		status = loader->system->boot->get_memory_map(
			&mmap_size,
			mmap,
			&mmap_key,
			&desc_size,
			&desc_version);
		if (status == EFI_SUCCESS)
			break;

		loader->system->boot->free_pool(mmap);

		// If the buffer size turned out too small then get_memory_map
		// should have updated mmap_size to contain the buffer size
		// needed for the memory map. However subsequent free_pool and
		// allocate_pool might change the memory map and therefore I
		// additionally multiply it by 2.
		if (status == EFI_BUFFER_TOO_SMALL) {
			mmap_size *= 2;
			continue;
		}

		return status;
	}

	status = loader->system->boot->exit_boot_services(
		loader->handle,
		mmap_key);
	if (status != EFI_SUCCESS)
		loader->system->boot->free_pool(mmap);

	return status;
}

efi_status_t start_kernel(struct loader *loader)
{
	efi_status_t status = EFI_SUCCESS;
	void (ELFABI *entry)(struct reserve *, size_t);

	info(loader->system, "Shutting down UEFI boot services\r\n");
	status = exit_efi_boot_services(loader);
	if (status != EFI_SUCCESS)
		return status;

	/* If we got this far there is no way back since all the EFI services
	 * have been shut down by this point. */
	entry = (void (ELFABI *)(struct reserve *, size_t))
		loader->kernel_image_entry;
	(*entry)(loader->reserve, loader->reserves);

	while (1) {}
	return EFI_LOAD_ERROR;
}
