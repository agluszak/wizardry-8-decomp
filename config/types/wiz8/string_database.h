#ifndef WIZ8_FORMATS_STRING_DATABASE_H
#define WIZ8_FORMATS_STRING_DATABASE_H

#include <stdint.h>

#pragma pack(push, 1)

typedef struct W8StringDatabaseHeader {
    uint32_t version;                   /* 0x00: one in the reviewed corpus */
    uint8_t encoded;                    /* 0x04: nonzero applies code-unit transform */
} W8StringDatabaseHeader;               /* 0x05 */

typedef struct W8StringDatabaseRecordHeader {
    uint32_t metadata_00;               /* 0x00 */
    uint32_t metadata_04;               /* 0x04 */
    uint32_t code_unit_count;           /* 0x08: maximum accepted value is 2000 */
} W8StringDatabaseRecordHeader;         /* 0x0c */

typedef struct W8StringDatabaseFooter {
    /* uint32_t record_offsets[record_count] precedes this fixed suffix. */
    uint32_t record_count;
    uint32_t reserved;                  /* zero in the reviewed corpus */
} W8StringDatabaseFooter;               /* 0x08 */

#pragma pack(pop)

#endif
