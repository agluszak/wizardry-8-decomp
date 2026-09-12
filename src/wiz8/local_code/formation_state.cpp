#include "wiz8/local_code/FormationAndFacing.h"

#include <string.h>

/* This initializer lies in the reviewed attribution gap between Magic
   Effects.cpp and Formation & Facing.cpp. Keep the file classified as an
   unresolved fragment; the descriptive name does not prove original-TU
   identity. */

// FUNCTION: WIZ8 0x00554580
void InitializePartyFormation(W8PartyFormationState* formation)
{
    unsigned int index;

    memset(formation, 0, sizeof(*formation));
    for (index = 0; index < 5; ++index) {
        formation->rows[index].slots[0] = -1;
        formation->rows[index].slots[1] = -1;
        formation->rows[index].slots[2] = -1;
        formation->flags_0f[index] = 0;
    }
    for (index = 0; index < 8; ++index) {
        formation->positions[index].row = 0xff;
        formation->positions[index].unknown_01[0] = 0xff;
        formation->positions[index].unknown_01[1] = 0xff;
        formation->positions[index].facing = 4;
    }
}
