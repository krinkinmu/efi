#include "log.h"

#include "clib.h"
#include "efi/efi.h"


static void debug(
	struct efi_simple_text_output_protocol *out,
	const char *fmt,
	va_list args)
{
	uint16_t msg[512];

	vsnprintf(msg, sizeof(msg), fmt, args);
	out->output_string(out, msg);
}

void info(struct efi_system_table *system, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	debug(system->out, fmt, args);
	va_end(args);
}

void err(struct efi_system_table *system, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	debug(system->err, fmt, args);
	va_end(args);
}
