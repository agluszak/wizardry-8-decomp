#ifndef WIZ8_NPC_STATE_H
#define WIZ8_NPC_STATE_H

#include "wiz8/3d_code/PList.h"
#include "wiz8/item_instance.h"
#include "wiz8/layouts/gameplay_databases.h"

#pragma pack(push, 1)

typedef struct W8NpcItemEntry {
    int state;                            /* 0x00: zero is the ordinary trade entry */
    W8ItemInstance item;                 /* 0x04 */
    unsigned char quantity;              /* 0x10: non-stack remaining quantity */
} W8NpcItemEntry;                        /* 0x11 established */

/* The database pointer sits unaligned at 0x06, which is what the byte-offset
   loads through it show, and everything reached by the recovered NPC bodies is
   placed off it. */
typedef struct W8NpcState {
    unsigned char unknown_00[6];
    W8NpcDatabaseRecord* record;          /* 0x06 */
    W8PList* items;                       /* 0x0a: W8NpcItemEntry* elements */
    unsigned char unknown_0e[8];
    int location_id;                      /* 0x16 */
    unsigned char has_monster;            /* 0x1a */
    /* 0x1b: the NPC's disposition. Setting a band writes one of three
       representative values rather than a range. */
    unsigned char disposition;
    unsigned char unknown_1c[9];
    unsigned char is_present;             /* 0x25 */
    unsigned char is_grouped;             /* 0x26 */
    unsigned char unknown_27[4];
    signed char group_index;              /* 0x2b */
    unsigned char unknown_2c[2];
    /* 0x2e: the space character selects the naming style whose name a fact can
       substitute. */
    char name_style;
    unsigned char unknown_2f[0x5a];
    /* 0x089: five topics stored one more than their id so zero means empty. */
    int topics[5];
    unsigned char unknown_9d[0x4c];
    /* 0x0e9 and 0x114: two flags raised together when the NPC is marked. */
    unsigned char marked_e9;
    unsigned char unknown_ea[8];
    /* 0x0f2: fourteen facts, appended in order and terminated by zero. */
    short known_facts[14];
    unsigned char unknown_10e[6];
    unsigned char marked_114;
    unsigned char unknown_115[0x15];
} W8NpcState;                             /* 0x12a partitioned */

#pragma pack(pop)

W8NpcItemEntry* GetNpcItemAt(W8NpcState* npc, int index);
unsigned int GetNpcItemCount(W8NpcState* npc);
unsigned char ConsumeNpcItemQuantity(W8NpcState* npc, int index, unsigned char quantity);

#endif
