#pragma once

#include "wiz8/layouts/targeting.h"

void SetPendingMoveKind(W8ActionKind kind);
unsigned char GetPartyHasteSteps(unsigned int* out_steps);
void CompletePartyMovementTurns(void);
void BeginFreeTurnPhase(void);
void BeginPartyMovement(void);
void EndPartyMovementPhase(void);
void CancelPartyMovement(void);
void InterruptActivePartyMovement(void);
bool CanPartyMove(void);
void ClearPendingPartyMovement(int excluded_party_slot);
void StartPartyMovementAction(W8PartyAction move_kind);
void UpdateActivePartyMovement(void);
void UpdatePartyMovementControl(void);
void RoundPhaseToStep(unsigned int* phase, unsigned int base);
int GetPhaseStep(void);
float GetPartyMovementSpeed(void);
void BeginPartyMovementPhase(void);
void FinishPartyMovementAction(void);
bool PartyMovementReachedPhaseLimit(void);
/* Combat.cpp calls it when a party movement action is pending
   while phases are assigned, so it is not file-local. */
void InitializePartyMovementPhase(void);
