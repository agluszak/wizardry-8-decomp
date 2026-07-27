#ifndef WIZ8_FORMATS_NPC_DATABASE_H
#define WIZ8_FORMATS_NPC_DATABASE_H

#include <stdint.h>

#pragma pack(push, 1)

typedef struct W8NpcFactRule {
    uint32_t fact_id;                   /* 0x00 */
    uint16_t flags;                     /* 0x04: predicate/operator bits */
} W8NpcFactRule;                        /* 0x06 */

typedef struct W8NpcDatabaseRecord {
    uint16_t version;                   /* 0x000: two in the reviewed corpus */
    uint16_t unknown_002;               /* 0x002 */
    uint16_t name_aliases[40];          /* 0x004: packed UTF-16 aliases */
    uint8_t spawn_mode;                 /* 0x054: zero creates a persistent node */
    uint8_t has_inventory;              /* 0x055 */
    uint8_t unknown_056;                /* 0x056 */
    uint8_t has_rpc_character;          /* 0x057: allocates full character state */
    uint32_t record_id;                 /* 0x058: equals the zero-based index */
    uint8_t unknown_05c[0x18];          /* 0x05c */
    char entity_aliases[0x29];          /* 0x074: packed narrow aliases */
    uint8_t flag_9d;                    /* 0x09d: zero enables the v2 rule tail */
    char script_aliases[0x26];          /* 0x09e: packed narrow aliases */
    uint8_t unknown_0c4[0x206];         /* 0x0c4 */
    uint32_t fact_rules_runtime;        /* 0x2ca: runtime-owned rule container */
    uint8_t unknown_2ce[0x1d];          /* 0x2ce */
    uint32_t unknown_2eb;               /* 0x2eb */
    uint8_t unknown_2ef[0x1a];          /* 0x2ef */
} W8NpcDatabaseRecord;                  /* 0x309 */

#pragma pack(pop)

#endif
