#ifndef WIZ8_FORMATS_SLF_H
#define WIZ8_FORMATS_SLF_H

#include <stdint.h>

#pragma pack(push, 1)

typedef struct LIBHEADER {
    char sLibName[256];                 /* 0x000 */
    char sPathToLibrary[256];           /* 0x100 */
    int32_t iEntries;                   /* 0x200 */
    int32_t iUsed;                      /* 0x204 */
    uint16_t iSort;                     /* 0x208 */
    uint16_t iVersion;                  /* 0x20a */
    uint8_t fContainsSubDirectories;    /* 0x20c */
    uint8_t padding_20d[3];             /* 0x20d */
    int32_t iReserved;                  /* 0x210 */
} LIBHEADER;                            /* 0x214: SGP source name */
typedef LIBHEADER W8SlfHeader;          /* compatibility alias */

typedef struct DIRENTRY {
    char sFileName[256];                /* 0x000 */
    uint32_t uiOffset;                  /* 0x100 */
    uint32_t uiLength;                  /* 0x104 */
    uint8_t ubState;                    /* 0x108: FILE_OK is zero */
    uint8_t ubReserved;                 /* 0x109 */
    uint8_t padding_10a[2];             /* 0x10a */
    uint64_t sFileTime;                 /* 0x10c: Win32 FILETIME */
    uint16_t usReserved2;               /* 0x114 */
    uint8_t padding_116[2];             /* 0x116 */
} DIRENTRY;                             /* 0x118: SGP source name */
typedef DIRENTRY W8SlfDirectoryEntry;   /* compatibility alias */

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
