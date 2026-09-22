#include "surrender/srWindow.h"

#include <windows.h>

// FUNCTION: SURRENDER 0x10045A30
int srWindow::isWindow(unsigned long handle)
{
    /* reinterpret-ok: Win32 window handle arrives as a raw ulong across the
       srGERD ABI boundary. */
    return IsWindow(reinterpret_cast<HWND>(handle));
}
