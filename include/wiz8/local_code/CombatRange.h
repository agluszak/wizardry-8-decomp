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
unsigned char Function5194E0(int party_slot, int arg_2, W8MonsterInfo* monster_info,
                             int arg_4, int arg_5);
unsigned char Function519F80(W8MonsterInfo* monster_info, W8MonsterRecord* record, int arg_3,
                             W8CombatSlot* combat_slot);
unsigned char IsSlotInRangeOfGroup(int party_slot, int group_id, W8TargetingContext context,
                                   int arg_4); /* 0x00519920 */
float MonsterChooseTarget(W8MonsterInfo* monster_info, int* out, int kind);
float GetRangeConstant5EC35C(void);
