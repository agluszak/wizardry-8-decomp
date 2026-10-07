#pragma once

#include "wiz8/attack_modes.h"

unsigned char LoadHitSoundDatabase(void);
void ReleaseHitSoundDatabase(void);

struct W8CombatCharacterRow;
struct W8CombatSlot;
struct W8HandAttack;
struct W8MonsterAttack;
class W8Missile;

/* Play one Data\Sound\Combat\<name>.wav; a variant count above
   one appends a random digit onto the caller's writable name buffer, and the
   flag registers the handle on the combat state. */
void PlayCombatSound(char* sound_name, unsigned int variant_count, bool store_handle, int volume);
/* Shared weapon-class/armour-material impact lookup. */
char* GetMaterialImpactSound(int weapon_class, int target_material);
/* Play the attacking hand's swing sound; the attack mode is unused. */
void MakePCAttackSound(W8CombatCharacterRow* row, const W8HandAttack* hand_attack, W8AttackMode,
                       bool store_handle, int volume);
/* Melee hit sound for a PC's paired weapon striking the target's
   armour at one location. */
void MakePCMeleeHitSound(int iChar, const W8HandAttack* hand_attack, W8CombatSlot* target,
                         int hit_location, int volume);
/* Play the hit sound for a missile striking the target's armour
   at one location. */
void MakePCHitSound(W8Missile* missile, W8CombatSlot* target, int hit_location, int volume);
/* Melee hit sound for a monster striking the target's armour at
   one location; the selected attack supplies its signed weapon class. */
void MakeMonsterHitSound(const W8MonsterAttack* attack, W8CombatSlot* target, int hit_location,
                         int volume);
