#include "wiz8/character.h"

extern "C" {
extern unsigned char g_flag_68c4f4;
extern unsigned char g_flag_68c4f7;
extern unsigned char g_flag_68c4fa;
extern unsigned char g_flag_68c500;
// GLOBAL: WIZ8 0x0068c4f4
unsigned char g_flag_68c4f4;
// GLOBAL: WIZ8 0x0068c4fa
unsigned char g_flag_68c4fa;
// GLOBAL: WIZ8 0x0068c500
unsigned char g_flag_68c500;
}

/* Local Code\NPC Scripting.cpp. The NPC-scripting flag gates the scripted
   monster state; the four accessors below are its only owners. */

// FUNCTION: WIZ8 0x00529560
void SetFlag68C4F4(void)
{
    g_flag_68c4f4 = 1;
}
// FUNCTION: WIZ8 0x00529BC0
void SetFlag68C4F7(void)
{
    g_flag_68c4f7 = 1;
}
// FUNCTION: WIZ8 0x00529BD0
void ClearFlag68C4F7(void)
{
    g_flag_68c4f7 = 0;
}
// FUNCTION: WIZ8 0x0052A070
unsigned char GetFlag68C4FA(void)
{
    return g_flag_68c4fa;
}
// FUNCTION: WIZ8 0x0052A1A0
void SetFlag68C500(unsigned char value)
{
    g_flag_68c500 = value;
}

/* Rebuilds the hit-point ceiling from scratch every time it is called: each
   profession the character has levels in contributes its own per-level factor,
   scaled by a figure derived from the fourth attribute record's effective
   value, and the profession the character started in counts one level more
   than it has taken. The running total lives in the x87 stack across the whole
   loop, which is why the zero it starts from is loaded before the profession
   guard and discarded by an `fstp` on the early return.
   The level is unsigned - the emitted test is `jbe`, not `jle` - and the
   attribute is widened through a zeroed high dword, which is the unsigned
   conversion rather than the signed one.
   Losing the last hit point applies condition 0x12 with the ceiling duration
   the party notice uses, which is the one place this writes anything beyond
   the two pools. */
