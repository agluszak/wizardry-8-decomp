#include "surrender/srWindow.h"

#include <windows.h>

// FUNCTION: SURRENDER 0x100459D0
long srWindow::getWidth(unsigned long handle)
{
    RECT client;
    /* reinterpret-ok: Win32 window handle arrives as a raw ulong across the
       srGERD ABI boundary. */
    HWND window = reinterpret_cast<HWND>(handle);
    if (isWindow(handle) == 0) {
        return 0;
    }
    GetClientRect(window, &client);
    return client.right;
}

// FUNCTION: SURRENDER 0x10045A00
long srWindow::getHeight(unsigned long handle)
{
    RECT client;
    /* reinterpret-ok: Win32 window handle arrives as a raw ulong across the
       srGERD ABI boundary. */
    HWND window = reinterpret_cast<HWND>(handle);
    if (isWindow(handle) == 0) {
        return 0;
    }
    GetClientRect(window, &client);
    return client.bottom;
}

// FUNCTION: SURRENDER 0x10045A30
int srWindow::isWindow(unsigned long handle)
{
    /* reinterpret-ok: Win32 window handle arrives as a raw ulong across the
       srGERD ABI boundary. */
    return IsWindow(reinterpret_cast<HWND>(handle));
}
