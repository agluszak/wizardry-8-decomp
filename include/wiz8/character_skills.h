#pragma once

#include "surrender/srMath.h"

struct W8Character;
struct W8MonsterInfo;

/* 0x00553F10: add usage points to one skill, roll its increase, and report
   the result; suppress_notification keeps silent practice calls silent. */
void PracticeCharacterSkill(W8Character* character, int skill_id, int usage_points,
                            unsigned char suppress_notification);

void Function5477D0(W8MonsterInfo* monster_info, int flag);
void Function547A50(int party_slot);
float ScaleValueByProfessionLevel005479B0(W8Character* character, int trait, float base);
/* 0x005539E0: rebuild the effective attributes from the modifier block's
   seven adjustment bytes and every skill's base level from the attribute
   pair g_skill_attributes names. */
void ResetCharacterAttributes005539E0(W8Character* character);
/* 0x00553A60: rebuild every skill level from its base, the profession bonus
   and the race and profession skill adjustments. */
void ResetCharacterSkills00553A60(W8Character* character);
