#pragma once

#include "surrender/srMath.h"

struct W8NpcState;

void ChooseNewGameStartLocation(int* level, int* entrance); /* 0x005092F0 */
void Function509560(void);                                  /* 0x00509560 */
int SelectNewGameStartLevel(void);                          /* 0x00509750 */
/* 0x0050B9B0: how many leading party slots are occupied. */
unsigned char CountLeadingPartySlots(void);
char GetNpcDisposition(W8NpcState* npc);                                           /* 0x0050A280 */
unsigned char UpdateNpcAt(W8NpcState* npc, int arg_2, srVector3T<float>* scratch); /* 0x0050B2F0 */
struct W8MonsterInfo;
struct W8Character;
W8MonsterInfo* GetNpcMonsterInfo(W8NpcState* npc);  /* 0x0050A3C0 */
void MarkNpcOfKind(int kind);                       /* 0x0050CA30 */
W8Character* GetNpcGroupCharacter(W8NpcState* npc); /* 0x0050B8B0 */
