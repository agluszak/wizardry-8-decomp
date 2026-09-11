#pragma once

struct W8PartyFormationState;

void TurnPartyToImmediate(unsigned int degrees, char snap);

extern double g_facing_tolerance_005ee858;
extern float g_facing_tolerance_005ebcf4;

signed char DecideFacingForPosition(int position, int arg_2);  /* 0x00555E70 */


void RebuildPartyStatus00555FA0(W8PartyFormationState* status);

void RestoreCombatFormation(void);  /* 0x00554A60 */
/* 0x00554AE0: give one joining party slot its formation position, scanning
   the row order for the first free one. */
void Function554AE0(W8PartyFormationState* formation, int slot);
/* 0x00554BD0: move one party position into a formation row and column,
   updating both rows' occupant lists. */
void Function554BD0(
    W8PartyFormationState* formation, int slot, int new_row, int new_column,
    int announce, int detach, int update_facing);
/* 0x00554DD0: re-place the positions that shared a row whose occupant set
   changed. */
void Function554DD0(W8PartyFormationState* formation, int row);
