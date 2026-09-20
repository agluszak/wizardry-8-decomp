#pragma once

#include "surrender/srMath.h"
#include "wiz8/layouts/party_formation.h"

struct W8MonsterInfo;
struct W8CombatSlot;

unsigned int TurnPartyTo(unsigned int degrees); /* 0x005553C0 */
void TurnPartyToImmediate(unsigned int degrees, char snap);
/* Sync party facing/heading from the camera yaw and refresh the compass. */
void SyncPartyFacingFromCamera(void); /* 0x005552F0 */

extern double g_facing_tolerance_005ee858;
extern float g_facing_tolerance_005ebcf4;

signed char DecideFacingForPosition(int position, int arg_2); /* 0x00555E70 */

void RebuildPartyStatus00555FA0(W8PartyFormationState* status);

/* 0x005557E0: turn the position's formation facing toward the second position
   the way DecideFacingForPosition resolved it. */
void FacePositionAsDecided(int position, int arg_2);
/* 0x00555920: whether the position already faces the second position the way
   DecideFacingForPosition resolved it. */
bool PositionFacesAsDecided(int position, int arg_2);
/* 0x00555C20: whether the position faces opposite to the second position the
   way DecideFacingForPosition resolved it. */
bool PositionFacesOppositeToDecided(int arg_1, int position);
/* 0x00555D60: whether the party's world position looks away from the
   monster. */
bool IsPartyLookingAwayFrom(int party_slot, W8MonsterInfo* monster_info);
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
void CopyPartyFormationState(W8PartyFormationState* dst, const W8PartyFormationState* src);
/* 0x00554A20: remember the formation combat started with. */
void SaveCombatFormation(void);
/* 0x005545F0: reconcile an edited formation against the live one, re-seating
   characters that can no longer hold their edited place and writing the
   result back into both formations. */
void ReconcilePartyFormation(W8PartyFormationState* edited, W8PartyFormationState* live);
/* 0x00554E70: keep one slot's seat in step with its liveness - death stashes
   its quadrant in bOldQuadrant and unseats it, revival re-seats it there or
   in a fallback row. */
void UpdateFormationSlotState(W8PartyFormationState* formation, int slot);
/* 0x00555080: seat one party slot in a formation row, shuffling its occupants
   to make room, or refuse a full row with a notice. */
void SeatFormationSlotInRow(W8PartyFormationState* formation, int slot, int row);
/* 0x00555160: swap two party slots' formation positions. */
void SwapFormationSlots(W8PartyFormationState* formation, int slot_a, int slot_b);
/* 0x005554A0: re-aim party_heading at the selected character's formation
   facing and swing the camera to match, while the camera is not being
   rotated by hand. */
void FaceCameraToSelection(int party_slot);
int GetQuadrantForPosition(srVector3T<float> position);

/* 0x00555A60: whether the monster faces the party's own position. */
int IsMonsterFacingParty(W8MonsterInfo* monster_info);
/* 0x00555B00: whether the first monster faces the second. */
int IsMonsterFacingMonster(W8MonsterInfo* first, W8MonsterInfo* second);
/* 0x00555BA0: whether the party faces a world point. */
bool IsPartyLookingAt(W8MonsterInfo* monster_info, srVector3T<float> point);
/* 0x00555DE0: whether the second monster looks away from the first. */
int IsMonsterLookingAwayFrom(W8MonsterInfo* first, W8MonsterInfo* second);
/* 0x00555960: whether the monster sits on the screen side the character
   faces. */
bool IsCharacterFacingMonster(int party_slot, W8MonsterInfo* monster_info);
/* 0x00555820: turn the character's formation facing toward the monster. */
void TurnCharacterTowardMonster(int party_slot, W8MonsterInfo* monster_info);
/* 0x00555C60: whether the monster is on the side opposite the character's
   facing. */
int IsMonsterBehindCharacter(W8MonsterInfo* monster_info, int party_slot);
/* 0x00555560: face the character toward its committed combat slot's target;
   asserts the slot ids are not BAD_INDEX. */
void Function555560(int party_slot, W8CombatSlot* target);
