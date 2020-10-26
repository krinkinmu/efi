#ifndef __EFI_FILE_PROTOCOL_H__
#define __EFI_FILE_PROTOCOL_H__

#include "types.h"

#define EFI_FILE_INFO_GUID \
    { 0x09576e92, 0x6d3f, 0x11d2, \
      { 0x8e39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } }

// Open modes
static const uint64_t EFI_FILE_MODE_READ = 0x0000000000000001;
static const uint64_t EFI_FILE_MODE_WRITE = 0x0000000000000002;
static const uint64_t EFI_FILE_MODE_CREATE = 0x8000000000000000;

// File attributes
static const uint64_t EFI_FILE_READ_ONLY = 0x1;
static const uint64_t EFI_FILE_HIDDEN = 0x2;
static const uint64_t EFI_FILE_SYSTEM = 0x4;
static const uint64_t EFI_FILE_RESERVED = 0x8;
static const uint64_t EFI_FILE_DIRECTORY = 0x10;
static const uint64_t EFI_FILE_ARCHIVE = 0x20;

struct efi_file_info {
    uint64_t size;
    uint64_t file_size;
    uint64_t physical_size;
    struct efi_time create_time;
    struct efi_time last_access_time;
    struct efi_time modifiction_time;
    uint64_t attribute;
    // The efi_file_info structure is supposed to be a variable size structure,
    // but it's really a pain to always dynamically allocate enough space for
    // the structure, so I explicitly allocated some space in the structure, so
    // we will be able to cover at least some simple cases without dynamic
    // memory allocation.
    uint16_t file_name[256];
};

struct efi_file_protocol {
    uint64_t revision;
    efi_status_t (*open)(
        struct efi_file_protocol *,
        struct efi_file_protocol **,
        uint16_t *,
        uint64_t,
        uint64_t);
    efi_status_t (*close)(struct efi_file_protocol *);

    void (*unused1)();

    efi_status_t (*read)(struct efi_file_protocol *, efi_uint_t *, void *);

    void (*unused2)();
    void (*unused3)();
    void (*unused4)();

    efi_status_t (*get_info)(
        struct efi_file_protocol *, struct efi_guid *, efi_uint_t *, void *);

    void (*unused6)();
    void (*unused7)();
    void (*unused8)();
    void (*unused9)();
    void (*unused10)();
    void (*unused11)();
};

#endif  // __EFI_FILE_PROTOCOL_H__

