#ifndef WIZ8_LAYOUTS_COMBAT_STATE_H
#define WIZ8_LAYOUTS_COMBAT_STATE_H

#include "wiz8/gameplay_modifiers.h"
#include "wiz8/layouts/character.h"
#include "wiz8/layouts/party_formation.h"
#include "wiz8/layouts/targeting.h"
#include "wiz8/local_code/SpellEffect.h"
#include "wiz8/vector.h"

struct W8Character;

struct W8MonsterInfo;

struct W8SpellDamageReport;
class W8Missile;
class W8SpellVisual;

#pragma pack(push, 1)
/* One party slot row. Only the fields reached by recovered combat and
   targeting code are named. */
struct W8PartySlotRow {
    bool fOccupied; /* 0x00: gStatus.XChar[slot].fOccupied assertion spelling */
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
    /* 0x0d0: the non-melee W8_ACTION_* code ChooseAction stored for the slot,
       0xff when none; the action-key paths dispatch on it as "iActionState". */
    unsigned char queued_action;
    W8CombatSlot target_context_5;
    /* 0x0f1: the slot's place in the marching order, the index of its entry
       in g_status_685170.party_order_slots. */
    int party_order_index;
    unsigned char flag_0f5;
    /* 0x0f6: distance-scaled fatigue accumulator; every 2500 units convert
       into real fatigue via FatigueCharacter. */
    float movement_fatigue;
    int animation_0fa;
    /* 0x0fe: cleared by the level-entry NPC-binding reset. */
    unsigned char flag_fe;
    unsigned int pending_event_type_ff; /* 0xff: last queued portrait event type */
    /* 0x103: portrait advance is only allowed while this is set. */
    unsigned char flag_103;
    unsigned char action_is_berserk;
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

/* Per-hand best-outcome tracking inside a W8CombatCharacterRow, 0x10 bytes.
   The score is only overwritten when a swing resolves better than the stored
   one, at which point the hand's skills and the character's dual-wield flag
   are refreshed. */
struct W8CombatHandRecord {
    int score;
    int weapon_skill;
    int combat_skill;
    int dual_wielding;
};

static_assert(sizeof(W8CombatHandRecord) == 0x10, "W8CombatHandRecord_must_be_0x10");

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
    /* 0x40: the same two per-hand reach values PrepareCharacterAttacks writes
       into saved_attack_value; no recovered reader names the copy yet. */
    int hand_attack_values_40[2];
    /* 0x48: per-hand best-outcome tracking; the score is only overwritten
       when a swing resolves better than the stored one, at which point the
       hand's skills and the character's dual-wield flag are refreshed. */
    W8CombatHandRecord hand_records[2];
    unsigned int uiSwingsRemaining; /* 0x68: exact name from the attack assertions */
    int current_hand;               /* 0x6c: indexes the slot row's attack modes */
    int current_equip_slot;         /* 0x70: indexes the character's equipment */
    /* 0x74: the paired weapon slot GetPairedEquipSlot answered for
       current_equip_slot, -1 when nothing is paired with it. */
    int paired_equip_slot;
    /* 0x78/0x7c: the item record indexes of the weapon in the attacking hand
       and of the paired weapon (the primary's own when nothing is paired). */
    int weapon_item_id_78;
    int paired_item_id_7c;
    /* 0x80: berserk latch - interrupt case 8 sets it; while set the slot
       retargets onto friends and skips the enemy-hostility bookkeeping.
       Same interrupt sets berserk_015 on monsters. */
    bool berserk_80;
    /* 0x81: toggled when the slot swaps to its alternate hand in PC Item;
       while set the pending hand-attack values are rebuilt. */
    bool alternate_hand_81;
    unsigned char unknown_82[2];
    /* 0x84/0x88: the slot's combat-portrait catalog image and the alternate the
       combat portrait strip draws while the slot is the hovered combat slot
       (party_slots_170[4]); -1 draws nothing. */
    int portrait_image_084;
    int portrait_image_alternate_088;
    /* 0x8c: the slot's combat-strip status recomputed each combat-mode frame:
       -1 slot empty or out of the fight, 0 ready, 1 cannot switch to combat,
       2 dead or ineligible, 3 the acting combatant (portrait pulses). */
    char combat_status_8c;
    unsigned char unknown_8d[3];
    /* 0x90: how many times the character already rolled to notice an attacker
       this round; the first attempt always succeeds and each later one is 25
       points harder on the senses check. */
    int spot_attempts_90;
    /* 0x94: incremented when an out-of-combat action is repicked during
       combat; the eight rows are addressed with the established 0xd4 stride.
       0x00541c00 compares it with JBE, so it is unsigned. */
    unsigned int pending_action_repick_count;
    /* 0x98/0x99: per-slot once-per-combat action-use flags read by
       CanPartySlotPray and CanPartySlotTurnUndead. */
    unsigned char pray_used;
    unsigned char turn_undead_used;
    unsigned char unknown_9a[2];
    /* 0x9c: the combat clock value when CatchUpCombatActor last advanced this
       row's phase (its inlined copies stamp g_combat_state->round_counter
       here); the spell-scaling paths read it as the character's combat pace. */
    unsigned int phase_clock_stamp;
    /* 0xa0: the round's interception count, checked against the guarding
       hand's attack count before another intercept is allowed and bumped on
       each successful one. */
    unsigned int interception_count;
    unsigned char flag_a4; /* 0xa4: raised when switching to an attack */
    /* 0xa5: the attack's sound/roll state; set once MakePCAttackSound has
       played so a resumed swing does not replay it, cleared when the row's
       attack finishes. */
    unsigned char flag_a5;
    unsigned char cheat_death_used;
    /* 0xa7: SetCharacterCombatAction raises it when the slot's queued action
       changes while an action runs; committing the pending block clears it. */
    bool action_changed_a7;
    /* 0xa8: one byte per skill id recording defensive use this round. Attack
       init raises Shield (0x06), Locks & Traps (0x0b) and Reflextion (0x26)
       when the target has the skill trained; the round-end pass awards
       practice credit to exactly those three entries and clears them. */
    unsigned char skill_use_flags[0x29];
    unsigned char unknown_d1[3];
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
    /* 0x001: set when combat begins and when continuous combat resumes;
       cleared at the round boundary while continuous_combat is off. Gates
       party movement and the combat-sensitive UI panels. */
    bool round_active_001;
    unsigned char unknown_002[2];
    /* 0x004: current round number - incremented at each round boundary,
       shown in the round notices and gating the round-one specials. */
    unsigned int round_count_004;
    unsigned int round_counter; /* 0x008: bounded combat phase, 1..100 */
    /* 0x00c: the combat outcome the end-of-combat pass reports - zero while no
       result is recorded, otherwise the kill count formatted next to
       "kill"/"kills". */
    int combat_result_00c;
    /* 0x010: experience from kills, divided among the active party members
       at combat end. */
    int experience_pool_010;
    /* 0x014: flat bonus experience accumulated by hostility events, added to
       the pool at award time. */
    int experience_bonus_014;
    W8CombatCharacterRow characters[8]; /* 0x018, 0xd4 stride */
    /* 0x6b8: the attack announcement the monster-attack message builder
       swprintf's into and ShowNotice displays; 0x78 wide chars. */
    wchar_t attack_message_6b8[0x78];
    /* 0x7a8: continuous-combat UI pacing; the confirm button resets it while
       ClockIsTicking reports it still running. */
    unsigned int combat_ui_timer_7a8;
    /* 0x7ac: the pacing clock the scheduler arms through SetCountdownClock
       before the scheduled actor's action may execute. */
    unsigned int action_clock_7ac;
    /* 0x7b0: the exact member names the Combat.cpp action assertions report. */
    int eCombatActionStatus;                  /* 0x7b0 */
    int iActionChar;                          /* 0x7b4: -1 when nobody's turn */
    struct W8MonsterInfo* pActionMonsterInfo; /* 0x7b8 */
    unsigned int hit_sound_7bc;
    bool hit_sound_active_7c0;
    W8EffectSlot effect_slots[9];     /* 0x7c1, 0x11 stride */
    W8EffectSlot effect_slots_85a[6]; /* 0x85a..0x8bf */
    W8Missile* engaged_missile;       /* 0x8c0: live missile that blocks ending combat */
    /* 0x8c4: staged hit result of the in-flight missile (0 = pending, 1 = hit, 2 = deflected) */
    char missile_hit_result;
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
    /* 0x91c: countdown used to pace synthetic movement progress when
       continuous combat is enabled and the world did not advance this frame. */
    unsigned int party_movement_clock;
    W8PartyFormationState saved_formation; /* 0x920 */
    /* 0x9a4: the running attack's target did not see it coming - the monster
       stands behind the character or the target monster is looking away. The
       announcement appends the caught-unaware text. */
    unsigned char unaware_9a4;
    /* 0x9a5: the running attack is a kind-3 monster's natural attack inside
       short range, so the announcement skips the weapon-name string and uses
       the natural-attack verbs instead. */
    unsigned char natural_attack_9a5;
    /* 0x9a6: the running attack's outcome record, cleared to a fresh 0xa2-byte
       block before each attack resolves. The swing and hit tallies feed the
       "hit once"/"hit %d of %d" notices, one flag per condition records what
       the swings inflicted (index 0x12 doubles as the killed gate), the report
       list is a W8SpellDamageReport* vector drained front-first, and the six
       notice values carry the amounts the notices print - [3] and [4]
       are the running damage totals the character/monster damage paths add to. */
    W8SpellEffectResult attack_report;
    /* 0xa48: the once-per-combat difficulty evaluation has run; the update
       tick calls the evaluator on the first frame it sees this clear. */
    unsigned char combat_evaluated_a48;
    unsigned char unknown_a49[3];
    /* 0xa4c: the in-flight breath visual for a character's special-attack
       action. The executor refuses while its `finished` flag is clear and
       hands the previous one to the world updater through `auto_release`. */
    W8SpellVisual* breath_visual_a4c;
    unsigned char flag_a50;
    unsigned char flag_a51;
    /* 0xa52/0xa53: the side is surprised and cannot act this round - +0xa52
       gates the party slots and the neutral/friendly monsters, +0xa53 the
       hostile ones; the surprise roll at combat start sets them, mutual
       surprise cancels both, and the round-end pass clears them. */
    unsigned char party_surprised_a52;
    unsigned char monsters_surprised_a53;
    unsigned char flag_a54;
    unsigned char flag_a55;
    /* 0xa56: consecutive rounds that ended with an engaged flag still set but
       no monster group able to engage; the combat-over check reads it. */
    unsigned char unengaged_rounds_a56;
    bool notice_scroll_pending_a57;
    /* 0xa58: queued refusal script for NPC party slots 0 and 1. The sweep
       at 0x004ed710 sets a flag after queuing events 0x3a and 0x36; event
       0x36 clears it. 0x004ed460 tests exactly these two slots. */
    bool npc_combat_script_pending[2];
    unsigned char unknown_a5a[2];
    /* 0xa5c: the combat updates elapsed; the engagement sweep waits for the
       third before it touches group states. */
    unsigned int combat_update_count;
    /* 0xa60: the scheduler's pacing latch - set by the unrecovered combat
       code, it suppresses a second action delay for a monster's turn and
       caps the armed delay at 800 ms. */
    unsigned char unknown_a60;
    /* 0xa61: remembered search-mode state; the combat teardown toggles search
       mode back on when it reads nonzero. */
    unsigned char unknown_a61;
    unsigned char flag_a62;    /* 0xa62: party combat-ready bit */
    unsigned char unknown_a63; /* 0xa63: the allocation is 0xa64 bytes */
}; /* 0xa64 */

static_assert(sizeof(W8CombatState) == 0xa64, "W8CombatState_must_be_0xa64");
static_assert(offsetof(W8CombatState, combat_ui_timer_7a8) == 0x7a8,
              "W8CombatState_combat_ui_timer_7a8_offset");
static_assert(offsetof(W8CombatState, eCombatActionStatus) == 0x7b0,
              "W8CombatState_eCombatActionStatus_offset");
static_assert(offsetof(W8CombatState, npc_combat_script_pending) == 0xa58,
              "W8CombatState_npc_combat_script_pending_offset");
static_assert(offsetof(W8CombatState, combat_update_count) == 0xa5c,
              "W8CombatState_combat_update_count_offset");
#pragma pack(pop)

extern W8CombatState* g_combat_state; /* 0x006836A8 */

#endif
