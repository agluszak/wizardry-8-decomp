#pragma once

#include "surrender/srMath.h"

/* An RGB colour kept in a float vector. Retail's static initialisers fill the
   colour tables and g_light_direction with 1.0f per component (0x004821E0,
   0x00482200, 0x00482220), so the inline default constructor makes white.
   The renderer takes these colours wherever it expects srVector3T<float>
   (fog colour, SaturateColor) with no conversion. The clamping
   constructor and Set are recovered in AutomapScreen.cpp (0x005806B0,
   0x00580940). */
struct EnvironmentColour : public srVector3T<float> {
    EnvironmentColour()
    {
        x = 1.0f;
        y = 1.0f;
        z = 1.0f;
    }
    EnvironmentColour(double red_value, double green_value, double blue_value);
    EnvironmentColour& operator=(double value)
    {
        Set(value, value, value);
        return *this;
    }
    void Set(double red_value, double green_value, double blue_value);
};

static_assert(sizeof(EnvironmentColour) == 0x0c, "EnvironmentColour_must_be_0x0c");
