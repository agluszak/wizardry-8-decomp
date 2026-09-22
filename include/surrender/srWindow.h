#pragma once

/* Retail exposes only the Win32 window-handle probe (0x10045A30); the rest of
   the class is unrecovered. */
class srWindow {
public:
    static int isWindow(unsigned long handle);
    static long getWidth(unsigned long handle);
    static long getHeight(unsigned long handle);
};
