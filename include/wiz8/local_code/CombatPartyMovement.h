#pragma once

void SetPendingMoveKind(int kind); /* 0x004F0520 */
unsigned char Function4F0010(unsigned int* out_steps);
void Function4F06B0(void);
void BeginFreeTurnPhase(void);  /* 0x004F0630 */
void Function4F0560(int value); /* 0x004F0560 */
void RoundPhaseToStep(unsigned int* phase, unsigned int base);
