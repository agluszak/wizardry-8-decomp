#pragma once

/* The one-byte assignment at 0x100458A0 is explained by ordinary empty-class
   copying; it does not establish semantic instance storage. Provider exports
   do not establish consumer dllimport visibility. */
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srWindow {
public:
    static int isWindow(unsigned long handle);
    static long getWidth(unsigned long handle);
    static long getHeight(unsigned long handle);
};
