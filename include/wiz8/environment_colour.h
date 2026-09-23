#pragma once

/* RGB triple stored by the world environment. */
struct EnvironmentColour {
    EnvironmentColour() {}
    EnvironmentColour(double red_value, double green_value, double blue_value);
    EnvironmentColour& operator=(double value)
    {
        Set(value, value, value);
        return *this;
    }
    void Set(double red_value, double green_value, double blue_value);
    float red;
    float green;
    float blue;
};

static_assert(sizeof(EnvironmentColour) == 0x0c, "EnvironmentColour_must_be_0x0c");
