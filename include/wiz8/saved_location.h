#ifndef WIZ8_SAVED_LOCATION_H
#define WIZ8_SAVED_LOCATION_H

#include "surrender/srMath.h"

#pragma pack(push, 1)

/* The 0x3c-byte anchor a character carries and the recall effect restores.
   Only the leading point is read field by field; the rest travels as one
   block, so nothing beyond it is named. */
struct W8SavedLocation {
    srVector3T<float> point;              /* 0x00 */
    unsigned char unknown_0c[0x30];
};                                       /* 0x3c */

#pragma pack(pop)

static_assert(sizeof(W8SavedLocation) == 0x3c,
              "W8SavedLocation_must_be_0x3c");

#endif
