#ifndef WIZ8_GAMEPLAY_MODIFIERS_H
#define WIZ8_GAMEPLAY_MODIFIERS_H

#include <stddef.h>

/*
 * The two packed records the gameplay-modifier passes accumulate through.
 *
 * A W8EffectSlot is one running effect or condition: the party keeps twelve in
 * the status block, a monster keeps twelve more, and combat keeps nine records
 * at +0x7c1. The six 0x11-byte records at combat +0x85a share storage with
 * later independently typed fields; their originating array bound is
 * unresolved. A W8GameplayModifierBlock is the 0x67-byte
 * accumulator those slots and the worn equipment fold into, one block per
 * character plus the party-wide block in the status record. Monsters keep
 * the same record at W8MonsterInfo::modifiers_1db.
 */

#pragma pack(push, 1)

/* One 0x11-byte effect slot. The id at +0x01 is both the condition id the
   HasCondition predicates compare and the index into the effect-visual table
   the clear path drops. ApplyPartyEffectSlots scales the amount at +0x05 by
   the percentage at +0x09 before accumulating it. */
struct W8EffectSlot {
    bool active;              /* 0x00 */
    int effect_id;            /* 0x01 */
    int amount;               /* 0x05: dword magnitude - ApplyCombatEffectSlot and the
                             0x3e detonation pass read and write it as one dword */
    unsigned int percent;     /* 0x09 */
    unsigned int duration_0d; /* 0x0d: remaining lifetime, aged down in whole
                                  minutes by AgeMonsterSight */
}; /* 0x11 */

static_assert(sizeof(W8EffectSlot) == 0x11, "W8EffectSlot_must_be_0x11");
static_assert(offsetof(W8EffectSlot, active) == 0x00, "W8EffectSlot_active");
static_assert(offsetof(W8EffectSlot, effect_id) == 0x01, "W8EffectSlot_effect_id");
static_assert(offsetof(W8EffectSlot, amount) == 0x05, "W8EffectSlot_amount");
static_assert(offsetof(W8EffectSlot, percent) == 0x09, "W8EffectSlot_percent");
static_assert(offsetof(W8EffectSlot, duration_0d) == 0x0d, "W8EffectSlot_duration_0d");

/* The 0x67-byte modifier accumulator. Its byte runs are fixed by the fold at
   0x0050F090: bytes 0x00..0x0b, 0x0c..0x12, 0x13..0x3b and 0x3c..0x41 are
   added, bytes 0x42..0x46 and 0x4a are boolean-OR'd, and bytes 0x47..0x49
   keep the larger value. Only the offsets another recovered pass names are
   labelled; the rest stay runs. */
struct W8GameplayModifierBlock {
    signed char value_00;       /* 0x00: added to initiative and to displayed hand damage */
    signed char value_01;       /* 0x01: added to the hand attack hit bonus */
    signed char value_02;       /* 0x02 */
    signed char value_03;       /* 0x03: added to the hand attack damage bonus */
    signed char armor_bonus_04; /* 0x04 */
    signed char armor_bonus_05; /* 0x05 */
    signed char damage_reduction_adjustment; /* 0x06: added to damage reduction */
    signed char resistance_bonus_all;        /* 0x07: added to every resistance */
    /* 0x08: flat damage applied once per elapsed minute. The character and
       monster aging paths both multiply it by the elapsed minute count. */
    unsigned char damage_per_minute;
    /* 0x09..0x0b: signed flat adjustments to the corresponding regeneration
       channels. Character aging applies them directly; monster regen rebuild
       folds the HP/stamina values into its rates. */
    signed char health_regen_adjustment;
    signed char stamina_regen_adjustment;
    signed char spell_regen_adjustment;
    signed char attribute_adjustments[7]; /* 0x0c .. 0x12 */
    signed char unknown_13[0x29];         /* 0x13 .. 0x3b: one per skill id */
    signed char resistance_bonus[6];      /* 0x3c .. 0x41 */
    /* 0x42..0x44: independent +50% regeneration-rate latches. */
    unsigned char boost_health_regen;
    unsigned char boost_stamina_regen;
    unsigned char boost_spell_regen;
    unsigned char out_of_formation; /* 0x45 */
    unsigned char flag_46;          /* 0x46: set by effect id 0x11 */
    unsigned char light_47;         /* 0x47: the doubled light value the sky node reads */
    unsigned char value_48;         /* 0x48: max-combined, effect id 0x21 */
    unsigned char value_49;         /* 0x49: max-combined, effect id 0x1a */
    unsigned char flag_4a;          /* 0x4a: set by effect id 0x2d, the sight light gate */
    unsigned char value_4b;         /* 0x4b: added to armor-class component 8 */
    unsigned char unknown_4c[0x1b]; /* 0x4c .. 0x66 */
}; /* 0x67 */

static_assert(offsetof(W8GameplayModifierBlock, damage_reduction_adjustment) == 0x06,
              "W8GameplayModifierBlock_damage_reduction_offset");
static_assert(offsetof(W8GameplayModifierBlock, attribute_adjustments) == 0x0c,
              "W8GameplayModifierBlock_attribute_adjustments_offset");
static_assert(sizeof(W8GameplayModifierBlock) == 0x67, "W8GameplayModifierBlock_must_be_0x67");

#pragma pack(pop)

/* The spell that fills each being effect slot; the monster side indexes
   W8MonsterInfo::effect_slots_10f, the party side
   W8GameStatus::effect_slots_17af. */
extern const int g_being_effect_slot_spells_00616d84[12];
/* The combat-state spell per effect slot, walked against
   W8CombatState::effect_slots and the monster's effect_slots_3e. */
extern const int g_combat_effect_slot_spells_00616db4[9];
/* The same mapping for the second combat effect block, indexed against
   W8CombatState::effect_slots_85a and the monster's effect_slots_d7. */
extern const int g_combat_effect_slot_spells_and_cast_success_00616dd8[23];

#endif
