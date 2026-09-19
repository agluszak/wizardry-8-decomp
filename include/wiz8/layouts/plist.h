#ifndef WIZ8_LAYOUTS_PLIST_H
#define WIZ8_LAYOUTS_PLIST_H

/*
 * 3D Code\PList.cpp. A growable array of void pointers - the engine's
 * general-purpose list, distinct from the typed growable-vector template.
 * All twelve source-owned functions agree on this layout.
 */

struct W8PList {
    void** data;  /* 0x00 */
    int capacity; /* 0x04: PListInit allocates 10 */
    int iNumUsed; /* 0x08 */
};

static_assert(sizeof(W8PList) == 0x0c, "W8PList_must_be_0x0c");

#endif
