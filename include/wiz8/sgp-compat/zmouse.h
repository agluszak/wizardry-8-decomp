#ifndef WIZ8_SGP_COMPAT_ZMOUSE_H
#define WIZ8_SGP_COMPAT_ZMOUSE_H

#include <windows.h>

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif
#ifndef WHEEL_DELTA
#define WHEEL_DELTA 120
#endif
#ifndef MSH_MOUSEWHEEL
#define MSH_MOUSEWHEEL "MSWHEEL_ROLLMSG"
#endif

extern BOOL SGPMouseGetPos(LPPOINT point);

#endif
