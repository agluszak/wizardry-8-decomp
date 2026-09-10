#pragma once

#include "surrender/srMath.h"

struct W8CombatSlot;
struct W8SpellEffectEntry;

/* Local Code\Magic Effects.cpp. Function554540 at 0x00554540 follows the
   unit's assertion-backed hull (0x00553910) and precedes Formation & Facing's
   at 0x005545F0, so it lives with its defining unit. */

unsigned char Function554540(void);

void ApplyEffectToTarget(
    int* result, W8CombatSlot* target, int arg_3, int arg_4);            /* 0x00552250 */
void Function54BA00(W8SpellEffectEntry* effect);         /* 0x0054BA00 */
void Function54C930(W8SpellEffectEntry* effect);         /* 0x0054C930 */

