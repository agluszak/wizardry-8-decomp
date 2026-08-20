#include "wiz8/game_state.h"

#include <string.h>

/* This initializer lies in the reviewed attribution gap between Magic
   Effects.cpp and Formation & Facing.cpp. */

// FUNCTION: WIZ8 0x00554580
void Function554580(unsigned char* storage)
{
    W8PartyStatusState* state = (W8PartyStatusState*)storage;
    unsigned int index;

    memset(state, 0, sizeof(*state));
    for (index = 0; index < 5; ++index) {
        state->selections[index].index = 0xffff;
        state->selections[index].kind = 0xff;
        state->flags_0f[index] = 0;
    }
    for (index = 0; index < 8; ++index) {
        state->rows_14[index].indices[0] = 0xff;
        state->rows_14[index].indices[1] = 0xff;
        state->rows_14[index].indices[2] = 0xff;
        state->rows_14[index].mode = 4;
    }
}
