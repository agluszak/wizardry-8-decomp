#pragma once

struct W8MonsterInfo;
struct W8TargetSource;
template <class T> class W8GrowableVector;

/* Local Code\Combat Hostility.cpp: whether two monsters count as hostile to
   each other, and whether a spell can be aimed by monster AI. */
char MonsterHostility00546F80(W8MonsterInfo* first, W8MonsterInfo* second);
unsigned char MonsterCanAimSpell005474B0(int spell_id);
unsigned char CombatAllowsLiveGroups(void);

void CollectHostileMonsters00547120(W8TargetSource* source,
                                    W8GrowableVector<int>* monsters); /* 0x00547120 */
unsigned char MonsterIsHostileTo(int party_slot, W8MonsterInfo* monster_info);
char MonsterVsCharDisposition(int character_slot, W8MonsterInfo* monster_info);
