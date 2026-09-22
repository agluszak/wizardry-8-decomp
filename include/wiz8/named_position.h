#pragma once

#include "surrender/srMath.h"

struct W8NamedPosition {
    W8NamedPosition()
    {
        name[0] = '\0';
        position.SetZero();
        angle_bits_08c = 0;
        direction_090.SetZero();
    }

    char name[0x80];
    srVector3T<float> position;
    /* Stored as int bits and returned through a float* out-param by
       FindEntityByName. */
    int angle_bits_08c;
    srVector3T<float> direction_090;
};

static_assert(sizeof(W8NamedPosition) == 0x9c, "W8NamedPosition_must_be_0x9c");
