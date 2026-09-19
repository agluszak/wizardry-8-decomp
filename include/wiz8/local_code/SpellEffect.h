#ifndef WIZ8_LOCAL_CODE_SPELL_EFFECT_H
#define WIZ8_LOCAL_CODE_SPELL_EFFECT_H

#include "wiz8/dice.h"
#include "wiz8/layouts/character.h"
#include "wiz8/layouts/targeting.h"
#include "wiz8/vector.h"

#include <stddef.h>

class W8SpellVisual;
class W8Missile;

/* Local Code\Magic.cpp. FindMonsterControlSpellEffect returns this same object
   to Levels.cpp, which places the target at 0x90 and the lure argument at 0xd0;
   SpawnLureEffects places the effect vector at 0x100. The retail destructor
   proves later subobjects exist, but their fields remain unrecovered.

   The queued effect's own frame update (0x00500930) proves the rest of the
   front: the source block at 0x08, the two flags at 0x77/0x78, the monster
   index list at 0xf0, the spawned visual list at 0x100, the missiles the
   cast owns at 0x110, and the four state bytes from 0x120. The destructor
   proves the fifth list sits unaligned at 0x17e, which is what makes the
   struct packed. */
/* One pending condition report. Kind 1 names a character by party slot and
   carries no text; kind 3 carries its own inline text from 0x08. Every creator
   allocates and clears the complete 0x6c-byte record, and the message pass
   frees it after posting. */
struct W8SpellDamageReport {
    int kind;         /* 0x00 */
    int value;        /* 0x04 */
    wchar_t text[50]; /* 0x08 */
};

static_assert(sizeof(W8SpellDamageReport) == 0x6c, "W8SpellDamageReport_must_be_0x6c");

#pragma pack(push, 1)
/* One spell effect definition, 0x30 bytes. A missile carries its own copy at
   0x1fc. The radius at 0x00 bounds an area effect (0 for a single target),
   the dice at 0x04 are rolled for the effect's size, the three values at
   0x20 through 0x2c combine into its duration, and the percentage at 0x24
   scales both. */
struct W8SpellEffectDefinition {
    float radius;     /* 0x00: an area effect reaches this far; 0 is single-target */
    W8Dice magnitude; /* 0x04 */
    /* 0x08: the percentage chance of each condition the effect can inflict,
       rolled by ApplyEffectConditions. */
    unsigned char condition_chances[0x10];
    int power_level;        /* 0x18 */
    int value_1c;           /* 0x1c */
    int duration_scale;     /* 0x20 */
    unsigned int percent;   /* 0x24 */
    int duration_base;      /* 0x28 */
    int duration_per_power; /* 0x2c */
};
#pragma pack(pop)

static_assert(sizeof(W8SpellEffectDefinition) == 0x30, "W8SpellEffectDefinition_must_be_0x30");

/* What one missile or queued effect accumulates while it resolves: the total
   amount, the number of hits, one count per condition, and the report records
   handed to the message pass. Both the missile and the effect embed this at
   their own offset. The totals and condition counts are unsigned: the message
   pass divides or tests them with unsigned instructions. */
#pragma pack(push, 1)
struct W8SpellEffectResult {
    unsigned int amount;                               /* 0x00 */
    unsigned int count;                                /* 0x04 */
    unsigned int condition_counts[W8_CONDITION_COUNT]; /* 0x08 */
    W8GrowableVector<W8SpellDamageReport*> reports;    /* 0x58 */
    /* 0x68..0xa2: the retail initializer clears the whole 0xa2-byte block;
       no field past the reports vector has been proven. */
    unsigned char unknown_68[0x3a];
};
#pragma pack(pop)

static_assert(sizeof(W8SpellEffectResult) == 0xa2, "W8SpellEffectResult_must_be_0xa2");

#pragma pack(push, 1)
struct W8SpellEffectEntry {
    ~W8SpellEffectEntry(); /* 0x0042BAC0 */

    int kind;            /* 0x000 */
    int turns_remaining; /* 0x004 */
    /* Magic.cpp asserts this as pOrigSource: the cast's original source. */
    W8TargetSource OrigSource; /* 0x008 */
    unsigned char unknown_03c[0x20];
    /* Magic Effects.cpp asserts this as pQueue->Source: the working source the
       hostility walk and effect bodies hand to CollectHostileMonsters / damage. */
    W8TargetSource Source; /* 0x05c */
    W8CombatSlot target;   /* 0x090 */
    /* 0x0b0: queued spell casts carry the whole 0x30-byte effect definition
       here (CastSpellFromSource copies it in); control/lure effects place
       their own argument pair at the same storage. */
    union {
        W8SpellEffectDefinition definition;
        struct {
            unsigned char unknown_0b0[0x20];
            int argument; /* 0x0d0 */
            /* 0x0d4: the second cast argument the 0x4f finalizer forwards. */
            int value_0d4;
            /* 0x0d8: the lingering-condition turns the 0x23 branch seeds from
               the rolled argument plus the target's existing count. */
            int value_0d8;
            unsigned char unknown_0dc[4];
        };
    };
    /* Two integer lists this body walks: the monster location ids
       CollectHostileMonsters gathers at 0x0e0, and a second index list at
       0x0f0 used both as party-slot indices and as monster-manager entry
       indices depending on the effect path. */
    W8GrowableVector<int> monster_ids_0e0;    /* 0x0e0 */
    W8GrowableVector<int> target_indices_0f0; /* 0x0f0 */
    /* 0x100: spawned visuals. The vector's data pointer is at +0x10c. */
    W8GrowableVector<W8SpellVisual*> spell_visuals; /* 0x100 */
    W8GrowableVector<W8Missile*> missiles;          /* 0x110 */
    unsigned char flag_120;                         /* 0x120 */
    unsigned char flag_121;                         /* 0x121 */
    unsigned char flag_122;                         /* 0x122 */
    unsigned char flag_123;                         /* 0x123 */
    /* 0x124: set once this effect's result has been reported. */
    unsigned char reported_124;
    unsigned char unknown_125;
    W8SpellEffectResult result_126; /* 0x126 */
};
#pragma pack(pop)

static_assert(sizeof(W8SpellEffectEntry) == 0x1c8, "W8SpellEffectEntry_must_be_0x1c8");
static_assert(offsetof(W8SpellEffectEntry, OrigSource) == 0x008, "W8SpellEffectEntry_OrigSource");
static_assert(offsetof(W8SpellEffectEntry, Source) == 0x05c, "W8SpellEffectEntry_Source");
static_assert(offsetof(W8SpellEffectEntry, target) == 0x090, "W8SpellEffectEntry_target");
static_assert(offsetof(W8SpellEffectEntry, definition) == 0x0b0, "W8SpellEffectEntry_definition");
static_assert(offsetof(W8SpellEffectEntry, monster_ids_0e0) == 0x0e0,
              "W8SpellEffectEntry_monster_ids");
static_assert(offsetof(W8SpellEffectEntry, target_indices_0f0) == 0x0f0,
              "W8SpellEffectEntry_target_indices");
static_assert(offsetof(W8SpellEffectEntry, spell_visuals) == 0x100,
              "W8SpellEffectEntry_spell_visuals");
static_assert(offsetof(W8SpellEffectEntry, missiles) == 0x110, "W8SpellEffectEntry_missiles");
static_assert(offsetof(W8SpellEffectEntry, reported_124) == 0x124, "W8SpellEffectEntry_reported");
static_assert(offsetof(W8SpellEffectEntry, result_126) == 0x126, "W8SpellEffectEntry_result");

extern W8GrowableVector<W8SpellEffectEntry*> g_spell_effects;

W8SpellEffectEntry* FindMonsterControlSpellEffect(void);
/* Advance every queued spell effect one frame. */
void UpdateSpellEffects00500930(void);
/* Fold one missile's accumulated damage and reports into the queued effect
   that owns it. */
void AbsorbMissileDamage00500460(W8Missile* missile);
void ReportSpellResult005005C0(W8SpellEffectEntry* effect);
void SpawnLureEffects(W8SpellEffectEntry* owner, int argument, const W8CombatSlot* target);

#endif
