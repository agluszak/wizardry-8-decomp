#pragma once

void SetPendingMoveKind(int kind);                         /* 0x004F0520 */
unsigned char GetPartyHasteSteps(unsigned int* out_steps); /* 0x004F0010 */
void CompletePartyMovementTurns(void);                     /* 0x004F06B0 */
void BeginFreeTurnPhase(void);                             /* 0x004F0630 */
void BeginPartyMovement(void);                             /* 0x004EFBE0 */
void CancelPartyMovement(void);                            /* 0x004F0860 */
void InterruptActivePartyMovement(void);                   /* 0x004F0990 */
unsigned char CanPartyMove(void);                          /* 0x004F0800 */
void ClearPendingPartyMovement(int excluded_party_slot);   /* 0x004F0560 */
void StartPartyMovementAction(int move_kind);              /* 0x004F0AF0 */
void UpdateActivePartyMovement(void);                      /* 0x004F01D0 */
void UpdatePartyMovementControl(void);                     /* 0x004F0AA0 */
void RoundPhaseToStep(unsigned int* phase, unsigned int base);
