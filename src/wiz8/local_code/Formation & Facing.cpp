#include "wiz8/gameplay_boundaries.h"

/* Local Code\Formation & Facing.cpp. */

/* Fills in the party's own world position. */
extern void GetPartyPosition(srVector3T<float>* position);          /* 0x00421070 */
/* The bearing in degrees from the first position to the second. */
extern float BearingBetween(const srVector3T<float>* from,
                            const srVector3T<float>* to);           /* 0x004BE420 */
/* The party's current facing in degrees. */
extern int g_party_facing;                                          /* 0x00686A40 */

/* A full turn, and the half-quadrant the bearing is biased by so that a
   quadrant is centred on its facing rather than starting at it. */
enum { W8_DEGREES_PER_TURN = 360, W8_DEGREES_PER_QUADRANT = 90 };

/* Which of the four quadrants around the party a world position falls in.
   The bearing to the position is taken relative to the party's facing, wrapped
   into a single turn, then biased by half a quadrant before the divide - so
   quadrant zero is the 90 degrees centred on straight ahead rather than the 90
   beginning there. */
// FUNCTION: WIZ8 0x00555F30
int GetQuadrantForPosition(srVector3T<float> position)
{
    srVector3T<float> party;
    int bearing;

    GetPartyPosition(&party);
    bearing = static_cast<int>(NormalizeAngle(BearingBetween(&party, &position)));
    bearing -= g_party_facing;
    if (bearing < 0) {
        bearing += W8_DEGREES_PER_TURN;
    }
    return ((bearing + W8_DEGREES_PER_QUADRANT / 2) % W8_DEGREES_PER_TURN) /
           W8_DEGREES_PER_QUADRANT;
}
