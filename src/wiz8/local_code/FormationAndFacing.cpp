#include "wiz8/local_code/FormationAndFacing.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/MGSFormation.h"
#include "wiz8/local_screens/MGSRadarMap.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/screen_state.h"
#include "wiz8/startup_world.h"
#include "wiz8/xstatus.h"
#include "wiz8/character.h"
#include "wiz8/combat_state.h"
#include "wiz8/notices.h"
#include "wiz8/utility.h"
#include "surrender/srMath.h"

#include <math.h>
#include <string.h>

/* Original translation unit: Local Code\Formation & Facing.cpp. */

/* Fills in the party's own world position. */
/* A full turn, and the half-quadrant the bearing is biased by so that a
   quadrant is centred on its facing rather than starting at it. */
enum { W8_DEGREES_PER_TURN = 360, W8_DEGREES_PER_QUADRANT = 90 };

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

#include "wiz8/sr_api.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/local_screens/RCSCommon.h"

#define FORMATION_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Formation & Facing.cpp"

/* The party formation. It is one thirty-three dword block that combat saves
   into its own state and compares against, and within it two per-position
   tables twelve bytes apart give each position's row and facing, plus a table
   of the three positions standing in each of the five rows. */
enum { W8_FORMATION_ROWS = 5, W8_POSITIONS_PER_ROW = 3 };

/* The facing answer that means "no preference", which never disagrees with
   whatever a position is already facing. */
enum { W8_FACING_ANY = 4 };

/* The state a character has to be under to hold a place in the formation.
   Tighter than the eligibility window the party sweeps use. */
enum { W8_FORMATION_ELIGIBLE_LIMIT = 0xd };


// GLOBAL: WIZ8 0x005ee858
double g_facing_tolerance_005ee858 = 2.3561944500000003;

// GLOBAL
float g_facing_tolerance_005ebcf4;

/* Whether one character can hold a place in the formation at all: they have to
   be alive and in better shape than the party sweeps demand. */
// FUNCTION: WIZ8 0x005549e0
bool CanHoldFormationPlace(int party_slot)
{
    const W8Character* character = &g_party_characters[party_slot];

    return character->hp_current != 0 &&
           character->unknown_0b01 < W8_FORMATION_ELIGIBLE_LIMIT;
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
        Function5B1C80();
        Function5A24A0();
        ShowNotice(8, gppStringList[0x92c / 4], 0, -1, 0);
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
        Function5B1E70();
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
    Function5B1E70();
    if ((unsigned int)GetCameraYawDegrees() % W8_DEGREES_PER_TURN == degrees) {
        return;
    }
    if (snap) {
        TurnCameraToDegrees((float)degrees);
    }
    else {
        SetCameraYawDegrees((float)degrees);
    }
}

/* Face one position the way the rules say it should, unless the rules have no
   preference or it already faces that way. */
// FUNCTION: WIZ8 0x005557e0
void FacePositionAsDecided(int position, int arg_2)
{
    signed char facing = DecideFacingForPosition(position, arg_2);

    if (facing != W8_FACING_ANY &&
        g_status_685170.formation.positions[position].facing != facing) {
        g_status_685170.formation.positions[position].facing = facing;
        Function5B1C80();
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
    return facing ==
           g_status_685170.formation.positions[position].facing;
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
    difference =
        facing - g_status_685170.formation.positions[position].facing;
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
    float bearing = NormalizeAngle(BearingBetween(
        monster_info->monster->GetPosition(),
        g_startup_world_659c0c->GetPosition()));
    float facing = monster_info->monster->GetYaw();

    return ShortestAngleDistance(bearing, facing) >=
           g_facing_tolerance_005ee858;
}

/* Whether the party is looking at a point, measured as a plain difference
   rather than the shortest way round - so a bearing either side of the wrap
   answers no. */
// FUNCTION: WIZ8 0x00555ba0
bool IsPartyLookingAt(
    W8MonsterInfo* monster_info, srVector3T<float> point)
{
    float bearing = NormalizeAngle(BearingBetween(
        monster_info->monster->GetPosition(), point));

    return fabsf(bearing - monster_info->monster->GetYaw()) <=
           g_facing_tolerance_005ebcf4;
}

/* The five formation rows' display names, indexed by row. Two of the five
   carry no name in the data. */
// GLOBAL: WIZ8 0x00649e54
W8WideChar g_formation_row_names_00649e54[5][10] = {
    L"Front", L"", L"Right", L"", L"Rear",
};

/* Give one joining party slot its formation position: clear the position and
   walk the rows in the 0, 4, 2 order. An empty row takes the joiner in its
   first column; a row led by one character moves that leader to the second
   column and puts the joiner in the third; a row already holding two takes
   the joiner directly. */
// FUNCTION: WIZ8 0x00554ae0
void Function554AE0(W8PartyFormationState* formation, int slot)
{
    unsigned char row_order[3] = {0, 4, 2};
    W8PartyFormationPosition* position = &formation->positions[slot];

    position->row = 0xff;
    position->unknown_01[1] = 0xff;
    for (unsigned int index = 0; index < 3; ++index) {
        unsigned char row = row_order[index];
        signed char occupants = formation->flags_0f[row];
        if (occupants == 0) {
            Function554BD0(formation, slot, row, 0, 1, 1, 1);
            return;
        }
        if (occupants == 1) {
            signed char leader = formation->rows[row].slots[0];
            if (leader == -1) {
                srAssertFail("iChar != -1", FORMATION_CPP, 0x159, 0);
            }
            Function554BD0(formation, leader, row, 1, 0, 0, 1);
            Function554BD0(formation, slot, row, 2, 1, 1, 1);
            return;
        }
        if (occupants == 2) {
            if (formation->rows[row].slots[0] != -1) {
                srAssertFail("iChar == -1", FORMATION_CPP, 0x172, 0);
            }
            Function554BD0(formation, slot, row, 0, 1, 1, 1);
            return;
        }
    }
}

/* Move one party position into a formation row and column: clear the old
   entry from its row's occupant list, install the new row and column, place
   the slot in the new row and update the facing. The detach flag re-places
   the positions left in the old row, and the announce flag posts the row
   name to the slot. */
// FUNCTION: WIZ8 0x00554bd0
void Function554BD0(W8PartyFormationState* formation, int slot, int new_row,
                    int new_column, int announce, int detach,
                    int update_facing)
{
    W8PartyFormationPosition* position = &formation->positions[slot];
    signed char old_row = position->row;
    signed char old_column = position->unknown_01[1];

    if (old_row != -1) {
        signed char* occupant = &formation->rows[old_row].slots[old_column];
        if (*occupant == -1) {
            srAssertFail("pFormation->bOccupantChar[bOldQuadrant] != -1",
                         FORMATION_CPP, 0x18e, 0);
        }
        if (formation->flags_0f[old_row] == 0) {
            srAssertFail("pFormation->ubQuadrantOccupants[bOldQuadrant] > 0",
                         FORMATION_CPP, 0x18f, 0);
        }
        *occupant = -1;
        --formation->flags_0f[old_row];
    }
    position->row = (signed char)new_row;
    position->unknown_01[1] = (unsigned char)new_column;
    if (new_row != -1) {
        signed char* occupant = &formation->rows[new_row].slots[new_column];
        if (*occupant != -1) {
            srAssertFail("pFormation->bOccupantChar[bNewQuadrant] == -1",
                         FORMATION_CPP, 0x19d, 0);
        }
        if (formation->flags_0f[new_row] >= 3) {
            srAssertFail("pFormation->ubQuadrantOccupants[bNewQuadrant] <= 3",
                         FORMATION_CPP, 0x19e, 0);
        }
        *occupant = (signed char)slot;
        ++formation->flags_0f[new_row];
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
        Function554DD0(formation, old_row);
    }
    if (g_status_685170.game_started != 0 && g_flag_00683f97 == 0) {
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
            g_flag_006850ce != 2 && formation == &g_status_685170.formation) {
            Function5B1C80();
            Function5A24A0();
        }
        if (announce != 0) {
            if (new_row != -1) {
                PostCharacterNotice(
                    slot, gppStringList[0x930 / 4],
                    &g_formation_row_names_00649e54[new_row][0]);
                return;
            }
            if (slot != -1) {
                PostCharacterNotice(
                    slot, gppStringList[0x934 / 4],
                    &g_formation_row_names_00649e54[slot][0]);
            }
        }
    }
}
