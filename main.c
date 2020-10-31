#include "clib.h"
#include "efi/efi.h"

struct efi_app {
	struct efi_system_table *system;
	efi_handle_t handle;
	struct efi_loaded_image_protocol *image;
	efi_handle_t root_device;
	struct efi_simple_file_system_protocol *rootfs;
	struct efi_file_protocol *rootdir;
};

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

efi_status_t efi_main(
	efi_handle_t handle,
	struct efi_system_table *system)
{
	struct efi_guid guid = EFI_FILE_INFO_GUID;
	uint16_t path[] = u"efi\\boot\\bootx64.efi";
	uint16_t buffer[128];
	struct efi_file_protocol *kernel;
	struct efi_file_info file_info;
	efi_uint_t size;
	struct efi_app app;
	efi_status_t status;

	status = setup(handle, system, &app);
	if (status != 0)
		goto out;

	status = app.rootdir->open(
		app.rootdir, &kernel, path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
	if (status != 0)
		goto out;

	size = sizeof(file_info);
	status = kernel->get_info(kernel, &guid, &size, (void *)&file_info);
	if (status != 0)
		goto close;

	u16snprintf(
		buffer, sizeof(buffer),
		"file %w size %llu (%llu on disk) bytes\r\n",
		file_info.file_name,
		(unsigned long long)file_info.file_size,
		(unsigned long long)file_info.physical_size);
	system->out->output_string(system->out, buffer);

close:
	status = app.rootdir->close(kernel);

out:
	cleanup(&app);
	return status;
}
