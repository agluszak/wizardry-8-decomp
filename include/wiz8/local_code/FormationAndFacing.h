#pragma once

#include "surrender/srMath.h"
#include "wiz8/layouts/party_formation.h"

struct W8MonsterInfo;
struct W8CombatSlot;

unsigned int TurnPartyTo(unsigned int degrees);
void TurnPartyToImmediate(unsigned int degrees, char snap);
/* Sync party facing/heading from the camera yaw and refresh the compass. */
void SyncPartyFacingFromCamera(void);

extern const double g_facing_tolerance1;
extern const float g_facing_tolerance0;

signed char DecideFacingForPosition(int position, int target_position);

void RebuildPartyStatus(W8PartyFormationState* status);

/* Turn the position's formation facing toward the second position
   the way DecideFacingForPosition resolved it. */
void FacePositionAsDecided(int position, int target_position);
/* Whether the position already faces the second position the way
   DecideFacingForPosition resolved it. */
bool PositionFacesAsDecided(int position, int target_position);
/* Whether the position faces opposite to the second position the
   way DecideFacingForPosition resolved it. */
bool PositionFacesOppositeToDecided(int target_position, int position);
/* Whether the party's world position looks away from the
   monster. */
bool IsPartyLookingAwayFrom(int party_slot, W8MonsterInfo* monster_info);
/* Whether the character can hold a formation place at all: alive
   and in better shape than the hostile conditions. */
bool CanHoldFormationPlace(int party_slot);
void RestoreCombatFormation(void);
/* Give one joining party slot its formation position, scanning
   the row order for the first free one. */
void PlaceCharacterInFormation(W8PartyFormationState* formation, int slot);
/* Move one party position into a formation row and column,
   updating both rows' occupant lists. */
void SetFormationPosition(W8PartyFormationState* formation, int slot, signed char new_row,
                          signed char new_column, bool announce, bool detach, bool update_facing);
/* Re-place the positions left in a row after one moved out: a
   single occupant goes to column 0, two pack toward column 2. */
void CompactFormationRow(W8PartyFormationState* formation, unsigned char row);

void InitializePartyFormation(W8PartyFormationState* state);
void CopyPartyFormationState(W8PartyFormationState* dst, const W8PartyFormationState* src);
/* Remember the formation combat started with. */
void SaveCombatFormation(void);
/* Reconcile an edited formation against the live one, re-seating
   characters that can no longer hold their edited place and writing the
   result back into both formations. */
void ReconcilePartyFormation(W8PartyFormationState* edited, W8PartyFormationState* live);
/* Keep one slot's seat in step with its liveness - death stashes
   its quadrant in bOldQuadrant and unseats it, revival re-seats it there or
   in a fallback row. */
void UpdateFormationSlotState(W8PartyFormationState* formation, int slot);
/* Seat one party slot in a formation row, shuffling its occupants
   to make room, or refuse a full row with a notice. */
void SeatFormationSlotInRow(W8PartyFormationState* formation, int slot, int row);
/* Swap two party slots' formation positions. */
void SwapFormationSlots(W8PartyFormationState* formation, int slot_a, int slot_b);
/* Re-aim party_heading at the selected character's formation
   facing and swing the camera to match, while the camera is not being
   rotated by hand. */
void FaceCameraToSelection(int party_slot);
int GetQuadrantForPosition(srVector3T<float> position);

/* Whether the monster faces the party's own position. */
int IsMonsterFacingParty(W8MonsterInfo* monster_info);
/* Whether the first monster faces the second. */
int IsMonsterFacingMonster(W8MonsterInfo* first, W8MonsterInfo* second);
/* Whether the party faces a world point. */
bool IsPartyLookingAt(W8MonsterInfo* monster_info, srVector3T<float> point);
/* Whether the second monster looks away from the first. */
int IsMonsterLookingAwayFrom(W8MonsterInfo* first, W8MonsterInfo* second);
/* Whether the monster sits on the screen side the character
   faces. */
bool IsCharacterFacingMonster(int party_slot, W8MonsterInfo* monster_info);
/* Turn the character's formation facing toward the monster. */
void TurnCharacterTowardMonster(int party_slot, W8MonsterInfo* monster_info);
/* Whether the monster is on the side opposite the character's
   facing. */
int IsMonsterBehindCharacter(W8MonsterInfo* monster_info, int party_slot);
/* Face the character toward its committed combat slot's target;
   asserts the slot ids are not BAD_INDEX. */
void FaceCharacterTowardCombatTarget(int party_slot, W8CombatSlot* target);
