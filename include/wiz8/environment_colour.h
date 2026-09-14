#pragma once

/* The RGB triple the world environment carries. Its two out-of-line methods
   are recovered in AutomapScreen.cpp (0x005806B0, 0x00580940); the default
   constructor and assignment operator are header-emitted. */
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
