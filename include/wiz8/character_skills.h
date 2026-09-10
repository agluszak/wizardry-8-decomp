#pragma once

#include "surrender/srMath.h"

struct W8Character;
struct W8MonsterInfo;

/* 0x00553F10: add usage points to one skill, roll its increase, and report
   the result; suppress_notification keeps silent practice calls silent. */
void PracticeCharacterSkill(
    W8Character* character, int skill_id, int usage_points,
    unsigned char suppress_notification);

void Function5477D0(W8MonsterInfo* monster_info, int flag);
void Function547A50(int party_slot);
