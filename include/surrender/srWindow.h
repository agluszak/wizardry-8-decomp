#pragma once

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
