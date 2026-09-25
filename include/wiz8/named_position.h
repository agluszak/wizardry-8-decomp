#pragma once

#include "surrender/srMath.h"

struct W8NamedPosition {
    W8NamedPosition()
    {
        name[0] = '\0';
        position.SetZero();
        angle = 0.0f;
        direction_090.SetZero();
    }

    char name[0x80];
    srVector3T<float> position;
    float angle; /* 0x8c */
    srVector3T<float> direction_090;
};

static_assert(sizeof(W8NamedPosition) == 0x9c, "W8NamedPosition_must_be_0x9c");
