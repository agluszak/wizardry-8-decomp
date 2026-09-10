#pragma once

struct W8MonsterInfo;

/* Local Code\Combat Hostility.cpp: whether two monsters count as hostile to
   each other, and whether a spell can be aimed by monster AI. */
char MonsterHostility00546F80(W8MonsterInfo* first, W8MonsterInfo* second);
unsigned char MonsterCanAimSpell005474B0(int spell_id);
