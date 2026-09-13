#pragma once

void SetPendingMoveKind(int kind);                         /* 0x004F0520 */
unsigned char GetPartyHasteSteps(unsigned int* out_steps); /* 0x004F0010 */
void Function4F06B0(void);
void BeginFreeTurnPhase(void);  /* 0x004F0630 */
void Function4F0560(int value); /* 0x004F0560 */
