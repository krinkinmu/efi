#include "loader.h"

#include "clib.h"
#include "io.h"
#include "log.h"


static size_t skip_ws(const char *data, size_t i)
{
	while (isspace(data[i]))
		++i;
	return i;
}

static size_t skip_name(const char *data, size_t i)
{
	while (isalnum(data[i])
	       || data[i] == '_'
	       || data[i] == '-'
	       || data[i] == '.')
		++i;
	return i;
}

static size_t skip_path(const char *data, size_t i)
{
	while (isalnum(data[i])
	       || data[i] == '_'
	       || data[i] == '-'
	       || data[i] == '.'
	       || data[i] == '\\'
	       || data[i] == '/')
		++i;
	return i;
}

static efi_status_t add_module(
	struct loader *loader,
	const char *name,
	const uint16_t *path)
{
	if (loader->modules == loader->module_capacity) {
		efi_status_t status = EFI_SUCCESS;
		size_t new_size = 2 * loader->modules;
		struct module *new_module = NULL;
		struct module *old_module = loader->module;

		if (new_size == 0)
			new_size = 16;

		status = loader->system->boot->allocate_pool(
			EFI_LOADER_DATA,
			new_size * sizeof(struct module),
			(void **)&new_module);
		if (status != EFI_SUCCESS) {
			err(
				loader->system,
				"failed to allocate buffer for modules\r\n");
			return status;
		}

		memcpy(
			new_module,
			old_module,
			loader->modules * sizeof(struct module));
		loader->module = new_module;
		loader->module_capacity = new_size;

		if (old_module != NULL) {
			status = loader->system->boot->free_pool(
				(void *)old_module);
			if (status != EFI_SUCCESS) {
				err(
					loader->system,
					"failed to free buffer for modules\r\n");
				return status;
			}
		}
	}

	memset(&loader->module[loader->modules], 0, sizeof(struct module));
	loader->module[loader->modules].name = name;
	loader->module[loader->modules].path = path;
	++loader->modules;
	return EFI_SUCCESS;
}

efi_status_t load_config(
	struct loader *loader,
	const uint16_t *config_path)
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
	if (status != EFI_SUCCESS) {
		err(
			loader->system,
			"failed to open %w: %llu\r\n",
			config_path,
			(unsigned long long)status);
		return status;
	}

	size = sizeof(file_info);
	status = loader->config->get_info(
		loader->config,
		&guid,
		&size,
		(void *)&file_info);
	if (status != EFI_SUCCESS) {
		err(
			loader->system,
			"failed to find size of %w: %llu\r\n",
			config_path,
			(unsigned long long)status);
		return status;
	}

	status = loader->system->boot->allocate_pool(
		EFI_LOADER_DATA,
		file_info.file_size + 1,
		(void **)&loader->config_data);
	if (status != EFI_SUCCESS) {
		err(
			loader->system,
			"failed to allocate buffer for %w data: %llu\r\n",
			config_path,
			(unsigned long long)status);
		return status;
	}

	memset((void *)loader->config_data, 0, file_info.file_size + 1);

	return efi_read_fixed(
		loader->system,
		loader->config,
		/* offset */0,
		/* size */file_info.file_size,
		(void *)loader->config_data);
}

efi_status_t parse_config(struct loader *loader)
{
	size_t i = 0;

	while (1) {
		efi_status_t status = EFI_SUCCESS;
		uint16_t *path = NULL;
		char *name = NULL;
		size_t name_begin, name_size;
		size_t path_begin, path_size;

		i = skip_ws(loader->config_data, i);
		if (loader->config_data[i] == '\0')
			break;

		name_begin = i;
		i = skip_name(loader->config_data, i);
		name_size = i - name_begin;
		i = skip_ws(loader->config_data, i);

		/* We expect ':' after name and before the path, and if it's
		 * not there, then something went wrong, so we can fail here. */
		if (loader->config_data[i] != ':') {
			err(
				loader->system,
				"incorrect config format: missing ':'\r\n");
			return EFI_INVALID_PARAMETER;
		}

		i = skip_ws(loader->config_data, i + 1);
		path_begin = i;
		i = skip_path(loader->config_data, i);
		path_size = i - path_begin;

		/* skip_* functions do not return errors and it may happen that
		 * we hit the end of the file prematurely or encountered an
		 * unsupported character. Check here that the name and path
		 * arent't actually empty to catch problems of that kind. */
		if (name_size == 0) {
			err(
				loader->system,
				"invalid config format: empty module name\r\n");
			return EFI_INVALID_PARAMETER;
		}

		if (path_size == 0) {
			err(
				loader->system,
				"invalid config format: empty module path\r\n");
			return EFI_INVALID_PARAMETER;
		}

		status = loader->system->boot->allocate_pool(
			EFI_LOADER_DATA,
			name_size + 1,
			(void **)&name);
		if (status != EFI_SUCCESS) {
			err(
				loader->system,
				"failed to allocate buffer for module name\r\n");
			return status;
		}
		strncpy(name, &loader->config_data[name_begin], name_size);
		name[name_size] = '\0';

		status = loader->system->boot->allocate_pool(
			EFI_LOADER_DATA,
			2 * (path_size + 1),
			(void **)&path);
		if (status != EFI_SUCCESS) {
			err(
				loader->system,
				"failed to allocate buffer for module path\r\n");
			return status;
		}
		to_u16strncpy(
			path, &loader->config_data[path_begin], path_size);
		path[path_size] = '\0';

		status = add_module(loader, name, path);
		if (status != EFI_SUCCESS) {
			err(
				loader->system,
				"failed to add module %s\r\n",
				name);
			return status;
		}
	}

	for (size_t j = 0; j < loader->modules; ++j) {
		if (strcmp(loader->module[j].name, "kernel") != 0)
			continue;

		loader->kernel = j;
		return EFI_SUCCESS;
	}

	/* If we got here, it means that we couldn't find the kernel module. */
	err(
		loader->system,
		"invalid config format: no kernel module\r\n");
	return EFI_INVALID_PARAMETER;
}
