#ifndef WIZ8_NPC_STATE_H
#define WIZ8_NPC_STATE_H

#include "wiz8/3d_code/PList.h"
#include "wiz8/item_instance.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/record_file_0055a480.h"

class Trigger;

struct W8Character;

/* One entry in an NPC's stock list. This one is deliberately outside the
   pack(1) block below: AddNpcItem allocates 0x14 bytes for it, which the packed
   size of 0x11 cannot produce. An ordinary naturally-aligned struct places
   every field at the offset the recovered bodies read and pads the tail to
   0x14, so the allocation is the size evidence. Offsets are identical either
   way; only the tail padding differs. See wiz8-4of.8 for the wider packing
   rule. */
struct W8NpcItemEntry {
    /* 0x00: the game-clock stamp before which the entry is not ordinary trade
       stock. Zero is the ordinary tradeable entry, and the restock helper at
       0x0055AA80 writes a clock reading plus a delay here. */
    unsigned int available_at;
    W8ItemInstance item;    /* 0x04 */
    unsigned char quantity; /* 0x10: non-stack remaining quantity */
}; /* 0x14 by allocation */

#pragma pack(push, 1)

/* The database pointer sits unaligned at 0x06, which is what the byte-offset
   loads through it show, and everything reached by the recovered NPC bodies is
   placed off it. */
struct W8NpcState {
    W8RecordFile0055A480* record_file; /* 0x00: quote/script file from 0x0055A480 */
    unsigned short unknown_04;
    W8NpcDatabaseRecord* record; /* 0x06 */
    W8PList* items;              /* 0x0a: W8NpcItemEntry* elements */
    /* 0x0e and 0x12: two world-clock stamps, both set when the stock is first
       populated at 0x0055A630. 0x0055AFA0 reads them as separate windows: it
       reruns the stock-rule pass once 0x0e is more than 0xa8c0 old, and the
       decay and restock pass once 0x12 is more than 0x15180 old. */
    int restock_clock;     /* 0x0e */
    int maintenance_clock; /* 0x12 */
    int location_id;       /* 0x16 */
    bool has_monster;      /* 0x1a */
    /* 0x1b: the NPC's disposition. Setting a band writes one of three
       representative values rather than a range. */
    unsigned char disposition;
    unsigned char unknown_1c;
    /* 0x1d: set by CreateNpcRuntimeNode when the node is built. */
    unsigned char unknown_1d;
    unsigned char unknown_1e[6];
    /* 0x24: the level-band byte Function42B740 returns for the bound level. */
    unsigned char level_band;
    bool is_present; /* 0x25 */
    bool is_grouped; /* 0x26 */
    /* 0x27: the NPC's group-member character. CreateNpcRuntimeNode allocates
       the 0x1862-byte block only when the record belongs to a group, and the
       state reset deletes it here. */
    W8Character* character;
    signed char group_index; /* 0x2b */
    /* 0x2c: this node's own slot in g_npc_states, written by
       CreateNpcRuntimeNode; the release pass follows the index a partner
       names. */
    unsigned char partner_index_2c;
    unsigned char unknown_2d;
    /* 0x2e: the space character selects the naming style whose name a fact can
       substitute. */
    char name_style;
    /* 0x2f: the loaded level id the binding is stamped for. */
    unsigned char bound_level;
    /* 0x30: the forty item ids 0x0050B9E0 copies out of the record's item
       table, -1 for an unused slot. */
    unsigned short item_ids_30[40];
    /* 0x80: the purse 0x004F8CB0 hands to AddPartyGold, from the record's
       gold field. */
    int gold_80;
    unsigned char unknown_84[5];
    /* 0x089: five topics stored one more than their id so zero means empty. */
    int topics[5];
    char restore_entity_name[0x28]; /* 0x9d: FindEntityByName key for restore */
    /* 0x0c5/0x0c6: the pending-restore flag and the level it belongs to. */
    unsigned char pending_restore;
    unsigned char pending_restore_level;
    /* 0x0c7: set when the NPC binding is released while its record flag at
       0x054 is set, and tested before handing the binding back out. */
    unsigned char binding_unavailable;
    unsigned char unknown_c8[2];
    /* 0x0ca: the record's word at 0x002, copied by CreateNpcRuntimeNode. */
    unsigned short unknown_ca;
    unsigned char unknown_cc[0x1c];
    /* 0x0e8: cleared by the level-entry NPC-binding reset. */
    unsigned char flag_e8;
    /* 0x0e9 and 0x114: two flags raised together when the NPC is marked. */
    unsigned char marked_e9;
    /* 0x0ea: this NPC is a candidate for the scripted event pass. */
    unsigned char flag_ea;
    /* 0x0eb: world clock of the last event that ran for this NPC. */
    int event_clock_eb;
    unsigned char unknown_ef[3];
    /* 0x0f2: fourteen facts, appended in order and terminated by zero. */
    short known_facts[14];
    /* 0x10e: the item-count dice of the item table 0x0050B9E0 copied. */
    W8Dice item_count_dice_10e;
    /* 0x112/0x113: the monster-binding release flag and the level it is
       stamped for. */
    unsigned char flag_112;
    unsigned char flag_113;
    unsigned char marked_114;
    /* 0x115: the forty entry weights matching item_ids_30; only the slots
       whose table selector was set carry a weight. */
    unsigned char item_weights_115[40];
}; /* 0x13d by allocation */

#pragma pack(pop)

/* The NPC-side global frame operation: timed world events and the per-frame
   NPC state passes. */
void UpdateNpcEvents0050D530(void);

/* 0x00509890 and 0x00509920: lazily create and then empty the shared NPC-state
   vector, recreating a runtime node for every database record still in use. */
void InitializeNpcStates(void);
void ResetNpcStates(void);
/* 0x00509AA0: build one runtime state from its database record and return it,
   reusing the slot of a released node when one is free. */
W8NpcState* CreateNpcRuntimeNode(int npc_id);
/* 0x0050AED0: expand the record's character block into a fresh character. */
unsigned char InitializeNpcCharacter(W8NpcState* npc, W8Character* character);
/* 0x0050B9E0: copy the record's item table into the state's runtime arrays. */
void InitializeNpcItemTable(W8NpcState* npc);

int AddNpcItem(W8NpcState* npc, int item_id, unsigned int quantity);
/* The NPC-side consequence pass the sight code runs when a marked NPC's
   binding is released. */
void Function50CF70(W8NpcState* npc, int mode);
/* 0x0050E650: the per-party-slot companion reset the binding reset runs. */
void Function50E650(int party_slot);
/* 0x0050C560: place or move the NPC's monster at the named world entity. */
unsigned char RestoreNpcMonster0050C560(W8NpcState* npc, char* entity_name);
/* 0x0050ABF0: the activation callback the rebinding installs on the level's
   NPC triggers. */
unsigned char Function50ABF0(Trigger* trigger);
/* 0x00524CA0: the NPC-side rebinding pass. */
void ReloadNpcScriptResources(W8NpcState* npc);
/* 0x00526E90: message-box idle processor for queued script lines. */
void ProcessMessageBoxQueue(void);
void ResetNpcBindingsForParty0050DB50(void);
void ClearPendingNpcLevelFlags0050C270(void);
void ReleaseNpcMonsterBindings0050C2E0(void);
void ReleaseMarkedNpcBindings0050DA00(void);
void RebindNpcLevelTriggers0050AC60(void);
W8NpcState* GetNpcState(int index);
W8NpcState* GetNpcStateByKind(int kind);
bool NpcLeadHasNameStyle(unsigned int kind);
/* 0x00509EA0: clear one NPC binding's monster link and hand the handle to the
   owned item-list teardown. */
void ReleaseNpcBinding(int value);
/* 0x0050A440: the NPC binding selected by a monster-list index, or null. */
W8NpcState* FindNpcBindingForMonster(unsigned int monster_list_index);
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
