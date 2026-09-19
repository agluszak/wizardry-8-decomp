#include "wiz8/local_code/FormationAndFacing.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/PolyPick.h"
#include "wiz8/engine_code/quad.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/MGSFormation.h"
#include "wiz8/local_screens/MGSRadarMap.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/startup_world.h"
#include "wiz8/xstatus.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/notices.h"
#include "wiz8/utility.h"
#include "surrender/srMath.h"

#include <math.h>
#include <string.h>
#include "wiz8/sr_api.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/local_screens/RCSCommon.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/engine_code/GameData.h"

/* Original translation unit: Local Code\Formation & Facing.cpp. */

/* Fills in the party's own world position. */
/* A full turn, and the half-quadrant the bearing is biased by so that a
   quadrant is centred on its facing rather than starting at it. */
enum { W8_DEGREES_PER_TURN = 360, W8_DEGREES_PER_QUADRANT = 90 };

#define FORMATION_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Formation & Facing.cpp"

extern wchar_t g_formation_row_names_00649e54[5][20];

/* Copy a party formation state. The 0x84-byte structure is copied as 33
   dwords via REP MOVSD. */
// FUNCTION: WIZ8 0x005545d0
void CopyPartyFormationState(W8PartyFormationState* dst, const W8PartyFormationState* src)
{
    *dst = *src;
}

/* Reconcile an edited formation against the live one. Characters still able
   to hold a place take their edited seat - a moved slot announces its new row
   and picks up the row's facing - while characters that can no longer hold a
   place are returned to their live row when it still has room, or pushed into
   a fallback row with a notice. Dead characters keep the live position record
   verbatim. The reconciled state is written back into both formations. */
// FUNCTION: WIZ8 0x005545f0
void ReconcilePartyFormation(W8PartyFormationState* edited, W8PartyFormationState* live)
{
    W8PartyFormationState working;
    int slot;
    int index;
    int row;
    signed char column;

    InitializePartyFormation(&working);
    for (slot = 0; slot < 8; ++slot) {
        const W8Character* character = &g_status_685170.buffers.characters[slot];
        signed char row;
        bool row_changed;

        if (g_status_685170.buffers.party_rows[slot].occupied == 0 || character->hp_current <= 0 ||
            character->highest_condition >= W8_CONDITION_HOSTILE) {
            continue;
        }
        row = edited->positions[slot].bQuadrant;
        row_changed = row != live->positions[slot].bQuadrant;
        if (row_changed ||
            edited->positions[slot].bQuadrantSlot != live->positions[slot].bQuadrantSlot) {
            SetFormationPosition(&working, slot, row, edited->positions[slot].bQuadrantSlot,
                                 row_changed, 0, row_changed);
        } else {
            SetFormationPosition(&working, slot, row, edited->positions[slot].bQuadrantSlot, 0, 0,
                                 0);
        }
        if (!row_changed) {
            working.positions[slot].facing = live->positions[slot].facing;
        }
    }
    for (slot = 0; slot < 8; ++slot) {
        const W8Character* character = &g_status_685170.buffers.characters[slot];
        W8PartyFormationPosition* live_position = &live->positions[slot];
        W8PartyFormationPosition* edited_position = &edited->positions[slot];

        if (g_status_685170.buffers.party_rows[slot].occupied == 0 ||
            (character->hp_current > 0 && character->highest_condition < W8_CONDITION_HOSTILE)) {
            continue;
        }
        if (character->highest_condition >= W8_CONDITION_DEAD) {
            *edited_position = *live_position;
            continue;
        }
        if (edited_position->bQuadrant != live_position->bQuadrant) {
            PostCharacterNotice(slot, gppStringList[0x920 / 4]);
        }
        row = live_position->bQuadrant;
        if (working.ubQuadrantOccupants[row] < 3) {
            if (working.bOccupantChar[row][live_position->bQuadrantSlot] == -1) {
                column = live_position->bQuadrantSlot;
            } else {
                column = -1;
                for (index = 0; index < 3; ++index) {
                    if (working.bOccupantChar[row][index] == -1) {
                        column = static_cast<signed char>(index);
                        break;
                    }
                }
                if (column == -1) {
                    srAssertFail("bQuadrantSlot != -1", FORMATION_CPP, 0xbe, 0);
                }
            }
            SetFormationPosition(&working, slot, row, column, 0, 0, 0);
            working.positions[slot].facing = live_position->facing;
            if (edited_position->bQuadrant != row) {
                PostCharacterNotice(slot, gppStringList[0x924 / 4],
                                    &g_formation_row_names_00649e54[row][0]);
            }
        } else {
            signed char new_rows[3] = {4, 2, 3};

            for (index = 0; index < 3; ++index) {
                signed char new_row = new_rows[index];

                for (column = 0; column < 3; ++column) {
                    if (working.bOccupantChar[new_row][column] == -1) {
                        SetFormationPosition(&working, slot, new_row, column, 0, 0, 1);
                        PostCharacterNotice(slot, gppStringList[0x928 / 4],
                                            &g_formation_row_names_00649e54[new_row][0]);
                        goto next_slot;
                    }
                }
            }
            srAssertFail("bQuadrantSlot != -1", FORMATION_CPP, 0xe1, 0);
        }
    next_slot:;
    }
    for (row = 0; row < 5; ++row) {
        CompactFormationRow(&working, row);
    }
    *live = working;
    *edited = working;
    RefreshFormationBoard();
    RefreshRadarMap();
}

/* Which of the four quadrants around the party a world position falls in.
   The bearing to the position is taken relative to the party's facing, wrapped
   into a single turn, then biased by half a quadrant before the divide - so
   quadrant zero is the 90 degrees centred on straight ahead rather than the 90
   beginning there. */
// FUNCTION: WIZ8 0x00555f30
int GetQuadrantForPosition(srVector3T<float> position)
{
    srVector3T<float> party;
    int bearing;

    GetCameraPosition(&party);
    bearing = static_cast<int>(NormalizeAngle(BearingBetween(party, position)));
    bearing -= g_status_685170.party_facing;
    if (bearing < 0) {
        bearing += W8_DEGREES_PER_TURN;
    }
    return ((bearing + W8_DEGREES_PER_QUADRANT / 2) % W8_DEGREES_PER_TURN) /
           W8_DEGREES_PER_QUADRANT;
}

/* The party formation. It is one thirty-three dword block that combat saves
   into its own state and compares against, and within it two per-position
   tables twelve bytes apart give each position's row and facing, plus a table
   of the three positions standing in each of the five rows. */
enum { W8_FORMATION_ROWS = 5, W8_POSITIONS_PER_ROW = 3 };

/* The facing answer that means "no preference", which never disagrees with
   whatever a position is already facing. */
enum { W8_FACING_ANY = 4 };

/* The state a character has to be under to hold a place in the formation:
   highest_condition below HOSTILE, tighter than the party-wide death window. */

// GLOBAL: WIZ8 0x005ee858
double g_facing_tolerance_005ee858 = 2.3561944500000003;

// GLOBAL: WIZ8 0x005ebcf4
float g_facing_tolerance_005ebcf4 = 0.05f;

/* Whether one character can hold a place in the formation at all: they have to
   be alive and in better shape than the party sweeps demand. */
// FUNCTION: WIZ8 0x005549e0
bool CanHoldFormationPlace(int party_slot)
{
    const W8Character* character = &g_status_685170.buffers.characters[party_slot];

    return character->hp_current > 0 && character->highest_condition < W8_CONDITION_HOSTILE;
}

/* Remember the formation combat started with. */
// FUNCTION: WIZ8 0x00554a20
void SaveCombatFormation(void)
{
    if (gXStatus.fCombatMode == 0) {
        srAssertFail("gXStatus.fCombatMode", FORMATION_CPP, 258, 0);
    }
    memcpy(&g_combat_state->saved_formation, &g_status_685170.formation,
           sizeof(W8PartyFormationState));
}

/* Put the formation combat started with back, if anything moved. Comparing the
   whole block is what makes it one object rather than a set of tables. */
// FUNCTION: WIZ8 0x00554a60
void RestoreCombatFormation(void)
{
    if (gXStatus.fCombatMode == 0) {
        srAssertFail("gXStatus.fCombatMode", FORMATION_CPP, 266, 0);
    }

    if (memcmp(&g_combat_state->saved_formation, &g_status_685170.formation,
               sizeof(W8PartyFormationState)) != 0) {
        memcpy(&g_status_685170.formation, &g_combat_state->saved_formation,
               sizeof(W8PartyFormationState));
        RefreshFormationBoard();
        RefreshRadarMap();
        ShowNotice(8, gppStringList[0x92c / 4], 0, -1, 0);
    }
}

/* When set during combat, camera yaw updates party_heading only and leaves
   party_facing alone. Cleared, the free-look path also writes party_facing. */
// GLOBAL: WIZ8 0x0069B7D4
static unsigned char g_combat_preserve_party_facing_0069b7d4;

/* Sync party facing/heading from the camera yaw and refresh the formation
   compass. Level-data flag 6 and combat-with-preserve skip writing facing. */
// FUNCTION: WIZ8 0x005552F0
void SyncPartyFacingFromCamera(void)
{
    unsigned int yaw;
    unsigned int flag6;

    yaw = static_cast<unsigned int>(GetCameraYawDegrees()) % W8_DEGREES_PER_TURN;
    flag6 = GetLevelDataFlag6();
    if ((gXStatus.fCombatMode == 0 || g_combat_preserve_party_facing_0069b7d4 == 0) &&
        static_cast<unsigned char>(flag6) == 0) {
        g_status_685170.party_facing = static_cast<int>(yaw);
        if (yaw != g_status_685170.party_heading) {
            g_status_685170.party_heading = yaw;
            UpdateFormationCompass();
            if (static_cast<unsigned int>(GetCameraYawDegrees()) % W8_DEGREES_PER_TURN != yaw) {
                SetCameraYawDegrees(static_cast<float>(yaw));
            }
        }
    } else if (yaw != g_status_685170.party_heading) {
        g_status_685170.party_heading = yaw;
        UpdateFormationCompass();
        if (static_cast<unsigned int>(GetCameraYawDegrees()) % W8_DEGREES_PER_TURN != yaw) {
            SetCameraYawDegrees(static_cast<float>(yaw));
        }
    }
}

/* Turn the party to a new heading, moving the camera with it unless it is
   already looking that way. */
// FUNCTION: WIZ8 0x005553c0
unsigned int TurnPartyTo(unsigned int degrees)
{
    unsigned int previous;

    g_status_685170.party_facing = degrees;
    previous = g_status_685170.party_heading;
    if (degrees != g_status_685170.party_heading) {
        g_status_685170.party_heading = degrees;
        UpdateFormationCompass();
        previous = (unsigned int)GetCameraYawDegrees() / W8_DEGREES_PER_TURN;
        if ((unsigned int)GetCameraYawDegrees() % W8_DEGREES_PER_TURN != degrees) {
            SetCameraYawDegrees((float)degrees);
        }
    }
    return previous;
}

/* The same turn, with the option of snapping the camera round rather than
   swinging it. */
// FUNCTION: WIZ8 0x00555420
void TurnPartyToImmediate(unsigned int degrees, char snap)
{
    if (degrees == g_status_685170.party_heading) {
        return;
    }
    g_status_685170.party_heading = degrees;
    UpdateFormationCompass();
    if ((unsigned int)GetCameraYawDegrees() % W8_DEGREES_PER_TURN == degrees) {
        return;
    }
    if (snap) {
        TurnCameraToDegrees((float)degrees);
    } else {
        SetCameraYawDegrees((float)degrees);
    }
}

/* While the camera is not being rotated by hand, re-aim the party heading at
   the selected slot's formation facing and swing the camera to match -
   snapping or gliding per the rotation style. */
// FUNCTION: WIZ8 0x005554a0
void FaceCameraToSelection(int party_slot)
{
    unsigned int heading;

    if (g_settings_6850c8.camera_rotation_mode != 0 ||
        g_status_685170.selected_character != party_slot) {
        return;
    }
    heading = g_status_685170.party_facing +
              static_cast<unsigned char>(g_status_685170.formation.positions[party_slot].facing) *
                  W8_DEGREES_PER_QUADRANT;
    if (heading == g_status_685170.party_heading) {
        return;
    }
    g_status_685170.party_heading = heading;
    UpdateFormationCompass();
    if (static_cast<unsigned int>(GetCameraYawDegrees()) % W8_DEGREES_PER_TURN == heading) {
        return;
    }
    if (g_settings_6850c8.camera_rotation_style) {
        TurnCameraToDegrees(static_cast<float>(heading));
    } else {
        SetCameraYawDegrees(static_cast<float>(heading));
    }
}

/* Face one position the way the rules say it should, unless the rules have no
   preference or it already faces that way. */
// FUNCTION: WIZ8 0x005557e0
void FacePositionAsDecided(int position, int arg_2)
{
    signed char facing = DecideFacingForPosition(position, arg_2);

    if (facing != W8_FACING_ANY && g_status_685170.formation.positions[position].facing != facing) {
        g_status_685170.formation.positions[position].facing = facing;
        RefreshFormationBoard();
    }
}

/* Whether a position already faces the way the rules want. No preference
   always agrees. */
// FUNCTION: WIZ8 0x00555920
bool PositionFacesAsDecided(int position, int arg_2)
{
    signed char facing = DecideFacingForPosition(position, arg_2);

    if (facing == W8_FACING_ANY) {
        return true;
    }
    return facing == g_status_685170.formation.positions[position].facing;
}

/* Whether a position is facing exactly away from where the rules want it -
   two of the four facings apart. */
// FUNCTION: WIZ8 0x00555c20
bool PositionFacesOppositeToDecided(int arg_1, int position)
{
    signed char facing = DecideFacingForPosition(position, arg_1);
    int difference;

    if (facing == W8_FACING_ANY) {
        return false;
    }
    difference = facing - g_status_685170.formation.positions[position].facing;
    if (difference < 0) {
        difference = -difference;
    }
    return difference == 2;
}

/* Whether the party is looking far enough away from a point to count as not
   facing it, measured as the shortest way round. */
// FUNCTION: WIZ8 0x00555d60
bool IsPartyLookingAwayFrom(int, W8MonsterInfo* monster_info)
{
    float bearing = NormalizeAngle(BearingBetween(monster_info->monster->GetPosition(),
                                                  g_startup_world_659c0c->GetPosition()));
    float facing = monster_info->monster->GetYaw();

    return ShortestAngleDistance(bearing, facing) >= g_facing_tolerance_005ee858;
}

/* Whether the party is looking at a point, measured as a plain difference
   rather than the shortest way round - so a bearing either side of the wrap
   answers no. */
// FUNCTION: WIZ8 0x00555ba0
bool IsPartyLookingAt(W8MonsterInfo* monster_info, srVector3T<float> point)
{
    float bearing = NormalizeAngle(BearingBetween(monster_info->monster->GetPosition(), point));

    return fabsf(bearing - monster_info->monster->GetYaw()) <= g_facing_tolerance_005ebcf4;
}

/* The five formation rows' display names, indexed by row. */
// GLOBAL: WIZ8 0x00649e54
wchar_t g_formation_row_names_00649e54[5][20] = {
    L"Front", L"Right", L"Rear", L"Left", L"Center",
};

/* Give one joining party slot its formation position: clear the position and
   walk the rows in the 0, 4, 2 order. An empty row takes the joiner in its
   first column; a row led by one character moves that leader to the second
   column and puts the joiner in the third; a row already holding two takes
   the joiner directly. */
// FUNCTION: WIZ8 0x00554ae0
void PlaceCharacterInFormation(W8PartyFormationState* formation, int slot)
{
    unsigned char row_order[3] = {0, 4, 2};
    W8PartyFormationPosition* position = &formation->positions[slot];

    position->bQuadrant = 0xff;
    position->bQuadrantSlot = -1;
    for (unsigned int index = 0; index < 3; ++index) {
        unsigned char row = row_order[index];
        signed char occupants = formation->ubQuadrantOccupants[row];
        if (occupants == 0) {
            SetFormationPosition(formation, slot, row, 0, 1, 1, 1);
            return;
        }
        if (occupants == 1) {
            signed char leader = formation->bOccupantChar[row][0];
            if (leader == -1) {
                srAssertFail("iChar != -1", FORMATION_CPP, 0x159, 0);
            }
            SetFormationPosition(formation, leader, row, 1, 0, 0, 1);
            SetFormationPosition(formation, slot, row, 2, 1, 1, 1);
            return;
        }
        if (occupants == 2) {
            if (formation->bOccupantChar[row][0] != -1) {
                srAssertFail("iChar == -1", FORMATION_CPP, 0x172, 0);
            }
            SetFormationPosition(formation, slot, row, 0, 1, 1, 1);
            return;
        }
    }
}

/* Move one party position into a formation row and column: clear the old
   entry from its row's occupant list, install the new row and column, place
   the slot in the new row and update the facing. The detach flag re-places
   the positions left in the old row, and the announce flag posts the row
   name to the slot. */
// FUNCTION: WIZ8 0x00554dd0
void CompactFormationRow(W8PartyFormationState* formation, unsigned char row)
{
    signed char last_free = -1;
    signed char column;
    int slot;

    switch (formation->ubQuadrantOccupants[row]) {
    case 1:
        for (column = 0; column < 3; ++column) {
            slot = formation->bOccupantChar[row][column];
            if (slot != -1) {
                SetFormationPosition(formation, slot, row, 0, 0, 0, 1);
                return;
            }
        }
        break;
    case 2:
        for (column = 2; column >= 0; --column) {
            slot = formation->bOccupantChar[row][column];
            if (slot == -1) {
                last_free = column;
            } else if (last_free != -1) {
                SetFormationPosition(formation, slot, row, last_free, 0, 0, 1);
                last_free = column;
            }
        }
        break;
    }
}

// FUNCTION: WIZ8 0x00554bd0
void SetFormationPosition(W8PartyFormationState* formation, int slot, signed char new_row,
                          signed char new_column, char announce, char detach, char update_facing)
{
    W8PartyFormationPosition* position = &formation->positions[slot];
    signed char old_row = position->bQuadrant;
    signed char old_column = position->bQuadrantSlot;

    if (old_row != -1) {
        signed char* occupant = &formation->bOccupantChar[old_row][old_column];
        if (*occupant == -1) {
            srAssertFail("pFormation->bOccupantChar[bOldQuadrant] != -1", FORMATION_CPP, 0x18e, 0);
        }
        if (formation->ubQuadrantOccupants[old_row] == 0) {
            srAssertFail("pFormation->ubQuadrantOccupants[bOldQuadrant] > 0", FORMATION_CPP, 0x18f,
                         0);
        }
        *occupant = -1;
        --formation->ubQuadrantOccupants[old_row];
    }
    position->bQuadrant = new_row;
    position->bQuadrantSlot = new_column;
    if (new_row != -1) {
        signed char* occupant = &formation->bOccupantChar[new_row][new_column];
        if (*occupant != -1) {
            srAssertFail("pFormation->bOccupantChar[bNewQuadrant] == -1", FORMATION_CPP, 0x19d, 0);
        }
        if (formation->ubQuadrantOccupants[new_row] >= 3) {
            srAssertFail("pFormation->ubQuadrantOccupants[bNewQuadrant] <= 3", FORMATION_CPP, 0x19e,
                         0);
        }
        *occupant = (signed char)slot;
        ++formation->ubQuadrantOccupants[new_row];
        if (update_facing != 0) {
            switch (new_row) {
            case 0:
            case 4:
                position->facing = 0;
                break;
            case 1:
                position->facing = 1;
                break;
            case 2:
                position->facing = 2;
                break;
            case 3:
                position->facing = 3;
                break;
            }
        }
    }
    if (detach != 0 && old_row != -1) {
        CompactFormationRow(formation, old_row);
    }
    if (g_status_685170.game_started != 0 && gXStatus.fNpcDialogueMode == 0) {
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
            g_settings_6850c8.main_ui_mode != W8_MAIN_UI_MODE_RADAR &&
            formation == &g_status_685170.formation) {
            RefreshFormationBoard();
            RefreshRadarMap();
        }
        if (announce != 0) {
            if (new_row != -1) {
                PostCharacterNotice(slot, gppStringList[0x930 / 4],
                                    &g_formation_row_names_00649e54[new_row][0]);
                return;
            }
            if (old_row != -1) {
                PostCharacterNotice(slot, gppStringList[0x934 / 4],
                                    &g_formation_row_names_00649e54[old_row][0]);
            }
        }
    }
}

/* Keep one slot's seat in step with its liveness. A member who can no longer
   hold a place remembers its quadrant in bOldQuadrant and is lifted out; a
   member who can hold one again is returned to that remembered quadrant under
   the usual seating rules, or placed like a joiner when it had none. The
   remembered quadrant is cleared once the slot is dealt with. */
// FUNCTION: WIZ8 0x00554e70
void UpdateFormationSlotState(W8PartyFormationState* formation, int slot)
{
    signed char row_order[3] = {0, 4, 2};
    W8PartyFormationPosition* position = &formation->positions[slot];
    int row;
    signed char seat;
    int index;

    if (g_status_685170.buffers.characters[slot].highest_condition >= W8_CONDITION_DEAD) {
        if (position->bQuadrant == -1) {
            return;
        }
        position->bOldQuadrant = position->bQuadrant;
        SetFormationPosition(formation, slot, -1, -1, 0, 1, 1);
        return;
    }
    if (position->bQuadrant != -1) {
        return;
    }
    row = position->bOldQuadrant;
    if (row != -1) {
        switch (formation->ubQuadrantOccupants[row]) {
        case 0:
            seat = 0;
            goto seat_old;
        case 1: {
            signed char leader = formation->bOccupantChar[row][0];
            if (leader == -1) {
                srAssertFail("iChar != -1", FORMATION_CPP, 0x159, 0);
            }
            SetFormationPosition(formation, leader, row, 1, 0, 0, 1);
            seat = 2;
            goto seat_old;
        }
        case 2:
            if (formation->bOccupantChar[row][0] != -1) {
                srAssertFail("iChar == -1", FORMATION_CPP, 0x172, 0);
            }
            seat = 0;
            goto seat_old;
        default:
            break;
        }
    }
    position->bQuadrant = -1;
    position->bQuadrantSlot = -1;
    for (index = 0; index < 3; ++index) {
        row = row_order[index];
        switch (formation->ubQuadrantOccupants[row]) {
        case 0:
            seat = 0;
            goto seat_new;
        case 1: {
            signed char leader = formation->bOccupantChar[row][0];
            if (leader == -1) {
                srAssertFail("iChar != -1", FORMATION_CPP, 0x159, 0);
            }
            SetFormationPosition(formation, leader, row, 1, 0, 0, 1);
            seat = 2;
            goto seat_new;
        }
        case 2:
            if (formation->bOccupantChar[row][0] != -1) {
                srAssertFail("iChar == -1", FORMATION_CPP, 0x172, 0);
            }
            seat = 0;
            goto seat_new;
        default:
            break;
        }
    }
    position->bOldQuadrant = -1;
    return;
seat_old:
    SetFormationPosition(formation, slot, position->bOldQuadrant, seat,
                         formation == &g_status_685170.formation, 1, 1);
    position->bOldQuadrant = -1;
    return;
seat_new:
    SetFormationPosition(formation, slot, row, seat, 1, 1, 1);
    position->bOldQuadrant = -1;
}

/* Seat one party slot in a formation row, shuffling the occupants to make
   room: an empty row takes it in column 0, a row led by one character moves
   the leader to column 1 and seats the slot at column 2, and a row already
   holding two puts it in the free lead column. A full row refuses with a
   notice naming the row. */
// FUNCTION: WIZ8 0x00555080
void SeatFormationSlotInRow(W8PartyFormationState* formation, int slot, int row)
{
    signed char seat;

    switch (formation->ubQuadrantOccupants[row]) {
    case 0:
        seat = 0;
        break;
    case 1: {
        signed char leader = formation->bOccupantChar[row][0];
        if (leader == -1) {
            srAssertFail("iChar != -1", FORMATION_CPP, 0x159, 0);
        }
        SetFormationPosition(formation, leader, row, 1, 0, 0, 1);
        seat = 2;
        break;
    }
    case 2:
        if (formation->bOccupantChar[row][0] != -1) {
            srAssertFail("iChar == -1", FORMATION_CPP, 0x172, 0);
        }
        seat = 0;
        break;
    default:
        seat = -1;
        break;
    }
    if (seat != -1) {
        SetFormationPosition(formation, slot, row, seat, 0, 1, 1);
        return;
    }
    PostCharacterNotice(slot, gppStringList[0x938 / 4], &g_formation_row_names_00649e54[row][0]);
}

/* Swap two party slots' formation positions: the second takes the first's
   quadrant and quadrant slot, then the first takes the place the second
   vacated. When the second slot holds no quadrant, the first is seated in the
   row the second last occupied instead. */
// FUNCTION: WIZ8 0x00555160
void SwapFormationSlots(W8PartyFormationState* formation, int slot_a, int slot_b)
{
    W8PartyFormationPosition* position_a = &formation->positions[slot_a];
    W8PartyFormationPosition* position_b = &formation->positions[slot_b];

    if (slot_a == slot_b) {
        srAssertFail("uiChar1 != uiChar2", FORMATION_CPP, 0x296, 0);
    }
    if (position_b->bQuadrant != -1) {
        signed char row_a = position_a->bQuadrant;
        signed char column_a = position_a->bQuadrantSlot;
        signed char row_b = position_b->bQuadrant;
        signed char column_b = position_b->bQuadrantSlot;

        SetFormationPosition(formation, slot_a, -1, -1, 0, 0, 1);
        SetFormationPosition(formation, slot_b, row_a, column_a, 0, 0, 1);
        SetFormationPosition(formation, slot_a, row_b, column_b, 0, 0, 1);
        return;
    }
    if (position_b->bOldQuadrant == -1) {
        return;
    }
    {
        int row = position_b->bOldQuadrant;
        signed char seat;

        switch (formation->ubQuadrantOccupants[row]) {
        case 0:
            seat = 0;
            break;
        case 1: {
            signed char leader = formation->bOccupantChar[row][0];
            if (leader == -1) {
                srAssertFail("iChar != -1", FORMATION_CPP, 0x159, 0);
            }
            SetFormationPosition(formation, leader, row, 1, 0, 0, 1);
            seat = 2;
            break;
        }
        case 2:
            if (formation->bOccupantChar[row][0] != -1) {
                srAssertFail("iChar == -1", FORMATION_CPP, 0x172, 0);
            }
            seat = 0;
            break;
        default:
            seat = -1;
            break;
        }
        if (seat != -1) {
            SetFormationPosition(formation, slot_a, row, seat, 0, 1, 1);
            return;
        }
        PostCharacterNotice(slot_a, gppStringList[0x938 / 4],
                            &g_formation_row_names_00649e54[row][0]);
    }
}

/* Whether the monster currently faces the party's own position, inside the
   usual facing tolerance - the mirror of IsPartyLookingAt. */
// FUNCTION: WIZ8 0x00555a60
int IsMonsterFacingParty(W8MonsterInfo* monster_info)
{
    srVector3T<float> party_position = g_startup_world_659c0c->GetPosition();
    srVector3T<float> monster_position = monster_info->monster->GetPosition();
    float bearing = NormalizeAngle(GetHeadingAngle(&monster_position, &party_position));
    float facing = monster_info->monster->GetYaw();

    if (fabsf(bearing - facing) <= g_facing_tolerance_005ebcf4) {
        return 1;
    }
    return 0;
}

/* Whether the first monster faces the second, inside the usual tolerance. */
// FUNCTION: WIZ8 0x00555b00
int IsMonsterFacingMonster(W8MonsterInfo* first, W8MonsterInfo* second)
{
    srVector3T<float> second_position = second->monster->GetPosition();
    srVector3T<float> first_position = first->monster->GetPosition();
    float bearing = NormalizeAngle(GetHeadingAngle(&first_position, &second_position));
    float facing = first->monster->GetYaw();

    if (fabsf(bearing - facing) <= g_facing_tolerance_005ebcf4) {
        return 1;
    }
    return 0;
}

/* Whether the second monster is looking away from the first, measured the
   shortest way round like IsPartyLookingAwayFrom. */
// FUNCTION: WIZ8 0x00555de0
int IsMonsterLookingAwayFrom(W8MonsterInfo* first, W8MonsterInfo* second)
{
    srVector3T<float> first_position = first->monster->GetPosition();
    srVector3T<float> second_position = second->monster->GetPosition();
    float bearing = NormalizeAngle(GetHeadingAngle(&second_position, &first_position));
    float facing = second->monster->GetYaw();

    if (static_cast<float>(g_facing_tolerance_005ee858) <= ShortestAngleDistance(bearing, facing)) {
        return 1;
    }
    return 0;
}

/* The monster's bearing off the camera direction folded into one of the four
   screen sides - the same 0..3 facing values the formation positions carry.
   The three checks below expand the same switch, which is why all of them
   carry the one assert line. */
// FUNCTION: WIZ8 0x00555960
bool IsCharacterFacingMonster(int party_slot, W8MonsterInfo* monster_info)
{
    srVector3T<float> camera_position;
    srVector3T<float> monster_position = monster_info->monster->GetPosition();
    signed char side;
    int angle;

    GetCameraPosition(&camera_position);
    angle = static_cast<int>(NormalizeAngle(GetHeadingAngle(&camera_position, &monster_position)));
    angle -= g_status_685170.party_facing;
    if (angle < 0) {
        angle += 0x168;
    }
    switch (((angle + 0x2d) % 0x168) / 0x5a) {
    case 0:
        side = 0;
        break;
    case 1:
        side = 1;
        break;
    case 2:
        side = 2;
        break;
    case 3:
        side = 3;
        break;
    default:
        srAssertFail("FALSE", FORMATION_CPP, 0x456, 0);
        return true;
    }
    return side == g_status_685170.formation.positions[party_slot].facing;
}

/* Face the committed target while retaining the formation's four-side model. */
// FUNCTION: WIZ8 0x00555560
void FaceCharacterTowardCombatTarget(int party_slot, W8CombatSlot* target)
{
    signed char facing;
    srVector3T<float> position;
    bool has_position = false;

    switch (target->iType) {
    case W8_TARGET_KIND_CHARACTER:
    case W8_TARGET_KIND_CHARACTER_INDIRECT:
        if (target->iChar == -1) {
            srAssertFail("pTarget->iChar != BAD_INDEX", FORMATION_CPP, 0x33a, 0);
        }
        facing = DecideFacingForPosition(party_slot, target->iChar);
        if (facing == W8_FACING_ANY) {
            return;
        }
        break;
    case W8_TARGET_KIND_MONSTER:
        if (target->iMonsterID == -1) {
            srAssertFail("pTarget->iMonsterID != BAD_INDEX", FORMATION_CPP, 0x33f, 0);
        }
        position = GetMonsterByLocationID(target->iMonsterID)->GetPosition();
        has_position = true;
        break;
    case W8_TARGET_KIND_PLACE:
        position = target->point;
        has_position = true;
        break;
    default:
        return;
    }
    if (has_position) {
        srVector3T<float> camera_position;
        GetCameraPosition(&camera_position);
        int angle = static_cast<int>(NormalizeAngle(GetHeadingAngle(&camera_position, &position)));
        angle -= g_status_685170.party_facing;
        if (angle < 0) {
            angle += W8_DEGREES_PER_TURN;
        }
        int quadrant =
            ((angle + W8_DEGREES_PER_QUADRANT / 2) % W8_DEGREES_PER_TURN) / W8_DEGREES_PER_QUADRANT;
        switch (quadrant) {
        case 0:
            facing = 0;
            break;
        case 1:
            facing = 1;
            break;
        case 2:
            facing = 2;
            break;
        case 3:
            facing = 3;
            break;
        default:
            srAssertFail("FALSE", FORMATION_CPP, 0x456, 0);
            return;
        }
    }
    if (g_status_685170.formation.positions[party_slot].facing != facing) {
        g_status_685170.formation.positions[party_slot].facing = facing;
        RefreshFormationBoard();
    }
}

/* Turn the character's formation facing toward the side the monster is on. */
// FUNCTION: WIZ8 0x00555820
void TurnCharacterTowardMonster(int party_slot, W8MonsterInfo* monster_info)
{
    srVector3T<float> camera_position;
    srVector3T<float> monster_position = monster_info->monster->GetPosition();
    signed char side;
    int angle;

    GetCameraPosition(&camera_position);
    angle = static_cast<int>(NormalizeAngle(GetHeadingAngle(&camera_position, &monster_position)));
    angle -= g_status_685170.party_facing;
    if (angle < 0) {
        angle += 0x168;
    }
    switch (((angle + 0x2d) % 0x168) / 0x5a) {
    case 0:
        side = 0;
        break;
    case 1:
        side = 1;
        break;
    case 2:
        side = 2;
        break;
    case 3:
        side = 3;
        break;
    default:
        srAssertFail("FALSE", FORMATION_CPP, 0x456, 0);
        return;
    }
    if (g_status_685170.formation.positions[party_slot].facing != side) {
        g_status_685170.formation.positions[party_slot].facing = side;
        RefreshFormationBoard();
    }
}

/* Whether the monster sits on the side opposite the one the character faces -
   the being-snuck-up-on test. */
// FUNCTION: WIZ8 0x00555c60
int IsMonsterBehindCharacter(W8MonsterInfo* monster_info, int party_slot)
{
    srVector3T<float> camera_position;
    srVector3T<float> monster_position = monster_info->monster->GetPosition();
    signed char side;
    int angle;
    int difference;

    GetCameraPosition(&camera_position);
    angle = static_cast<int>(NormalizeAngle(GetHeadingAngle(&camera_position, &monster_position)));
    angle -= g_status_685170.party_facing;
    if (angle < 0) {
        angle += 0x168;
    }
    switch (((angle + 0x2d) % 0x168) / 0x5a) {
    case 0:
        side = 0;
        break;
    case 1:
        side = 1;
        break;
    case 2:
        side = 2;
        break;
    case 3:
        side = 3;
        break;
    default:
        srAssertFail("FALSE", FORMATION_CPP, 0x456, 0);
        return 0;
    }
    difference = side - g_status_685170.formation.positions[party_slot].facing;
    if (difference < 0) {
        difference = -difference;
    }
    return difference == 2;
}
