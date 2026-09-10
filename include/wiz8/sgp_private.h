#pragma once

#include "sgp.h"
#include "wiz8/wiz8_windows.h"

/* Released sgp.c interfaces which its public header does not declare. */
#ifdef __cplusplus
extern "C" {
#endif
extern HINSTANCE ghInstance;
INT32 FAR PASCAL WindowProcedure(
    HWND window, UINT16 message, WPARAM wparam, LPARAM lparam);
BOOLEAN InitializeStandardGamingPlatform(
    HINSTANCE instance, int show_command);
void ShutdownStandardGamingPlatform(void);
#ifdef __cplusplus
}
#endif
