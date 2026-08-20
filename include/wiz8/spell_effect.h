#pragma once

#include "wiz8/targeting.h"
#include "wiz8/vector.h"

class W8VectorElement005EBFE4;

/* Local Code\Magic.cpp. FindMonsterControlSpellEffect returns this same object
   to Levels.cpp, which places the target at 0x90 and the lure argument at 0xd0;
   SpawnLureEffects places the effect vector at 0x100. The retail destructor
   proves later subobjects exist, but their fields remain unrecovered. */
struct W8SpellEffectEntry {
    ~W8SpellEffectEntry();

    int kind;                                           /* 0x000 */
    int turns_remaining;                                /* 0x004 */
    unsigned char unknown_008[0x88];
    W8CombatSlot target;                                 /* 0x090 */
    unsigned char unknown_0b0[0x20];
    int argument;                                       /* 0x0d0 */
    unsigned char unknown_0d4[0x2c];
    W8GrowableVector<W8VectorElement005EBFE4*> effects; /* 0x100 */
};

extern W8GrowableVector<W8SpellEffectEntry*> g_spell_effects;

W8SpellEffectEntry* FindMonsterControlSpellEffect(void);
void SpawnLureEffects(
    W8SpellEffectEntry* owner, int argument, const W8CombatSlot* target);
