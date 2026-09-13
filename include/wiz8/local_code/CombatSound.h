#pragma once

unsigned char LoadHitSoundDatabase(void);
void ReleaseHitSoundDatabase(void);

struct W8CombatSlot;
class W8Missile;

/* 0x0054A0E0: play the hit sound for a missile striking the target's armour
   at one location; its own assertion spells the name. */
void MakePCHitSound(W8Missile* missile, W8CombatSlot* target, int hit_location, int arg_4);
