#ifndef WIZ8_LAYOUTS_COMBAT_STATE_H
#define WIZ8_LAYOUTS_COMBAT_STATE_H

#include "wiz8/gameplay_modifiers.h"
#include "wiz8/layouts/party_formation.h"
#include "wiz8/layouts/targeting.h"

struct W8Character;

struct W8MonsterInfo;
class W8Missile;

#pragma pack(push, 1)
/* One party slot row. Only the fields reached by recovered combat and
   targeting code are named. */
struct W8PartySlotRow {
    unsigned char occupied; /* 0x00: gStatus.XChar[slot].fOccupied */
    int pending_action;
    int attack_mode[4];
    /* 0x15: the pending action's own two-word block, the same shape a chosen
       action carries. ChooseCombatAction returns it for the out-of-combat
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
    /* The spell's two-word detail block: power level plus an unused second
       word. ChooseCombatAction hands this block out for the spell context;
       StartCharacterSpellCast copies it through ChooseAction the same way. */
    W8ActionDetailBlock spell_detail;
    W8CombatSlot spell_target;
    /* The item-use two-word detail block: the use kind plus the item. */
    W8ActionDetailBlock item_detail;
    W8CombatSlot item_target;
    int item_id_0c9;
    unsigned char item_origin;
    unsigned short item_slot;
    unsigned char flag_0d0;
    W8CombatSlot target_context_5;
    /* 0x0f1: the slot's place in the marching order, the index of its entry
       in g_status_685170.dwords_18e0. */
    int party_order_0f1;
    unsigned char flag_0f5;
    unsigned char unknown_0f6[4];
    int animation_0fa;
    /* 0x0fe: cleared by the level-entry NPC-binding reset. */
    unsigned char flag_fe;
    unsigned int pending_event_type_ff; /* 0xff: last queued portrait event type */
    /* 0x103: portrait advance is only allowed while this is set. */
    unsigned char flag_103;
    unsigned char action_is_kind_one;
    unsigned char flag_105;
};

static_assert(sizeof(W8PartySlotRow) == 0x106, "W8PartySlotRow_must_be_0x106");

/* Nine 0x11-byte effect records at +0x7c1 are independently established.
   +0x85a..+0x8f3 overlaps later independently proven fields: six 0x11-byte
   records occupy +0x85a..+0x8bf, then engaged_missile at +0x8c0 and TargetHit
   at +0x8c5. CombatHasCondition at 0x00501250 walks g_combat_state from +0x85a
   with stride 0x11 and bound 9, so retail does read across that overlap. The
   stride is not a typed nine-element array. */
static_assert(sizeof(W8EffectSlot) == 0x11, "W8EffectSlot_must_be_0x11");

/* One combat participant's row, 0xd4 bytes per character. The eight rows live
   at +0x18 of the combat state, 0xd4 apart, so a row's offsets are
   element-relative: the +0x18 that once prefixed this record is the state's
   header, not part of every row. Only the fields the fatigue, death and
   engagement paths touch are established. */
struct W8CombatCharacterRow {
    unsigned int phase; /* 0x00: combat phase; cleared when the character dies */
    unsigned char unknown_04[0x30];
    unsigned char flag_34; /* 0x34: raised when the character dies */
    unsigned char unknown_35[3];
    /* 0x38: the two hand values GetCharacterTurnValue reuses once this row's
       turn is already set up. Retail indexes them from the combat-state base
       as dword stride 0x35; that is this field, not a second BSS array. */
    int saved_attack_value[2];
    unsigned char unknown_40[0x28];
    unsigned int uiSwingsRemaining; /* 0x68: exact name from the attack assertions */
    int current_hand;               /* 0x6c: indexes the slot row's attack modes */
    int current_equip_slot;         /* 0x70: indexes the character's equipment */
    unsigned char unknown_74[0x0c];
    unsigned char flag_80; /* 0x80 */
    unsigned char flag_81; /* 0x81: toggled when an attack action is chosen */
    unsigned char unknown_82[0x1a];
    /* 0x9c: the combat clock value when CatchUpCombatActor last advanced this
       row's phase (its inlined copies stamp g_combat_state->round_counter
       here); the spell-scaling paths read it as the character's combat pace. */
    unsigned int phase_clock_stamp;
    unsigned char unknown_a0[4];
    unsigned char flag_a4; /* 0xa4: raised when switching to an attack */
    unsigned char unknown_a5[0x2f];
}; /* 0xd4 */

static_assert(sizeof(W8CombatCharacterRow) == 0xd4, "W8CombatCharacterRow_must_be_0xd4");
static_assert(offsetof(W8CombatCharacterRow, saved_attack_value) == 0x38,
              "W8CombatCharacterRow_saved_attack_value_offset");

/* The block the pointer at 0x006836A8 addresses: the engine's combat state.
   The allocation is 0xa64 bytes and the eight per-character rows live at
   +0x18, 0xd4 apart. Only what a ported body reaches is named, and only where
   the use establishes a meaning. */
struct W8CombatState {
    unsigned char flag_000; /* 0x000: blocks ending combat while set */
    unsigned char flag_001;
    unsigned char unknown_002[2];
    unsigned int value_004; /* 0x004: blocks ending combat while non-zero */
    int round_counter;      /* 0x008 */
    unsigned char unknown_00c[4];
    int value_010;
    int value_014;
    W8CombatCharacterRow characters[8]; /* 0x018, 0xd4 stride */
    unsigned char unknown_6b8[0xf8];
    /* 0x7b0: the exact member names the Combat.cpp action assertions report. */
    int eCombatActionStatus;                  /* 0x7b0 */
    int iActionChar;                          /* 0x7b4: -1 when nobody's turn */
    struct W8MonsterInfo* pActionMonsterInfo; /* 0x7b8 */
    unsigned char unknown_7bc[5];
    W8EffectSlot effect_slots[9];           /* 0x7c1, 0x11 stride */
    unsigned char effect_storage_85a[0x66]; /* 0x85a..0x8bf: six 0x11-byte records */
    W8Missile* engaged_missile;             /* 0x8c0: live missile that blocks ending combat */
    unsigned char unknown_8c4;              /* 0x8c4 */
    /* 0x8c5: exact name from the attack assertions; the slot is unaligned
       after the byte above, which packing makes representable. */
    W8CombatSlot TargetHit;
    unsigned char unknown_8e5[3];
    int pending_deaths[8];   /* 0x8e8 */
    int pending_death_count; /* 0x908 */
    /* 0x90c: the party-action fields the movement assertions pin. */
    unsigned int uiNextPartyAction;          /* 0x90c */
    unsigned int uiCurrentPartyAction;       /* 0x910 */
    unsigned int uiPartyActionPhase;         /* 0x914 */
    unsigned int uiCurrentPartyActionStatus; /* 0x918 */
    unsigned char unknown_91c[4];
    W8PartyFormationState saved_formation; /* 0x920 */
    unsigned char unknown_9a4[0xac];
    unsigned char flag_a50;
    unsigned char flag_a51;
    unsigned char unknown_a52[2];
    unsigned char flag_a54;
    unsigned char unknown_a55[0xd];
    unsigned char flag_a62;    /* 0xa62: party combat-ready bit */
    unsigned char unknown_a63; /* 0xa63: the allocation is 0xa64 bytes */
}; /* 0xa64 */

static_assert(sizeof(W8CombatState) == 0xa64, "W8CombatState_must_be_0xa64");
#pragma pack(pop)

extern W8CombatState* g_combat_state;          /* 0x006836A8 */
extern unsigned int g_combat_countdown_6850b0; /* 0x006850B0 */

#endif
