#pragma once

#include "wiz8/targeting.h"

struct W8MonsterInfo;
struct W8MonsterRecord;
struct W8CombatSlot;

/* Local Code\Combat Range.cpp: the party's own world position and the trace
   wrapper that decides whether a line of sight counts as unobstructed. */
unsigned char Function51B3F0(int mode);

unsigned char CanReachTarget(int party_slot, int kind, W8MonsterInfo* monster_info,
                             W8TargetingContext context, int arg_5);
char CountRowsBetween(int from_position, int to_position); /* 0x0051AEC0 */
unsigned char Function519180(int party_slot, int arg_2, W8TargetingContext context);
/* 0x005194E0: whether `party_slot` may aim at `monster_info` under mode `arg_2`. */
unsigned char Function5194E0(int party_slot, int arg_2, W8MonsterInfo* monster_info, int arg_4,
                             int arg_5);
/* Whether the front rank stands between two formation positions. */
bool FrontRankScreens(unsigned int from_position, unsigned int to_position); /* 0x0051B000 */
/* Whether the monster's attack `attack` reaches the character in `party_slot`,
   or the monster `target`, given what it can see and how far away they stand. */
unsigned char MonsterAttackReachesCharacter(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                                            unsigned int attack, int party_slot); /* 0x0051A2F0 */
unsigned char MonsterAttackReachesMonster(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                                          unsigned int attack,
                                          W8MonsterInfo* target); /* 0x0051A510 */
/* Whether the monster's attack `attack` reaches anyone at all; `hostile_only`
   counts only those it is hostile to. */
unsigned char MonsterAttackReachesAnyone(W8MonsterInfo* monster_info, unsigned int attack,
                                         char hostile_only); /* 0x00519C00 */
unsigned char Function519F80(W8MonsterInfo* monster_info, W8MonsterRecord* record, int arg_3,
                             W8CombatSlot* combat_slot);
unsigned char IsSlotInRangeOfGroup(int party_slot, int group_id, W8TargetingContext context,
                                   int arg_4); /* 0x00519920 */
float MonsterChooseTarget(W8MonsterInfo* monster_info, int* out, int kind);
float GetRangeConstant5EC35C(void);
