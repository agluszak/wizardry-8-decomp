#pragma once

template <class T> class srVector3T;

struct W8NpcState;

void Function5092F0(int* level, int* entrance);
void Function509560(void); /* 0x00509560 */
int Function509750(void);
/* 0x0050B9B0: how many leading party slots are occupied. */
unsigned char CountLeadingPartySlots(void);
char GetNpcDisposition(W8NpcState* npc);                                           /* 0x0050A280 */
unsigned char UpdateNpcAt(W8NpcState* npc, int arg_2, srVector3T<float>* scratch); /* 0x0050B2F0 */
