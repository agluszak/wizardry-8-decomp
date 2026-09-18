#pragma once

unsigned char LoadHitSoundDatabase(void);
void ReleaseHitSoundDatabase(void);

struct W8CombatCharacterRow;
struct W8CombatSlot;
struct W8HandAttack;
struct W8MonsterCombatState;
class W8Missile;

/* 0x005499D0: play one Data\Sound\Combat\<name>.wav; a variant count above
   one appends a random digit onto the caller's writable name buffer, and the
   flag registers the handle on the combat state. */
void PlayCombatSound005499D0(char* sound_name, unsigned int variant_count, bool store_handle,
                             int volume);
/* 0x00549EB0: shared weapon-class/armour-material impact lookup; the sibling
   callers inline it while Combat Attack.cpp calls the emitted copy. */
char* GetMaterialImpactSound00549EB0(int weapon_class, int target_material);
/* 0x00549EF0: play the attacking hand's swing sound; retail callers pass a
   third argument the body never reads. */
void MakePCAttackSound00549EF0(W8CombatCharacterRow* row, const W8HandAttack* hand_attack,
                               int arg_3, bool store_handle, int volume);
/* 0x00549F50: melee hit sound for a PC's paired weapon striking the target's
   armour at one location. */
void MakePCMeleeHitSound00549F50(int iChar, const W8HandAttack* hand_attack, W8CombatSlot* target,
                                 int hit_location, int volume);
/* 0x0054A0E0: play the hit sound for a missile striking the target's armour
   at one location; its own assertion spells the name. */
void MakePCHitSound(W8Missile* missile, W8CombatSlot* target, int hit_location, int volume);
/* 0x0054A270: melee hit sound for a monster striking the target's armour at
   one location; the attack's material class rides inside the combat state's
   character_hate storage. */
void MakeMonsterHitSound0054A270(const W8MonsterCombatState* combat, W8CombatSlot* target,
                                 int hit_location, int volume);
