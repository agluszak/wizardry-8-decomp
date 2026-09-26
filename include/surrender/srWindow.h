#pragma once

/* Retail exposes only the Win32 window-handle probe (0x10045A30); the rest of
   the class is unrecovered. */
class srWindow {
public:
    static int isWindow(unsigned long handle);
    static long getWidth(unsigned long handle);
    static long getHeight(unsigned long handle);

    /* The exported assignment copies a single byte; the class carries one
       unnamed member. */
    // FUNCTION: SURRENDER 0x100458A0
    // ??4srWindow@@QAEAAV0@ABV0@@Z
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srWindow& operator=(const srWindow& other)
    {
        unknown_00 = other.unknown_00;
        return *this;
    }

private:
    unsigned char unknown_00;
};
