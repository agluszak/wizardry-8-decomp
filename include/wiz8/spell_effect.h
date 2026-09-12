#pragma once

#include "wiz8/targeting.h"
#include "wiz8/text_types.h"
#include "wiz8/vector.h"

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
/* One pending damage or effect report. Kind 1 names a character by party slot
   and carries no text; kind 3 carries its own inline text from 0x08. The
   message pass frees each record after posting it. */
struct W8SpellDamageReport {
    int kind;           /* 0x00 */
    int value;          /* 0x04 */
    W8WideChar text[1]; /* 0x08 */
};

/* What one missile or queued effect accumulates while it resolves: the total
   amount, the number of hits, the twenty-band damage table, and the report
   records handed to the message pass. Both the missile and the effect embed
   this at their own offset. The totals and bands are unsigned: the message
   pass divides them with `div` and tests them with the unsigned branches. */
struct W8SpellEffectResult {
    unsigned int amount;                            /* 0x00 */
    unsigned int count;                             /* 0x04 */
    unsigned int damage[20];                        /* 0x08 */
    W8GrowableVector<W8SpellDamageReport*> reports; /* 0x58 */
};

static_assert(sizeof(W8SpellEffectResult) == 0x68, "W8SpellEffectResult_must_be_0x68");

#pragma pack(push, 1)
struct W8SpellEffectEntry {
    ~W8SpellEffectEntry(); /* 0x0042BAC0 */

    int kind;              /* 0x000 */
    int turns_remaining;   /* 0x004 */
    W8TargetSource source; /* 0x008 */
    unsigned char unknown_03c[0x20];
    /* The second source the hostility walk hands to 0x00547120; its
       reflection and backfire bytes are the two flags 0x00500930 tests
       before releasing the effect. */
    W8TargetSource target_source_05c; /* 0x05c */
    W8CombatSlot target;              /* 0x090 */
    unsigned char unknown_0b0[0x20];
    int argument; /* 0x0d0 */
    /* 0x0d4: the second cast argument the 0x4f finalizer forwards. */
    int value_0d4;
    unsigned char unknown_0d8[8];
    /* Two integer lists this body walks against the monster manager entries. */
    W8GrowableVector<int> values_0e0;          /* 0x0e0 */
    W8GrowableVector<int> monster_indices_0f0; /* 0x0f0 */
    W8GrowableVector<W8SpellVisual*> effects;  /* 0x100 */
    W8GrowableVector<W8Missile*> missiles;     /* 0x110 */
    unsigned char flag_120;                    /* 0x120 */
    unsigned char flag_121;                    /* 0x121 */
    unsigned char flag_122;                    /* 0x122 */
    unsigned char flag_123;                    /* 0x123 */
    /* 0x124: set once this effect's result has been reported. */
    unsigned char reported_124;
    unsigned char unknown_125;
    W8SpellEffectResult result_126; /* 0x126 */
    unsigned char unknown_18e[0x3a];
};
#pragma pack(pop)

static_assert(sizeof(W8SpellEffectEntry) == 0x1c8, "W8SpellEffectEntry_must_be_0x1c8");

extern W8GrowableVector<W8SpellEffectEntry*> g_spell_effects;

W8SpellEffectEntry* FindMonsterControlSpellEffect(void);
/* Advance every queued spell effect one frame. */
void UpdateSpellEffects00500930(void);
/* Fold one missile's accumulated damage and reports into the queued effect
   that owns it. */
void AbsorbMissileDamage00500460(W8Missile* missile);
void SpawnLureEffects(W8SpellEffectEntry* owner, int argument, const W8CombatSlot* target);
