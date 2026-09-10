#pragma once

#include "wiz8/wiz8_windows.h"

struct Controls;

extern Controls* g_panel_69bf4c;
extern Controls* g_panel_69bf40;

void RedrawPanel69BF4C(void);
void RedrawPanel69BF40(void);
void EnablePanel69BF40005A1EA0(void);
/* Releases the party-movement panels at 0x0069BF40/0x0069BF4C and clears the
   combat-UI teardown flag; called when the party regains movement. */
void ReleasePartyMovement(void);                                 /* 0x005A1890 */

void Function5A1EB0(POINT* point, unsigned int* value);

