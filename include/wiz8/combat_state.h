#ifndef WIZ8_COMBAT_STATE_H
#define WIZ8_COMBAT_STATE_H

void RoundPhaseToStep(unsigned int* phase, unsigned int base);

#include "wiz8/targeting.h"
#include "wiz8/game_status.h"

struct W8Character;
struct W8MonsterInfo;

#pragma pack(push, 1)
/* One party slot row. Only the fields reached by recovered combat and
   targeting code are named. */
typedef struct W8PartySlotRow {
    unsigned char occupied;               /* 0x00: gStatus.XChar[slot].fOccupied */
    int pending_action;
    int attack_mode[4];
    /* 0x15: the pending action's own two-word block, the same shape a chosen
       action carries. GetSlotChosenAction returns it for the out-of-combat
       context; its item member is restored from the saved item reference. */
    W8ActionDetailBlock pending_action_detail_015;
    W8CombatSlot target_out_of_combat;
    /* 0x3d: the action chosen for the in-combat context, its detail word, and
       the action's own two-word block. A use-item action holds the aimed item
       in the block's item member. */
    int action_03d;
    int action_detail_041;
    W8ActionDetailBlock action_detail_045;
    W8CombatSlot target_in_combat;
    int action_kind;
    int action_detail;
    int spell_id;
    int spell_power_level;
    int spell_power_extra;
    W8CombatSlot spell_target;
    int item_use_kind;
    W8ItemInstance* item_in_use;
    W8CombatSlot item_target;
    int item_id_0c9;
    unsigned char item_origin;
    unsigned short item_slot;
    unsigned char flag_0d0;
    W8CombatSlot target_context_5;
    unsigned char unknown_0f1[4];
    unsigned char flag_0f5;
    unsigned char unknown_0f6[4];
    int animation_0fa;
    /* 0x0fe: cleared by the level-entry NPC-binding reset. */
    unsigned char flag_fe;
    unsigned char unknown_ff[4];
    /* 0x103: portrait advance is only allowed while this is set. */
    unsigned char flag_103;
    unsigned char action_is_kind_one;
    unsigned char flag_105;
} W8PartySlotRow;

static_assert(sizeof(W8PartySlotRow) == 0x106,
              "W8PartySlotRow_must_be_0x106");

/* One record per character class, 0x1e5 bytes, indexed by the class index a
   combat actor carries at its +0x1d8. Only the flag the combat toggle reads is
   established. */
typedef struct W8CharacterClassRecord {
    unsigned char unknown_000[0x154];
    unsigned char flag_154;               /* 0x154 */
    unsigned char unknown_155[0x90];
} W8CharacterClassRecord;                 /* 0x1e5 */

/* What the engaged-actor iterator at 0x004A2760 hands back. Only the class
   index is placed; the object is much larger and otherwise unrecovered. */
typedef struct W8CombatActor {
    unsigned char unknown_000[0x1d8];
    int class_record_index;               /* 0x1d8 */
} W8CombatActor;

/* One combat participant's row, 0xd4 bytes per character. The eight of them
   begin at the combat state's own address, so W8CombatState's leading fields
   are the first row's; only the fields the fatigue, death and engagement paths
   touch are established. */
typedef struct W8CombatCharacterRow {
    unsigned char unknown_00[0x18];
    int value_18;                         /* 0x18: cleared when the character dies */
    unsigned char unknown_1c[0x30];
    unsigned char flag_4c;                /* 0x4c: raised when the character dies */
    unsigned char unknown_4d[0x37];
    int current_hand;                     /* 0x84: indexes the slot row's attack modes */
    int current_equip_slot;               /* 0x88: indexes the character's equipment */
    unsigned char unknown_8c[0x0d];
    unsigned char flag_099;               /* 0x99: toggled when an attack action is chosen */
    unsigned char unknown_9a[0x3a];
} W8CombatCharacterRow;                  /* 0xd4 */

/* The block the pointer at 0x006836A8 addresses: the engine's combat state.
   Only what a ported body reaches is named, and only where the use establishes
   a meaning. */
typedef struct W8CombatState {
    unsigned char flag_000;               /* 0x000: blocks ending combat while set */
    unsigned char flag_001;
    unsigned char unknown_002[2];
    unsigned int value_004;               /* 0x004: blocks ending combat while non-zero */
    int round_counter;                    /* 0x008 */
    unsigned char unknown_00c[0x7a4];
    int selected_slot;                    /* 0x7b0: cleared with selected_monster */
    int selected_character;               /* 0x7b4: -1 when nobody's turn */
    struct W8MonsterInfo* selected_monster; /* 0x7b8 */
    unsigned char unknown_7bc[0x104];
    W8CombatActor* engaged_actor;         /* 0x8c0 */
    unsigned char unknown_8c4[0x24];
    int pending_deaths[8];                /* 0x8e8 */
    int pending_death_count;              /* 0x908 */
    int pending_move_kind;                /* 0x90c */
    int movement_mode;                    /* 0x910 */
    unsigned char unknown_914[4];
    int turn_phase;                       /* 0x918 */
    unsigned char unknown_91c[4];
    int saved_formation[0x21];            /* 0x920 */
    unsigned char unknown_9a4[0xac];
    unsigned char flag_a50;
    unsigned char flag_a51;
    unsigned char unknown_a52[2];
    unsigned char flag_a54;
    unsigned char unknown_a55[0xd];
    unsigned char flag_a62;               /* 0xa62: party combat-ready bit */
} W8CombatState;
#pragma pack(pop)

extern "C" {

extern W8CombatState* g_combat_state;    /* 0x006836A8 */
extern W8CombatCharacterRow* g_combat_character_rows;
extern W8CharacterClassRecord* g_character_class_records; /* 0x0065BDE0 */

}

/* These are the two heap-buffer fields at the head of gXStatus, not separate
   globals.  Their retail addresses are the addresses of those pointer fields. */
#define g_party_characters \
    (g_status_685170.buffers.characters)
#define g_party_slot_rows \
    (g_status_685170.buffers.party_rows)


void RecordCharacterDeath(int party_slot);
void DropCharacterFromRound(int party_slot);
/* 0x004E79A0: whether one party slot may switch to the given targeting
   context, in the two forms the target-refresh pass asks. */
unsigned char CharacterCanSwitchTo(
    int party_slot, int context, int arg_3, int arg_4);
unsigned char TryCharacterAction(int party_slot, int action, char commit);
void NotifyNearbyMonsters(int what);
void CombatLog(const char* format, ...);

struct W8MonsterRecord;

void BeginCombatRound(void);
bool AnyoneStandsAhead(unsigned char position);
int GetBestMonsterAttackRange(const W8MonsterRecord* record, char close_quarters_only);
float CalcRangeDistance(int range_category);
void EndMonsterTurn(W8MonsterInfo* monster_info);
void SetSlotAction(int party_slot, int action_kind, int action_detail);
int GetHandAttackValue(int party_slot, unsigned int hand);
int NormalizeAttackMode(int attack_mode);
void ClearAttackBlock(void* block);
unsigned int ChooseAttackMode(unsigned int attack_modes);

unsigned char CanCharReBreathe(int party_slot);

#endif
