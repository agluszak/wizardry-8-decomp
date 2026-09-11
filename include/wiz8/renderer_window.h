#pragma once

#include "surrender/srMath.h"

void PublishLightDirection(const int* direction);                 /* 0x00427380 */
/* Clamp a colour triple to the unit range in place and return it. */
srVector3T<float>* __fastcall SaturateColor004299B0(srVector3T<float>* color);
