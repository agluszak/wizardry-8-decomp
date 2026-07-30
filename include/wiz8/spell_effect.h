#pragma once

#include "wiz8/vector.h"

/* Local Code\Magic.cpp. Only the leading scheduler fields are established in
   that unit. The ordinary destructor is non-trivial and tears down the five
   vector subobjects later in the full effect object; declaring it here lets
   owners use normal delete without hand-writing compiler lifecycle output. */
struct W8SpellEffectEntry {
    ~W8SpellEffectEntry();

    int kind;
    int turns_remaining;
};

extern W8GrowableVector<W8SpellEffectEntry*> g_spell_effects;

W8SpellEffectEntry* FindMonsterControlSpellEffect(void);
