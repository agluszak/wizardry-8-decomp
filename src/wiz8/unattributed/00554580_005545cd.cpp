#include "wiz8/game_state.h"

#include <string.h>

/* This initializer lies in the reviewed attribution gap between Magic
   Effects.cpp and Formation & Facing.cpp. */

// FUNCTION: WIZ8 0x00554580
void Function554580(unsigned char* storage)
{
    W8PartyFormationState* state = (W8PartyFormationState*)storage;
    unsigned int index;

    memset(state, 0, sizeof(*state));
    for (index = 0; index < 5; ++index) {
        state->rows[index].slots[0] = -1;
        state->rows[index].slots[1] = -1;
        state->rows[index].slots[2] = -1;
        state->flags_0f[index] = 0;
    }
    for (index = 0; index < 8; ++index) {
        state->positions[index].row = 0xff;
        state->positions[index].unknown_01[0] = 0xff;
        state->positions[index].unknown_01[1] = 0xff;
        state->positions[index].facing = 4;
    }
}
