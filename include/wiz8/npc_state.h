#ifndef WIZ8_NPC_STATE_H
#define WIZ8_NPC_STATE_H

#include "wiz8/3d_code/PList.h"
#include "wiz8/item_instance.h"
#include "wiz8/layouts/gameplay_databases.h"

struct W8Character;

/* One entry in an NPC's stock list. This one is deliberately outside the
   pack(1) block below: AddNpcItem allocates 0x14 bytes for it, which the packed
   size of 0x11 cannot produce. An ordinary naturally-aligned struct places
   every field at the offset the recovered bodies read and pads the tail to
   0x14, so the allocation is the size evidence. Offsets are identical either
   way; only the tail padding differs. See wiz8-4of.8 for the wider packing
   rule. */
typedef struct W8NpcItemEntry {
    /* 0x00: the game-clock stamp before which the entry is not ordinary trade
       stock. Zero is the ordinary tradeable entry, and the restock helper at
       0x0055AA80 writes a clock reading plus a delay here. */
    unsigned int available_at;
    W8ItemInstance item;                 /* 0x04 */
    unsigned char quantity;              /* 0x10: non-stack remaining quantity */
} W8NpcItemEntry;                        /* 0x14 by allocation */

#pragma pack(push, 1)

/* The database pointer sits unaligned at 0x06, which is what the byte-offset
   loads through it show, and everything reached by the recovered NPC bodies is
   placed off it. */
typedef struct W8NpcState {
    int unknown_00;
    unsigned short unknown_04;
    W8NpcDatabaseRecord* record;          /* 0x06 */
    W8PList* items;                       /* 0x0a: W8NpcItemEntry* elements */
    /* 0x0e and 0x12: two world-clock stamps, both set when the stock is first
       populated at 0x0055A630. 0x0055AFA0 reads them as separate windows: it
       reruns the stock-rule pass once 0x0e is more than 0xa8c0 old, and the
       decay and restock pass once 0x12 is more than 0x15180 old. */
    int restock_clock;                    /* 0x0e */
    int maintenance_clock;                /* 0x12 */
    int location_id;                      /* 0x16 */
    unsigned char has_monster;            /* 0x1a */
    /* 0x1b: the NPC's disposition. Setting a band writes one of three
       representative values rather than a range. */
    unsigned char disposition;
    unsigned char unknown_1c[9];
    unsigned char is_present;             /* 0x25 */
    unsigned char is_grouped;             /* 0x26 */
    /* 0x27: the NPC's group-member character. CreateNpcRuntimeNode allocates
       the 0x1862-byte block only when the record belongs to a group, and the
       state reset deletes it here. */
    W8Character* character;
    signed char group_index;              /* 0x2b */
    /* 0x2c: g_npc_states index whose monster binding this NPC's release
       follows. */
    unsigned char partner_index_2c;
    unsigned char unknown_2d;
    /* 0x2e: the space character selects the naming style whose name a fact can
       substitute. */
    char name_style;
    unsigned char unknown_2f[0x5a];
    /* 0x089: five topics stored one more than their id so zero means empty. */
    int topics[5];
    unsigned char unknown_9d[0x2a];
    /* 0x0c7: set when the NPC binding is released while its record flag at
       0x054 is set, and tested before handing the binding back out. */
    unsigned char unknown_c7;
    unsigned char unknown_c8[0x21];
    /* 0x0e9 and 0x114: two flags raised together when the NPC is marked. */
    unsigned char marked_e9;
    /* 0x0ea: this NPC is a candidate for the scripted event pass. */
    unsigned char flag_ea;
    /* 0x0eb: world clock of the last event that ran for this NPC. */
    int event_clock_eb;
    unsigned char unknown_ef[3];
    /* 0x0f2: fourteen facts, appended in order and terminated by zero. */
    short known_facts[14];
    unsigned char unknown_10e[6];
    unsigned char marked_114;
    unsigned char unknown_115[0x15];
} W8NpcState;                             /* 0x12a partitioned */

#pragma pack(pop)

/* The NPC-side global frame operation: timed world events and the per-frame
   NPC state passes. */
void UpdateNpcEvents0050D530(void);

/* 0x00509890 and 0x00509920: lazily create and then empty the shared NPC-state
   vector, recreating a runtime node for every database record still in use. */
void InitializeNpcStates(void);
void ResetNpcStates(void);
/* 0x00509AA0: build one runtime state from its database record. */
void CreateNpcRuntimeNode(int npc_id);

int AddNpcItem(W8NpcState* npc, int item_id, unsigned int quantity);
W8NpcState* GetNpcState(int index);
W8NpcState* GetNpcStateByKind(int kind);
unsigned char Function50B8F0(unsigned int kind);
unsigned char GetNpcDispositionBand(W8NpcState* npc);
int AddNpcItemFromInstance(W8NpcState* npc, const W8ItemInstance* item, char quantity);
int AddNpcItemWithDelay(W8NpcState* npc, int item_id, unsigned int quantity, int delay);
char RateItemIdentifyDifficulty(W8NpcState* npc, int item_id);
int RestockNpcItems(W8NpcState* npc);
void ClearNpcItems(W8NpcState* npc);
void SortNpcItems(W8NpcState* npc);
unsigned char PopulateNpcStock(W8NpcState* npc);
unsigned char MaintainNpcStock(W8NpcState* npc, char force);
W8NpcItemEntry* GetNpcItemAt(W8NpcState* npc, int index);
unsigned int GetNpcItemCount(W8NpcState* npc);
unsigned char ConsumeNpcItemQuantity(W8NpcState* npc, int index, unsigned char quantity);
void DecayNpcInventory(W8NpcState* npc);

struct W8MonsterManagerEntry;

W8MonsterManagerEntry* GetNpcGroupEntry(W8NpcState* npc);

#endif
