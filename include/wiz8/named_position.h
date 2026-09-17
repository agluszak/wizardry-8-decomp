#pragma once

#include "surrender/srMath.h"

struct W8NamedPosition {
    W8NamedPosition()
    {
        name[0] = '\0';
        position.SetZero();
        value_08c = 0;
        value_090 = 0.0f;
        value_094 = 0.0f;
        value_098 = 0.0f;
    }

    char name[0x80];
    srVector3T<float> position;
    int value_08c;
    float value_090;
    float value_094;
    float value_098;
};

static_assert(sizeof(W8NamedPosition) == 0x9c, "W8NamedPosition_must_be_0x9c");
