#include "wiz8/local_code/FormationAndFacing.h"

#include <string.h>

/* This initializer lies in the reviewed attribution gap between Magic
   Effects.cpp and Formation & Facing.cpp, 0x70 below the anchored Formation
   & Facing hull start at 0x005545F0. Keep the file classified as an
   unresolved fragment; the descriptive name does not prove original-TU
   identity. */

// FUNCTION: WIZ8 0x00554580
void InitializePartyFormation(W8PartyFormationState* formation)
{
    unsigned int index;

    memset(formation, 0, sizeof(*formation));
    for (index = 0; index < 5; ++index) {
        formation->bOccupantChar[index][0] = -1;
        formation->bOccupantChar[index][1] = -1;
        formation->bOccupantChar[index][2] = -1;
        formation->ubQuadrantOccupants[index] = 0;
    }
    for (index = 0; index < 8; ++index) {
        formation->positions[index].bQuadrant = 0xff;
        formation->positions[index].bOldQuadrant = 0xff;
        formation->positions[index].bQuadrantSlot = -1;
        formation->positions[index].facing = 4;
    }
}
