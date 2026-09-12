#pragma once

#include <stddef.h>

void TurnPartyToImmediate(unsigned int degrees, char snap);

extern double g_facing_tolerance_005ee858;
extern float g_facing_tolerance_005ebcf4;

signed char DecideFacingForPosition(int position, int arg_2); /* 0x00555E70 */

#pragma pack(push, 1)
struct W8PartyFormationRow {
    signed char slots[3];
};

struct W8PartyFormationPosition {
    unsigned char row;
    unsigned char unknown_01[2];
    signed char facing;
    unsigned char unknown_04[8];
};

/* Shared formation record. It is embedded in both W8GlobalStatus and
   W8CombatState; the functions in this header are the recovered owner. */
struct W8PartyFormationState {
    W8PartyFormationRow rows[5];
    unsigned char flags_0f[5];
    W8PartyFormationPosition positions[8];
    unsigned char unknown_74[0x10];
};
#pragma pack(pop)

static_assert(sizeof(W8PartyFormationRow) == 0x03, "W8PartyFormationRow_must_be_0x03");
static_assert(sizeof(W8PartyFormationPosition) == 0x0c, "W8PartyFormationPosition_must_be_0x0c");
static_assert(sizeof(W8PartyFormationState) == 0x84, "W8PartyFormationState_must_be_0x84");

void RebuildPartyStatus00555FA0(W8PartyFormationState* status);

void RestoreCombatFormation(void); /* 0x00554A60 */
/* 0x00554AE0: give one joining party slot its formation position, scanning
   the row order for the first free one. */
void PlaceCharacterInFormation(W8PartyFormationState* formation, int slot);
/* 0x00554BD0: move one party position into a formation row and column,
   updating both rows' occupant lists. */
void SetFormationPosition(W8PartyFormationState* formation, int slot, int new_row, int new_column,
                          int announce, int detach, int update_facing);
/* 0x00554DD0: re-place the positions that shared a row whose occupant set
   changed. */
void Function554DD0(W8PartyFormationState* formation, int row);

/* 0x00554580 sits in the attribution gap between Magic Effects.cpp and this
   file. The pointer ABI is the formation record, not a byte buffer. */
void InitializePartyFormation(W8PartyFormationState* state);
