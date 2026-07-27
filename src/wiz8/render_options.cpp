#include "wiz8/gameplay_boundaries.h"

/*
 * The render-option table. 0x0047B630 is a switch over 0x11 options that pushes
 * each one into the SurRender device - texture filtering among them - and then
 * records its new state. Its second argument is a boolean: two of the cases
 * store it as `argument != 0` outright, and the rest read as off against on.
 * The three helpers here sit on top of it: turn one option off, turn every
 * option on, and read an option's recorded state back.
 *
 * The stored bytes live two bytes into the block at 0x0065A118, which is what
 * the +2 in the accessor is. Nothing here establishes what that leading pair
 * holds, so it is not modelled.
 */

extern "C" {

extern unsigned char* g_render_options_65a118;

/* 0x0047B630: pushes one option to the device and records it. Not ported yet -
   it calls four srGERD methods that the import library does not declare. */
extern void SetRenderOption(int option, int enabled);

// FUNCTION: WIZ8 0x0047B5B0
void DisableRenderOption(int option)
{
    if (option < 0x11) {
        SetRenderOption(option, 0);
    }
}

/* The original carries a dead entry test: it compares the counter against the
   bound before the first iteration and, when that fails, jumps to the increment
   rather than past the loop. Starting at zero it can never fire, and VC6 folds
   it away here whichever way the loop is written - for, while and do-while all
   give the same 22 bytes. The five-byte difference is that fold, not a
   difference in what the loop does. */
// FUNCTION: WIZ8 0x0047B5F0
void EnableAllRenderOptions(void)
{
    int option;

    option = 0;
    while (option < 0x11) {
        SetRenderOption(option, 1);
        option++;
    }
}

/* Out-of-range reads report zero rather than indexing past the block. */
// FUNCTION: WIZ8 0x0047B610
unsigned char GetRenderOptionState(int option)
{
    if (option >= 0x11) {
        return 0;
    }
    return g_render_options_65a118[2 + option];
}

}
