#ifndef WIZ8_LAYOUTS_NPC_STATE_H
#define WIZ8_LAYOUTS_NPC_STATE_H

#include "compat/ptr32.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/layouts/item_instance.h"
#include "wiz8/layouts/plist.h"

struct W8Character;
struct W8NpcScriptFile;

/* One entry in an NPC's stock list. */
struct W8NpcItemEntry {
    /* 0x00: the game-clock stamp before which the entry is not ordinary trade
       stock. Zero is the ordinary tradeable entry; restocking writes a clock
       reading plus a delay here. */
    unsigned int available_at;
    W8ItemInstance item;    /* 0x04 */
    unsigned char quantity; /* 0x10: non-stack remaining quantity */
}; /* 0x14 */

#pragma pack(push, 1)

struct W8NpcState {
    W8_PTR32(W8NpcScriptFile) script_file; /* 0x00: the NPC's .nsf script file */
    /* 0x04: spawned flag; FindNpcOfKind returns it as the "in the world"
       answer, facts/combat paths set it, and 0xffff is the never-set
       sentinel normalized to zero on load. */
    unsigned short spawned;
    W8_PTR32(W8NpcDatabaseRecord) record; /* 0x06 */
    W8_PTR32(W8PList) items;              /* 0x0a: W8NpcItemEntry* elements */
    /* 0x0e and 0x12: two world-clock stamps, both set when the stock is first
       populated. The stock-rule pass reruns once the first is more than 0xa8c0
       old, and the decay and restock pass once the second is more than 0x15180
       old. */
    int restock_clock;     /* 0x0e */
    int maintenance_clock; /* 0x12 */
    int location_id;       /* 0x16 */
    bool has_monster;      /* 0x1a */
    /* 0x1b: the NPC's disposition. Setting a band writes one of three
       representative values rather than a range. */
    unsigned char disposition;
    bool dismissed_flag;
    /* 0x1d: raised by CreateNpcRuntimeNode; the greeting quote (0) and the
       first-interaction paths lower it once the NPC has been greeted. */
    bool greeting_pending;
    unsigned int dismissed_timer;
    /* 0x22/0x23: cleared when the dialogue NPC is staged. */
    bool flag;
    bool flag0;
    /* 0x24: the level-band byte GetLevelBand returns for the bound level. */
    unsigned char level_band;
    bool is_present; /* 0x25 */
    bool is_grouped; /* 0x26 */
    /* 0x27: the NPC's group-member character. CreateNpcRuntimeNode allocates
       the 0x1862-byte block only when the record belongs to a group, and the
       state reset deletes it here. */
    W8_PTR32(W8Character) character;
    signed char group_index; /* 0x2b */
    /* 0x2c: this node's own slot in g_npc_states, written by
       CreateNpcRuntimeNode; the release pass follows the index a partner
       names. */
    unsigned char partner_index;
    /* 0x2d: the NPC's monster has noticed the party once; the sight path
       raises it to fire the one-shot surprise/bark event. */
    bool party_noticed;
    /* 0x2e: the naming-style id a fact can substitute; values run to 0x85,
       past the signed-char range. */
    unsigned char name_style;
    /* 0x2f: the loaded level id the binding is stamped for. */
    unsigned char bound_level;
    /* 0x30: the forty item ids copied out of the record's item table, -1 for
       an unused slot. */
    short item_ids[40];
    /* 0x80: the purse, from the record's gold field. */
    int gold;
    /* 0x84: theft suspicion; each theft attempt raises it toward 0x64 and it
       scales the theft score down. */
    unsigned char suspicion;
    /* 0x85: bitmask the refusal callback reads and sets one bit per queued
       refusal quote (0x67, 0x68, 0x69). */
    unsigned int refusal_flags;
    /* 0x089: five topics stored one more than their id so zero means empty. */
    int topics[5];
    char restore_entity_name[0x28]; /* 0x9d: FindEntityByName key for restore */
    /* 0x0c5/0x0c6: the pending-restore flag and the level it belongs to. */
    bool pending_restore;
    unsigned char pending_restore_level;
    /* 0x0c7: set when the NPC binding is released while its record flag at
       0x054 is set, and tested before handing the binding back out. */
    unsigned char binding_unavailable;
    bool talk_cooldown_active;
    bool trade_cooldown_active;
    /* 0x0ca: the record's word at 0x002, copied by CreateNpcRuntimeNode. */
    unsigned short trade_pool;
    int talk_cooldown_clock;
    int trade_cooldown_clock;
    unsigned char service_flags[0x14];
    /* 0x0e8: the bound character's highest condition reached a serious
       band; cleared when the character recovers or the level-entry binding
       reset runs. */
    unsigned char incapacitated;
    /* 0x0e9 and 0x114: two flags raised together when the NPC is marked. */
    unsigned char event_pending;
    /* 0x0ea: the NPC is restored into the current level and available for
       binding; cleared while a restore is pending. */
    bool restored;
    /* 0x0eb: world clock of the last event that ran for this NPC. */
    int event_clock;
    /* 0x0ef: disposition band snapshot taken when dialogue opens. */
    unsigned char disposition_at_open;
    /* 0x0f0/0x0f1: death-save assist offer, selected by name style. */
    unsigned char healer_assist;
    unsigned char item_assist;
    /* 0x0f2: fourteen facts, appended in order and terminated by zero. */
    short known_facts[14];
    /* 0x10e: the item-count dice of the record's item table. */
    W8Dice item_count_dice;
    /* 0x112/0x113: the monster-binding release flag and the level it is
       stamped for. */
    bool pending_release;
    unsigned char pending_release_level;
    unsigned char restore_done;
    /* 0x115: the forty entry weights matching item_ids; only the slots
       whose table selector was set carry a weight. */
    unsigned char item_weights[40];
}; /* 0x13d by allocation */

#pragma pack(pop)

static_assert(sizeof(W8NpcState) == 0x13d, "W8NpcState_size");

#endif
