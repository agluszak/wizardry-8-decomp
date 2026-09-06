#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/startup_world.h"
#include "wiz8/xstatus.h"
#include "wiz8/character.h"
#include "wiz8/combat_state.h"
#include "wiz8/notices.h"
#include "wiz8/utility.h"
#include "surrender/srMath.h"

#include <math.h>

/* Original translation unit: Local Code\Formation & Facing.cpp. */

/* Fills in the party's own world position. */
extern void GetPartyPosition(srVector3T<float>* position);          /* 0x00421070 */
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

    GetPartyPosition(&party);
    bearing = static_cast<int>(NormalizeAngle(BearingBetween(party, position)));
    bearing -= g_status_685170.party_facing;
    if (bearing < 0) {
        bearing += W8_DEGREES_PER_TURN;
    }
    return ((bearing + W8_DEGREES_PER_QUADRANT / 2) % W8_DEGREES_PER_TURN) /
           W8_DEGREES_PER_QUADRANT;
}

#include "wiz8/sr_api.h"

#define FORMATION_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Formation & Facing.cpp"

/* The party formation. It is one thirty-three dword block that combat saves
   into its own state and compares against, and within it two per-position
   tables twelve bytes apart give each position's row and facing, plus a table
   of the three positions standing in each of the five rows. */
enum { W8_FORMATION_BLOCK_DWORDS = 0x21, W8_FORMATION_ROWS = 5, W8_POSITIONS_PER_ROW = 3 };

/* The facing answer that means "no preference", which never disagrees with
   whatever a position is already facing. */
enum { W8_FACING_ANY = 4 };

/* The state a character has to be under to hold a place in the formation.
   Tighter than the eligibility window the party sweeps use. */
enum { W8_FORMATION_ELIGIBLE_LIMIT = 0xd };

extern signed char DecideFacingForPosition(int position, int arg_2);  /* 0x00555E70 */
extern void Function5B1C80(void);
extern void Function5A24A0(void);
extern void Function5B1E70(void);
extern unsigned int GetCameraHeading(void);                 /* 0x00421550 */
extern unsigned int SetCameraHeading(float degrees);        /* 0x00421000 */
extern void SnapCameraHeading(float degrees);               /* 0x00420FD0 */
extern double g_facing_tolerance_005ee858;
extern float g_facing_tolerance_005ebcf4;

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
    int index;
    int* saved;

    if (gXStatus.fCombatMode == 0) {
        srAssertFail("gXStatus.fCombatMode", FORMATION_CPP, 258, 0);
    }
    saved = (int*)((char*)g_combat_state + 0x920);
    for (index = 0; index < W8_FORMATION_BLOCK_DWORDS; ++index) {
        saved[index] =
            reinterpret_cast<int*>(&g_status_685170.formation)[index];
    }
}

/* Put the formation combat started with back, if anything moved. Comparing the
   whole block is what makes it one object rather than a set of tables. */
// FUNCTION: WIZ8 0x00554a60
void RestoreCombatFormation(void)
{
    int index;
    const int* saved;
    bool unchanged = true;

    if (gXStatus.fCombatMode == 0) {
        srAssertFail("gXStatus.fCombatMode", FORMATION_CPP, 266, 0);
    }
    saved = (const int*)((char*)g_combat_state + 0x920);
    for (index = 0; index < W8_FORMATION_BLOCK_DWORDS; ++index) {
        unchanged =
            reinterpret_cast<int*>(&g_status_685170.formation)[index] ==
            saved[index];
        if (!unchanged) {
            break;
        }
    }

    if (!unchanged) {
        for (index = 0; index < W8_FORMATION_BLOCK_DWORDS; ++index) {
            reinterpret_cast<int*>(&g_status_685170.formation)[index] =
                saved[index];
        }
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
        previous = GetCameraHeading() / W8_DEGREES_PER_TURN;
        if (GetCameraHeading() % W8_DEGREES_PER_TURN != degrees) {
            previous = SetCameraHeading((float)degrees);
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
    if (GetCameraHeading() % W8_DEGREES_PER_TURN == degrees) {
        return;
    }
    if (snap) {
        SnapCameraHeading((float)degrees);
    }
    else {
        SetCameraHeading((float)degrees);
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
