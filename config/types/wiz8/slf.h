#ifndef WIZ8_FORMATS_SLF_H
#define WIZ8_FORMATS_SLF_H

#include <stdint.h>

#pragma pack(push, 1)

typedef struct W8SlfHeader {
    char archive_name[256];             /* 0x000 */
    char base_path[256];                /* 0x100 */
    uint32_t file_count;                /* 0x200 */
    uint32_t second_count;              /* 0x204: meaning not yet established */
    uint32_t unknown_208;               /* 0x208 */
    uint32_t unknown_20c;               /* 0x20c */
    uint32_t unknown_210;               /* 0x210 */
} W8SlfHeader;                          /* 0x214 */

typedef struct W8SlfDirectoryEntry {
    char path[256];                     /* 0x000 */
    uint32_t data_offset;               /* 0x100 */
    uint32_t data_size;                 /* 0x104 */
    uint32_t status_108;                /* 0x108: low byte zero means active */
    uint64_t file_time;                 /* 0x10c: Windows FILETIME representation */
    uint32_t unknown_114;               /* 0x114 */
} W8SlfDirectoryEntry;                  /* 0x118 */

typedef struct W8SlfLiveEntry {
    char *path;                          /* 0x00: separately allocated */
    uint32_t data_size;                 /* 0x04 */
    uint32_t data_offset;               /* 0x08 */
} W8SlfLiveEntry;                       /* 0x0c */

typedef struct W8SlfArchiveState {
    char *base_path;                    /* 0x00 */
    void *archive_file;                 /* 0x04: Win32 HANDLE */
    uint16_t active_entry_count;        /* 0x08 */
    uint8_t is_open;                    /* 0x0a */
    uint8_t unknown_0b;                 /* 0x0b */
    uint32_t unknown_0c;                /* 0x0c */
    uint32_t unknown_10;                /* 0x10 */
    uint32_t lookup_bucket_count;       /* 0x14: initialized to 0x14 */
    W8SlfLiveEntry *entries;            /* 0x18 */
    void *lookup_buckets;               /* 0x1c: 0x140-byte allocation */
    void *mapping_handle;               /* 0x20 */
    void *mapping_view;                 /* 0x24 */
} W8SlfArchiveState;                    /* 0x28 */

typedef struct W8SlfConfiguration {
    char archive_path[256];             /* 0x000 */
    uint8_t enabled;                    /* 0x100 */
    uint8_t allow_fallback;             /* 0x101 */
    uint8_t map_file;                   /* 0x102 */
} W8SlfConfiguration;                   /* 0x103 */

#pragma pack(pop)

#endif
