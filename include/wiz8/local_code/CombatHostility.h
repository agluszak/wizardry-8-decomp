#pragma once

struct W8MonsterInfo;
struct W8MonsterGroup;
struct W8TargetSource;
template <class T> class W8GrowableVector;

/* Local Code\Combat Hostility.cpp: whether two monsters count as hostile to
   each other, and whether a spell can be aimed by monster AI. */
char MonsterHostility00546F80(W8MonsterInfo* first, W8MonsterInfo* second);
unsigned char MonsterCanAimSpell005474B0(int spell_id);
unsigned char CombatAllowsLiveGroups(void);
void SetMonsterHostility(W8MonsterInfo* monster, unsigned char hostility); /* 0x005477D0 */
void RecountCombatMonsters(void); /* 0x00546E70 */
void SetMonsterGroupHostility(W8MonsterGroup* group, unsigned int hostility,
                              char recurse); /* 0x00547570 */

void CollectHostileMonsters00547120(W8TargetSource* source,
                                    W8GrowableVector<int>* monsters); /* 0x00547120 */
unsigned char MonsterIsHostileTo(int party_slot, W8MonsterInfo* monster_info);
char MonsterVsCharDisposition(int character_slot, W8MonsterInfo* monster_info);
/* Give every other same-faction group that can see this one its disposition -
   one group going hostile brings the rest of its faction with it. */
void AlertSameFactionGroups(W8MonsterGroup* monster_group); /* 0x005478A0 */
