#pragma once

struct W8MonsterInfo;
struct W8CombatSlot;

/* MonsterAI.cpp GLOBAL at 0x0061EEFC: two dwords per AI kind. */
extern const int g_ai_kind_table[32][2];

unsigned char AimMonsterAtSpellTarget(W8MonsterInfo* monster_info, int spell_id);
unsigned char Function5327E0(W8MonsterInfo* monster_info, int spell_id, W8CombatSlot* combat_slot);
unsigned char Function5330E0(W8MonsterInfo* monster_info, int spell_id, W8CombatSlot* combat_slot);
unsigned char Function5353E0(W8MonsterInfo* monster_info, int spell_id, W8CombatSlot* slot);
void Function5354E0(void);
void UpdateMonsterAI(W8MonsterInfo* monster_info); /* 0x00531540 */
