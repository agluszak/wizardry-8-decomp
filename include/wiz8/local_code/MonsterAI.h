#pragma once

struct W8MonsterInfo;
struct W8MonsterRecord;
struct W8CombatSlot;

/* MonsterAI.cpp GLOBAL at 0x0061EEFC: two dwords per AI kind. */
extern const int g_ai_kind_table[32][2];

unsigned char AimMonsterAtSpellTarget(W8MonsterInfo* monster_info, int spell_id);
unsigned char Function5327E0(W8MonsterInfo* monster_info, int spell_id, W8CombatSlot* combat_slot);
unsigned char Function5330E0(W8MonsterInfo* monster_info, int spell_id, W8CombatSlot* combat_slot);
unsigned char Function5353E0(W8MonsterInfo* monster_info, int spell_id, W8CombatSlot* slot);
void Function5354E0(void);
unsigned char IsMonsterActionUsable(W8MonsterInfo* monster_info); /* 0x00535150 */
/* Decide what one monster does this round: give up, hold, flee, cast, attack
   or move, then fall back to holding when the choice cannot be carried out. */
void UpdateMonsterAI(W8MonsterInfo* monster_info); /* 0x00531540 */
/* The percentage chance the monster holds back this round. */
unsigned int Function531C00(W8MonsterInfo* monster_info, W8MonsterRecord* record); /* 0x00531C00 */
/* Whether the monster can flee at all: it has a flee chance, a flee
   animation, enough of its stat left, and somewhere to run. */
unsigned char CanMonsterFlee(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                             char exclude_special); /* 0x00534A40 */
/* Whether the monster may cast `spell_id` now; `needs_target` also demands
   something to aim it at. */
unsigned char IsSpellUsableByMonster(W8MonsterInfo* monster_info, int spell_id,
                                     char needs_target); /* 0x00532550 */
/* Which of the monster's ten spells to cast, weighted by the spell table. */
int ChooseMonsterSpell(W8MonsterInfo* monster_info, W8MonsterRecord* record); /* 0x00533260 */
/* Build the monster's list of possible actions and take one of them at
   random into its action fields and target. */
unsigned char ChooseRandomMonsterAction(W8MonsterInfo* monster_info, int arg_2, int arg_3,
                                        char set_attack_rate); /* 0x005323F0 */
