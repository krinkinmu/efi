#ifndef __EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_H__
#define __EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_H__

#include "file_protocol.h"
#include "types.h"

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
    { 0x0964e5b22, 0x6459, 0x11d2, \
      { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } }

struct efi_simple_file_system_protocol {
    uint64_t revision;
    efi_status_t (*open_volume)(
        struct efi_simple_file_system_protocol *, struct efi_file_protocol **);
};

#endif  // __EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_H__

