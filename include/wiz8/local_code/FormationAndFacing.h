#pragma once

#include "surrender/srMath.h"
#include "wiz8/layouts/party_formation.h"

void TurnPartyToImmediate(unsigned int degrees, char snap);

extern double g_facing_tolerance_005ee858;
extern float g_facing_tolerance_005ebcf4;

signed char DecideFacingForPosition(int position, int arg_2); /* 0x00555E70 */

void RebuildPartyStatus00555FA0(W8PartyFormationState* status);

/* 0x005549E0: whether the character can hold a formation place at all: alive
   and in better shape than the hostile conditions. */
bool CanHoldFormationPlace(int party_slot);
void RestoreCombatFormation(void); /* 0x00554A60 */
/* 0x00554AE0: give one joining party slot its formation position, scanning
   the row order for the first free one. */
void PlaceCharacterInFormation(W8PartyFormationState* formation, int slot);
/* 0x00554BD0: move one party position into a formation row and column,
   updating both rows' occupant lists. */
void SetFormationPosition(W8PartyFormationState* formation, int slot, signed char new_row,
                          signed char new_column, char announce, char detach, char update_facing);
/* 0x00554DD0: re-place the positions left in a row after one moved out: a
   single occupant goes to column 0, two pack toward column 2. */
void CompactFormationRow(W8PartyFormationState* formation, unsigned char row);

/* 0x00554580 sits in the attribution gap between Magic Effects.cpp and this
   file. The pointer ABI is the formation record, not a byte buffer. */
void InitializePartyFormation(W8PartyFormationState* state);
/* Unrecovered neighbours in the same attribution area; declared so the
   formation screen can call them. 0x005545D0 copies a whole 0x84-byte
   formation record, 0x005545F0 reconciles an edited formation against the
   live one, 0x00555080 re-seats one slot's row, and 0x00555160 swaps two
   slots' positions. */
void CopyPartyFormationState(W8PartyFormationState* dst, const W8PartyFormationState* src);
void Function5545F0(W8PartyFormationState* edited, W8PartyFormationState* live);
void Function555080(W8PartyFormationState* formation, int slot, int row);
void Function555160(W8PartyFormationState* formation, int slot_a, int slot_b);
/* 0x005554A0: re-aim party_heading at the selected character's formation
   facing and swing the camera to match. */
void Function5554A0(int party_slot);
int GetQuadrantForPosition(srVector3T<float> position);
